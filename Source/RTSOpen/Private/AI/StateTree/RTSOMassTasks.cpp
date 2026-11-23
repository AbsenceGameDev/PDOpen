/* @author: Ario Amin @ Permafrost Development. @copyright: Full BSL(1.1) License included at bottom of the file  */

// PD Definitions
#include "PDRTSBaseSubsystem.h"
#include "RTSOpenCommon.h"
#include "Interfaces/PDInteractInterface.h"
#include "Interfaces/RTSOActionLogInterface.h"

// PDAI
#include "Pawns/PDRTSBaseUnit.h"
#include "AI/StateTree/RTSOMassTasks.h"
#include "AI/Mass/PDMassFragments.h"
#include "AI/Mass/RTSOMassFragments.h"

// PD UI
#include "Widgets/Slate/SRTSOActionLog.h"
#include "Actors/Interactables/Buildings/RTSOInteractableBuildingBase.h"
#include "Actors/Interactables/Resources/RTSOInteractableResourceBase.h"

// Mass
#include "MassEntitySubsystem.h"
#include "MassSignalSubsystem.h"
#include "MassStateTreeExecutionContext.h"
#include "MassCommonFragments.h"
#include "MassMovementFragments.h"
#include "MassEntityView.h"
#include "MassNavigationFragments.h"
#include "NavigationPath.h"

// StateTree 
#include "StateTreeExecutionContext.h"
#include "StateTreeLinker.h"

bool FRTSOTask_ActionLog::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(EntitySubsystemHandle);
	return FMassStateTreeTaskBase::Link(Linker);
}

EStateTreeRunStatus FRTSOTask_ActionLog::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	const FMassStateTreeExecutionContext& MassContext = static_cast<FMassStateTreeExecutionContext&>(Context);
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	const UMassEntitySubsystem& EntitySubsystem = Context.GetExternalData(EntitySubsystemHandle);
	
	const FMassEntityManager& EntityManager = EntitySubsystem.GetEntityManager();
	check(EntityManager.IsEntityValid(MassContext.GetEntity()));

	const FPDMFragment_RTSEntityBase& EntityBase = EntityManager.GetFragmentDataChecked<FPDMFragment_RTSEntityBase>(MassContext.GetEntity());
	const FRTSOActionLogEvent NewActionEvent{
		FString::Printf(TEXT("EntityID(%i) -- %s "), EntityBase.OwnerID, *InstanceData.ActionMessage.ToString() )};

	if (EntityBase.OwnerID != INDEX_NONE)
	{
		URTSActionLogSubsystem::DispatchEvent(EntityBase.OwnerID, NewActionEvent);	
	}
	
	return FMassStateTreeTaskBase::EnterState(Context, Transition);
}

//
// BRING BACK RESOURCES INTERACT TASK


// @note Some more mapped data and I won't need to have to search like this at all
FPDRTSTSetActorWrapper FRTSOTask_BringBackResource::FindAmountOfResourceActorsNearGridCell(const FMassEntityHandle& EntityHandle, const FRTSOFindResourcesParameters& Params)
{
	UE_LOG(LogTemp, Warning, TEXT("FRTSOTask_BringBackResource::FindResourceActors"));
	UPDRTSBaseSubsystem* RTSSubsystem = UPDRTSBaseSubsystem::Get();
	RTSSubsystem->RemoveEntityResourceTarget(EntityHandle);

	// @note THIS ONLY LOOKS AT THE CURRENT GRIDCELL; NEED TO WRITE A FUNCTION THAT ITERATES OUTWARD FROM THIS GRIDCELL AND LOOKS THERE
	// OR POTENTIALLY MAPPING THINGS IN THE SUBSYSTEM ENOUGH THAT I DO NOT HAVE TO SEARCH
	FPDRTSTSetActorWrapper ViableTargets;
	{
		int32 Remainder = Params.TargetResourceAmount;
		TSet<const AActor*> ActorSet = RTSSubsystem->GetResourceActorsNearGridCellWithResourceType(Params);
		for (const AActor* Actor : ActorSet)
		{
			UE_LOG(LogTemp, Warning, TEXT("======== - Step - %s"), *Actor->GetName());

			if (const ARTSOInteractableResourceBase* AsResource = Cast<ARTSOInteractableResourceBase>(Actor))
			{
				const int32 ActorsTotalItemCount = AsResource->GetInventoryFragment().GetItems().FindRef(Params.ResourceType).TotalItemCount;
				if (ActorsTotalItemCount <= 0) 
				{
					UE_LOG(LogTemp, Warning, TEXT("================= Skip -- Actor has no items"));
					continue;
				}
				
				ViableTargets.Actors.Emplace(Actor);
				
				Remainder -= ActorsTotalItemCount;
				FRTSEntityResourceGatherTarget GatherTarget = FRTSEntityResourceGatherTarget{Actor, (Remainder >= 0 ? INDEX_NONE : ActorsTotalItemCount)};
				
				RTSSubsystem->AddEntityResourceTarget(EntityHandle, GatherTarget);
				UE_LOG(LogTemp, Warning, TEXT("======== - Added as Resource target"));

				if (Remainder <= 0) {break;}
			}
		}
	}

	return ViableTargets;
}


