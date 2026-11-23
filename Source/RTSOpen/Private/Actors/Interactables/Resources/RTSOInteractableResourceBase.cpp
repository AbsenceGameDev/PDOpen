/* @author: Ario Amin @ Permafrost Development. @copyright: Full BSL(1.1) License included at bottom of the file  */
#include "Actors/Interactables/Resources/RTSOInteractableResourceBase.h"

#include "MassEntitySubsystem.h"
#include "PDInventorySubsystem.h"
#include "PDRTSBaseSubsystem.h"
#include "PDItemCommon.h"
#include "PDRTSCommon.h"
#include "AI/Mass/RTSOMassFragments.h"
#include "Components/PDInventoryComponent.h"


ARTSOInteractableResourceBase::ARTSOInteractableResourceBase()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
	JobTag = TAG_AI_Job_GatherResource;
}

void ARTSOInteractableResourceBase::BeginPlay()
{
	Super::BeginPlay();

	FRTSOResourceBehaviourSettings* BehaviourSettings = ResourceBehaviourSettings.GetRow<FRTSOResourceBehaviourSettings>("ARTSOInteractableResourceBase::BeginPlay");
	CachedBehaviourSettings = BehaviourSettings != nullptr ? *BehaviourSettings : FRTSOResourceBehaviourSettings{};
	
	// Initial copy so the rest can be offloaded to the interface
	ProgressionTagSetsToGrant.Empty();
	for (const TTuple<FGameplayTag, FDataTableRowHandle>& TagSetHandle : OnSuccessfulInteraction_GrantedProgressionTagSets)
	{
		FRTSOMissionProgressionTagSets* ProgressionTags = TagSetHandle.Value.GetRow<FRTSOMissionProgressionTagSets>("ARTSOInteractableResourceBase::BeginPlay");
		
		ProgressionTagSetsToGrant.Emplace(TagSetHandle.Key) = ProgressionTags != nullptr ? *ProgressionTags : FRTSOMissionProgressionTagSets{};
	}
	MissionProgressionTagsToGive = ProgressionTagSetsToGrant;

	UPDRTSBaseSubsystem* RTSSubsystem = UPDRTSBaseSubsystem::Get();
	
	for (const TPair<FGameplayTag, int32 /*count*/>& ResourceReward : TradeArchetype)
	{
		InventoryFragment.AddItem(ResourceReward.Key, ResourceReward.Value);

		RTSSubsystem->TrackResource(ResourceReward.Key, this);
	}

	EntitySubsystem = GetWorld()->GetSubsystem<UMassEntitySubsystem>();
	EntManager = &EntitySubsystem->GetEntityManager();
}

void ARTSOInteractableResourceBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	RefreshTickAcc += DeltaTime;

	//FPDGridCell CurrentGridCell = UPDHashGridSubsystem::GetCellIndexStatic(GetActorLocation());
	FPDGridCell CurrentGridCell = UPDHashGridSubsystem::StaticCell(GetActorLocation(), UPDRTSBaseSubsystem::ResourceGridSize);

	const bool bShouldUpdateGridCellMapping = CurrentGridCell != PreviousGridCell;
	if (bShouldUpdateGridCellMapping)
	{
		UPDRTSBaseSubsystem* RTSSubsystem = UPDRTSBaseSubsystem::Get();
		RTSSubsystem->UpdateResources(this);

		PreviousGridCell = CurrentGridCell;
	}
}

void ARTSOInteractableResourceBase::BeginDestroy()
{
	if (UPDRTSBaseSubsystem* RTSSubsystem = UPDRTSBaseSubsystem::Get())
	{
		for (const TPair<FGameplayTag, int32 /*count*/>& ResourceReward : TradeArchetype)
		{
			RTSSubsystem->UntrackAllFromResourceActor(ResourceReward.Key, this);
		}	
	}
	
	Super::BeginDestroy();
}


void ARTSOInteractableResourceBase::AddTagToCaller_Implementation(AActor* Caller, const FGameplayTag& NewTag)
{
	IRTSOMissionProgressor::AddTagToCaller_Implementation(Caller, NewTag);
}

FGameplayTagContainer ARTSOInteractableResourceBase::SelectorTagToTagContainer_Implementation(AActor* Caller, const FGameplayTag& SelectorTag)
{
	return IRTSOMissionProgressor::SelectorTagToTagContainer_Implementation(Caller, SelectorTag);
}

void ARTSOInteractableResourceBase::OnInteractionSuccessful(AActor* InstigatorActor) const
{
	if (InstigatorActor == nullptr
		|| InstigatorActor->GetClass()->ImplementsInterface(URTSOConversationInterface::StaticClass()) == false)
	{
		return;
	}

	ARTSOInteractableResourceBase* MutableThis = const_cast<ARTSOInteractableResourceBase*>(this);
	const FGameplayTagContainer SelectorTagsInOrder = IPDInteractInterface::Execute_GetGenericTagContainer(this); //.GetByIndex(0)
	IRTSOMissionProgressor::Execute_AddTagContainerToCallerFromSelectorTag(MutableThis, InstigatorActor, SelectorTagsInOrder.GetByIndex(TagOrder));
}

