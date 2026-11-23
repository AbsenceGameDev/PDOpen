/* @author: Ario Amin @ Permafrost Development. @copyright: Full BSL(1.1) License included at bottom of the file  */

#include "AI/Mass/RTSOMassFragments.h"
#include "Components/PDInventoryComponent.h"

void FRTSOLightInventoryFragment::ClearItems()
{
	Inner.Empty();
}

int32 FRTSOLightInventoryFragment::CalculateItemTransfer(const FGameplayTag& ResourceTag, int32 MaxRequestedCount)
{
	if (false == Inner.Contains(ResourceTag)) {return INDEX_NONE;}

	const int32 CountAfterTransaction = Inner[ResourceTag].TotalItemCount - MaxRequestedCount;
	const bool bCanAfford = CountAfterTransaction >= 0;
	return bCanAfford 
		? MaxRequestedCount
		: MaxRequestedCount + CountAfterTransaction;	
}


FString FRTSOLightInventoryFragment::TransferItems(const FRTSOLightInventoryFragment& MaxItemsRequested, FRTSOLightInventoryFragment& TargetFragment, bool bLogAction)
{
	FString RetVal;
	for (const auto&[ResourceTag, MaxRequestedItem] : MaxItemsRequested.Inner)
	{
		const int32 FinalTransferCount = CalculateItemTransfer(ResourceTag, MaxRequestedItem.TotalItemCount);
		TargetFragment.AddItem(ResourceTag, FinalTransferCount);
		RemoveItem(ResourceTag, FinalTransferCount);

		if (bLogAction) 
		{
			FString StrippedResourceName = ResourceTag.ToString();
			StrippedResourceName = StrippedResourceName.RightChop(StrippedResourceName.Find(TEXT("."), ESearchCase::IgnoreCase, ESearchDir::FromEnd));
			RetVal += FString::Printf(TEXT("(%i : %s),"), FinalTransferCount, *StrippedResourceName);
		}
	}

	return RetVal;
}

FString FRTSOLightInventoryFragment::TransferItems(const FRTSOLightInventoryFragment& MaxItemsRequested, UPDInventoryComponent& TargetInventory, bool bLogAction)
{
	FString RetVal = bLogAction ? TEXT("Transfered Items:") : FString{};
	for (auto&[ResourceTag, MaxRequestedItem] : MaxItemsRequested.Inner)
	{
		const int32 FinalTransferCount = CalculateItemTransfer(ResourceTag, MaxRequestedItem.TotalItemCount);
		TargetInventory.RequestUpdateItem(EPDItemNetOperation::CHANGE, ResourceTag, FinalTransferCount);
		RemoveItem(ResourceTag, FinalTransferCount);
	}

	return RetVal;
}


void FRTSOLightInventoryFragment::TransferItems(FRTSOLightInventoryFragment& OtherFragment)
{
	for (TTuple<FGameplayTag, FPDLightItemDatum>& Item : Inner)
	{
		OtherFragment.Inner.FindOrAdd(Item.Key).TotalItemCount += Item.Value.TotalItemCount;
		Item.Value.TotalItemCount = 0;
	}
	
	// OtherFragment.Inner = Inner; // Some leftover from ages ago, no we should not overwrite OtherFragment without care
}

void FRTSOLightInventoryFragment::TransferItems(UPDInventoryComponent& OtherInventory)
{
	for (TTuple<FGameplayTag, FPDLightItemDatum>& Item : Inner)
	{
		OtherInventory.RequestUpdateItem(EPDItemNetOperation::CHANGE, Item.Key, Item.Value.TotalItemCount);
		Item.Value.TotalItemCount = 0;
	}	
}

void FRTSOLightInventoryFragment::AddItems(const TArray<TTuple<FGameplayTag, FPDLightItemDatum>>& AppendList)
{
	for (const TTuple<FGameplayTag, FPDLightItemDatum>& AppendItem : AppendList)
	{
		Inner.FindOrAdd(AppendItem.Key).TotalItemCount += AppendItem.Value.TotalItemCount;
	}	
}