bool FRTSOTask_BringBackResource::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(EntitySubsystemHandle);
	Linker.LinkExternalData(InventoryHandle);
	Linker.LinkExternalData(MoveTargetHandle);
	Linker.LinkExternalData(TransformHandle);
	Linker.LinkExternalData(MassSignalSubsystemHandle);
	Linker.LinkExternalData(MoveParametersHandle);
	Linker.LinkExternalData(RTSDataHandle);	
	Linker.LinkExternalData(ActionHandle);

	return FMassStateTreeTaskBase::Link(Linker);
}

// @note @todo remember to check if entity is marked as non_idle or busy, can't remember if I ever put that in before the hiatus from the project
EStateTreeRunStatus FRTSOTask_BringBackResource::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	UE_LOG(LogTemp, Warning, TEXT("FRTSOTask_BringBackResource::EnterState"));

	UPDRTSBaseSubsystem* RTSSubsystem = UPDRTSBaseSubsystem::Get();
	UMassEntitySubsystem& EntitySubsystem = Context.GetExternalData(EntitySubsystemHandle);
	FMassMoveTargetFragment& MoveTarget = Context.GetExternalData(MoveTargetHandle);
	const FMassMovementParameters& MoveParameters = Context.GetExternalData(MoveParametersHandle);
	FPDMFragment_RTSEntityBase& RTSData = Context.GetExternalData(RTSDataHandle);
	const FTransformFragment& TransformFragment = Context.GetExternalData(TransformHandle);
	FPDMFragment_Action& ActionFragment = Context.GetExternalData(ActionHandle);
	const FRTSOLightInventoryFragment& EntityInventoryFragment = Context.GetExternalData(InventoryHandle);
	
	const FMassStateTreeExecutionContext& MassContext = static_cast<FMassStateTreeExecutionContext&>(Context);
	const FMassEntityHandle& EntityHandle = MassContext.GetEntity();	
	
	ARTSOInteractableBuildingBase* AsBuildingBase = Cast<ARTSOInteractableBuildingBase>(ActionFragment.OptTargets.ActionTargetAsActor);
	if (AsBuildingBase)
	{
		UE_LOG(LogTemp, Warning, TEXT("======= Target Is Building"));

		const FRTSOLightInventoryFragment& BuildingAvailableInventorySpace = AsBuildingBase->CalculateFreeInventorySpace();

		bool bCantAfford = false;
		TMap<FGameplayTag, int32> MissingItems;
		for (const auto&[ResourceType, ItemDatum] : BuildingAvailableInventorySpace.GetItems())
		{
			const int32 DeltaItemStorage = ItemDatum.TotalItemCount - EntityInventoryFragment.GetItemCount(ResourceType); 
			const bool bCantAffordItem = DeltaItemStorage > 0;
			if (bCantAffordItem)
			{
				MissingItems.Emplace(ResourceType, DeltaItemStorage);
				bCantAfford = true;
			}
		}

		UE_LOG(LogTemp, Warning, TEXT("======= Building needs resources:"));
		for (auto&[ResourceType, Count] : MissingItems)
		{
			UE_LOG(LogTemp, Warning, TEXT("                                                   -- %s : %i"), *ResourceType.ToString(), Count);
		}
		


		// 
		// TODO: Finish the changes needed to dispatch the list of items needed
		if (bCantAfford)
		{
			UE_LOG(LogTemp, Warning, TEXT("============== Can't Afford -> Build resource path"));

			const FVector& EntityLocation = TransformFragment.GetTransform().GetLocation();
			int32 TaskCounter = 0;
			for (const auto&[ResourceType, MissingItemCount] : MissingItems) 
			{
				if (MissingItemCount > 0) 
				{
					FPDGridCell EntityAsResourceCell = UPDHashGridSubsystem::StaticCell(EntityLocation, UPDRTSBaseSubsystem::ResourceGridSize);
					FindAmountOfResourceActorsNearGridCell(EntityHandle, FRTSOFindResourcesParameters(EntityAsResourceCell, ResourceType, MissingItemCount, 2));
					++TaskCounter;
				}
			}
			PathLimit = TaskCounter;
			FPDRTSTGatherTargetsWrapper* ResourceTargets = RTSSubsystem->GetEntityResourceTargets(EntityHandle);
			if (TaskCounter != 0 && ResourceTargets)
			{
				// @done: also check if overwriting the movetarget here is one and done or if I need to do something else in the moveto processor -- Looks like we are fine, just need to handle things in the tick here below
				const FRTSEntityResourceGatherTarget& FirstPath = ResourceTargets->Targets[CurrentPathIndex++];

				UE_LOG(LogTemp, Warning, TEXT("============== Found resource targets(%i):"), ResourceTargets->Targets.Num());
				for (const FRTSEntityResourceGatherTarget& ResourceTarget : ResourceTargets->Targets)
				{
					const AActor* Target = ResourceTarget.Target;
					UE_LOG(LogTemp, Warning, TEXT("                                                   -- %s : %i"), Target ? *Target->GetName() : *FString("INVALID OBJECT") );
				}
						
				
				UE_LOG(LogTemp, Warning, TEXT("======= Going to first target(%s):"), FirstPath.Target ? *FirstPath.Target->GetName() : *FString("INVALID"));
				FRTSOTask_BringBackResource::FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
				FPDTargetCompound CachedOptTargets = InstanceData.OptTargets;
				InstanceData.OptTargets = FPDTargetCompound{FMassEntityHandle{0, 0}, FMassInt16Vector{}, const_cast<AActor*>(FirstPath.Target)}; // We only read from this actor pointer after this point, so the const cast should not cause problems
				
				EStateTreeRunStatus TriggerMoveResult = FPDMTaskStatics::TriggerMove<FRTSOTask_BringBackResource>(this, Context, EntitySubsystem, MoveTarget, MoveParameters, RTSData, TransformFragment);
				InstanceData.OptTargets = CachedOptTargets;

				return TriggerMoveResult;
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("======= Can already Afford"));
		}
	}
	
	// We have enough resources on this entity, going directly to the building
	return FPDMTaskStatics::TriggerMove<FRTSOTask_BringBackResource>(this, Context, EntitySubsystem, MoveTarget, MoveParameters, RTSData, TransformFragment);
}

