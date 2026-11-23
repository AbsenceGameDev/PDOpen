/* @author: Ario Amin @ Permafrost Development. @copyright: Full BSL(1.1) License included at bottom of the file  */

#include "Subsystems/PDProgressionSubsystem.h"

#include "Components/PDProgressionComponent.h"
#include "Net/PDProgressionNetDatum.h"

UPDStatSubsystem* UPDStatSubsystem::Get()
{
	static UPDStatSubsystem* Self = nullptr;
	if (Self == nullptr)
	{
		Self = GEngine->GetEngineSubsystem<UPDStatSubsystem>();
		Self->LatentInitialization();
	}
	
	return Self;
}

void UPDStatSubsystem::LatentInitialization()
{
	const UPDProgressionSubsystemSettings* DefaultSubsystemSettings =
		GetDefault<UPDProgressionSubsystemSettings>();

	for (const TSoftObjectPtr<UDataTable>& Elem
	     : DefaultSubsystemSettings->ProgressionClassTables)
	{
		const UDataTable* Table = Elem.LoadSynchronous();
		if (Table == nullptr) { continue; }
		
		TArray<FPDProgressionClassRow*> ClassRows;
		Table->GetAllRows("", ClassRows);
		for (FPDProgressionClassRow* ClassRow : ClassRows)
		{
			if (ClassRow == nullptr) { continue; }
			ClassTypes.Emplace(ClassRow->Tag, ClassRow);
		}
	}
	for (const TSoftObjectPtr<UDataTable>&  Elem
		: DefaultSubsystemSettings->ProgressionStatTables)
	{
		const UDataTable* Table = Elem.LoadSynchronous();
		if (Table == nullptr) { continue; }
		
		TArray<FPDStatsRow*> StatRows;
		Table->GetAllRows("", StatRows);
		for (FPDStatsRow* StatRow : StatRows)
		{
			if (StatRow == nullptr) { continue; }
			DefaultStats.Emplace(StatRow->ProgressionTag, StatRow);

			for (const TTuple<FGameplayTag, FPDStatsCrossBehaviourRules>& Rule
				: StatRow->RulesAffectedBy)
			{
				StatCrossBehaviourMap.FindOrAdd(Rule.Key).AddUnique(StatRow->ProgressionTag);
				StatCrossBehaviourBackMapped.FindOrAdd(StatRow->ProgressionTag).AddUnique(Rule.Key);
			}
			
		}		
	}
	for (const TSoftObjectPtr<UDataTable>&  Elem
		: DefaultSubsystemSettings->ProgressionTreeTables)
	{
		const UDataTable* Table = Elem.LoadSynchronous();
		if (Table == nullptr) { continue; }
		
		TArray<FPDSkillTree*> TreeRows;
		Table->GetAllRows("", TreeRows);
		for (FPDSkillTree* TreeRow : TreeRows)
		{
			if (TreeRow == nullptr) { continue; }
			TreeTypes.Emplace(TreeRow->Tag, TreeRow);

			for (const TTuple<FGameplayTag, FPDSkillBranch>& SkillBranchTuple
				: TreeRow->Skills)
			{
				// Root
				SkillToTreeMapping.Emplace(SkillBranchTuple.Key, TreeRow->Tag);
				

				const FPDStatsRow* BranchStat = SkillBranchTuple.Value.BranchRootSkill.GetRow<FPDStatsRow>("");
				if (BranchStat == nullptr) { continue; }
				SkillToTreeMapping.Emplace(BranchStat->ProgressionTag, TreeRow->Tag);

				static const FPDSkillBranch DummySkillBranch;

				
				// TArray<FPDSkillBranch> BranchPaths. Recursively map all skills in a tree back to the tree, for fast access downstream
				TFunction<void(const TArray<FDataTableRowHandle>&)> BranchMapper;
				BranchMapper = [&, TreeRowTag = TreeRow->Tag](const TArray<FDataTableRowHandle>& BranchPaths)
				{
					for (const FDataTableRowHandle& SkillBranchHandle : BranchPaths)
					{
						const FPDSkillBranch& SkillBranch = SkillBranchHandle.IsNull() ? DummySkillBranch : *SkillBranchHandle.GetRow<FPDSkillBranch>("");
						const FPDStatsRow* InnerBranchStat = SkillBranch.BranchRootSkill.GetRow<FPDStatsRow>("");
						if (InnerBranchStat == nullptr) { continue; }
						SkillToTreeMapping.Emplace(InnerBranchStat->ProgressionTag, TreeRowTag);

						BranchMapper(SkillBranch.BranchPaths);
					}
				};

				BranchMapper(SkillBranchTuple.Value.BranchPaths);
				
			}

		}		
	}
}

