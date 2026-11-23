/* @author: Ario Amin @ Permafrost Development. @copyright: Full BSL(1.1) License included at bottom of the file  */

#include "Components/PDInteractComponent.h"
#include "PDInteractCommon.h"
#include "Interfaces/PDInteractInterface.h"

#include <CollisionQueryParams.h>
#include <Kismet/KismetSystemLibrary.h>

/* Engine - Components */
#include "Camera/CameraComponent.h"

#include "GameFramework/GameStateBase.h"

FPDTraceResult DummyTrace;

UPDInteractComponent::UPDInteractComponent(const class FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.bCanEverTick = true;
}

void UPDInteractComponent::BeginPlay()
{
	Super::BeginPlay();
	Prerequisites();
}

void UPDInteractComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (GetOwner<APawn>() == nullptr || GetOwner<APawn>()->IsLocallyControlled() == false)
	{
		SetComponentTickEnabled(false);
		return;
	}

	// UE_LOG(LogTemp, Warning, TEXT("UPDInteractComponent::TickComponent"))
	
	CurrentTraceIndex = 0;
	const int32 TraceLim = TraceSettings.TickTraceTypeSettings.Num();
	for (int32 TraceIndex = 0; TraceIndex < TraceLim; TraceIndex++)
	{
		CurrentTraceIndex = TraceIndex;
		const FPDTraceTickSettings& Setting = TraceSettings.TickTraceTypeSettings[TraceIndex];

		switch(Setting.TickTraceType)
		{
		case EPDTickTraceType::TRACE_RADIAL:
			// UE_LOG(LogTemp, Warning, TEXT("UPDInteractComponent::TickComponent -- Radial Tick"))
			PerformRadialTrace(DeltaTime, Setting);
			break;
		case EPDTickTraceType::TRACE_LINESHAPE:
			// UE_LOG(LogTemp, Warning, TEXT("UPDInteractComponent::TickComponent -- Lineshape Tick"))
			PerformLineShapeTrace(DeltaTime, Setting);
			break;
		case EPDTickTraceType::TRACE_MAX:
		default: ;
			// UE_LOG(LogTemp, Warning, TEXT("UPDInteractComponent::TickComponent -- All Tick Types"))
			PerformLineShapeTrace(DeltaTime, Setting);
			PerformRadialTrace(DeltaTime, Setting);
			break;
		}
	}
}

void UPDInteractComponent::FindClosestRadialTraceActor(const FVector& OwnerActorLocation, const AActor*& ClosestInteractable)
{
	for (const AActor* TraceActor : TraceBuffer[CurrentTraceIndex].RadialTraceActors)
	{
		if (TraceActor == nullptr)
		{
			continue;
		}

		if (ClosestInteractable == nullptr)
		{
			ClosestInteractable = TraceActor;
			continue;
		}
		const double AbsoluteDistanceToNewActor = (TraceActor->GetActorLocation() - OwnerActorLocation).Length(); 
		const double AbsoluteDistanceToCurrentClosest = (ClosestInteractable->GetActorLocation() - OwnerActorLocation).Length();
				
		if (AbsoluteDistanceToNewActor >= AbsoluteDistanceToCurrentClosest) { continue; }

		ClosestInteractable = TraceActor; 
	}
}