EStateTreeRunStatus FRTSOTask_BringBackResource::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FMassMoveTargetFragment& MoveTarget = Context.GetExternalData(MoveTargetHandle);
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	FPDMFragment_RTSEntityBase& RTSData = Context.GetExternalData(RTSDataHandle);
	FPDMFragment_Action& ActionFragment = Context.GetExternalData(ActionHandle);
	UMassEntitySubsystem& EntitySubsystem = Context.GetExternalData(EntitySubsystemHandle);
	const FMassMovementParameters& MoveParameters = Context.GetExternalData(MoveParametersHandle);
	const FMassEntityHandle& OtherEntityHandle = InstanceData.PotentialEntityHandle;
	const FMassStateTreeExecutionContext& MassContext = static_cast<FMassStateTreeExecutionContext&>(Context);
	const FTransformFragment& TransformFragment = Context.GetExternalData(TransformHandle);


	//
	// Should ensure that we are on our way to a resource or not, and if we are, as sson as we trigger the interaction we move on to the next resource
	EStateTreeRunStatus TickMoveResult = FPDMTaskStatics::TickMove<FRTSOTask_BringBackResource>(this, Context, DeltaTime, MoveTarget, RTSData);
	
	switch(TickMoveResult)
	{
		case EStateTreeRunStatus::Succeeded:
		{
			UPDRTSBaseSubsystem* RTSSubsystem = UPDRTSBaseSubsystem::Get();
			FPDRTSTGatherTargetsWrapper* ResourceTargets = RTSSubsystem->GetEntityResourceTargets(MassContext.GetEntity());
			const bool bIsAtStorageBuilding = ResourceTargets == nullptr ? true : CurrentPathIndex >= ResourceTargets->Targets.Num();
			const FRTSEntityResourceGatherTarget& CurrentPath = bIsAtStorageBuilding ? FRTSEntityResourceGatherTarget{} : ResourceTargets->Targets[CurrentPathIndex++];

			FMassEntityHandle ThisEntity = MassContext.GetEntity();
			const IPDInteractInterface* OtherInteractable = bIsAtStorageBuilding 
				? Cast<IPDInteractInterface>(InstanceData.PotentialInteractableActor) 
				: Cast<IPDInteractInterface>(CurrentPath.Target);

			EStateTreeRunStatus InteractResult = FRTSOTask_Interact::TaskInteract<FRTSOTask_BringBackResource, false>(this, Context, OtherEntityHandle, OtherInteractable, EntitySubsystem);
			if (InteractResult != EStateTreeRunStatus::Succeeded)
			{
				// Log error
				const FRTSOActionLogEvent NewActionEvent{
					FString::Printf(TEXT("Entity(%i) -- BBR::Tick -- Fail interact with %s"),
						ThisEntity.AsNumber(), InstanceData.PotentialInteractableActor ?  *InstanceData.PotentialInteractableActor->GetName() : *FString("INVALID ACTOR") )}; 
			}
			if (bIsAtStorageBuilding) {return EStateTreeRunStatus::Succeeded;}

			FPDTargetCompound CachedOptTargets = InstanceData.OptTargets;
			InstanceData.OptTargets = FPDTargetCompound{FMassEntityHandle{0,0}, FMassInt16Vector{}, const_cast<AActor*>(CurrentPath.Target)}; 
		
			EStateTreeRunStatus TriggerNextMoveResult = FPDMTaskStatics::TriggerMove<FRTSOTask_BringBackResource>(this, Context, EntitySubsystem, MoveTarget, MoveParameters, RTSData, TransformFragment);
			InstanceData.OptTargets = CachedOptTargets;

			return EStateTreeRunStatus::Running;
		}
		break;

	default:
		return TickMoveResult;
	}
}
void FRTSOTask_BringBackResource::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	const FMassStateTreeExecutionContext& MassContext = static_cast<FMassStateTreeExecutionContext&>(Context);
	UMassEntitySubsystem& EntitySubsystem = Context.GetExternalData(EntitySubsystemHandle);
	UPDRTSBaseSubsystem* RTSSubsystem = UPDRTSBaseSubsystem::Get();

	UPDRTSBaseUnit** UnitHandlerDoublePtr = RTSSubsystem->WorldToEntityHandler.Find(EntitySubsystem.GetWorld());
	if (UnitHandlerDoublePtr && (*UnitHandlerDoublePtr)) { (*UnitHandlerDoublePtr)->OnTaskFinished(MassContext.GetEntity()); }

	Super::ExitState(Context, Transition);
}

