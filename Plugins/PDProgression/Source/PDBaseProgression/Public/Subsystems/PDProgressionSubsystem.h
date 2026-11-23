/* @author: Ario Amin @ Permafrost Development. @copyright: Full BSL(1.1) License included at bottom of the file  */

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "PDProgressionCommon.h"
#include "PDProgressionSubsystem.generated.h"

class UPDStatHandler;
struct FPDProgressionClassRow;

/** @brief Our source for shared, project-wide progression system settings.
 * @details Our source for all stat-tables, progression class tables and skill-tree tables  */
UCLASS(Config = "Game", DefaultConfig)
class PDBASEPROGRESSION_API UPDProgressionSubsystemSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	/** @brief List of stat tables to use as global stat source */
	UPROPERTY(Config, EditAnywhere, Category = "ProgressionTables", Meta = (RequiredAssetDataTags="RowStructure=/Script/PDBaseProgression.PDStatsRow"))
	TArray<TSoftObjectPtr<UDataTable>> ProgressionStatTables;

	/** @brief List of stat classes and skill-trees made available to them. Think 'Bard','Warlock', etc */
	UPROPERTY(Config, EditAnywhere, Category = "ProgressionTables", Meta = (RequiredAssetDataTags="RowStructure=/Script/PDBaseProgression.PDProgressionClassRow"))
	TArray<TSoftObjectPtr<UDataTable>> ProgressionClassTables;

	/** @brief List of actual skill-tree tables, These tables define what stats counts as skills and their progression paths */
	UPROPERTY(Config, EditAnywhere, Category = "ProgressionTables", Meta = (RequiredAssetDataTags="RowStructure=/Script/PDBaseProgression.PDSkillTree"))
	TArray<TSoftObjectPtr<UDataTable>> ProgressionTreeTables;	
};

/** @brief The subsystem for the stat system.
 * @details Handles mapping the stat-system tables for quick downstream access to data and common calculations in the stat-system */
UCLASS(Blueprintable)
class PDBASEPROGRESSION_API UPDStatSubsystem : public UEngineSubsystem
{
	GENERATED_BODY()
public:
	/** @brief Singleton-style getter, used for simplifying access, thus it is used for code-productivity */
	static UPDStatSubsystem* Get();

	/** @brief Maps all the the classes, skilltrees and stats, along-side stat crossbehaviours*/
	void LatentInitialization();

	/** @brief Resolves the cross behaviour for the given stat, in case it has a 'RuleSetLevelCurveMultiplier' set. Otherwise do not modify per level  */
	void ResolveCrossBehaviours(
		int32 StatSourceLevel,
		const FPDStatsRow& SelectedStat,
		const FGameplayTag& StatSourceTag,
		FPDStatsCrossBehaviourRules& CrossBehaviourRules,
		double& CrossBehaviourOffset_Normalized) const;
	/** @brief Resolves the delta between two levels different cross behaviours, for a given pair of stats.
	 * @note Only resolve in case it has a 'RuleSetLevelCurveMultiplier' set. Otherwise do not modify per level.  */
	void ResolveCrossBehaviourDelta(
		int32 SelectedStat_OldLevel,
		int32 SelectedStat_LevelDelta,
		const FPDStatsRow& SelectedStat_Datum,
		const FGameplayTag& StatSourceTag,
		double& CrossBehaviourOffset_Normalized) const;

	/** @brief Tree-tag keyed access to table entries for 'Skill Trees',
	 * @note Has faster reads compared to searching the actual datatables.
	 * @note Returns a dummy ref if the tree is not found */
	UFUNCTION(BlueprintCallable)
	FPDSkillTree& GetTreeTypeData(const FGameplayTag& RequestedTree) const;
	/** @brief Tree-tag keyed access to table entries for 'Skill Trees',
	 * @note Has faster reads compared to searching the actual datatables.
	 * @note Returns a nullptr if the tree is not found */	
	FPDSkillTree* GetTreeTypeDataPtr(const FGameplayTag& RequestedTree) const;

