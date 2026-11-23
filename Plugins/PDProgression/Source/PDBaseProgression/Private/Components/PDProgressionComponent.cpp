/* @author: Ario Amin @ Permafrost Development. @copyright: Full BSL(1.1) License included at bottom of the file  */

#include "Components/PDProgressionComponent.h"

#include "Net/PDProgressionNetDatum.h"

#include "Net/Core/PushModel/PushModel.h"
#include "Net/UnrealNetwork.h"
#include "Subsystems/PDProgressionSubsystem.h"


void UPDStatHandler::BeginPlay()
{
	Super::BeginPlay();
	
	UPDStatSubsystem::Get()->StatHandlers.Emplace(OwnerID, this);
}

double UPDStatHandler::GetStatValue(const FGameplayTag& StatTag)
{
	if (LocalStatMappings.Contains(StatTag) == false) { return 0.0; }

	const FPDStatMapping* StatMapping = LocalStatMappings.Find(StatTag);
	check(StatMapping)
	
	return StatList.Items[StatMapping->Index].GetAppliedValue();
}

void UPDStatHandler::ModifySkill(const FGameplayTag& SkillTag, const bool bUnlock)
{
	static UPDStatSubsystem* StatSubsystem = UPDStatSubsystem::Get();
	FPDSkillTree* Tree = StatSubsystem->GetTreeTypeDataPtrFromSkill(SkillTag);
	if (Tree == nullptr)
	{
		UE_LOG(PDLog_Progression, Error,
			TEXT("UPDStatHandler::ModifySkill "
				"-- Failed finding a tree that matched with skill(%s) "), *SkillTag.GetTagName().ToString());
		return;
	}

	const auto ProcessFoundSkill =
		[&](const FGameplayTag& InSkillTag)
		{
			const FPDSkillBranch& SkillBranch = Tree->Skills.FindRef(InSkillTag);
			
			const int32 MinimumTokensSpent = SkillBranch.TokenRules.MinTokenLevel;
			const int32 SkillCost = SkillBranch.TokenRules.TokenValue;

			const FGameplayTag& TokenCategory = SkillBranch.TokenRules.TokenType;
			FPDSkillTokenBase* CategoryTokenValues = Tokens.FindByKey(TokenCategory);
			FPDSkillTokenBase* TotalTokensSpentOnCategory = TokensSpentTotal.FindByKey(TokenCategory);
		
			const bool bMetCategoryProgressionRequirements = TotalTokensSpentOnCategory->TokenValue >= MinimumTokensSpent;
			const bool bCanAffordSkill = CategoryTokenValues->TokenValue >= SkillCost;

			if (bCanAffordSkill && bMetCategoryProgressionRequirements)
			{
				// actually unlock skill here, set it's stat level to 1, should have been 0 before
				StatList.AddStat(InSkillTag, 0, bUnlock);

				CategoryTokenValues->TokenValue -=  (bUnlock)* SkillCost;      // Remove from user
				TotalTokensSpentOnCategory->TokenValue += (bUnlock)*SkillCost; // Add to tally
			}
		};
	

	// First check if it is in the top-level
	if (Tree->Skills.Contains(SkillTag))
	{
		MARK_PROPERTY_DIRTY_FROM_NAME(UPDStatHandler, StatList, this);		
		ProcessFoundSkill(SkillTag);
		return;
	}
	
	static FPDSkillBranch DummyBranch;
	
	TFunction<void(const TArray<FDataTableRowHandle>&)> BranchMapper;

	// @todo slow recursion, re-do
	// TArray<FPDSkillBranch> BranchPaths. Recursively map all skills in a tree back to the tree, for fast access downstream
	BranchMapper= [&, SkillTag, TreeRowTag = Tree->Tag](const TArray<FDataTableRowHandle>& BranchPaths)
	{
		for (const FDataTableRowHandle& SkillBranchHandle : BranchPaths)
		{
			FPDSkillBranch& SkillBranch = SkillBranchHandle.IsNull() ? DummyBranch : *SkillBranchHandle.GetRow<FPDSkillBranch>("");
			
			const FPDStatsRow* InnerBranchStat = SkillBranch.BranchRootSkill.GetRow<FPDStatsRow>("");
			if (InnerBranchStat == nullptr) { continue; }

			// found the skill
			if (SkillTag == InnerBranchStat->ProgressionTag)
			{
				MARK_PROPERTY_DIRTY_FROM_NAME(UPDStatHandler, StatList, this);		
				ProcessFoundSkill(SkillTag);
				
				break;
			}
			
			BranchMapper(SkillBranch.BranchPaths);
		}
	};
	
	// Otherwise, Find the skill inside the tree
	for (const TTuple<FGameplayTag, FPDSkillBranch>& SkillBranchTuple : Tree->Skills)
	{
		// Root
		BranchMapper(SkillBranchTuple.Value.BranchPaths);
	}		
}