void FRTSOTask_BringBackResource::OnPathSelected(FPDMFragment_RTSEntityBase& RTSData, bool bShouldUseSharedNavigation, const FVector& LastPoint) const
{
	const FRTSOActionLogEvent NewActionEvent{
		FString::Printf(TEXT("Entity Group ID(%i) -- Moving To Target [%4.2f ,%4.2f, %4.2f] "),
			RTSData.SelectionGroupIndex, LastPoint.X, LastPoint.Y, LastPoint.Z)}; 
	if (bShouldUseSharedNavigation)
	{
		URTSActionLogSubsystem::DispatchBatchedEvent(RTSData.OwnerID, RTSData.SelectionGroupIndex, NewActionEvent);
	}
	else
	{
		URTSActionLogSubsystem::DispatchEvent(RTSData.OwnerID, NewActionEvent);		
	}
}


//
// GENERIC INTERACT TASK
bool FRTSOTask_Interact::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(EntitySubsystemHandle);
	Linker.LinkExternalData(InventoryHandle);
	return true;
}

EStateTreeRunStatus FRTSOTask_Interact::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	const FMassEntityHandle& OtherEntityHandle = InstanceData.PotentialEntityHandle;
	const IPDInteractInterface* OtherInteractable = Cast<IPDInteractInterface>(InstanceData.PotentialInteractableActor);
	const UMassEntitySubsystem& EntitySubsystem = Context.GetExternalData(EntitySubsystemHandle);

	return TaskInteract<FRTSOTask_Interact, true>(this, Context, OtherEntityHandle, OtherInteractable, EntitySubsystem);
}