void UPDStatSubsystem::ResolveCrossBehaviours(
	int32 StatSourceLevel,
	const FPDStatsRow& SelectedStat,
	const FGameplayTag& StatSourceTag,
	FPDStatsCrossBehaviourRules& CrossBehaviourRules,
	double& CrossBehaviourOffset_Normalized) const
{
	const FPDStatsRow* StatSourceDefaultDataPtr = GetStatTypeDataPtr(StatSourceTag);
	if (StatSourceDefaultDataPtr == nullptr) { return; }
	
	CrossBehaviourRules = SelectedStat.RulesAffectedBy.FindRef(StatSourceTag);

	float CrossBehaviourMultiplier = 1.0;
	if (CrossBehaviourRules.RuleSetLevelCurveMultiplier != nullptr)
	{
		CrossBehaviourMultiplier = 
			CrossBehaviourRules.RuleSetLevelCurveMultiplier->GetFloatValue(StatSourceLevel);
	}

	const int32 StatSourceBaseDivisor = StatSourceDefaultDataPtr->Representation.BaseDivisor;
	const int32 CrossBehaviourOffset_NotNormalized = CrossBehaviourRules.CrossBehaviourBaseValue * CrossBehaviourMultiplier;
	CrossBehaviourOffset_Normalized =
		static_cast<double>(CrossBehaviourOffset_NotNormalized) / static_cast<double>(StatSourceBaseDivisor);
}

void UPDStatSubsystem::ResolveCrossBehaviourDelta(
	int32 SelectedStat_OldLevel,
	int32 SelectedStat_LevelDelta,
	const FPDStatsRow& SelectedStat_Datum,
	const FGameplayTag& StatTarget_Tag,
	double& DeltaNewLevelOffset) const
{
	const FPDStatsRow* StatTarget_DefaultDataPtr = GetStatTypeDataPtr(StatTarget_Tag);
		
	if (StatTarget_DefaultDataPtr == nullptr) { return; }

	// Find out our (SelectedStatTag) effect on target stat (Stats that are affected by us)
	float CrossBehaviourMultiplier = 1.0;
	float NextCrossBehaviourMultiplier = 1.0;
	const FPDStatsCrossBehaviourRules& CrossBehaviourRules = StatTarget_DefaultDataPtr->RulesAffectedBy.FindRef(SelectedStat_Datum.ProgressionTag);
	if (CrossBehaviourRules.RuleSetLevelCurveMultiplier != nullptr)
	{
		CrossBehaviourMultiplier = 
			CrossBehaviourRules.RuleSetLevelCurveMultiplier->GetFloatValue(SelectedStat_OldLevel);
		NextCrossBehaviourMultiplier = 
			CrossBehaviourRules.RuleSetLevelCurveMultiplier->GetFloatValue(SelectedStat_OldLevel + SelectedStat_LevelDelta);
	}

	const int32 StatTargetBaseDivisor = StatTarget_DefaultDataPtr->Representation.BaseDivisor;
	const int32 CurrentCrossBehaviourOffset_NotNormalized = CrossBehaviourRules.CrossBehaviourBaseValue * CrossBehaviourMultiplier;
	const double CurrentCrossBehaviourOffset_Normalized =
		static_cast<double>(CurrentCrossBehaviourOffset_NotNormalized) / static_cast<double>(StatTargetBaseDivisor);
	const int32 NextCrossBehaviourOffset_NotNormalized = CrossBehaviourRules.CrossBehaviourBaseValue * NextCrossBehaviourMultiplier;
	const double NextCrossBehaviourOffset_Normalized =
		static_cast<double>(NextCrossBehaviourOffset_NotNormalized) / static_cast<double>(StatTargetBaseDivisor);
		
	DeltaNewLevelOffset = NextCrossBehaviourOffset_Normalized - CurrentCrossBehaviourOffset_Normalized;
}