void UPDStatHandler::AttemptUnlockSkill_Implementation(const FGameplayTag& SkillTag)
{
	ModifySkill(SkillTag, true);
}


void UPDStatHandler::AttemptLockSkill_Implementation(const FGameplayTag& SkillTag)
{
	ModifySkill(SkillTag, false);
}

bool UPDStatHandler::HasCompleteAuthority() const
{
	switch (GetOwnerRole())
	{
	case ROLE_None:
	case ROLE_SimulatedProxy:
	case ROLE_AutonomousProxy:
	case ROLE_MAX:
		return false;
	case ROLE_Authority:
		break;
	}

	switch (GetOwner()->GetNetMode())
	{ 
	case NM_MAX:
	case NM_Client:
		return false;
	case NM_Standalone:
	case NM_DedicatedServer:
	case NM_ListenServer:
		break;
	}
	return true;
}

void UPDStatHandler::GrantTokens(const FGameplayTag& StatTag, int32 LevelDelta, const int32 CurrentLevel)
{
	static const UPDStatSubsystem* StatSubsystem = UPDStatSubsystem::Get();
	
	MARK_PROPERTY_DIRTY_FROM_NAME(UPDStatHandler, Tokens, this)
	const FPDStatsRow& StatDefaultValue = StatSubsystem->GetStatTypeData(StatTag);
	for (const TTuple<FGameplayTag, UCurveFloat*>& TokenCategoryCurveTuple
	     : StatDefaultValue.TokensToGrantPerLevel)
	{
		if (TokenCategoryCurveTuple.Value == nullptr)
		{
			UE_LOG(PDLog_Progression, Error,
			       TEXT("UPDStatHandler::GrantTokens "
				       "-- Iterating the token list, entry(%s) has not valid curve applied ot it"), *TokenCategoryCurveTuple.Key.GetTagName().ToString());
			continue;
		}

		int32 AllNewlyGrantedCategoryTokens = 0;
		int32 LevelDeltaCopy = LevelDelta;

		for (; LevelDeltaCopy > 0; LevelDeltaCopy--)
		{
			const float TokensAsFloats = TokenCategoryCurveTuple.Value->GetFloatValue(CurrentLevel - LevelDeltaCopy);
			AllNewlyGrantedCategoryTokens += TokensAsFloats + 0.5f;
		}

		FPDSkillTokenBase* CategoryTokenValues = Tokens.FindByKey(TokenCategoryCurveTuple.Key);
		if (CategoryTokenValues == nullptr)
		{
			FPDSkillTokenBase TokenBase = {TokenCategoryCurveTuple.Key, AllNewlyGrantedCategoryTokens};
			Tokens.Add(TokenBase);
			continue;
		}

		CategoryTokenValues->TokenValue += AllNewlyGrantedCategoryTokens;
	}
}

