/* @author: Ario Amin @ Permafrost Development. @copyright: Full BSL(1.1) License included at bottom of the file  */

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

#include "PDProgressionCommon.generated.h"

DECLARE_LOG_CATEGORY_CLASS(PDLog_Progression, Log, All);


/*
 *
- Stats Overview 
	- (DONE) Enum  EPDProgressionBehaviourType { EClassic, EActionBased }
	- (DONE) Enum  EPDProgressionType { EActiveEffect, EPassiveEffect, EStat}
	- (TODO) If Actionbased, 
		- Action Event Delegate signature: bool (void* OpaqueActionDataPacket){}
		- run an automated test-level before packaging to ensure an action event has been tied to the stat to increase it. 

	- (DONE) FPDStatsValue
		- TArray<int32> BaseValueRepresentations;
		- int32 BaseDivisor = 1;

	- (DONE) FPDStatsRow
		- FGameplayTag ProgressionTag;
		- TMap<FGameplayTag ///StatTag///, FGameplayTag ///RuleSetTag///> RulesAffectedBy;
		- EPDProgressionBehaviourType BehaviourType;
		- EPDProgressionType ProgressionType;
		- FPDStatsValue ProgressionValueRepresentation;
		- int32 MaxLevel = 1;
		- UCurveFloat ExperienceCurve;

	- (DONE) FPDStatMapping
		- int32 Index = 0;
	- FGameplayTag Tag;
	- bool operator ==(const int32 OtherIndex) { return this->Index == OtherIndex; }
	- bool operator ==(const FGameplayTag& OtherTag) { return this->Tag == OtherTag; }

 */

struct FPDStatList;

/** @brief Progression behaviour selector. do we earns progression via classic forms (missions, scripts, etc) or do we tie it to an action? */
UENUM()
enum class EPDProgressionBehaviourType : uint8
{
	EClassic,
	EActionBased
};

/** @brief Progression type selector : are we an active effect, a passive effect or a basic stat? */
UENUM()
enum class EPDProgressionType : uint8
{
	EActiveEffect,
	EPassiveEffect,
	EStat
};

/** @brief Stat value default data -- base value and a value progression curve (how will the value change as the stat levels up) */
USTRUCT(Blueprintable)
struct PDBASEPROGRESSION_API FPDStatsValue
{
	GENERATED_BODY()
	
	/** @brief Attempts to resolve 'ValueProgression->GetFloatValue(Level)'
	 * @details Applies the ValueProgression result for 'Level' to the BaseValue, before applying the BaseDivisor to that result and returning the final result 
	 * @note Not thread safe! Lock accesses to BaseValueRepresentations, BaseDivisor */
	double ResolveValue(int32 Level) const;
	
	/** @brief The base/default value of this stat */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 BaseValue;

	/** @brief The value modifier progression curve of this stat, different levels give different values */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UCurveFloat* ValueProgression = nullptr;	

	/** @brief
	 * @todo Needs a slider with minimum value of 1,
	 * @todo And needs a setter function that clamps any requested new value to minimum 1 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 BaseDivisor = 1;
};

/** @brief Cross-behaviour rules:
 * - Ruleset tag that tells which ruleset to use
 * - Basevalue, 'CrossBehaviourBaseValue', that will be scaled using the 'RuleSetLevelCurveMultiplier'
 */
USTRUCT(Blueprintable)
struct PDBASEPROGRESSION_API FPDStatsCrossBehaviourRules
{
	GENERATED_BODY()
	
	/** @brief This is the ruleset for this cross behaviour*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag RuleSetTag;

	/** @brief This is the level curve multiplier for this cross behaviour */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 CrossBehaviourBaseValue = 0;	
	
	/** @brief This is the level curve multiplier for this cross behaviour */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UCurveFloat* RuleSetLevelCurveMultiplier;	
};

/** @brief Base compound struct for a skill-token: Composes of a tag and a value */
USTRUCT(Blueprintable)
struct FPDSkillTokenBase
{
	GENERATED_BODY()

	/** @brief The type of token this represents. Is used when being accessed and compared against */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag TokenType{};	

	/** @brief The type of token this represents. For now is mainly used to indicate tokens in store or used tokens */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 TokenValue = 0;

	bool operator==(const FGameplayTag& OtherTokenType) const
	{
		return this->TokenType == OtherTokenType;
	}
	bool operator!=(const FGameplayTag& OtherTokenType) const
	{
		return (*this == OtherTokenType) == false;
	}	
	
	bool operator==(const FPDSkillTokenBase& Other) const
	{
		return this->TokenType == Other.TokenType;
	}
	bool operator!=(const FPDSkillTokenBase& Other) const
	{
		return (*this == Other) == false;
	}	
};

/** @brief Extension of 'FPDSkillTokenBase' which adds a 'MinTokenLevel' member, is used to gate unlocks */
USTRUCT(Blueprintable)
struct FPDSkillTokenRules : public FPDSkillTokenBase
{
	GENERATED_BODY()
	
