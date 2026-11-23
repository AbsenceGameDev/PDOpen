/* @author: Ario Amin @ Permafrost Development. @copyright: Full BSL(1.1) License included at bottom of the file  */
#pragma once

#include "CoreMinimal.h"
#include "MassEntityTypes.h"
#include "PDItemCommon.h"
#include "AI/Mass/PDMassFragments.h"
#include "Net/PDItemNetDatum.h"
#include "RTSOMassFragments.generated.h"

class UPDInventoryComponent;
class UAnimToTextureDataAsset;


/**
 * @brief The entities inventory fragment, in lieu of a full inventory component.
 * @note FPDItemNetDatum has a function to export its values to this type of structure to allow for some interoperability between actors and entities inventories
 * @note this handles all logic pertaining to a given FRTSOLightInventoryFragment
 */
USTRUCT(BlueprintType)
struct FRTSOLightInventoryFragment : public FMassFragment
{
	GENERATED_BODY();
	
	FRTSOLightInventoryFragment() {};
	
	/** @brief If we have nothing in our list at all we count  */
	bool HasFiniteSpace() 
	{
		return false;
	}

	/** @brief Assigns the other fragments inner to our inner. then overwrite the handler with a new handler bound to the new inner */
	FRTSOLightInventoryFragment& operator=(const FRTSOLightInventoryFragment& Other)
	{
		Inner = TMap<FGameplayTag, FPDLightItemDatum>(Other.Inner);
		// Handler = FRTSOLightInventoryFragmentHandler(Inner);
		return *this;
	}


	/** @brief Clear the items this fragment holds */
	void ClearItems();

	/** @brief Returns how many of the item we are able to transfer to the targer */
	int32 CalculateItemTransfer(const FGameplayTag& ResourceTag, int32 MaxRequestedCount);

	/** @brief Transfer selection of items from the bound fragment to a target fragment */
	FString TransferItems(const FRTSOLightInventoryFragment& MaxItemsRequested, FRTSOLightInventoryFragment& TargetFragment, bool bLogAction = false);
	/** @brief Transfer selection of items from the bound fragment to a inventory component */
	FString TransferItems(const FRTSOLightInventoryFragment& MaxItemsRequested, UPDInventoryComponent& TargetInventory, bool bLogAction = false);
	
	/** @brief Transfer items from the bound fragment to a target fragment */
	void TransferItems(FRTSOLightInventoryFragment& OtherFragment);
	/** @brief Transfer items from the bound fragment to a target inventory component */
	void TransferItems(UPDInventoryComponent& OtherInventory);
	
	/** @brief Add the input list to the bound item list, using a TArray<TTuple<FGameplayTag, FPDLightItemDatum>> container */
	void AddItems(const TArray<TTuple<FGameplayTag, FPDLightItemDatum>>& AppendList);
	/** @brief Removes the input list to the bound item list, using a TArray<TTuple<FGameplayTag, FPDLightItemDatum>> container  */
	void RemoveItems(const TArray<TTuple<FGameplayTag, FPDLightItemDatum>>& RemoveList);

	/** @brief Add the input list to the bound item list, using a TMap<FGameplayTag, FPDLightItemDatum> container */
	void AddItems(const TMap<FGameplayTag, FPDLightItemDatum>& AppendList);
	/** @brief Removes the input list to the bound item list, using a TMap<FGameplayTag, FPDLightItemDatum> container  */
	void RemoveItems(const TMap<FGameplayTag, FPDLightItemDatum>& RemoveList);

	/** @brief Add singular item to the fragments item list, using FPDLightItemDatum */
	void AddItem(const FPDLightItemDatum& AppendItem);
	/** @brief Remove singular item to the fragments item list, using FPDLightItemDatum  */
	void RemoveItem(const FPDLightItemDatum& RemoveItem);

	/** @brief Add singular item to the fragments item list, using item tag and item count */
	void AddItem(const FGameplayTag& AddTag, const int32 Count);
	/** @brief Remove singular item to the fragments item list, using item tag and item count */
	void RemoveItem(const FGameplayTag& RemoveTag, const int32 Count);

	/** @brief Gets item count, via tag */
	int32 GetItemCount(const FGameplayTag& Key) const;
	const TMap<FGameplayTag, FPDLightItemDatum>& GetItems() const { return Inner;};

	bool IsEmpty() const;


protected:
	/** @brief Inner/Item list, keyed by item tag, value by actual item datum */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (AllowPrivateAccess="true"))
	TMap<FGameplayTag, FPDLightItemDatum> Inner{};
};

USTRUCT(BlueprintType)
struct FRTSOInventoryDefaultRow : public FTableRowBase
{
	GENERATED_BODY();

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FRTSOLightInventoryFragment InventoryConfig{};

};

/** @brief Base and extended classes are unused for now, will likely remove fully. Reserved Subclass */
USTRUCT()
struct RTSOPEN_API FRTSOFragment_SimpleMovement : public FPDMFragment_SimpleMovement
{
	GENERATED_BODY();
};


/**
 * @brief Base fragment for RTS Agents. Reserved Subclass
 * @note extended class unused for now, will likely remove
 */
USTRUCT(BlueprintType)
struct RTSOPEN_API FRTSOFragment_Agent : public FPDMFragment_RTSEntityBase
{
	GENERATED_BODY()
};

/**
 * @brief Base fragment for RTS Agents. Reserved Subclass
 * @note extended class unused for now, will likely remove
 */
USTRUCT()
struct RTSOPEN_API FRTSOFragment_Animation : public FPDMFragment_EntityAnimation
{
	GENERATED_BODY()
};

/**
 * @brief Fragment given to entities to perform actions. Reserved Subclass
 * @note extended class is unused for now, will likely remove
 */
USTRUCT()
struct RTSOPEN_API FRTSOFragment_Action : public FPDMFragment_Action
{
	GENERATED_BODY();
};

/**
 * @brief In case all agents share animation data, use this fragment. Reserved Subclass 
 */
USTRUCT()
struct RTSOPEN_API FRTSOFragment_SharedAnimData : public FPDMFragment_SharedAnimData
{
	GENERATED_BODY()
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