/* @author: Ario Amin @ Permafrost Development. @copyright: Full BSL(1.1) License included at bottom of the file  */

#include "Net/PDItemNetDatum.h"
#include "Components/PDInventoryComponent.h"

#include "PDItemCommon.h"
#include "PDInventorySubsystem.h"

static TMap<int32,int32> ConstructTMapInPlace(int32 StackIdx, int32 Count)
{
	TMap<int32,int32> ConstructedMap;
	ConstructedMap.Emplace(StackIdx) = Count;
	return ConstructedMap;
}

FPDItemNetDatum::FPDItemNetDatum(const FGameplayTag& InItemTag, int32 StackIndex, int32 InCount)
	: ItemTag(InItemTag), LastEditedStackIndex(StackIndex), TotalItemCount(InCount), Stacks(ConstructTMapInPlace(StackIndex, InCount))
{
}

FPDItemNetDatum::FPDItemNetDatum(const FGameplayTag& InItemTag, int32 InCount)
	: ItemTag(InItemTag), LastEditedStackIndex(0), TotalItemCount(InCount), Stacks(ConstructTMapInPlace(0, InCount))
{
}

void FPDItemNetDatum::PreReplicatedRemove(const FPDItemList& OwningList)
{
	check(OwningList.GetOwningInventory() != nullptr)
	OwningList.GetOwningInventory()->OnDatumUpdated(this, EPDItemNetOperation::REMOVEALL);
}

void FPDItemNetDatum::PostReplicatedAdd(const FPDItemList& OwningList)
{
	check(OwningList.GetOwningInventory() != nullptr)
	OwningList.GetOwningInventory()->OnDatumUpdated(this, EPDItemNetOperation::ADDNEW);
	OwningList.ItemToIndexMapping;
	ReplicationID;
}

void FPDItemNetDatum::PostReplicatedChange(const FPDItemList& OwningList)
{
	check(OwningList.GetOwningInventory() != nullptr)
	OwningList.GetOwningInventory()->OnDatumUpdated(this, EPDItemNetOperation::CHANGE);
}

bool FPDItemList::NetSerialize(FNetDeltaSerializeInfo& DeltaParams)
{
	return FFastArraySerializer::FastArrayDeltaSerialize<FPDItemNetDatum, FPDItemList>(Items, DeltaParams, *this);
}

bool FPDItemList::RemoveAllItemsOfType(FGameplayTag& ItemToRemove)
{
	if (ItemToIndexMapping.Contains(ItemToRemove) == false) { return false; }

	MARK_PROPERTY_DIRTY_FROM_NAME(UPDInventoryComponent, ItemList, OwningInventory)
	FPDItemNetDatum& NetDatum = Items[ItemToIndexMapping.FindRef(ItemToRemove)];
	NetDatum.Stacks.Empty(); // clear data
	NetDatum.LastEditedStackIndex = 0;
	NetDatum.TotalItemCount = 0;
	
	return true;
}

bool FPDItemList::RemoveStack(FGameplayTag& ItemToRemove, int32 StackIdx)
{
	MARK_PROPERTY_DIRTY_FROM_NAME(UPDInventoryComponent, ItemList, OwningInventory)
	FPDItemNetDatum& NetDatum = Items[ItemToIndexMapping.FindRef(ItemToRemove)];

	const int32 CopyOldValue = NetDatum.Stacks[StackIdx];
	NetDatum.Stacks[StackIdx] = 0; // clear data
	NetDatum.LastEditedStackIndex = StackIdx;
	NetDatum.TotalItemCount -= CopyOldValue;
	
	return true;
}

bool FPDItemList::UpdateItem(FGameplayTag& ItemToUpdate, int32 AmountToAdd)
{
	if (ItemToIndexMapping.Contains(ItemToUpdate))
	{
		const FPDItemNetDatum& Item = Items[ItemToIndexMapping.FindRef(ItemToUpdate)]; // Stacks;
		return UpdateItemAtStackIdx(ItemToUpdate, Item.LastEditedStackIndex, AmountToAdd);
	}

	return UpdateItemAtStackIdx(ItemToUpdate, INDEX_NONE, AmountToAdd);
}

bool FPDItemList::_Remove(FGameplayTag& ItemToUpdate, int32& AmountToAdd)
{
	FPDItemNetDatum& Item = Items[ItemToIndexMapping.FindRef(ItemToUpdate)]; // Stacks;
	// remove all items if that is the request
	if( Item.TotalItemCount <= FMath::Abs(AmountToAdd))
	{
		return RemoveAllItemsOfType(ItemToUpdate);
	}

	// Tally how many stack we otherwise might need to remove
	TArray<int32> StackIndicesToRemove{};
	for (const TPair<int32,int32>& Stack : Item.Stacks)
	{
		if (Stack.Value <= FMath::Abs(AmountToAdd))
		{
			AmountToAdd += Stack.Value; // AmountToAdd is negative at this point
			StackIndicesToRemove.Emplace(Stack.Key);
			continue;
		}
		Item.LastEditedStackIndex = Stack.Key;
		break;
	}

	// Removed tallied stacks
	for (int32 StackIdxToRemove : StackIndicesToRemove)
	{
		RemoveStack(ItemToUpdate, StackIdxToRemove);
	}
			
	MARK_PROPERTY_DIRTY_FROM_NAME(UPDInventoryComponent, ItemList, OwningInventory)
	int32& StackValue = Item.Stacks[Item.LastEditedStackIndex];
	if (Item.Stacks[Item.LastEditedStackIndex] > FMath::Abs(AmountToAdd))
	{
		StackValue += AmountToAdd;
	}
	return true;
}