void ARTSOInteractableResourceBase::ProcessTradeIfInfiniteInventory(
	const FPDInteractionParamsWithCustomHandling& InteractionParams,
	EPDInteractResult& InteractResult,
	UPDInventoryComponent* InstigatorInvComponent,
	FRTSOLightInventoryFragment* InstigatorInventoryFragment,
	UPDInventorySubsystem* InvSubsystem) const
{
	for (const TPair<FGameplayTag, int32 /*count*/>& ResourceReward : TradeArchetype)
	{
		const FString CtxtString = FString::Printf(TEXT("Entry in LinkedItemResources in interactable(%s) is not pointing to a valid item/resource entry "), *GetName());
		const FPDItemDefaultDatum* DefaultDatum = InvSubsystem->GetDefaultDatum(ResourceReward.Key);
		if (DefaultDatum == nullptr) { continue; }

		if (InstigatorInventoryFragment != nullptr)
		{
			InstigatorInventoryFragment->AddItem(DefaultDatum->ItemTag, ResourceReward.Value);
		}
		if (InstigatorInvComponent != nullptr)
		{
			InstigatorInvComponent->RequestUpdateItem(EPDItemNetOperation::ADDNEW, DefaultDatum->ItemTag, ResourceReward.Value);
		}
	}
	
	InteractResult = EPDInteractResult::INTERACT_SUCCESS;

	AActor* InstigatorActor = InteractionParams.InstigatorActor;
	OnInteractionSuccessful(InstigatorActor); 
}

void ARTSOInteractableResourceBase::ProcessTradeIfLimitedInventory(
	const FPDInteractionParamsWithCustomHandling& InteractionParams,
	EPDInteractResult& InteractResult,
	UPDInventoryComponent* InstigatorInvComponent,
	FRTSOLightInventoryFragment* InstigatorInventoryFragment,
	UPDInventorySubsystem* InvSubsystem,
	bool& bMustWaitForRegeneration) const
{
	ERTSResourceRequirement SelectedRequirement = CachedBehaviourSettings.OverrideRequirements;
	if (SelectedRequirement == PD::Interactable::Behaviour::Requirements::EUndefined)
	{
		SelectedRequirement = GetDefault<URTSInteractableResourceSettings>()->DefaultRequirements; 
	}
	
	FRTSOLightInventoryFragment* MutableInventoryFragment = const_cast<FRTSOLightInventoryFragment*>(&InventoryFragment);
	for (const TPair<FGameplayTag, int32 /*count*/>& ResourceReward : TradeArchetype)
	{
		const int32 ItemsRemaining = MutableInventoryFragment->GetItemCount(ResourceReward.Key);

		const FString CtxtString = FString::Printf(TEXT("Entry in LinkedItemResources in interactable(%s) is not pointing to a valid item/resource entry according to the inventory subsystem"), *GetName());
		const FPDItemDefaultDatum* DefaultDatum = InvSubsystem->GetDefaultDatum(ResourceReward.Key);
		if (DefaultDatum == nullptr || ItemsRemaining < 1)
		{
			continue;
		}
		
		switch (SelectedRequirement)
		{
		default:
		case PD::Interactable::Behaviour::Requirements::EUndefined:
		case PD::Interactable::Behaviour::Requirements::EAlwaysAllowTrade:
			break;
		case PD::Interactable::Behaviour::Requirements::EMustHaveMinimumCount:
			if (ItemsRemaining < ResourceReward.Value)
			{
				UE_LOG(PDLog_Inventory, Warning, TEXT("ARTSOInteractableResourceBase::ProcessTradeIfLimitedInventory -- Resource count is below minimum value"));
				
				return;
			}
			break;
		}
		
		FPDLightItemDatum RemoveDatum = FPDLightItemDatum(ResourceReward.Key, ResourceReward.Value);
		MutableInventoryFragment->RemoveItem(RemoveDatum);

		if (DefaultDatum->bRegenerateResourceIfInContainer)
		{
			bMustWaitForRegeneration = true;

			FTimerHandle Hndl{};
			FTimerDelegate RegenDlgt = FTimerDelegate::CreateLambda(
				[&, ResourceRewardCopy = ResourceReward]()
				{
					MutableInventoryFragment->AddItem(ResourceRewardCopy.Key, ResourceRewardCopy.Value);
				});

			if (DefaultDatum->SecondsToWaitIfRegenerationIsEnabled > KINDA_SMALL_NUMBER)
			{
				GetWorld()->GetTimerManager().SetTimer(Hndl, RegenDlgt, DefaultDatum->SecondsToWaitIfRegenerationIsEnabled, false);
			}
			else
			{
				RegenDlgt.Execute();
			}
		}
		

		if (InstigatorInventoryFragment != nullptr)
		{
			InstigatorInventoryFragment->AddItem(DefaultDatum->ItemTag, ResourceReward.Value);
		}
		if (InstigatorInvComponent != nullptr)
		{
			InstigatorInvComponent->RequestUpdateItem(EPDItemNetOperation::ADDNEW, DefaultDatum->ItemTag, ResourceReward.Value);
		}
	}
	
	InteractResult = EPDInteractResult::INTERACT_SUCCESS;
	
	AActor* InstigatorActor = InteractionParams.InstigatorActor;
	OnInteractionSuccessful(InstigatorActor); 
}

