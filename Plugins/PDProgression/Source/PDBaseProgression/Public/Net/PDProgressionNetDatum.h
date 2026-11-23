/* @author: Ario Amin @ Permafrost Development. @copyright: Full BSL(1.1) License included at bottom of the file  */

#pragma once

#include "CoreMinimal.h"
#include "GameplayTags.h"
#include "PDProgressionCommon.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "PDProgressionNetDatum.generated.h"

/** @brief Replicated datum. This is the definition of a 'packet' of data we transmit per entry of a given 'fastarray'
 * @note Replicates values that represent the stats tag, it's current crossbehaviour value, it's current level and experience
 * @note Also has some helper functions to process and apply the crossbehaviour to the current stat value
 * @todo Make some variant or change that allows some stats to never update, and thus not change, so we avoid over-saturating the network
 */
USTRUCT(Blueprintable)
struct PDBASEPROGRESSION_API FPDStatNetDatum : public FFastArraySerializerItem
{
	GENERATED_BODY()
	
	/** @brief The tag of this stat. Is needed to track the different values that stat system handles */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag ProgressionTag;

	/** @brief The current crossbehaviour value, the current values of others stats total effect on this stat */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 CrossBehaviourValue = 0;	
	
	/** @brief The current level of the stat. Is needed for tracking and to resolve values from the expected crossbehaviour and token curves */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 CurrentLevel = 0;
	
	/** @brief The current total experience of the stat. Is needed for validation: resolving the expected level at this amount of total experience */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 CurrentExperience = 0;

	/** @brief The stat value of the stat, purpose is up to the stat definition. This is what the crossbehaviour value targets */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	double CurrentStatValue;

	/** @brief Processes and returns the current stat value with the crossbehaviour applied */
	double GetAppliedValue() const;

	/** @brief Processes the crossbehaviour.
	 * @note This means it applies this stats basedivisor to the current crossbehaviour,
	 * which is defined in the stats row in the stat-table it is contained within */
	double GetProcessedCrossBehaviour() const;	
	
	/** @brief This is called on the client when they receive a replicated update.
	 * Calls into the owner 'OnDatumUpdate(this, EPDStatNetOperation::REMOVE)'
	 * @note checks with an assertion if OwningList.OwningObject is valid */
	void PreReplicatedRemove(const FPDStatList& OwningList);
	/** @brief This is called on the client when they receive a replicated update.
	 * Calls into the owner 'OnDatumUpdate(this, EPDStatNetOperation::ADDNEW)'
	 * @note checks with an assertion if OwningList.OwningObject is valid */
	void PostReplicatedAdd(const FPDStatList& OwningList);
	/** @brief This is called on the client when they receive a replicated update.
	 * Calls into the owner 'OnDatumUpdate(this, EPDStatNetOperation::CHANGE)'
	 * @note checks with an assertion if OwningList.OwningObject is valid */
	void PostReplicatedChange(const FPDStatList& OwningList);
};

/** @brief Fastaarray boilerplate.
 * @note Keeps an actual array of items and mark it in NetSerialize so the replication system knows it should treat it differently */
USTRUCT(Blueprintable)
struct PDBASEPROGRESSION_API FPDStatList : public FFastArraySerializer
{
	GENERATED_BODY()
	
	/** @brief Marks 'Items' so the replication system knows it should treat it differently  */
	bool NetSerialize(FNetDeltaSerializeInfo& DeltaParams);

	/** @note Adds any given stat to the itemlist */
	void AddStat(const FGameplayTag& StatTag, int32 StatExperience, int32 StatLevel);
	/** @note Just calls AddStat, is a placeholder impl. reserved for later use */
	void AddActiveEffect(const FGameplayTag& StatTag, int32 StatExperience, int32 StatLevel);
	/** @note Just calls AddStat, is a placeholder impl. reserved for later use */
	void AddPassiveEffect(const FGameplayTag& StatTag, int32 StatExperience, int32 StatLevel);

	/** @brief Actual inner list of items that we want to replicate */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FPDStatNetDatum> Items;
};

/** @brief Engine boilerplate for fastarrays */
template<>
struct TStructOpsTypeTraits<FPDStatList> : TStructOpsTypeTraitsBase2<FPDStatList>
{
	enum
	{
		WithNetDeltaSerialize = true,
		
	};

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
