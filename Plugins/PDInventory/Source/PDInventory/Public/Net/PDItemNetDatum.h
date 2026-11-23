/* @author: Ario Amin @ Permafrost Development. @copyright: Full BSL(1.1) License included at bottom of the file  */
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 2
#include "Net/Serialization/FastArraySerializer.h"
#else
#include "Engine/NetSerialization.h"
#endif

#include "PDItemNetDatum.generated.h"

class UPDInventoryComponent;
struct FPDItemList;

/** @brief Light version for export to AI entities mainly, but anything that requires some lightweight data and does not care
 * about stacks being seperated but instead extrapolated from the total count and the default stack limit for the given item
 * 
 * Datum size: 16 bytes
 * Extreme case, 100.000 Entities, all with with inventory. Owned by 100 total players (1.000 units each)
 * Average TX-rate: updates inventory once every two seconds
 * Average Total TX per second; (100.000 * 16 * 0.5) 800.000 bytes per second, or ~800 kilobytes = inventory data per connection
 * For any given player this will consist of about 16.000 bytes out per second, and incoming packets consist of 98% (1 - (16 / 800)) of the load
 *
 * To offset the load we could have some level of detailing for the networked data, lowering update rate at different distances away from the player 
 */
USTRUCT(BlueprintType)
struct FPDLightItemDatum
{
	GENERATED_BODY();
	
	/** @brief The tag associated with the item */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	FGameplayTag ItemTag{};
	
	/** @brief The total item count this entity carries*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 TotalItemCount = INDEX_NONE;

	// Stack-count need to be extrapolated from the subsystem
};


/** 
 * @brief Inventory system network datum struct.
 * - Contains a Tag ID, Total Item Count, and index for the last edited stack, and a map of all actual stacks.
 * - Also contains a function to export the 'FPDItemNetDatum' as a 'FPDLightItemDatum'
 * 
 * Datum size: 112 bytes
 * Extreme case, 1000 players + 1000 NPCs, all with with inventory
 * Average TX-rate: 4 updates per second per inventory
 * Average Total TX per second; (448.000 * 2) 896,000 bytes per second, or ~900 kilobytes = inventory data per connection
 * For any given player this will consist of about 112 bytes out per second, and incoming packets consist of 99% of the load
 */
USTRUCT(BlueprintType)
struct FPDItemNetDatum : public FFastArraySerializerItem 
{
	GENERATED_BODY();

	// ConstructInPlace
	FPDItemNetDatum(){};
	FPDItemNetDatum(const FGameplayTag& InItemTag, int32 StackIndex, int32 InCount);
	FPDItemNetDatum(const FGameplayTag& InItemTag, int32 InCount);

	/** @brief This is called on the client when they receive a replicated update.
	 * Calls into the owners (receivers) inventory components 'OnDatumUpdate(this, EPDItemNetOperation::REMOVEALL)' */
	void PreReplicatedRemove(const FPDItemList& OwningList);
	/** @brief This is called on the client when they receive a replicated update.
	 * Calls into the owners (receivers) inventory components 'OnDatumUpdate(this, EPDItemNetOperation::ADDNEW)' */
	void PostReplicatedAdd(const FPDItemList& OwningList);
	/** @brief This is called on the client when they receive a replicated update.
	 * Calls into the owners (receivers) inventory components 'OnDatumUpdate(this, EPDItemNetOperation::CHANGE)' */
	void PostReplicatedChange(const FPDItemList& OwningList);
	
	/** @brief The tag associated with the item */
	UPROPERTY(BlueprintReadWrite)
	FGameplayTag ItemTag{};

	/** @brief the last edited stack.*/
	UPROPERTY(BlueprintReadWrite)
	int32 LastEditedStackIndex = INDEX_NONE;
	
	/** @brief The total item count this entity carries */
	UPROPERTY(BlueprintReadWrite)
	int32 TotalItemCount = INDEX_NONE;
	
	/** @brief Map of the players actual stacks
	 * @todo Might need to replace this, t-map is too weighty, an array could do with some index caching */
	UPROPERTY(BlueprintReadWrite)
	TMap<int32 /* StackIndex */, int32 /* ItemCount */> Stacks{};

	/** @brief Export data in the form of a 'FPDLightItemDatum' */
	FPDLightItemDatum ExportLight() const { return FPDLightItemDatum{ItemTag, TotalItemCount}; }

	// I'll avoid a full comparison for now, @todo Full comparison
	/** @brief Partial equality comparison */
	bool operator==(const FPDItemNetDatum& Other) const { return this->ItemTag == Other.ItemTag && this->TotalItemCount == Other.TotalItemCount; }
	/** @brief Partial inequality comparison */
	bool operator!=(const FPDItemNetDatum& Other) const { return (*this == Other) == false; }

	/** @brief Minimal equality comparison */
	bool operator==(const FGameplayTag& Tag) const { return Tag == ItemTag; }
	/** @brief Minimal inequality comparison */
	bool operator!=(const FGameplayTag& Tag) const { return Tag != ItemTag; }
	
};

/** @brief The item list fastarray serializer */
USTRUCT(BlueprintType)
struct FPDItemList : public FFastArraySerializer 
{
	GENERATED_USTRUCT_BODY()

	/** @brief Only calls into 'FFastArraySerializer::NetSerialize' for now, reserved for later use */
	bool NetSerialize(FNetDeltaSerializeInfo& DeltaParams);
	
	/** @brief Clears all items and stacks of the given type from the 'Items' array*/
	bool RemoveAllItemsOfType(FGameplayTag& ItemToRemove);
	/** @brief Removes a stack of the given type alongside the stacks given index from the 'Items' array*/
	bool RemoveStack(FGameplayTag& ItemToRemove, int32 StackIdx);

	/** @brief Adds an item of given type and amount to the 'Items' array*/
	bool UpdateItem(FGameplayTag& ItemToUpdate, int32 AmountToAdd);
	/** @brief Adds an item of given type and amount to the 'Items' array at the specified index */
	bool UpdateItemAtStackIdx(FGameplayTag& ItemToUpdate, int32 StackIdx, int32 AmountToAdd);

	/** @brief Return the inventory that owns this fastarray */
	FORCEINLINE UPDInventoryComponent* GetOwningInventory() const { return OwningInventory; }
	/** @brief Assign the inventory that should own this fastarray */
	FORCEINLINE void SetOwningInventory(UPDInventoryComponent* InInventory) { OwningInventory = InInventory; }

private:
	/** @brief Internal private function that performs removal of an item */
	bool _Remove(FGameplayTag& ItemToUpdate, int32& AmountToAdd);
	/** @brief Internal private function that performs addition of an item */
	bool _Add(FGameplayTag& ItemToUpdate, int32 StackIdx, int32& AmountToAdd);

public:
	/** @brief The underlying TArray data which the fastarray class is operating on */
	UPROPERTY()
	TArray<FPDItemNetDatum> Items;

	/** @brief A non replicated, local, mapping of an item tag and it's index in the underlying 'Items' array
	 * @note do not replicate, this is local */
	UPROPERTY(NotReplicated)
	TMap<FGameplayTag, int32> ItemToIndexMapping;
	
protected:
	/** @brief The owning inventory component, if it has been assigned. */
	UPROPERTY()
	UPDInventoryComponent* OwningInventory = nullptr;
};

/** @brief Defining struct type and marking it a being able to replicate */
template<>
struct TStructOpsTypeTraits<FPDItemList> : public TStructOpsTypeTraitsBase2<FPDItemList>
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