void UPDStatHandler::IncreaseStatLevel_Implementation(const FGameplayTag& StatTag, int32 LevelDelta)
{
	if (LocalStatMappings.Contains(StatTag) == false)
	{
		// @todo Set up developer settings to allow this to fail (as happens now) or to just add the missing stat upon this call
		UE_LOG(PDLog_Progression, Warning,
			TEXT("UPDStatHandler::IncreaseStatLevel_Implementation "
			"-- Owners statlist did not contain the requested stat(%s)"
			" @todo Set up developer settings to allow this to fail (as happens now) or to just add the missing stat upon this call "), *StatTag.GetTagName().ToString())
		return;
	}
	if (HasCompleteAuthority() == false) { return; }


	// @DONE Map all cross-behaviours in the subsystems latent initialization
	// @DONE cont.  And make use of it to resolve crossbehaviours

	const UPDStatSubsystem* StatSubsystem = UPDStatSubsystem::Get();
	const int32 StatIndex = LocalStatMappings.Find(StatTag)->Index;
	FPDStatNetDatum& StatNetDatum = StatList.Items[StatIndex];

	MARK_PROPERTY_DIRTY_FROM_NAME(UPDStatHandler, StatList, this)
	const int32 CurrentLevel = (StatNetDatum.CurrentLevel += LevelDelta);

	StatNetDatum.CurrentStatValue =
	UPDStatSubsystem::Get()->
		GetStatTypeData(StatTag).Representation.ResolveValue(CurrentLevel);

	// Granting Tokens
	GrantTokens(StatTag, LevelDelta, CurrentLevel);
	
	const TMap<FGameplayTag, TArray<FGameplayTag>>& StatCrossBehaviourMap = StatSubsystem->StatCrossBehaviourMap;
	if (StatCrossBehaviourMap.Contains(StatTag) == false) { return; }

	// The stat we just leveled up is now calling all stat that the levelled up stat is an effector to,
	// and updates their effect on said stat
	TArray<FGameplayTag> UpdateTargets = StatCrossBehaviourMap.FindRef(StatTag);
	for (const FGameplayTag& Target : UpdateTargets)
	{
		const FPDStatsRow& TargetStat = StatSubsystem->GetStatTypeData(Target);

		// @note The calling stat will always exist in the target stats 'RulesAffectedBy',
		// as this is what has decided the cross behaviour was mapped in the first place
		const FPDStatsCrossBehaviourRules& CrossBehaviourRules = TargetStat.RulesAffectedBy.FindRef(StatTag);
		if (CrossBehaviourRules.RuleSetLevelCurveMultiplier == nullptr)
		{
			continue;
		}

		const float OldCrossBehaviourMultiplier =
			CrossBehaviourRules.RuleSetLevelCurveMultiplier->GetFloatValue(CurrentLevel - LevelDelta);
		const float CrossBehaviourMultiplier =
			CrossBehaviourRules.RuleSetLevelCurveMultiplier->GetFloatValue(CurrentLevel);

		const float OldCrossBehaviourResult = CrossBehaviourRules.CrossBehaviourBaseValue * OldCrossBehaviourMultiplier;
		const float CrossBehaviourResult = CrossBehaviourRules.CrossBehaviourBaseValue * CrossBehaviourMultiplier;

		// Remove old result
		StatNetDatum.CrossBehaviourValue -= OldCrossBehaviourResult; 
		// Add New result
		StatNetDatum.CrossBehaviourValue += CrossBehaviourResult; 
	}
}