void ARTSOInteractableResourceBase::OnInteract_Implementation(
	const FPDInteractionParamsWithCustomHandling& InteractionParams,
	EPDInteractResult& InteractResult) const
{
	Super::OnInteract_Implementation(InteractionParams, InteractResult);
	switch (InteractResult)
	{
	case EPDInteractResult::INTERACT_SUCCESS:
	case EPDInteractResult::INTERACT_DELAYED:
	case EPDInteractResult::INTERACT_FAIL:
		// Was handled already by a custom processing delegate
		return;
	case EPDInteractResult::INTERACT_UNHANDLED:
		break;
	}
	
	InteractResult = EPDInteractResult::INTERACT_FAIL;
	if (RefreshTickAcc < GetInteractionSettings().RefreshInterval) { return ; }
	(float&)RefreshTickAcc = 0.0;

	// has instigator inv component?
	
	const AActor* InstigatorActor = InteractionParams.InstigatorActor;
	
	UPDInventoryComponent* InstigatorInvComponent = InstigatorActor != nullptr ? Cast<UPDInventoryComponent>(InstigatorActor->GetComponentByClass(InteractionParams.InstigatorComponentClass)) : nullptr;
	FRTSOLightInventoryFragment* InstigatorInventoryFragment = nullptr;
	if (InstigatorInvComponent == nullptr)
	{
		InstigatorInventoryFragment = EntManager->GetFragmentDataPtr<FRTSOLightInventoryFragment>(InteractionParams.InstigatorEntity);
		if (InstigatorInventoryFragment == nullptr)
		{
			UE_LOG(PDLog_Inventory, Warning, TEXT("ARTSOInteractableResourceBase::OnInteract -- Entity or Actor, that is interacting with the given resource object, does not have any form of inventory, cancelling interaction. "))
			return;
		}
	}

	UPDInventorySubsystem* InvSubsystem = UPDInventorySubsystem::Get();
	check(InvSubsystem != nullptr)

	if (HasInfiniteInventory())
	{
		ProcessTradeIfInfiniteInventory(InteractionParams, InteractResult, InstigatorInvComponent, InstigatorInventoryFragment, InvSubsystem);
	}
	else
	{
		bool bMustWaitForRegen = false;
		ProcessTradeIfLimitedInventory(InteractionParams, InteractResult, InstigatorInvComponent, InstigatorInventoryFragment, InvSubsystem, bMustWaitForRegen);

		const TMap<FGameplayTag, FPDLightItemDatum>& Items = InventoryFragment.GetItems();
		UPDRTSBaseSubsystem* RTSSubsystem = UPDRTSBaseSubsystem::Get();
		for (auto&[ResourceType, ItemDatum] : Items)
		{
			if (ItemDatum.TotalItemCount <= 0)
			{
				RTSSubsystem->UntrackResource(ResourceType, this);
			}
		}

		if (InventoryFragment.IsEmpty() && bMustWaitForRegen == false)
		{
			ARTSOInteractableResourceBase* MutableThis = const_cast<ARTSOInteractableResourceBase*>(this);
			MutableThis->Destroy();
		}
	}
}

FGameplayTagContainer ARTSOInteractableResourceBase::GetGenericTagContainer_Implementation() const
{
	FGameplayTagContainer GeneratedTags;
	GeneratedTags.AddTag(JobTag);
	GeneratedTags.AppendTags(MissionProgressionSelectorTags.Last());
	return GeneratedTags;
}

bool ARTSOInteractableResourceBase::HasInfiniteInventory() const
{
	switch (CachedBehaviourSettings.OverrideAvailability)
	{
	case ERTSResourceAvailability::EInfinitelyAvailable:
		return true;
	case ERTSResourceAvailability::EInventoryFragment:
		return false;

	case ERTSResourceAvailability::EUndefined:
	default:
		break;
	}

	// NO OVERRIDE FOUND, REVERT TO DEFAULT SETTINGS
	switch (GetDefault<URTSInteractableResourceSettings>()->DefaultAvailability)
	{
	default:
	case ERTSResourceAvailability::EUndefined:
	case ERTSResourceAvailability::EInfinitelyAvailable:
		return true;
	case ERTSResourceAvailability::EInventoryFragment:
		return false;
	}
}

#if WITH_EDITOR
bool ARTSOInteractableResourceBase::CanEditChange(const FProperty* InProperty) const
{
	if (Super::CanEditChange(InProperty) == false)
	{
		return false;
	}

	const FName PropertyName = InProperty->GetFName();
	if (PropertyName == GET_MEMBER_NAME_CHECKED(ARTSOInteractableResourceBase, InventoryFragment))
	{
		return HasInfiniteInventory() == false;
	}

	return Super::CanEditChange(InProperty);
}
#endif // WITH_EDITOR

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