	/** @brief Skill-tag keyed access to table entries for 'Skill Trees',
	 * @note Has faster reads compared to searching the actual datatables.
	 * @note Returns a dummy ref if the tree is not found */
	UFUNCTION(BlueprintCallable)
	FPDSkillTree& GetTreeTypeDataFromSkill(const FGameplayTag& RequestedSkill) const;

	/** @brief Skill-tag keyed access to table entries for 'Skill Trees',
	 * @note Has faster reads compared to searching the actual datatables.
	 * @note Returns a nullptr if the tree is not found */	
	FPDSkillTree* GetTreeTypeDataPtrFromSkill(const FGameplayTag& RequestedSkill) const;
	
	/** @brief Class-tag keyed access to table entries for 'Stat Classes',
	 * @note Has faster reads compared to searching the actual datatables.
	 * @note Returns a dummy ref if the class is not found */	
	UFUNCTION(BlueprintCallable)
	FPDProgressionClassRow& GetClassTypeData(const FGameplayTag& RequestedClass) const;
	/** @brief Class-tag keyed access to table entries for 'Stat Classes',
	 * @note Has faster reads compared to searching the actual datatables.
	 * @note Returns a nullptr if the class is not found */	
	FPDProgressionClassRow* GetClassTypeDataPtr(const FGameplayTag& RequestedClass) const;

	/** @brief Stat-tag keyed access to table entries for 'Stats',
	 * @note Has faster reads compared to searching the actual datatables.
	 * @note Returns a dummy ref if the class is not found */	
	UFUNCTION(BlueprintCallable)
	FPDStatsRow& GetStatTypeData(const FGameplayTag& RequestedStat) const;
	/** @brief Stat-tag keyed access to table entries for 'Stats',
	 * @note Has faster reads compared to searching the actual datatables.
	 * @note Returns a nullptr if the class is not found */
	FPDStatsRow* GetStatTypeDataPtr(const FGameplayTag& RequestedStat) const;

	/** @brief Gets the name of the tag, at the depth this tag is at. meaning ParentA.ParentB.ThisTag returns "ThisTag" */
	UFUNCTION(BlueprintCallable)
	static FString GetTagNameLeaf(const FGameplayTag& Tag);

	/** @brief Gets the name of the tag, at the depth this tag is at. meaning ParentA.ParentB.ThisTag returns "ParentB" */
	UFUNCTION(BlueprintCallable)
	static FString GetTagCategory(const FGameplayTag& Tag);	
	
	/** @brief Gets the name of the tag, at the depth this tag is at. meaning ParentA.ParentB.ThisTag returns "ParentB.ThisTag" */
	UFUNCTION(BlueprintCallable)
	static FString GetTagNameLeafAndParent(const FGameplayTag& Tag);	
	
	/** @brief Tree table row entries mapped by their tree-tag. Used for fast down-stream access */
	TMap<FGameplayTag, FPDSkillTree*> TreeTypes;

	/** @brief Tree table row tags mapped by their containing skill-tags. Used for fast down-stream access  */
	TMap<FGameplayTag, FGameplayTag> SkillToTreeMapping;
	
	/** @brief Class table row entries mapped by their class-tag. Used for fast down-stream access */
	TMap<FGameplayTag, FPDProgressionClassRow*> ClassTypes;
	/** @brief Stat (default values) table row entries mapped by their stat-tag. Used for fast down-stream access */
	TMap<FGameplayTag, FPDStatsRow*> DefaultStats;

	/** @brief Key is a 'Tag' that affect value of the 'List of Tags' */
	TMap<FGameplayTag, TArray<FGameplayTag>> StatCrossBehaviourMap;
	/** @brief Key is a 'Tag' that is affected by value of the 'List of Tags' */
	TMap<FGameplayTag, TArray<FGameplayTag>> StatCrossBehaviourBackMapped;


	/** @brief Mapped stat-handlers, mapped by owner-/player-id,
	 * @note Will have all the games stat-handlers on the server. If on the client, it will 1 entry per player connected via the same client  */
	UPROPERTY()
	TMap<int32, UPDStatHandler*> StatHandlers;
};
		
	

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