void UPDInteractComponent::BeginInteraction(FName TraceID)
{
	const bool bTraceIdxExists = TraceNameToIndexMappings.Contains(TraceID);
	const FPDTraceResult& TraceResult = bTraceIdxExists ? GetTraceResultByIdx(true, TraceNameToIndexMappings[TraceID]) : FPDTraceResult{EPDTraceResult::TRACE_FAIL};
	switch (TraceResult.ResultFlag)
	{
	case EPDTraceResult::TRACE_SUCCESS:
		break;
	case EPDTraceResult::TRACE_FAIL:
		return;
	}
	

	// First find the closes interactable

	const FVector& OwnerActorLocation = GetOwner()->GetActorLocation();

	const AActor* ClosestInteractable = nullptr;
	switch (TraceResult.TraceType)
	{
	case EPDTickTraceType::TRACE_RADIAL:
		{
			FindClosestRadialTraceActor(OwnerActorLocation, ClosestInteractable);
		}
		break;
	case EPDTickTraceType::TRACE_LINESHAPE:
		{
			ClosestInteractable = TraceResult.HitResult.GetActor();
		}
		break;
	case EPDTickTraceType::TRACE_RESERVED0:
		break;
	case EPDTickTraceType::TRACE_RESERVED1:
		break;
	case EPDTickTraceType::TRACE_RESERVED2:
		break;
	case EPDTickTraceType::TRACE_MAX:
		{
			const AActor* ClosestRadialInteractable = nullptr;
			FindClosestRadialTraceActor(OwnerActorLocation, ClosestRadialInteractable);

			const double DistanceToRadialActor = (ClosestInteractable->GetActorLocation() - OwnerActorLocation).Length();
			const double DistanceToShapeTraceActor = (TraceResult.HitResult.GetActor()->GetActorLocation() - OwnerActorLocation).Length();

			// Fallback to pick Radial actor if shape-traced actor was invalid
			ClosestInteractable = DistanceToRadialActor < DistanceToShapeTraceActor || TraceResult.HitResult.GetActor() == nullptr
				? ClosestRadialInteractable : TraceResult.HitResult.GetActor();
		}		
		break;
	}

	ActiveInteractionTargets.Emplace(TraceID, ClosestInteractable);
	ActiveInteractionTimers.Emplace(TraceID);
	StartInteractionTimes.Emplace(TraceID, GetWorld()->GetGameState()->GetServerWorldTimeSeconds());

	// This is ensured further back in the codepath to be true if the ActiveInteractionTarget is not null
	const IPDInteractInterface* AsInteractInterface = Cast<IPDInteractInterface>(ClosestInteractable);
	if (AsInteractInterface == nullptr || FMath::IsNearlyZero(AsInteractInterface->GetInteractionTime()))
	{
		return;
	}

	FTimerDelegate InteractionTick;
	InteractionTick.BindUObject(this, &UPDInteractComponent::TimerInteraction, TraceID);
	GetWorld()->GetTimerManager().SetTimer(ActiveInteractionTimers[TraceID], InteractionTick, 0.1, true);
}

void UPDInteractComponent::TimerInteraction(FName TraceID)
{
	const float CurrentInteractionTime = GetWorld()->GetGameState()->GetServerWorldTimeSeconds();
	CurrentInteractionTimes[TraceID] = CurrentInteractionTime;

	// This is ensured further back in the codepath to be true if the ActiveInteractionTarget is not null
	const IPDInteractInterface* AsInteractInterface = Cast<IPDInteractInterface>(ActiveInteractionTargets[TraceID]);

	if (FMath::IsNearlyZero(AsInteractInterface->GetInteractionTime()))
	{
		EndInteraction(TraceID);
		return;
	}

	const double InteractionPercent = (CurrentInteractionTime + SMALL_NUMBER) / AsInteractInterface->GetInteractionTime();
	if (InteractionPercent + SMALL_NUMBER > 1.0)
	{
		EndInteraction(TraceID);
		return;
	}
}