FPDSkillTree& UPDStatSubsystem::GetTreeTypeData(const FGameplayTag& RequestedTree) const
{
	static FPDSkillTree Dummy;
	return TreeTypes.Contains(RequestedTree)
		? **TreeTypes.Find(RequestedTree)
		: Dummy;
}

FPDSkillTree* UPDStatSubsystem::GetTreeTypeDataPtr(const FGameplayTag& RequestedTree) const
{
	return TreeTypes.Contains(RequestedTree)
		? *TreeTypes.Find(RequestedTree)
		: nullptr;
}

FPDSkillTree& UPDStatSubsystem::GetTreeTypeDataFromSkill(const FGameplayTag& RequestedSkill) const
{
	static FPDSkillTree Dummy; 
	const FGameplayTag* SkillTreeTagPtr = SkillToTreeMapping.Find(RequestedSkill);
	FPDSkillTree* const* SkillTree = SkillTreeTagPtr != nullptr ? TreeTypes.Find(*SkillTreeTagPtr) : nullptr;
	
	return SkillTree == nullptr ? Dummy : **SkillTree;
}

FPDSkillTree* UPDStatSubsystem::GetTreeTypeDataPtrFromSkill(const FGameplayTag& RequestedSkill) const
{
	static FPDSkillTree Dummy; 
	const FGameplayTag* SkillTreeTagPtr = SkillToTreeMapping.Find(RequestedSkill);
	FPDSkillTree* const* SkillTree = SkillTreeTagPtr != nullptr ? TreeTypes.Find(*SkillTreeTagPtr) : nullptr;
	
	return SkillTree == nullptr ? nullptr : *SkillTree;
}

FPDProgressionClassRow& UPDStatSubsystem::GetClassTypeData(const FGameplayTag& RequestedClass) const
{
	static FPDProgressionClassRow Dummy; 
	return ClassTypes.Contains(RequestedClass)
		? **ClassTypes.Find(RequestedClass)
		: Dummy;
}

FPDProgressionClassRow* UPDStatSubsystem::GetClassTypeDataPtr(const FGameplayTag& RequestedClass) const
{
	return ClassTypes.Contains(RequestedClass)
		? *ClassTypes.Find(RequestedClass)
		: nullptr;
}

FPDStatsRow& UPDStatSubsystem::GetStatTypeData(const FGameplayTag& RequestedStat) const
{
	static FPDStatsRow Dummy; 
	return DefaultStats.Contains(RequestedStat)
		? **DefaultStats.Find(RequestedStat)
		: Dummy;
}

FPDStatsRow* UPDStatSubsystem::GetStatTypeDataPtr(const FGameplayTag& RequestedStat) const
{
	return DefaultStats.Contains(RequestedStat)
		? *DefaultStats.Find(RequestedStat)
		: nullptr;
}

FString UPDStatSubsystem::GetTagNameLeaf(const FGameplayTag& Tag)
{
	const FString& TagProtoString = Tag.GetTagName().ToString();
	const int32 CutoffIndex = 1 + TagProtoString.Find(".", ESearchCase::IgnoreCase, ESearchDir::FromEnd);
	const FString TagResultString = TagProtoString.RightChop(CutoffIndex);

	return TagResultString;
}

FString UPDStatSubsystem::GetTagCategory(const FGameplayTag& Tag)
{
	return GetTagNameLeaf(Tag.RequestDirectParent());
}

FString UPDStatSubsystem::GetTagNameLeafAndParent(const FGameplayTag& Tag)
{
	const FString TagCategoryResultString = GetTagNameLeaf(Tag.RequestDirectParent());
	const FString TagResultString = GetTagNameLeaf(Tag);
	return TagCategoryResultString + "::" + TagResultString;
}