void UPDStatHandler::IncreaseStatExperience_Implementation(const FGameplayTag& StatTag, int32 ExperienceDelta)
{
	if (LocalStatMappings.Contains(StatTag) == false)
	{
		// @todo Set up developer settings to allow this to fail (as happens now) or to just add the missing stat upon this call
		UE_LOG(PDLog_Progression, Warning,
			TEXT("UPDStatHandler::IncreaseStatExperience_Implementation "
			"-- Owners statlist did not contain the requested stat(%s)"
			" @todo Set up developer settings to allow this to fail (as happens now) or to just add the missing stat upon this call "), *StatTag.GetTagName().ToString())
		return;
	}
	if (HasCompleteAuthority() == false) { return; }

	static UPDStatSubsystem* StatSubsystem = UPDStatSubsystem::Get();
	const FPDStatsRow& StatDefaults = StatSubsystem->GetStatTypeData(StatTag);

	if (StatDefaults.ExperienceCurve == nullptr)
	{
		// @todo Set up developer settings to allow this to apply some basic scaling operation if we hit this case, curves are math in the end
		UE_LOG(PDLog_Progression, Error,
			TEXT("UPDStatHandler::IncreaseStatExperience_Implementation "
			"-- Ensure your stat((%s)) has an experience curve applied to it"
			" @todo Set up developer settings to allow this to apply some basic scaling operation if we hit this case, curves are math in the end"), *StatTag.GetTagName().ToString())
		return;
	}
	
	MARK_PROPERTY_DIRTY_FROM_NAME(UPDStatHandler, StatList, this)
	const int32 StatIndex = LocalStatMappings.Find(StatTag)->Index;
	FPDStatNetDatum& StatNetDatum = StatList.Items[StatIndex];

	// Check if we may level up
	const int32 ExpLimitForCurrentLevel =
		StatDefaults.ExperienceCurve->GetFloatValue(StatNetDatum.CurrentLevel);
	StatNetDatum.CurrentExperience += ExperienceDelta;	
	if (StatNetDatum.CurrentExperience >= ExpLimitForCurrentLevel)
	{
		IncreaseStatLevel(StatTag);
	}
}

void UPDStatHandler::SetClass_Implementation(const FGameplayTag& ClassTag)
{
	MARK_PROPERTY_DIRTY_FROM_NAME(UPDStatHandler, StatList, this)
	FPDProgressionClassRow* ClassRow = UPDStatSubsystem::Get()->GetClassTypeDataPtr(ClassTag);
				
	FPDStatMapping ConstructedStatMapping;
	for (const FGameplayTag& StatTag : ClassRow->DefaultStats)
	{
		StatList.AddStat(StatTag, 0, 0);

		// Maps the index to the tag
		ConstructedStatMapping.Index = StatList.Items.Num() - 1;
		ConstructedStatMapping.Tag = StatTag;		
		LocalStatMappings.Emplace(ConstructedStatMapping);
	}

	for (const FGameplayTag& ActiveEffectTag : ClassRow->DefaultActiveEffects)
	{
		StatList.AddActiveEffect(ActiveEffectTag, 0, 0);

		// Maps the index to the tag
		ConstructedStatMapping.Index = StatList.Items.Num() - 1;
		ConstructedStatMapping.Tag = ActiveEffectTag;		
		LocalStatMappings.Emplace(ConstructedStatMapping);		
	}

	for (const FGameplayTag& PassiveEffectTag : ClassRow->DefaultPassiveEffects)
	{
		StatList.AddPassiveEffect(PassiveEffectTag, 0, 0);

		// Maps the index to the tag
		ConstructedStatMapping.Index = StatList.Items.Num() - 1;
		ConstructedStatMapping.Tag = PassiveEffectTag;		
		LocalStatMappings.Emplace(ConstructedStatMapping);			
	}	
}

void UPDStatHandler::GetLifetimeReplicatedProps(
	TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	FDoRepLifetimeParams Parameters{};
	Parameters.bIsPushBased = true;

	DOREPLIFETIME_WITH_PARAMS_FAST(UPDStatHandler, StatList, Parameters);	
	DOREPLIFETIME_WITH_PARAMS_FAST(UPDStatHandler, Tokens, Parameters);	
	DOREPLIFETIME_WITH_PARAMS_FAST(UPDStatHandler, TokensSpentTotal, Parameters);	
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