void UPDInteractComponent::EndInteraction(FName TraceID)
{
	CurrentInteractionTimes[TraceID] = GetWorld()->GetGameState()->GetServerWorldTimeSeconds() - StartInteractionTimes[TraceID];
	StartInteractionTimes[TraceID] = 0.0;
	GetWorld()->GetTimerManager().ClearTimer(ActiveInteractionTimers[TraceID]);

	const AActor* ActiveInteractionTarget = ActiveInteractionTargets[TraceID];
	if (ActiveInteractionTargets[TraceID] == nullptr)
	{
		CurrentInteractionTimes[TraceID] = 0;
		return;
	}

	// This is ensured further back in the codepath to be true if the ActiveInteractionTarget is not null
	const IPDInteractInterface* AsInteractInterface = Cast<IPDInteractInterface>(ActiveInteractionTarget);

	FPDInteractionParamsWithCustomHandling InteractionParams;
	InteractionParams.InstigatorActor = GetOwner();
	InteractionParams.InteractionPercent = FMath::IsNearlyZero(AsInteractInterface->GetInteractionTime())
		? 1.0 : (CurrentInteractionTimes[TraceID] + SMALL_NUMBER) / AsInteractInterface->GetInteractionTime() ;
	InteractionParams.InstigatorComponentClass; // @todo
	InteractionParams.OptionalInteractionTags;  // @todo
	
	EPDInteractResult InteractResult = EPDInteractResult::INTERACT_UNHANDLED;
	AsInteractInterface->OnInteract(InteractionParams, InteractResult);

	// @todo Finish the switch 
	switch (InteractResult)
	{
	case EPDInteractResult::INTERACT_SUCCESS:
		break;
	case EPDInteractResult::INTERACT_FAIL:
		break;
	case EPDInteractResult::INTERACT_DELAYED:
		break;
	case EPDInteractResult::INTERACT_UNHANDLED:
		break;
	}
}


const FPDTraceResult& UPDInteractComponent::GetTraceResult(const bool bSearchForValidCachedResults, const FName TraceID) const
{
	const bool bContainsTraceIdx = TraceNameToIndexMappings.Contains(TraceID);
	// if (false == bContainsTraceIdx) UE_LOG(LogTemp, Warning, TEXT("UPDInteractComponent::GetTraceResult -- Did not find TraceID(%s)"), *TraceID.ToString())
	return bContainsTraceIdx ? GetTraceResultByIdx(bSearchForValidCachedResults, TraceNameToIndexMappings[TraceID]) : DummyTrace;
}


const FPDTraceResult& UPDInteractComponent::GetTraceResultByIdx(const bool bSearchForValidCachedResults, const int32 TraceIndex) const
{
	const FPDTraceBuffer& PerTraceBuffer = TraceBuffer[TraceIndex];
	if (PerTraceBuffer.Frames.IsEmpty()) 
	{ 
		// UE_LOG(LogTemp, Warning, TEXT("UPDInteractComponent::GetTraceResult -- PerTraceBuffer has no frames"))
		return DummyTrace; 
	}
	
	const FPDTraceResult& PDTraceResult = bSearchForValidCachedResults ? PerTraceBuffer.GetLastValidResult() : PerTraceBuffer.GetLastTraceResult();
	const FHitResult& TraceResult = PDTraceResult.HitResult;
	
	const IPDInteractInterface* Interface = Cast<IPDInteractInterface>(TraceResult.GetActor());

	if (TEXT("LandscapeTrace") == TraceSettings.TickTraceTypeSettings[TraceIndex].TraceNameID.ToString())
	{
		const bool bValidHit = TraceResult.IsValidBlockingHit();		
		return bValidHit ? PDTraceResult : DummyTrace; 
	}
	else
	{	
		const bool bValidHit = Interface != nullptr && TraceResult.IsValidBlockingHit();
		const double InteractionDistance = bValidHit ? Interface->GetMaxInteractionDistance() : 0.0f;
		const double TraceDistance       = bValidHit ? TraceResult.Distance : 1.0f;
		
		return TraceDistance < InteractionDistance ? PDTraceResult : DummyTrace; // Do not allow returning items that are traced past their max interaction distance
	}
}

const FPDTraceSettings& UPDInteractComponent::GetTraceSettings() const
{
	return TraceSettings;
}

void UPDInteractComponent::SetTraceSettings(const FPDTraceSettings& NewSettings)
{
	TraceSettings = NewSettings;
}

bool UPDInteractComponent::ContainsValidTraceResults(FName TraceID) const
{
	const bool bTraceIdxExists = TraceNameToIndexMappings.Contains(TraceID);
	return bTraceIdxExists ? GetTraceResultByIdx(true, TraceNameToIndexMappings[TraceID]) != DummyTrace : false;
}