	/** @brief Is used to gate unlocks */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MinTokenLevel = 0;	
};

/** @brief A branch in the skill-tree type. is used to define deeper depths and more branches to the tree */
USTRUCT(Blueprintable)
struct FPDSkillBranch : public FTableRowBase
{
	GENERATED_BODY()
	
	/** @brief Unlock requirements:
	 * 1. How many tokens to unlock.
	 * 2. Of which type (token category).
	 * 3. What is the minimum token level needed to be able unlock this skill
	 *     ^ This is in practice spent points in the given token category this belongs to */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FPDSkillTokenRules TokenRules{};	
	
	/** @brief Table-row handle to the root skill of the owning tree (a skill is just a fancy stat, so filter by stat tables) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta=(RowType="/Script/PDBaseProgression.PDStatsRow"))
	FDataTableRowHandle BranchRootSkill;

	/** @brief Potential branches of this skill branch */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta=(RequiredAssetDataTags="RowStructure=/Script/PDBaseProgression.PDSkillBranch"))
	TArray<FDataTableRowHandle> BranchPaths;	
};

/** @brief Skill tree datatable row-entry. Is used by datatables to define skill-trees */
USTRUCT(Blueprintable)
struct FPDSkillTree : public FTableRowBase
{
	GENERATED_BODY()
	
	/** @brief Tag of the skill tree, used for comparison and hashing purposes */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag Tag;

	/** @brief Key: Skill_Category, Value: Category_Root_Skill */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<FGameplayTag, FPDSkillBranch> Skills;
};

/** @brief Stat datatable row entry. Is used by datatables to define skill-trees */
USTRUCT(Blueprintable)
struct PDBASEPROGRESSION_API FPDStatsRow : public FTableRowBase
{
	GENERATED_BODY()
	
	/** @brief Tag of the stat-row, used for comparison and hashing purposes */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag ProgressionTag;
	/** @brief Tags that we are affected by and the rules by which we are affected. oils down to a list of tags and cross-behaviour curvess*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<FGameplayTag /*StatTag*/, FPDStatsCrossBehaviourRules> RulesAffectedBy;
	/** @brief Behaviour of this stat, classic or action-based*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EPDProgressionBehaviourType BehaviourType;
	/** @brief Stat-type. Is stat, active affect or passive effects. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EPDProgressionType ProgressionType;
	/** @brief This is the value representation of the stat, has base-value, value-progression curve and base-divisor */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FPDStatsValue Representation;
	/** @brief Max (stat-)level of this stat @todo Move into FPDStatsValue? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MaxLevel = 1;
	/** @brief Experience progression curve. Tells us how much total experience is needed increasing a level.   */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UCurveFloat* ExperienceCurve;
	/** @brief Token grant curves. Tells us how much tokens we will gain, and of which types, for each level.   */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<FGameplayTag, UCurveFloat*> TokensToGrantPerLevel;	
};

/** @brief Stat-mapping struct to keep a mapping between a tag and the stat in our replicated stat-array */
USTRUCT(Blueprintable)
struct PDBASEPROGRESSION_API FPDStatMapping
{
	GENERATED_BODY()
	
	/** @brief Index in the fastarray for the stat of this mapping */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Index = 0;

	/** @brief Tag the stat of this mapping */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag Tag;
	
	bool operator ==(const FPDStatMapping& Other) const { return this->Index == Other.Index || this->Tag == Other.Tag; }
	bool operator ==(const int32 OtherIndex) const { return this->Index == OtherIndex; }
	bool operator ==(const FGameplayTag& OtherTag) const { return this->Tag == OtherTag; } 
};


/** @brief Hash the stat mapping data, really just pass hash the tag, the index is by our design more unreliable and would */
inline uint32 GetTypeHash(const FPDStatMapping& StatMapping)
{
	uint32 Hash = 0;
	Hash = HashCombine(Hash, GetTypeHash(StatMapping.Tag));
	return Hash;
}

/** @brief Progression class definitions: Default stats/effects, granted/available skill-trees and a self-referencing class-tag
 * @note think: 'bard', 'warlock', etc  */
USTRUCT(Blueprintable)
struct FPDProgressionClassRow : public FTableRowBase
{
	GENERATED_BODY()
	
	/** @brief Tag of the progression-class, used for comparison and hashing purposes */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) 
	FGameplayTag Tag;

	/** @brief Default stats to start enabled with */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) 
	TSet<FGameplayTag> DefaultStats;

	/** @brief Default (active) status effects to start enabled with */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) 
	TSet<FGameplayTag> DefaultActiveEffects;

	/** @brief Default (passive) status effects to start enabled with */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) 
	TSet<FGameplayTag> DefaultPassiveEffects;

	/** @brief Trees granted availability to the class. A character with this class will have theses tree available of unlocking, if they meet the additional unlock requirements ofcourse  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) 
	TSet<FGameplayTag> GrantedTrees;
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
