/* @author: Ario Amin @ Permafrost Development. @copyright: Full BSL(1.1) License included at bottom of the file  */

#pragma once

#include "CoreMinimal.h"
#include "Net/PDProgressionNetDatum.h"
#include "PDProgressionComponent.generated.h"

struct FPDStatMapping;
struct FGameplayTag;

/** @brief Key mapping comparison functions, used  for tset searching  */
struct FPDStatKeyFuncs : BaseKeyFuncs<FPDStatMapping, FGameplayTag, false>
{
	/** @brief Returns what the t-set should consider the actual key of FPDStatMapping  */
	static const FGameplayTag& GetSetKey(const FPDStatMapping& Element) { return Element.Tag; }
	/** @brief Comparison function used by the t-set   */
	static bool Matches(const FGameplayTag& A, const FGameplayTag& B) { return A == B; }
	/** @brief Gets inners typehash of the key, we want FPDStatMapping's with differing indexes and same tag to still match */
	static uint32 GetKeyHash(const FGameplayTag& Key) { return GetTypeHash(Key); }
};


/** @brief Progression system network manager */
UCLASS(Blueprintable)
class PDBASEPROGRESSION_API UPDStatHandler : public UActorComponent
{
	GENERATED_BODY()

public:

	/** @brief Registers this StatHandler with the UPDStatSubsystem  */
	virtual void BeginPlay() override;

	/** @brief Finds the stat and returns it's applied value, if it exists.
	 *  @details If it exists then return the applied stat value, otherwise return 0*/
	UFUNCTION(BlueprintCallable)
	double GetStatValue(const FGameplayTag& StatTag);
	
	/** @brief Finds the stat and increases it's level by the given amount, if the owner has complete authority .
	 *  @details Increases levels and propagates applied changes to other stats that that levelled up stat is set to modify, in the stats 'AffectedBy' array*/
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void IncreaseStatLevel(const FGameplayTag& StatTag, int32 LevelDelta = 1);

	/** @brief Finds the stat and increases it's experience by the given amount, if the owner has complete authority .
	 *  @details Increases experience and in-case we increase level it calls IncreaseStatLevel*/
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void IncreaseStatExperience(const FGameplayTag& StatTag, int32 ExperienceDelta);	

	/** @brief Finds a skill stat and attempts to unlock it, if the owner has complete authority .
	 *  @details Sets a skill stat to level 1, fails if we do not have the necessary tokens to unlock it
	 *  @todo Apply skill effects (Also, consider allowing stats to have stat tied to them, stats that unlock and activate upon this stat unlocking)*/
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void AttemptUnlockSkill(const FGameplayTag& SkillTag);

	/** @brief Finds a skill stat and attempts to lock it, if the owner has complete authority .
	 *  @details Sets a skill stat to level 0 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void AttemptLockSkill(const FGameplayTag& SkillTag);	
	
	/** @brief Used to fill the  */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void SetClass(const FGameplayTag& ClassTag);

	/** @brief Called by unlock and lock skill, handles the actual logic. See@LockSkill & @UnlockSkill  */
	void ModifySkill(const FGameplayTag& SkillTag, bool bUnlock);
	/** @brief Check owner net-mode/role. If we have appropriate authority in the correct context then return true. 
	 * @details - Returns true if we are standalone or server and we have authority.
	 * @details - Return false proxy or max/none owner roles. and if NetMode is NM_Client*/
	bool HasCompleteAuthority() const;

	/** @brief Grants the owner all tokens granted between the current level and the new level  */
	UFUNCTION(BlueprintCallable)
	void GrantTokens(const FGameplayTag& StatTag, int32 LevelDelta, int32 CurrentLevel);

	/** @brief Boiler plate for replication setup */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	/** @brief The replicated list of stats, fastarray serializer list as it is expected to have many elements.
	 * Indices are mapped locally to a gameplay tag so we can reduce search complexity drastically */
	UPROPERTY(Replicated)
	FPDStatList StatList{};

	/** @brief Replicated list of tokens, a regular replicated array is fine -- expecting small number of entries
	 * Will arguably never be above 50 entries in practice, even in a game like PoE,
	 * very likely even below 10 in most games */
	UPROPERTY(Replicated)
	TArray<FPDSkillTokenBase> Tokens;

	/** @brief Total tokens spent so-far, keep for tracking and comparison purposes.
	 * @note FPDSkillTokenBase keeps a tag and a number */
	UPROPERTY(Replicated)
	TArray<FPDSkillTokenBase> TokensSpentTotal; // Will arguably never be above 100 entries in practice, very likely even below 10 in many games

	/** @brief Persistent ID of the owner, used for routing and mapping  */
	UPROPERTY()
	int32 OwnerID; 
	
	/** @brief Locally tracked set of tags and indices in 'StatList',
	 * @note has a custom key matching function so we can resolve an FPDStatMapping::index from a given tag */
	TSet<FPDStatMapping, FPDStatKeyFuncs> LocalStatMappings;
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