TArray<AActor*> UPDInteractComponent::GetAllInteractablesInRadius(double Radius, bool bIgnorePerObjectInteractionDistance)
{
	TArray<AActor*> Aggregate{};
	APawn* OwnerPawn = GetOwner<APawn>();
	if (OwnerPawn == nullptr) { return Aggregate; }

	TArray<AActor*> OverlapActors{};
	UKismetSystemLibrary::SphereOverlapActors(
		OwnerPawn,
		OverridePosition == PD::Interact::Constants::INVALID_WORLD_LOC ? OwnerPawn->GetActorLocation() : OverridePosition,
		Radius,
		{TraceSettings.TickTraceTypeSettings[CurrentTraceIndex].GeneratedObjectType},
		nullptr, 
		{OwnerPawn},
		Aggregate);
	OverridePosition = bResetOverrideNextFrame ? PD::Interact::Constants::INVALID_WORLD_LOC : OverridePosition;
	
	if (bIgnorePerObjectInteractionDistance)
	{
		for (AActor* OverlappingActor : OverlapActors)
		{
			const IPDInteractInterface* ActorAsInterface = Cast<IPDInteractInterface>(OverlappingActor);
			if (ActorAsInterface == nullptr) { continue; }
			Aggregate.Emplace(OverlappingActor);
		}
	}
	else
	{
		for (AActor* OverlappingActor : OverlapActors)
		{
			const IPDInteractInterface* ActorAsInterface = Cast<IPDInteractInterface>(OverlappingActor);
			if (ActorAsInterface == nullptr) { continue; }

			FVector DistanceVector = OverlappingActor->GetActorLocation() - OwnerPawn->GetActorLocation();
			const bool bPastInteractionDistance = DistanceVector.Length() > ActorAsInterface->GetMaxInteractionDistance();

			if (bPastInteractionDistance) { continue; }
			Aggregate.Emplace(OverlappingActor);
		}		
	}
	
	return Aggregate;
}

void UPDInteractComponent::TraceToTarget(const FVector& TraceEnd)
{
	FCollisionQueryParams TraceParams;
	TraceToTarget(TraceEnd,TraceParams);
}

void UPDInteractComponent::TraceToTarget(const FVector& TraceStart, const FVector& TraceEnd)
{
	FCollisionQueryParams TraceParams;
	TraceToTarget(TraceStart, TraceEnd,TraceParams);
}

void UPDInteractComponent::TraceToTarget(const FVector& TraceEnd, FCollisionQueryParams& TraceParams)
{
	const FVector TraceStart = PD::Interact::Constants::INVALID_WORLD_LOC;
	TraceToTarget(TraceStart, TraceEnd, TraceParams);
}
void UPDInteractComponent::TraceToTarget(const FVector& TraceStart, const FVector& TraceEnd, FCollisionQueryParams& TraceParams)
{	
	FHitResult TraceHitResult;
	bool bInteractTraceResult = false;

	const FPDTraceTickSettings& CurrentTraceSettings = TraceSettings.TickTraceTypeSettings[CurrentTraceIndex];
	PerformSimpleTrace(TraceStart, TraceEnd, CurrentTraceSettings.TraceChannel, TraceParams, TraceHitResult, bInteractTraceResult);
	TraceBuffer[CurrentTraceIndex].AddTraceFrame(bInteractTraceResult ? EPDTraceResult::TRACE_SUCCESS : EPDTraceResult::TRACE_FAIL, TraceHitResult, CurrentTraceSettings.TickTraceType);
}
FPDTraceResult UPDInteractComponent::TraceToTargetAndReset(const FVector& TraceEnd)
{
	FCollisionQueryParams TraceParams;
	return TraceToTargetAndReset(TraceEnd, TraceParams);
}

FPDTraceResult UPDInteractComponent::TraceToTargetAndReset(const FVector& TraceStart, const FVector& TraceEnd)
{
	FCollisionQueryParams TraceParams;
	return TraceToTargetAndReset(TraceStart, TraceEnd, TraceParams);	
}