void FRTSOLightInventoryFragment::RemoveItems(const TArray<TTuple<FGameplayTag, FPDLightItemDatum>>& RemoveList)
{
	for (const TTuple<FGameplayTag, FPDLightItemDatum>& RemoveItem : RemoveList)
	{
		if (Inner.Contains(RemoveItem.Key) == false) { continue; }
		
		auto& [ItemTag, TotalItemCount] = Inner.FindChecked(RemoveItem.Key); 
		TotalItemCount -= (RemoveItem.Value.TotalItemCount >= TotalItemCount) ? TotalItemCount : RemoveItem.Value.TotalItemCount; 
	}		
}

void FRTSOLightInventoryFragment::AddItems(const TMap<FGameplayTag, FPDLightItemDatum>& AppendList)
{
	for (const TTuple<FGameplayTag, FPDLightItemDatum>& AppendItem : AppendList)
	{
		Inner.FindOrAdd(AppendItem.Key).TotalItemCount += AppendItem.Value.TotalItemCount;
	}			
}

void FRTSOLightInventoryFragment::RemoveItems(const TMap<FGameplayTag, FPDLightItemDatum>& RemoveList)
{
	for (const TTuple<FGameplayTag, FPDLightItemDatum>& RemoveItem : RemoveList)
	{
		if (Inner.Contains(RemoveItem.Key) == false) { continue; }
		
		auto& [ItemTag, TotalItemCount] = Inner.FindChecked(RemoveItem.Key); 
		TotalItemCount -= (RemoveItem.Value.TotalItemCount >= TotalItemCount) ? TotalItemCount : RemoveItem.Value.TotalItemCount; 
	}	
}

void FRTSOLightInventoryFragment::AddItem(const FPDLightItemDatum& AppendItem)
{
	Inner.FindOrAdd(AppendItem.ItemTag).TotalItemCount += AppendItem.TotalItemCount;
}

// We never actually remove and item, we may set it to zero and count it as not being here
void FRTSOLightInventoryFragment::RemoveItem(const FPDLightItemDatum& RemoveItem)
{
	if (Inner.Contains(RemoveItem.ItemTag) == false) { return;; }
		
	auto& [ItemTag, TotalItemCount] = Inner.FindChecked(RemoveItem.ItemTag); 
	TotalItemCount -= (RemoveItem.TotalItemCount >= TotalItemCount) ? TotalItemCount : RemoveItem.TotalItemCount; 
}

void FRTSOLightInventoryFragment::AddItem(const FGameplayTag& AddTag, const int32 Count)
{
	FPDLightItemDatum& Item = Inner.Contains(AddTag) == false ? Inner.Add(AddTag) : *Inner.Find(AddTag);
	Item.ItemTag = AddTag;
	Item.TotalItemCount = FMath::Max(Count, Item.TotalItemCount + Count);
}

void FRTSOLightInventoryFragment::RemoveItem(const FGameplayTag& RemoveTag, const int32 Count)
{
	if (Inner.Contains(RemoveTag) == false) { return;; }
		
	auto& [ItemTag, TotalItemCount] = Inner.FindChecked(RemoveTag); 
	TotalItemCount -= (Count >= TotalItemCount) ? TotalItemCount : Count;
}

int32 FRTSOLightInventoryFragment::GetItemCount(const FGameplayTag& Key) const
{
	const FPDLightItemDatum* DatumPtr = Inner.Find(Key);
	return DatumPtr ? DatumPtr->TotalItemCount : 0;
}

bool FRTSOLightInventoryFragment::IsEmpty() const
{
	for (const TTuple<FGameplayTag, FPDLightItemDatum>& Item : Inner)
	{
		if (Item.Value.TotalItemCount > 0)
		{
			return false;
		}
	}
	
	return true;
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
