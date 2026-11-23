/* @author: Ario Amin @ Permafrost Development. @copyright: Full BSL(1.1) License included at bottom of the file  */

#include "Pawns/PDRTSBaseUnit.h"
#include "GameplayTagContainer.h"
#include "MassCommonFragments.h"
#include "MassEntityManager.h"
#include "PDRTSBaseSubsystem.h"
#include "PDRTSCommon.h"
#include "AI/Mass/PDMassFragments.h"

FPDTargetCompound EmptyCompound{};
UPDRTSBaseUnit::UPDRTSBaseUnit(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UPDRTSBaseUnit::InitializeComponent()
{
	Super::InitializeComponent();

	// // @todo Resolve ISM instance materials not updating
	// // @todo cont. -- MASS seems to override the value of NumCustomDataFloats at some point, ignoring the hijacked value below
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 4
	if (bAddedNewInstanceCount == false)
	{
		bAddedNewInstanceCount = true;
		NumCustomDataFloats += 2;
	}
#endif
}

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 4
	TArray<int32> UPDRTSBaseUnit::AddInstances(const TArray<FTransform>& InstanceTransforms, bool bShouldReturnIndices, bool bWorldSpace, bool bUpdateNavigation)
#else 
	TArray<int32> UPDRTSBaseUnitAddInstances(const TArray<FTransform>& InstanceTransforms, bool bShouldReturnIndices, bool bWorldSpace)
#endif
{
	// // @todo Resolve ISM instance materials not updating
	// // @todo cont. -- MASS seems to override the value of NumCustomDataFloats at some point, ignoring the hijacked value below	

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 4
	if (bAddedNewInstanceCount == false)
	{
		bAddedNewInstanceCount = true;
		NumCustomDataFloats += 2;
	}
#endif
	
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 4
	return Super::AddInstances(InstanceTransforms, bShouldReturnIndices, bWorldSpace, bUpdateNavigation);
#else 
	return Super::AddInstances(InstanceTransforms, bShouldReturnIndices, bWorldSpace);
#endif	
}

TArray<int32> UPDRTSBaseUnit::GetInstancesOverlappingSphere(const FVector& Center, float Radius, bool bSphereInWorldSpace) const
{
	return Super::GetInstancesOverlappingSphere(Center, Radius, bSphereInWorldSpace);
}

void UPDRTSBaseUnit::ResetState(FMassEntityHandle RequestedEntityHandle)
{
	AssignTask(RequestedEntityHandle, TAG_AI_Job_Idle, EmptyCompound);
}

void UPDRTSBaseUnit::RequestAction(int32 CallingOwnerID, const FPDTargetCompound& OptTarget, FGameplayTag RequestedJob, FMassEntityHandle RequestedEntityHandle)
{
	if (OptTarget.IsValidCompound() == false) { return; }

	// UE_LOG(PDLog_RTSBase, Log, TEXT("UPDRTSBaseUnit::RequestAction(%s)"), *RequestedJob.ToString())
	AssignTask(RequestedEntityHandle, RequestedJob, OptTarget);

	// @todo Call GM and update unit save data here?
}

void UPDRTSBaseUnit::RequestActionMulti(
	int32 CallingOwnerID,
	const TArray<TTuple<
	const FPDTargetCompound& /*OptTarget*/,
	const FGameplayTag& /*RequestedJob*/,
	FMassEntityHandle /*RequestedEntityHandle*/>>& EntityHandleCompounds,
	int32                                          SelectionGroup)
{
	// // @todo come back to this and finish it within the ned of the week
	
	//
	// UPDRTSBaseSubsystem* RTSSubsystem = UPDRTSBaseSubsystem::Get();
	// if (RTSSubsystem != nullptr)
	// {
	// 	RTSSubsystem->RequestNavpathGenerationForSelectionGroup(SelectionGroup, SelectionCenter, TargetCompound);
	// }
	//
	// // Dispatch to different threads if it proves costly, 
	// for (const TTuple<
	// 	const FPDTargetCompound& /*OptTarget*/,
	// 	const FGameplayTag& /*RequestedJob*/,
	// 	FMassEntityHandle   /*RequestedEntityHandle*/>& EntityHandle : EntityHandleCompounds)
	// {
	// 	AssignTask(EntityHandle.Get<2>(), EntityHandle.Get<1>(), EntityHandle.Get<0>());
	// }	
}

void UPDRTSBaseUnit::RequestActionMulti(
	int32                                 CallingOwnerID,
	const FPDTargetCompound&              TargetCompound,
	const FGameplayTag&                   RequestedJob,
	const TMap<int32, FMassEntityHandle>& EntityHandleMap,
	const FVector&                        SelectionCenter,
	int32                                 SelectionGroup)
{
	UPDRTSBaseSubsystem* RTSSubsystem = UPDRTSBaseSubsystem::Get();
	if (RTSSubsystem != nullptr)
	{
		RTSSubsystem->RequestNavpathGenerationForSelectionGroup(CallingOwnerID, SelectionGroup, SelectionCenter, TargetCompound);
	}
	
	for (const TTuple<int, FMassEntityHandle>& SelectedEntityIt : EntityHandleMap)
	{
		const FMassEntityHandle& SelectedHandle = SelectedEntityIt.Value;
		RequestAction(CallingOwnerID, TargetCompound, RequestedJob, SelectedHandle);
	}	
}


void UPDRTSBaseUnit::BeginPlay()
{
	Super::BeginPlay();

	SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel14, ECR_Block);

	UPDRTSBaseSubsystem::Get()->WorldToEntityHandler.Emplace(GetWorld(), this);
	EntityManager = UPDRTSBaseSubsystem::Get()->EntityManager;

	//
	// Switch job?
	AssignTask(FMassEntityHandle{0,0}, TAG_AI_Job_Idle, EmptyCompound);
	// AddActorWorldOffset(FVector{0.0, 0.0, -Capsule->GetScaledCapsuleHalfHeight()});
}