FPDTraceResult UPDInteractComponent::TraceToTargetAndReset(const FVector& TraceEnd, FCollisionQueryParams& TraceParams)
{
	const FVector TraceStart = PD::Interact::Constants::INVALID_WORLD_LOC;
	return TraceToTargetAndReset(TraceStart, TraceEnd, TraceParams);
}
FPDTraceResult UPDInteractComponent::TraceToTargetAndReset(const FVector& TraceStart, const FVector& TraceEnd, FCollisionQueryParams& TraceParams)
{
	const APawn* OwnerPawn = GetOwner<APawn>();
	if (OwnerPawn == nullptr) { return FPDTraceResult(); }
	
	TraceToTarget(TraceStart, TraceEnd, TraceParams);
	FPDTraceResult InteractTraceResult = GetTraceResultByIdx(true, CurrentTraceIndex);
	ClearCurrentTraceResult();
	return InteractTraceResult;
}

const TArray<AActor*>& UPDInteractComponent::GetFirstRadialTraceResults() const 
{ 
	static const TArray<AActor*> DummyReturn;
	const FPDTraceBuffer* FoundBuffer = TraceBuffer.FindByPredicate(
		[](const FPDTraceBuffer& Elem) -> bool
		{
			return false == Elem.RadialTraceActors.IsEmpty();
		});
	return nullptr != FoundBuffer ? FoundBuffer->RadialTraceActors : DummyReturn;
}

void UPDInteractComponent::Prerequisites()
{
	const FLatentActionInfo DelayInfo{0,0, TEXT("Prerequisites"), this};
	const AController* PC = GetOwner<APawn>() != nullptr ? GetOwner<APawn>()->GetController() : nullptr;
	if (PC == nullptr)
	{
		UKismetSystemLibrary::Delay(GetOwner(), 1.0f, DelayInfo);
		return;
	}

	const FPDTraceSettings* TraceSettingsPtr = TraceSettingsHandle.GetRow<FPDTraceSettings>("");
	if (TraceSettingsPtr != nullptr) { TraceSettings = *TraceSettingsPtr; }
	
	const int32 TraceLim = TraceSettings.TickTraceTypeSettings.Num();
	TraceBuffer.SetNum(TraceLim);

	int32 TraceIdx = 0;
	for (FPDTraceTickSettings& PerTraceSetting : TraceSettings.TickTraceTypeSettings) 
	{
		PerTraceSetting.Setup();
		TraceNameToIndexMappings.Emplace(PerTraceSetting.TraceNameID, TraceIdx++);
	}
	for (FPDTraceBuffer& PerTraceBuffer : TraceBuffer) 
	{ 
		PerTraceBuffer.Setup(); 
	}
		

	SetComponentTickEnabled(true);
}

void UPDInteractComponent::ClearCurrentTraceResult()
{
	ClearTraceResult(CurrentTraceIndex);
}
void UPDInteractComponent::ClearTraceResult(int32 TraceIdx)
{
	if (false == TraceBuffer.IsValidIndex(TraceIdx)) { return; }

	TraceBuffer[TraceIdx].ClearTraceResults();
}
void UPDInteractComponent::ClearAllTraceResults()
{
	for(FPDTraceBuffer& Buffer : TraceBuffer) {Buffer.ClearTraceResults();}
}