template<typename TPDMassType, bool TFinishActualJobTask>
EStateTreeRunStatus FRTSOTask_Interact::TaskInteract(
	const TPDMassType* This,
	FStateTreeExecutionContext& Context, 
	const FMassEntityHandle& OtherEntityHandle,
	const IPDInteractInterface* OtherInteractable,
	const UMassEntitySubsystem& EntitySubsystem
)
{
	const auto& InstanceData = Context.GetInstanceData(*This);

	const FMassStateTreeExecutionContext& MassContext = static_cast<FMassStateTreeExecutionContext&>(Context);


	UPDRTSBaseSubsystem& RTSSubsystem = *UPDRTSBaseSubsystem::Get();
	
	UPDRTSBaseUnit** UnitHandlerDoublePtr = RTSSubsystem.WorldToEntityHandler.Find(EntitySubsystem.GetWorld());
	if (UnitHandlerDoublePtr == nullptr) { return EStateTreeRunStatus::Failed; }
	
	const FMassEntityManager& EntityManager = EntitySubsystem.GetEntityManager();
	check(EntityManager.IsEntityValid(MassContext.GetEntity()));

	FPDMFragment_RTSEntityBase& EntityBase = EntityManager.GetFragmentDataChecked<FPDMFragment_RTSEntityBase>(MassContext.GetEntity());
	FPDMFragment_Action& Action = EntityManager.GetFragmentDataChecked<FPDMFragment_Action>(MassContext.GetEntity());

	UPDRTSBaseUnit* UnitHandler = *UnitHandlerDoublePtr;

	if constexpr (TFinishActualJobTask)
	{
		UnitHandler->OnTaskFinished(MassContext.GetEntity()); // Make sure to use this on other tasks
	}
	
	if (OtherInteractable != nullptr)
	{
		// call interact function on interactables
		
		FPDInteractionParamsWithCustomHandling Params;
		// Params.CustomInteractionProcessor.BindDynamic(this, );
		// Params.InstigatorComponentClass = UPDRTSBaseUnit::StaticClass();
		// Params.OptionalInteractionTags;

		AController* InstigatorController = Cast<AController>(
			RTSSubsystem.SharedOwnerIDMappings.Contains(EntityBase.OwnerID)
				? RTSSubsystem.SharedOwnerIDMappings.FindRef(EntityBase.OwnerID)
				: nullptr);
		Params.InstigatorActor = InstigatorController != nullptr ? InstigatorController->GetPawn() : nullptr;
		
		Params.InteractionPercent = 1.01;
		Params.InstigatorEntity = MassContext.GetEntity();

		EPDInteractResult InteractResult;
		IPDInteractInterface::Execute_OnInteract(InstanceData.PotentialInteractableActor, Params, InteractResult);

		const FRTSOActionLogEvent NewActionEvent{
			FString::Printf(TEXT("EntityID(%i) -- Interacted sucessfully with %s "),
				MassContext.GetEntity().Index, *InstanceData.PotentialInteractableActor->GetName())}; 
		URTSActionLogSubsystem::DispatchEvent(EntityBase.OwnerID, NewActionEvent);
		
		return EStateTreeRunStatus::Succeeded;
		
	}

	if (EntityManager.IsEntityValid(OtherEntityHandle))
	{
		// @todo interact with other entity
		const FRTSOActionLogEvent NewActionEvent{
			FString::Printf(TEXT("EntityID(%i) -- @TODO: Interact with EntityID(%i) "),
				MassContext.GetEntity().Index, OtherEntityHandle.Index)}; 
		URTSActionLogSubsystem::DispatchEvent(EntityBase.OwnerID, NewActionEvent); // @todo pass message colouring, drive messages from table 		

		return EStateTreeRunStatus::Succeeded;
	}
	
	const FString InteractionTargetName =
		InstanceData.PotentialInteractableActor != nullptr
		? *InstanceData.PotentialInteractableActor->GetName()
		: InstanceData.PotentialEntityHandle.Index != INDEX_NONE
			? "Entity(" + FString::FromInt(InstanceData.PotentialEntityHandle.Index) + ")"
			: "N/A";

	const FRTSOActionLogEvent NewActionEvent{
		FString::Printf(TEXT("EntityID(%i) -- Failed interaction with %s "),
			MassContext.GetEntity().Index, *InteractionTargetName)}; 
	URTSActionLogSubsystem::DispatchEvent(EntityBase.OwnerID, NewActionEvent); // @todo pass message colouring, drive messages from table 		
	return EStateTreeRunStatus::Failed;
}

void FRTSOTask_MoveToTarget::OnPathSelected(FPDMFragment_RTSEntityBase& RTSData, bool bShouldUseSharedNavigation, const FVector& LastPoint) const
{
	const FRTSOActionLogEvent NewActionEvent{
		FString::Printf(TEXT("Entity Group ID(%i) -- Moving To Target [%4.2f ,%4.2f, %4.2f] "),
			RTSData.SelectionGroupIndex, LastPoint.X, LastPoint.Y, LastPoint.Z)}; 
	if (bShouldUseSharedNavigation)
	{
		URTSActionLogSubsystem::DispatchBatchedEvent(RTSData.OwnerID, RTSData.SelectionGroupIndex, NewActionEvent);
	}
	else
	{
		URTSActionLogSubsystem::DispatchEvent(RTSData.OwnerID, NewActionEvent);		
	}
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