void UPDRTSBaseUnit::AssignTask(FMassEntityHandle EntityHandle, const FGameplayTag& JobTag, const FPDTargetCompound& OptTarget)
{
	UPDRTSBaseSubsystem* RTSSubsystem = UPDRTSBaseSubsystem::Get();
	if (ensure(RTSSubsystem != nullptr) == false)
	{
		return; 
	}

	const FPDWorkUnitDatum* WorkUnitDatum = RTSSubsystem->GetWorkEntry(JobTag);
	if (WorkUnitDatum == nullptr || EntityManager->IsEntityValid(EntityHandle) == false)
	{
		UE_LOG(PDLog_RTSBase, Verbose, TEXT("UPDRTSBaseUnit::AssignTask(Ent:%i) - NO WORKUNITDATUM related to tag(%s) or invalid entity"), EntityHandle.SerialNumber, *JobTag.GetTagName().ToString());
		return;
	}
	
	ActiveJobMap.FindOrAdd(EntityHandle.Index) = JobTag;
	ActiveTargetMap.FindOrAdd(EntityHandle.Index) = OptTarget;

	// For our purposes action fragment should never be nullptr in case the handle itself is valid
	check(EntityManager->GetFragmentDataPtr<FPDMFragment_Action>(EntityHandle) != nullptr);
	FPDMFragment_Action* EntityAction = EntityManager->GetFragmentDataPtr<FPDMFragment_Action>(EntityHandle);
	
	EntityAction->ActionTag = JobTag;
	EntityAction->OptTargets = OptTarget;
	// EntityAction->Reward;
	// EntityAction->RewardAmount;
}

void UPDRTSBaseUnit::OnTaskFinished(FMassEntityHandle WorkerEntity, const FGameplayTag NewOptionalJobTag, const FPDTargetCompound& NewOptTarget)
{
	check(EntityManager->GetFragmentDataPtr<FPDMFragment_Action>(WorkerEntity) != nullptr);
	
	ActiveJobMap.Remove(WorkerEntity.Index);
	ActiveTargetMap.Remove(WorkerEntity.Index);
	
	FPDMFragment_Action* EntityAction = EntityManager->GetFragmentDataPtr<FPDMFragment_Action>(WorkerEntity);
	EntityAction->ActionTag = TAG_AI_Job_Idle;
	EntityAction->OptTargets = FPDTargetCompound{};

	if (NewOptionalJobTag.IsValid() == false) { return; }

	AssignTask(WorkerEntity, NewOptionalJobTag, NewOptTarget); // Chain new task and targets if we want
}

const FGameplayTag& UPDRTSBaseUnit::GetEntityJobChecked(FMassEntityHandle WorkerEntity) const
{
	check(EntityManager->IsEntityValid(WorkerEntity) && EntityManager->GetFragmentDataPtr<FPDMFragment_Action>(WorkerEntity) != nullptr);

	const FPDMFragment_Action* EntityAction = EntityManager->GetFragmentDataPtr<FPDMFragment_Action>(WorkerEntity);
	return EntityAction->ActionTag; // = TAG_AI_Job_Idle;
}

const FGameplayTag& UPDRTSBaseUnit::GetEntityJob(FMassEntityHandle WorkerEntity) const
{
	if (EntityManager->IsEntityValid(WorkerEntity) == false || EntityManager->GetFragmentDataPtr<FPDMFragment_Action>(WorkerEntity) == nullptr)
	{
		return FGameplayTag::EmptyTag;
	}

	const FPDMFragment_Action* EntityAction = EntityManager->GetFragmentDataPtr<FPDMFragment_Action>(WorkerEntity);
	return EntityAction->ActionTag; 
}

bool UPDRTSBaseUnit::IsEntityJobIdle(FMassEntityHandle WorkerEntity) const
{
	if (EntityManager->IsEntityValid(WorkerEntity) == false || EntityManager->GetFragmentDataPtr<FPDMFragment_Action>(WorkerEntity) == nullptr)
	{
		UE_LOG(PDLog_RTSBase, Warning, TEXT("UPDRTSBaseUnit::IsEntityJobIdle(Ent:%i) -- Invalid eneny or does not have FPDMFragment_Action applied"), WorkerEntity.SerialNumber);
		return false;
	}

	const FPDMFragment_Action* EntityAction = EntityManager->GetFragmentDataPtr<FPDMFragment_Action>(WorkerEntity);
	return EntityAction->ActionTag == TAG_AI_Job_Idle;	
}


void UPDRTSBaseUnit::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
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