static bool bLogTrace = false;
void UPDInteractComponent::PerformLineShapeTrace_Implementation(double DeltaSeconds, const FPDTraceTickSettings& PerTickTraceSettings)
{
	TraceBuffer[CurrentTraceIndex].LineTraceTickTime += DeltaSeconds;
	const bool bCanTick = TraceBuffer[CurrentTraceIndex].LineTraceTickTime >= PerTickTraceSettings.TickInterval && GetOwner<APawn>() != nullptr;
	if (bCanTick == false) { return; }

	FVector TraceStart, TraceEnd;
	FCollisionQueryParams TraceParams;
	FHitResult InteractHitResult;
	
	// UE_LOG(LogTemp, Warning, TEXT("UPDInteractComponent::PerformLineShapeTrace"))
	if (TEXT("LandscapeTrace") == PerTickTraceSettings.TraceNameID.ToString())
	{
		// bLogTrace = true;
		// UE_LOG(LogTemp, Warning, TEXT("UPDInteractComponent::PerformLineShapeTrace -- Performing Landscape Trace"))
		EPDTraceResult TraceResult;
		PerformCameraTrace(TraceStart, TraceEnd, PerTickTraceSettings, TraceParams, InteractHitResult, TraceResult);
		TraceBuffer[CurrentTraceIndex].AddTraceFrame(TraceResult, InteractHitResult, PerTickTraceSettings.TickTraceType);
	}
	else
	{
		// bLogTrace = false;
		EPDTraceResult TraceResult;
		PerformComparativeTraces(TraceStart, TraceEnd, PerTickTraceSettings, TraceParams, InteractHitResult, TraceResult);
		TraceBuffer[CurrentTraceIndex].AddTraceFrame(TraceResult, InteractHitResult, PerTickTraceSettings.TickTraceType);
	}
}

void UPDInteractComponent::PerformRadialTrace_Implementation(double DeltaSeconds, const FPDTraceTickSettings& PerTickTraceSettings)
{
	TraceBuffer[CurrentTraceIndex].RadialTraceTickTime += DeltaSeconds;
	const bool bCanTick = TraceBuffer[CurrentTraceIndex].RadialTraceTickTime >= PerTickTraceSettings.TickInterval;
	if (bCanTick == false) { return; }

	// Clear accumulated time and Store any interactables
	TraceBuffer[CurrentTraceIndex].RadialTraceTickTime = 0;
	TraceBuffer[CurrentTraceIndex].RadialTraceActors = GetAllInteractablesInRadius(PerTickTraceSettings.MaxTraceDistanceInUnrealUnits);
}
void UPDInteractComponent::PerformCameraTrace(FVector& TraceStart, FVector& TraceEnd, const FPDTraceTickSettings& PerTickTraceSettings, FCollisionQueryParams& TraceParams, FHitResult& TraceHitResult, EPDTraceResult& TraceResultFlag) const
{
	if (Camera == nullptr) 
	{
		// if (bLogTrace) UE_LOG(LogTemp, Warning, TEXT("UPDInteractComponent::PerformLineShapeTrace -- Performing Landscape Trace -- Camera is NULL"))

		TraceResultFlag = EPDTraceResult::TRACE_FAIL; 
		return; 
	}

	FMinimalViewInfo ViewInfo;
	Camera->GetCameraView(0.0, ViewInfo);

	TraceStart = ViewInfo.Location;
	FRotator PawnViewRotation = ViewInfo.Rotation;
	TraceEnd = TraceStart + (GetMaxTraceDistance(PerTickTraceSettings.TraceNameID) * PawnViewRotation.Vector());	

	bool bTraceResult = false;
	TracePass(TraceStart, TraceEnd, PerTickTraceSettings.TraceChannel, TraceParams, TraceHitResult, bTraceResult);	
	TraceResultFlag = bTraceResult ? EPDTraceResult::TRACE_SUCCESS : EPDTraceResult::TRACE_FAIL; 
	
	// if (bLogTrace) UE_LOG(LogTemp, Warning, TEXT("UPDInteractComponent::PerformLineShapeTrace -- Performing Landscape Trace -- Trace Start: %s"), *TraceStart.ToString())
	// if (bLogTrace) UE_LOG(LogTemp, Warning, TEXT("UPDInteractComponent::PerformLineShapeTrace -- Performing Landscape Trace -- Trace End: %s"), *TraceEnd.ToString())
	// if (bLogTrace) UE_LOG(LogTemp, Warning, TEXT("UPDInteractComponent::PerformLineShapeTrace -- Performing Landscape Trace -- Trace Result: %s"), *FString(bTraceResult ? TEXT("TRACE_SUCCESS") : TEXT("TRACE_FAIL")))
}