bool FPDItemList::_Add(FGameplayTag& ItemToUpdate, int32 StackIdx, int32& AmountToAdd)
{
	MARK_PROPERTY_DIRTY_FROM_NAME(UPDInventoryComponent, ItemList, OwningInventory)
	FPDItemNetDatum& Item = Items[ItemToIndexMapping.FindRef(ItemToUpdate)]; // Stacks;
	const UPDInventorySubsystem* InvSubsystem = UPDInventorySubsystem::Get();
	check(InvSubsystem != nullptr); // Should never be nullptr
	
	int32 ItemsInLastEditedStack = 0;
	if (Item.Stacks.Contains(StackIdx))
	{
		ItemsInLastEditedStack = Item.Stacks[Item.LastEditedStackIndex];
	}
		
	const FPDItemDefaultDatum* Datum = InvSubsystem->TagToItemMap.FindRef(ItemToUpdate);
	const int32 Space = Datum->StackLimit - ItemsInLastEditedStack;
		
	TMap<int32, int32>& CurrentStacks = Item.Stacks;
	const bool bFitsInCurrentStack = Space >= AmountToAdd;
	if (bFitsInCurrentStack)
	{
		Item.TotalItemCount += AmountToAdd; 
		*(CurrentStacks.Find(StackIdx)) += AmountToAdd;
		if (Space == AmountToAdd)// next add should target a new stack
		{
			Item.LastEditedStackIndex++; // increment last edited stack and use it for the new key
			const FPDItemNetDatum NetDatum{ItemToUpdate, Item.LastEditedStackIndex, 0};
			ItemToIndexMapping.Emplace(ItemToUpdate) = Items.Add(NetDatum);
		}
			
		return true;
	}
	
	// split
	AmountToAdd -= Space;
	*(CurrentStacks.Find(StackIdx)) += Space;
	OwningInventory->Stacks.Current++;
			

	// Final added count in last index
	const int32 AppendedStackRemainder = AmountToAdd != Datum->StackLimit ? AmountToAdd % Datum->StackLimit : AmountToAdd;
	// find how many stacks we need to create
	const int32 AppendedStackCount = AmountToAdd / Datum->StackLimit;
	for (int32 Step = 0; Step < AppendedStackCount; Step++)
	{
		Item.LastEditedStackIndex++;
		CurrentStacks.Emplace(Item.LastEditedStackIndex) = AmountToAdd;
		OwningInventory->Stacks.Current++;
	}
	Item.LastEditedStackIndex++;
	CurrentStacks.Emplace(Item.LastEditedStackIndex) = AppendedStackRemainder;
	return true;
}

bool FPDItemList::UpdateItemAtStackIdx(FGameplayTag& ItemToUpdate, int32 StackIdx, int32 AmountToAdd)
{
	// @todo when adding an item, map it's tag to array IDX for fast editing of the array after, without having to search for it
	const UPDInventorySubsystem* InvSubsystem = UPDInventorySubsystem::Get();
	check(InvSubsystem != nullptr); // Should never be nullptr

	if (InvSubsystem->TagToItemMap.Contains(ItemToUpdate) == false)
	{
		const FString BuildString = "FPDItemList::AddItem -- "
		+ FString::Printf(TEXT("\n Trying to add item with invalid tag (%s). No given ItemTables have this tag on any given entry "), *ItemToUpdate.GetTagName().ToString());
		UE_LOG(PDLog_Inventory, Warning, TEXT("%s"), *BuildString);

		return false;
	}

	// Check if item already exists, if true check its' last edited stack
	if (ItemToIndexMapping.Contains(ItemToUpdate))
	{

		//
		// Removing or Adding items
		const bool bSubtraction = AmountToAdd < 0; // Want to remove items, possibly full stacks
		 return bSubtraction
			? _Remove(ItemToUpdate, AmountToAdd)
			: _Add(ItemToUpdate, StackIdx, AmountToAdd);
		
	}

	MARK_PROPERTY_DIRTY_FROM_NAME(UPDInventoryComponent, ItemList, OwningInventory)
	const FPDItemNetDatum NetDatum{ItemToUpdate, 0, AmountToAdd};
	ItemToIndexMapping.Emplace(ItemToUpdate) = Items.Add(NetDatum);
	return true;	
}
// @todo when Item.LastEditedStackIndex reaches close to it's max representation, wrap it around and make sure there are no collisions on the way

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