/**
Business Source License 1.1

Parameters

Licensor:             Ario Amin (@ Permafrost Development)
Licensed Work:        RTSOpen (version 0.1.0, Source available on github)
                      The Licensed Work is (c) 2026 Ario Amin (@ Permafrost Development)
Additional Use Grant: You may make free use of the Licensed Work in a commercial product or service provided these additional conditions as met; 
                      1. Must give attributions to the original author of the Licensed Work, in 'Credits' if that is applicable.
                      2. The Licensed Work must be 'Compiled' before being redistributed.
                      3. The Licensed Work 'Source' may be linked but may not be packaged into the product or service being sold
                      4. Must not be resold or repackaged or redistributed as another product, is only allowed to be used within a commercial or non-commercial game project.
                      5. Teams whose 'Total Finances' exceed $100,000 USD for the most recent 12-month period must contact the owner for a custom license or buy the framework from a marketplace it has been made available on.

                      "Credits" indicate a scrolling screen with attributions. This is usually in a products end-state

                      "Total Finances" means the largest of your aggregate gross revenues, entire budget, or funding (no matter the source).
                      "Package" means the collection of files distributed by the Licensor, and derivatives of that collection
                      and/or of those files..   

                      "Source" form means the source code, documentation source, and configuration files for the Package, usually in human-readable format.

                      "Compiled" form means the compiled bytecode, object code, binary, or any other
                      form resulting from mechanical transformation or translation of the Source form.

Change Date:          2030-04-26

Change License:       Apache License, Version 2.0

For information about alternative licensing arrangements for the Software,
please visit: https://permadev.se/

Notice

The Business Source License (this document, or the “License”) is not an Open
Source license. However, the Licensed Work will eventually be made available
under an Open Source License, as stated in this License.

License text copyright (c) 2017 MariaDB Corporation Ab, All Rights Reserved.
“Business Source License” is a trademark of MariaDB Corporation Ab.

-----------------------------------------------------------------------------

Business Source License 1.1

Terms

The Licensor hereby grants you the right to copy, modify, create derivative
works, redistribute, and make non-production use of the Licensed Work. The
Licensor may make an Additional Use Grant, above, permitting limited
production use.

Effective on the Change Date, or the fourth anniversary of the first publicly
available distribution of a specific version of the Licensed Work under this
License, whichever comes first, the Licensor hereby grants you rights under
the terms of the Change License, and the rights granted in the paragraph
above terminate.

If your use of the Licensed Work does not comply with the requirements
currently in effect as described in this License, you must purchase a
commercial license from the Licensor, its affiliated entities, or authorized
resellers, or you must refrain from using the Licensed Work.

All copies of the original and modified Licensed Work, and derivative works
of the Licensed Work, are subject to this License. This License applies
separately for each version of the Licensed Work and the Change Date may vary
for each version of the Licensed Work released by Licensor.

You must conspicuously display this License on each original or modified copy
of the Licensed Work. If you receive the Licensed Work in original or
modified form from a third party, the terms and conditions set forth in this
License apply to your use of that work.

Any use of the Licensed Work in violation of this License will automatically
terminate your rights under this License for the current and all other
versions of the Licensed Work.

This License does not grant you any right in any trademark or logo of
Licensor or its affiliates (provided that you may use a trademark or logo of
Licensor as expressly required by this License).

TO THE EXTENT PERMITTED BY APPLICABLE LAW, THE LICENSED WORK IS PROVIDED ON
AN “AS IS” BASIS. LICENSOR HEREBY DISCLAIMS ALL WARRANTIES AND CONDITIONS,
EXPRESS OR IMPLIED, INCLUDING (WITHOUT LIMITATION) WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, NON-INFRINGEMENT, AND
TITLE.

MariaDB hereby grants you permission to use this License’s text to license
your works, and to refer to it using the trademark “Business Source License”,
as long as you comply with the Covenants of Licensor below.

Covenants of Licensor

In consideration of the right to use this License’s text and the “Business
Source License” name and trademark, Licensor covenants to MariaDB, and to all
other recipients of the licensed work to be provided by Licensor:

1. To specify as the Change License the GPL Version 2.0 or any later version,
   or a license that is compatible with GPL Version 2.0 or a later version,
   where “compatible” means that software provided under the Change License can
   be included in a program with software provided under GPL Version 2.0 or a
   later version. Licensor may specify additional Change Licenses without
   limitation.

2. To either: (a) specify an additional grant of rights to use that does not
   impose any additional restriction on the right granted in this License, as
   the Additional Use Grant; or (b) insert the text “None”.

3. To specify a Change Date.

4. Not to modify this License in any other way.
 **/