void UPDInteractComponent::PerformComparativeTraces(FVector& TraceStart, FVector& TraceEnd, const FPDTraceTickSettings& PerTickTraceSettings, FCollisionQueryParams& TraceParams, FHitResult& TraceHitResult, EPDTraceResult& TraceResultFlag) const
{
	APawn* OwnerPawn = GetOwner<APawn>();
	if (OwnerPawn == nullptr) { return; }

	// Trace 1 (Leading trace)
	PerformCameraTrace(TraceStart, TraceEnd, PerTickTraceSettings, TraceParams, TraceHitResult, TraceResultFlag);
	const bool bTraceResult = TraceResultFlag == EPDTraceResult::TRACE_SUCCESS;

	// Trace 2 (Comparison trace)	
	const FVector PawnEyeLocation = OwnerPawn->GetActorLocation() + FVector(0.f,0.f, OwnerPawn->BaseEyeHeight);
	const FVector TargetEndLocation = 
		TraceHitResult.ImpactPoint != PD::Interact::Constants::INVALID_WORLD_LOC ? TraceHitResult.ImpactPoint :
		TraceHitResult.Location    != PD::Interact::Constants::INVALID_WORLD_LOC ? TraceHitResult.Location    :
		PD::Interact::Constants::INVALID_WORLD_LOC;

	bool bComparativeTraces = false;
	TracePass(PawnEyeLocation, TargetEndLocation, PerTickTraceSettings.TraceChannel, TraceParams, TraceHitResult, bComparativeTraces);

	// Combine trace results
	bool bInteractTracesValid   = bTraceResult && bComparativeTraces && TraceHitResult.GetActor() != nullptr && TraceHitResult.GetActor() != nullptr;
	bool bCompareInteractTraces = TraceHitResult.GetActor() == TraceHitResult.GetActor();	
	TraceResultFlag = bTraceResult ? EPDTraceResult::TRACE_SUCCESS : EPDTraceResult::TRACE_FAIL; 
	
	if (bInteractTracesValid && bCompareInteractTraces) { return; }
	TraceHitResult = FHitResult(); // Clear hit-results if comparison checks or validity checks fail
}

void UPDInteractComponent::PerformSimpleTrace(const FVector& TraceStart, const FVector& TraceEnd, ECollisionChannel TraceChannel, FCollisionQueryParams& TraceParams, FHitResult& TraceHitResult, bool& bTraceResultFlag) const
{
	const APawn* OwnerPawn = GetOwner<APawn>();

	const bool bTraceFromView = (TraceStart == PD::Interact::Constants::INVALID_WORLD_LOC && OwnerPawn != nullptr);
	const FVector ViewOffset = FVector(0.f,0.f, OwnerPawn->BaseEyeHeight);
	const FVector SelectedStart = bTraceFromView ? OwnerPawn->GetActorLocation() + ViewOffset : TraceStart;

	TracePass(SelectedStart, TraceEnd, TraceChannel, TraceParams, TraceHitResult, bTraceResultFlag);
	if (bTraceResultFlag == false || TraceHitResult.GetActor() == nullptr)
	{
		TraceHitResult = FHitResult(); 
	}
}

void UPDInteractComponent::TracePass(const FVector& TraceFromLocation, const FVector& TraceEnd,  ECollisionChannel TraceChannel, FCollisionQueryParams& TraceParams, FHitResult& InteractHitResult, bool& bTraceResultFlag) const
{
	const APawn* OwnerPawn = GetOwner<APawn>();

	TraceParams = FCollisionQueryParams(FName(""), false, OwnerPawn);
	TraceParams.AddIgnoredActor(OwnerPawn);
	for (const AActor* ActorToIgnore: RuntimeIgnoreList)
	{
		TraceParams.AddIgnoredActor(ActorToIgnore);
	}
	
	InteractHitResult = FHitResult(ForceInit);
	bTraceResultFlag = GetWorld()->SweepSingleByChannel(InteractHitResult, TraceFromLocation, TraceEnd, FQuat::Identity, TraceChannel, FCollisionShape::MakeBox(FVector(BoxTraceExtent)), TraceParams);
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