/* @author: Ario Amin @ Permafrost Development. @copyright: Full BSL(1.1) License included at bottom of the file  */

/* Permadev - Actors */
#include "Actors/GodHandPawn.h"
#include "Actors/PDInteractActor.h"
#include "Actors/RTSOController.h"
#include "Pawns/PDRTSBaseUnit.h"
#include "PDRTSCommon.h"
#include "Actors/Interactables/Buildings/RTSOInteractableBuildingBase.h"

/* Permadev - Subsystems */
#include "PDRTSBaseSubsystem.h"

/* Permadev - Components */
#include "Components/PDInteractComponent.h"
#include "Components/PDInventoryComponent.h"

/* Permadev - Mass */
#include "AI/Mass/PDMassFragments.h"

/* Permadev - Input */
#include "Core/RTSOInputStackSubsystem.h"

/* Engine - Components */
#include "Camera/CameraComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "GameFramework/SpringArmComponent.h"

/* Engine - Input */
#include "EnhancedInputComponent.h"

/* Engine - Effects */
#include "MassCommonFragments.h"
#include "MassEntityManager.h"
#include "MassEntitySubsystem.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraDataInterfaceArrayFunctionLibrary.h"
#include "Chaos/DebugDrawQueue.h"

/* Engine - Mass Representation */
#include "MassCrowdRepresentationSubsystem.h"
#include "MassRepresentationFragments.h"

/* Engine - Conversation */
#include "ConversationParticipantComponent.h"

/* Engine - Navigation */
#include "NavigationPath.h"
#include "NavigationSystem.h"

/* Engine - Kismet */
#include "EnhancedInputSubsystems.h"
#include "MassSpawnerSubsystem.h"
#include "PDBuildCommon.h"
#include "PDBuilderSubsystem.h"
#include "PDRTSSharedHashGrid.h"
#include "PDRTSSharedUI.h"
#include "Core/RTSOBaseGM.h"
#include "Interfaces/PDRTSBuildableGhostInterface.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Subsystems/PDUserMessageSubsystem.h"
#include "Widgets/Slate/SRTSOActionLog.h"

#include "GlobalShaders/RTSEntityMinimapSplat.h"

//
// Godhand functionality

class UMassSpawnerSubsystem;
// Called every frame
void AGodHandPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (CameraInterpTarget != nullptr)
	{
		CameraInterpTick(DeltaTime);
		return;
	}
	
	TrackMovement(DeltaTime);
	HoverTick(DeltaTime);
	RotationTick(DeltaTime);

	BuildableGhostTick(DeltaTime);

	if (InstanceState.bUpdatePathOnTick)
	{
		RefreshPathingEffects();
	}
	
}

void AGodHandPawn::CameraInterpTick(float DeltaTime)
{
	const FVector InterpVector = UKismetMathLibrary::VInterpTo(
			GetActorLocation(),
			CameraInterpTarget->GetActorLocation(),
			DeltaTime,
			InstanceSettings.CameraTargetInterpSpeed);
	SetActorLocation(InterpVector);

	constexpr double OneMetreTolerance = 100.0;
	
	const FVector DeltaV = GetActorLocation() - CameraInterpTarget->GetActorLocation();
	if (DeltaV.IsNearlyZero(OneMetreTolerance)) { CameraInterpTarget = nullptr; }
}

void AGodHandPawn::TrackMovement(float DeltaTime)
{
	const ARTSOController* PC = GetController<ARTSOController>();
	if (PC == nullptr) { return; }
		
	// // Update collision and mesh placement
	FVector2D ScreenCoordinates;
	FVector IntersectionPoint;
	bool bFoundInputType;

	PC->ProjectMouseToGroundPlane(ScreenCoordinates,IntersectionPoint, bFoundInputType);

	IntersectionPoint += FVector{0.0, 0.0, 10.0};
	Collision->SetWorldLocation(IntersectionPoint);
	InteractComponent->SetOverrideTracePosition(IntersectionPoint);
	UpdateCursorLocation(DeltaTime);
}

void AGodHandPawn::TrackUnitMovement()
{
	const ARTSOController* PC = GetController<ARTSOController>();
	if (PC == nullptr) { return; }
	
	FVector2D v2Dummy{};
	FVector MousePlaneIntersection{};
	bool bHasInput{};
	PC->ProjectMouseToGroundPlane(v2Dummy, MousePlaneIntersection, bHasInput);
	if (bHasInput == false) { return; }
	
	const FVector CameraLagOffsetCompensation = FVector::ZeroVector; /**< @todo camera lag is controlled by the pd camera manager, might write a function there */
	const FVector FinalMove = InstanceState.WorkUnitTargetLocation - MousePlaneIntersection - CameraLagOffsetCompensation;
	AddActorWorldOffset(FVector{FinalMove.X, FinalMove.Y, 0.0});
}

void AGodHandPawn::HoverTick(const float DeltaTime)
{
	// Update Octree query position for 'QUERY_GROUP_1'
	UPDRTSBaseSubsystem* RTSSubsystem = UPDRTSBaseSubsystem::Get();
	const FVector& QueryLocation = CursorMesh->GetComponentLocation();

	FVector MinimapQueryLocation = QueryLocation;
	
	
	const FPDTraceResult& TraceResult = InteractComponent->GetTraceResult(true, TEXT("LandscapeTrace"));
	RTSSubsystem->OctreeUserQuery.UpdateQueryPosition(EPDQueryGroups::QUERY_GROUP_MINIMAP, TraceResult.HitResult.Location);
	RTSSubsystem->OctreeUserQuery.UpdateQueryPosition(EPDQueryGroups::QUERY_GROUP_HOVERSELECTION, QueryLocation);
	
	
	AActor* ClosestActor = FindClosestInteractableActor();
	// Overwrite HoveredActor if they are not the same
	if (ClosestActor != nullptr && ClosestActor != InstanceState.HoveredActor)
	{
		InstanceState.HoveredActor = ClosestActor;
		// UE_LOG(PDLog_RTSO, Warning, TEXT("HoverTick - Found New Hover Actor"))
	}
	
	const FMassEntityHandle ClosestEntity = FindClosestMassEntity();
	if (ClosestEntity.Index != 0 && InstanceState.SelectedWorkerUnitHandle.Index != ClosestEntity.Index)
	{
		// // Not functionally relevant, keep here in case we want to put something here
		// UE_LOG(PDLog_RTSO, Warning, TEXT("HoverTick - Found New Unit ISM"))
	}

	// Dont overwrite in case we are dragging a path from this
	if (InstanceState.bUpdatePathOnTick == false ) { InstanceState.SelectedWorkerUnitHandle = ClosestEntity; }
}

void AGodHandPawn::RotationTick(float& DeltaTime)
{
	// const float RotationWrapped = FMath::Wrap(GetActorRotation().Yaw, 0.0, 360.0);
	UPDRTSBaseSubsystem::SetUserRotationOnTick(FMath::DegreesToRadians(GetActorRotation().Yaw));
	
	// Not currently in rotation
	if (InstanceState.CurrentRotationLeft <= 0.0) { return; }

	
	// Current queued rotation direction && Modify the delta-time with the turn rate modifier
	constexpr double RotationStep = 90.0;
	const auto&[Direction, Target] = InstanceState.RotationDeque.First();
	DeltaTime *= InstanceSettings.RotationRateModifier;

	// Delta yaw value will grow DeltaTime with accelerating approach: DeltaYaw = DeltaTime / (RotationLeft / RotationStep) 
	const double DeltaYaw = FMath::Clamp(DeltaTime / (InstanceState.CurrentRotationLeft / RotationStep), 0.0, 2.0);
	InstanceState.CurrentRotationLeft -= DeltaYaw;

	UE_LOG(PDLog_RTSO, VeryVerbose, TEXT("AGodHandPawn::RotationTick - CurrentRotationLeft(%lf)"), InstanceState.CurrentRotationLeft)
	// Handle queued rotation finished
	if (InstanceState.CurrentRotationLeft < UE_SMALL_NUMBER)
	{
		UE_LOG(PDLog_RTSO, VeryVerbose, TEXT("AGodHandPawn::RotationTick - Rotation in queue finished"))

		// Is yaw near west/east/north/south but not quite there due to tick inconsistencies? Snap it to target
		FRotator RotationCopy = GetActorRotation();
		RotationCopy.Yaw = Target;
		SetActorRotation(RotationCopy);
		
		InstanceState.RotationDeque.PopFirst();
		InstanceState.CurrentRotationLeft = InstanceState.RotationDeque.IsEmpty() ? -1.0 : 90.0;
		InstanceState.bIsInRotation = InstanceState.RotationDeque.IsEmpty() == false;
		return;
	}
	
	UE_LOG(PDLog_RTSO, VeryVerbose, TEXT("AGodHandPawn::RotationTick - Local adjustment: {Pitch:0, Yaw:%lf, Roll:0}"), DeltaYaw * Direction)
	AddActorLocalRotation(FRotator(0, DeltaYaw * Direction, 0));
}

void AGodHandPawn::BuildableGhostTick(float DeltaTime)
{
	if (CurrentBuildableData == nullptr)
	{
		if (CurrentGhost != nullptr)
		{
			CurrentGhost->SetActorHiddenInGame(true);
		}
		
		return;
	}

	// Let UPDHashGridSubsystem calculate stepped position, collision is our source of truth for our location
	const UPDHashGridSubsystem* HashGridSubsystem = UPDHashGridSubsystem::Get();
	const FVector SteppedLocation = HashGridSubsystem->StepLocationConst(Collision->GetComponentLocation());
	const TSubclassOf<AActor> ActorClassToSpawn = CurrentBuildableData->ActorToSpawn;
	
	if (CurrentGhost == nullptr || CurrentGhost->GetClass() != ActorClassToSpawn)
	{
		if (CurrentGhost != nullptr )
		{
			CurrentGhost->Destroy();
			CurrentGhost = nullptr;
		}
		
		CurrentGhost = GetWorld()->SpawnActor(ActorClassToSpawn, &SteppedLocation);
		CurrentGhost->SetOwner(this);

		
		UE_LOG(PDLog_RTSO, Log, TEXT("AGodHandPawn::TickGhost -- Spawned buildable ghost"))
		if (ActorClassToSpawn->ImplementsInterface(UPDRTSBuildableGhostInterface::StaticClass()))
		{
			UE_LOG(PDLog_RTSO, Log, TEXT("AGodHandPawn::TickGhost -- Calling OnSpawnedAsBuildableGhost"))
			IPDRTSBuildableGhostInterface::Execute_OnSpawnedAsGhost(CurrentGhost, CurrentBuildableTag, CurrentBuildContextTag, true, false);

			// ARTSOInteractableBuildingBase* AsInteractableBuilding = Cast<ARTSOInteractableBuildingBase>(CurrentGhost); 
			// AsInteractableBuilding->ReturnBuildableInventories().StorageInventory;
		}
	}

	// Check for encroachment/overlap 
	if (CurrentGhost->GetClass()->ImplementsInterface(UPDRTSBuildableGhostInterface::StaticClass()))
	{
		CurrentGhost->SetActorEnableCollision(true);
		TArray<UStaticMeshComponent*> OverlapMeshes = IPDRTSBuildableGhostInterface::Execute_GetGhostMeshes(CurrentGhost);
		
		bool bIsEncroached = false;
		for (UStaticMeshComponent* Mesh : OverlapMeshes)
		{
			Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			TArray<AActor*> OverlappedActors;
			Mesh->GetOverlappingActors(OverlappedActors);
			Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

			// UE_LOG(PDLog_RTSO, Warning, TEXT("AGodHandPawn::TickGhost -- Encroachment -- Check for overlap"))
			for (const AActor* OverlappedActor : OverlappedActors)
			{
				if (OverlappedActor == CurrentGhost) { continue; }
				
				// UE_LOG(PDLog_RTSO, Log, TEXT("AGodHandPawn::TickGhost -- Encroachment -- Comparing with found component"))
				if (OverlappedActor->GetClass()->ImplementsInterface(UPDRTSBuildableGhostInterface::StaticClass()))
				{
					// UE_LOG(PDLog_RTSO, Warning, TEXT("AGodHandPawn::TickGhost -- Encroachment -- Failed check, is overlapping actor"))
					bIsEncroached = true;
					break;
				}
			}
		}
	
		IPDRTSBuildableGhostInterface::Execute_SetGhostAsEncroached(CurrentGhost, bIsEncroached);
		CurrentGhost->SetActorEnableCollision(false);
	}

	// Previously hidden ghost unhidden
	if (CurrentGhost->IsHidden())
	{
		CurrentGhost->SetActorHiddenInGame(false);
	}

	const FVector DeltaLocation = CurrentGhost->GetActorLocation() - SteppedLocation;
	if (DeltaLocation.IsNearlyZero(HashGridSubsystem->UniformCellSize * 0.5f))
	{
		return;
	}
	
	UE_LOG(PDLog_RTSO, Verbose, TEXT("AGodHandPawn::TickGhost -- CurrentGhost->SetActorLocation(SteppedLocation[%f,%f,%f])"), SteppedLocation.X, SteppedLocation.Y, SteppedLocation.Z)
	CurrentGhost->SetActorLocation(SteppedLocation);
}

void AGodHandPawn::UpdateMagnification()
{
	InstanceState.MagnificationValue = FMath::Clamp((InstanceState.MagnificationStrength * 0.01) + InstanceState.MagnificationValue, 0.01, 1.0);

	const float Alpha = InstanceSettings.AssetData->MagnificationCurve != nullptr
		? InstanceSettings.AssetData->MagnificationCurve->GetFloatValue(InstanceState.MagnificationValue)
		: Springarm->TargetArmLength / FMath::Clamp(Springarm->TargetArmLength * InstanceState.MagnificationValue, Springarm->TargetArmLength * 0.5, Springarm->TargetArmLength  * 1.2);
	
	Springarm->TargetArmLength = FMath::Lerp(600, 18000, Alpha);
	Springarm->SetRelativeRotation(FRotator{FMath::Lerp(-35.0,-55.0, Alpha),0,0});
	PawnMovement->MaxSpeed =  FMath::Lerp(1000, 5000, Alpha);

	// depth of field when magnifying
	FPostProcessSettings PPSettings{};
	PPSettings.DepthOfFieldFocalDistance = Springarm->TargetArmLength;
	PPSettings.DepthOfFieldSensorWidth = 175.f;
	Camera->PostProcessSettings = PPSettings;

	Camera->SetFieldOfView(FMath::Lerp(25.0f, 15.0f, Alpha));
}

void AGodHandPawn::UpdateCursorLocation(float DeltaTime)
{
	// @todo handle input types+ if so reuse enum typ ECommonInputType and set up some events to register different input from different types

	InstanceState.AccumulatedPlayTime += DeltaTime;
	InstanceState.HoveredActor = IsValid(InstanceState.HoveredActor) ? InstanceState.HoveredActor : nullptr;

	FVector Origin;
	FVector Extent;

	bool bProcessTargetOrigin = false;
	const bool bInvalidEntityHandle = InstanceState.SelectedWorkerUnitHandle.Index == 0 || EntityManager->IsEntityValid(InstanceState.SelectedWorkerUnitHandle) == false;
	if ((InstanceState.bUpdatePathOnTick && InstanceState.HoveredActor == nullptr) || (InstanceState.HoveredActor == nullptr && bInvalidEntityHandle))
	{
		InstanceState.TargetTransform = Collision->GetComponentTransform();
		InstanceState.TargetTransform.SetScale3D(FVector{2.0, 2.0,1.0});
	}
	else if (InstanceState.HoveredActor != nullptr)
	{
		InstanceState.HoveredActor->GetActorBounds(true, Origin, Extent);
		bProcessTargetOrigin = true;
	}
	else // elif (bInvalidEntityHandle == false) 
	{
		const FTransform& InstanceTransform = GetEntityTransform(InstanceState.SelectedWorkerUnitHandle);
		Origin = InstanceTransform.GetLocation();
		Extent = InstanceState.ISMAgentComponent != nullptr ? InstanceState.ISMAgentComponent->GetStaticMesh()->GetBounds().BoxExtent : FVector{30};
		bProcessTargetOrigin = true;
	}

	if (bProcessTargetOrigin)
	{
		Extent.Z = 0;
		Origin.Z += 20.0;
		const double CursorTargetScalar = Extent.GetAbsMax() / InstanceSettings.TargetPadding;
		const double CursorScalarOffset = FMath::Sin(InstanceState.AccumulatedPlayTime * InstanceSettings.CursorSelectedScalePhaseSpeed) * InstanceSettings.CursorSelectedSinScaleIntensity;
	
		const double CursorScalarFinalXY = FMath::Max(CursorTargetScalar + CursorScalarOffset, 0.01);
		const FVector ScalarXY = FVector{CursorScalarFinalXY, CursorScalarFinalXY, 1.0};

		const FTransform WorldSpace = CursorMesh->GetComponentTransform();
		//CursorMesh->GetInstanceTransform(0, WorldSpace,true);
		const FQuat OldQuat = WorldSpace.GetRotation();
		InstanceState.TargetTransform = FTransform{OldQuat, Origin, ScalarXY };
	}

	const FTransform WorldSpace =  CursorMesh->GetComponentTransform();	
	const FTransform InterpTransform = UKismetMathLibrary::TInterpTo(WorldSpace, InstanceState.TargetTransform, DeltaTime, InstanceSettings.SelectionRescaleSpeed);
	CursorMesh->SetWorldTransform(InterpTransform);
}


//
// Effects - Visuals 

void AGodHandPawn::RefreshPathingEffects()
{
	const bool bVisualizeWorkerPaths = GetDefault<UPDRTSSubsystemSettings>()->bVisualizeWorkerPaths;

	if (bVisualizeWorkerPaths == false
		|| EntityManager->IsEntityValid(InstanceState.SelectedWorkerUnitHandle) == false
		|| Collision == nullptr )
	{
		return;
	}
	
	const FVector CollisionLocation = Collision->GetComponentLocation();
	const FTransform& TargetInstanceTransform = GetEntityTransform(InstanceState.SelectedWorkerUnitHandle);
	const FVector EntityLocation = TargetInstanceTransform.GetLocation(); 
	const FVector TargetLocation = CollisionLocation;
	const UNavigationPath* Navpath = UNavigationSystemV1::FindPathToLocationSynchronously(this, EntityLocation, TargetLocation);
	if (Navpath == nullptr || Navpath->PathPoints.IsEmpty()) { return; }

	InstanceState.PathPoints = Navpath->PathPoints;
	InstanceState.PathPoints[0] = EntityLocation;
	InstanceState.PathPoints.Last() = TargetLocation;
	UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector(NC_WorkerPath, FName("TargetPath"), InstanceState.PathPoints);
}


//
// Enhanced Input - Action functions

void AGodHandPawn::ActionMove_Implementation(const FInputActionValue& Value)
{
	const FVector2D& ImmutableMoveInput = Value.Get<FVector2D>();

	if (CameraInterpTarget != nullptr) { return; } // Don't allow moving around while we are moving the player to our interp target 
	
	AddMovementInput(GetActorForwardVector(), ImmutableMoveInput.Y * 100);
	AddMovementInput(GetActorRightVector(), ImmutableMoveInput.X * 100);	
}

void AGodHandPawn::ActionMagnify_Implementation(const FInputActionValue& Value)
{
	const float ImmutableMoveInput = Value.Get<float>();

	const ARTSOController* PC = GetController<ARTSOController>();
	if (PC == nullptr) { return; }
	
	const UEnhancedInputLocalPlayerSubsystem* InputSubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer());
	const bool bIsChordActive = InputSubsystem->GetPlayerInput()->GetActionValue(PC->CtrlActionChordedBase).Get<bool>();
	if (bIsChordActive && CurrentBuildableData != nullptr && CurrentGhost != nullptr) 
	{
		// Rotate ghost @todo make this configurable, so ghost rotation is NOT tied to the scrolling or magnify key

		InstanceState.ScrollAccumulator += ImmutableMoveInput;
		if (FMath::Abs(InstanceState.ScrollAccumulator) > 1)
		{
			const int32 TruncatedAccumulator = InstanceState.ScrollAccumulator * 4.0;
			const float Fraction  = (InstanceState.ScrollAccumulator - TruncatedAccumulator);

			const bool bAboveZero = Fraction >= SMALL_NUMBER;
			const int32 Direction = 1*(bAboveZero == false) - 1*(bAboveZero);
			
			const FRotator DeltaRot = FRotator(0,90 * Direction,0);
			CurrentGhost->AddActorWorldRotation(DeltaRot);
			InstanceState.ScrollAccumulator = 0;
		}
		
		return; 
	}
	
	InstanceState.MagnificationStrength = ImmutableMoveInput;
	UpdateMagnification();
}

void AGodHandPawn::ActionRotate_Implementation(const FInputActionValue& Value)
{
	constexpr int32 MaxQueuedRotations = 3;
	
	const float ImmutableMoveInput = Value.Get<float>(); // rotate yaw
	const int8 Direction = ImmutableMoveInput > 0 ? 1 : -1;

	// If more than one rotation in queue and the new rotation opposes the latest previous rotation, just pop away the last rotation
	if (InstanceState.RotationDeque.Num() > 1 && (InstanceState.RotationDeque.Last().Key + Direction) == 0)
	{
		InstanceState.RotationDeque.PopLast();
	}
	else if (InstanceState.RotationDeque.Num() < MaxQueuedRotations)
	{
		const bool bIsEmpty = InstanceState.RotationDeque.IsEmpty();
		const double Step = bIsEmpty ? GetActorRotation().Yaw + (90 * Direction) : 90 * Direction;
		InstanceState.RotationDeque.EmplaceLast(TPair<int8, double> {Direction, bIsEmpty ? Step : InstanceState.RotationDeque.Last().Value + Step});
	}

	if (InstanceState.bIsInRotation == false)
	{
		InstanceState.CurrentRotationLeft = 90.0;
	}

	InstanceState.bIsInRotation = true;
}

void AGodHandPawn::ActionDragMove_Implementation(const FInputActionValue& Value)
{
	// Handles click&drag, does not handle touch&drag
	UE_LOG(PDLog_RTSO, Warning, TEXT("AGodHandPawn::ActionDragMove"))
	
	TrackUnitMovement();
}

// todo Refactor
bool AGodHandPawn::ClickPotentialBuildable(ARTSOController* PC)
{
	// trace to
	FCollisionQueryParams Params;
	Params.MobilityType = EQueryMobilityType::Static;
	Params.AddIgnoredActors(TArray<AActor*>{PC, this});
	const FVector2D ScreenCoordinates = PC->GetMouseScreenCoords();

	FHitResult HitResult{};
	PC->GetHitResultAtScreenPosition(ScreenCoordinates, ECC_Visibility, Params, HitResult);
	AActor* HitActor = HitResult.GetActor();

	// Poor mans double click @todo refactor
	const bool bHitValidBuildable = HitActor != nullptr && UPDBuilderSubsystem::GetBuildableWithActionsFromClassStatic(HitActor->GetClass()) != nullptr;
	if (bHitValidBuildable)
	{
		const bool bClickedSameActor = LastClickedBuildable == HitActor;
		HitValidBuildableCounter = bClickedSameActor == false ? 0 : HitValidBuildableCounter;
		
		if (HitValidBuildableCounter < 1)
		{
			TimeBetweenClicks = GetWorld()->TimeSeconds;
			
			LastClickedBuildable = HitActor;
			HitValidBuildableCounter++;
			return true;
		}
		if (HitValidBuildableCounter >= 1)
		{
			TimeBetweenClicks = GetWorld()->TimeSeconds - TimeBetweenClicks;

			if (TimeBetweenClicks > 1.0f)
			{
				HitValidBuildableCounter = 0;
				return false;
			}

			PC->ProcessPotentialBuildableMenu(HitActor);
			return true;
		}
	}


	if (bHitValidBuildable && HitValidBuildableCounter < 1)
	{
		LastClickedBuildable = HitActor;
		HitValidBuildableCounter++;
		return true;
	}
	if (bHitValidBuildable && HitValidBuildableCounter >= 1)
	{
		HitValidBuildableCounter = 0;
		PC->ProcessPotentialBuildableMenu(HitActor);
		return true;
	}
	return false;
}

void AGodHandPawn::ActionWorkerUnit_Started_Implementation(const FInputActionValue& Value)
{
	// const bool ImmutableMoveInput = Value.Get<bool>();
	UE_LOG(PDLog_RTSO, Warning, TEXT("ActionWorkerUnit_Started"))
	
	if (InstanceState.SelectedWorkerUnitHandle.Index != 0)
	{
		if (GetDefault<UPDRTSSubsystemSettings>()->bVisualizeWorkerPaths)
		{
			if (NS_WorkerPath != nullptr && NC_WorkerPath == nullptr)
			{
				NC_WorkerPath = UNiagaraFunctionLibrary::SpawnSystemAttached(NS_WorkerPath, SceneRoot, NAME_None, FVector(0.f), FRotator(0.f), EAttachLocation::SnapToTarget, false);
				InstanceState.bUpdatePathOnTick = true;
			}
			else if (NC_WorkerPath != nullptr)
			{
				NC_WorkerPath->ReinitializeSystem();
				NC_WorkerPath->Activate();
				InstanceState.bUpdatePathOnTick = true;
			}
		}
		return;
	}

	
	ARTSOController* PC = GetController<ARTSOController>();
	if (PC != nullptr)
	{
		// Buildable data takes priority now, but: @todo create and action/event PriorityQueue and handle each 'event' by the order of that queue!
		if (CurrentBuildableData != nullptr)
		{
			ProcessPlaceBuildable(PC);
			bJustPlacedBuildable = true;
			return;
		}
		
		// todo Refactor
		if (ClickPotentialBuildable(PC))
		{
			UE_LOG(PDLog_RTSO, Warning, TEXT("Buildable supposedly clicked"))
			return;
		}			
		
		PC->MarqueeSelection(EMarqueeSelectionEvent::STARTMARQUEE);
	}
}

void AGodHandPawn::ActionWorkerUnit_Triggered_Implementation(const FInputActionValue& Value)
{
	if (bJustPlacedBuildable) { return; }
	
	// const bool ImmutableMoveInput = Value.Get<bool>(); 

	// @todo write class to throttle output in functions that run at high rate
	// UE_LOG(PDLog_RTSO, Warning, TEXT("ActionWorkerUnit_Triggered"))


	ARTSOController* PC = GetController<ARTSOController>();
	if (PC != nullptr && PC->IsDrawingMarquee())
	{
		PC->MarqueeSelection(EMarqueeSelectionEvent::HOLDMARQUEE);
	}	
	
	InstanceState.WorkerUnitActionTarget = InstanceState.HoveredActor; 
}

void AGodHandPawn::ActionWorkerUnit_Cancelled_Implementation(const FInputActionValue& Value)
{
	if (bJustPlacedBuildable)
	{
		bJustPlacedBuildable = false;
		return;
	}
	
	UE_LOG(PDLog_RTSO, Warning, TEXT("ActionWorkerUnit_Cancelled"))
	IRTSOInputInterface::Execute_ActionWorkerUnit_Completed(this, Value);
}

void AGodHandPawn::SpawnFromGhost(bool bBuildable, bool bRequiresWorkersToBuild)
{
	ensure(CurrentGhost != nullptr);
	if (UNLIKELY(CurrentGhost == nullptr))
	{
		UE_LOG(PDLog_RTSO, Warning, TEXT("Spawn From Ghost -- Place buildable -- CurrentGhost == nullptr -- Exiting function "))
		return;
	}
	const FVector&& GhostLoc = CurrentGhost->GetActorLocation();
	const FQuat&& GhostRot = CurrentGhost->GetActorRotation().Quaternion();

	FTransform InitialSpawnTransform{GhostRot, GhostLoc, FVector::OneVector};
	AActor* SpawnedBuildable = GetWorld()->SpawnActorDeferred<AActor>(CurrentBuildableData->ActorToSpawn, InitialSpawnTransform, this);
	SpawnedBuildable->SetOwner(this);
	SpawnedBuildings.Emplace_GetRef(SpawnedBuildable);
	
	UPDBuilderSubsystem* BuilderSubsystem = UPDBuilderSubsystem::Get();
	ensure(BuilderSubsystem != nullptr);
	BuilderSubsystem->AddBuildActorArray( IPDRTSBuilderInterface::Execute_GetBuilderID(this), &SpawnedBuildings);
	
	AsyncTask(ENamedThreads::GameThread,
	[&,  InBuildableTag = CurrentBuildableTag, InContextTag = CurrentBuildContextTag, bInBuildable = bBuildable, bInRequiresWorkersToBuild = bRequiresWorkersToBuild ,InSpawnTransform = InitialSpawnTransform, InGhost = CurrentGhost, InSpawnedBuildable = SpawnedBuildable]()
	{
		if (InSpawnedBuildable == nullptr || InSpawnedBuildable->IsValidLowLevelFast() == false)
		{
			UE_LOG(PDLog_RTSO, Warning, TEXT("Spawn From Ghost -- Lambda -- Place buildable -- Class(%s) does nor implement interface(PDRTSBuildableGhostInterface)  -- Exiting function "),  *InSpawnedBuildable->GetClass()->GetName());
			SpawnedBuildings.Remove(InSpawnedBuildable);
			return;
		}
		const FVector&& NewGhostLoc = InGhost != nullptr ? InGhost->GetActorLocation() : InSpawnTransform.GetLocation();
		const FQuat&& NewGhostRot = InGhost != nullptr ? InGhost->GetActorRotation().Quaternion() : InSpawnTransform.GetRotation();
		const FTransform UpdatedSpawnTransform{NewGhostRot, NewGhostLoc, FVector::OneVector};
		UGameplayStatics::FinishSpawningActor(InSpawnedBuildable, UpdatedSpawnTransform);
		if (InSpawnedBuildable->GetClass()->ImplementsInterface(UPDRTSBuildableGhostInterface::StaticClass()))
		{
			if (bInBuildable)
			{
				IPDRTSBuildableGhostInterface::Execute_OnSpawnedAsMain(InSpawnedBuildable, InBuildableTag, InContextTag);
				
				const UPDBuilderSubsystemSettings* BuilderSettings = GetDefault<UPDBuilderSubsystemSettings>();
				switch (BuilderSettings->DefaultBuildSystemBehaviours.CameraBehaviour)
				{
				case EPDRTSBuildCameraBehaviour::Placement_SmoothInterp:
					// CameraInterpTarget
					CameraInterpTarget = InSpawnedBuildable;
					
					break;
				case EPDRTSBuildCameraBehaviour::Place_NoCameraMovement:
					// Do nothing
					break;
				}				
			}
			else
			{
				IPDRTSBuildableGhostInterface::Execute_OnSpawnedAsGhost(InSpawnedBuildable, InBuildableTag, InContextTag, false, bInRequiresWorkersToBuild);
			}
		}
		else
		{
			UE_LOG(PDLog_RTSO, Warning, TEXT("Spawn from Ghost -- Place buildable -- Class(%s) does nor implement interface(PDRTSBuildableGhostInterface)  -- Exiting function "),  *InSpawnedBuildable->GetClass()->GetName())			
		}
	});	
}

void AGodHandPawn::ProcessPlaceBuildable(ARTSOController* PC)
{
	PC->MarqueeSelection(EMarqueeSelectionEvent::ABORT);

	ensure(CurrentGhost != nullptr);
	if (CurrentGhost->GetClass()->ImplementsInterface(UPDRTSBuildableGhostInterface::StaticClass()) == false)
	{
		UE_LOG(PDLog_RTSO, Error, TEXT(
			"\nAGodHandPawn::ProcessPlaceBuildable "
			"\n-- Selected 'CurrentGhost' does not implement IPDRTSBuildableGhostInterface "
			"\n-- Exiting function "))
		return;
	}

	const bool bIsGhostEncroached = IPDRTSBuildableGhostInterface::Execute_GetGhostAsEncroached(CurrentGhost);
	if (bIsGhostEncroached)
	{
		// @done send error notification via message subsystem?
		// @todo Assign the given tag to the message datatable so this resolves to something
		UPDUserMessageSubsystem::Get()->SendMessageToUser_Server(TAG_MSG_Actions_CannotPlaceBuilding_Encroachment, GetController<APlayerController>());
		
		return; // don't place buildings that are encroached
	}
	

	const UPDBuilderSubsystemSettings* DeveloperSettings = GetDefault<UPDBuilderSubsystemSettings>();

	const PD::Build::Behaviour::Cost     CostBehaviour     = DeveloperSettings->DefaultBuildSystemBehaviours.Cost;
	const PD::Build::Behaviour::Progress ProgressBehaviour = DeveloperSettings->DefaultBuildSystemBehaviours.Progression;
	
	const bool NonImmediate_SpawnGhost  = CostBehaviour   != PD::Build::Behaviour::Cost::EFree && ProgressBehaviour != PD::Build::Behaviour::Progress::EImmediate;
	const bool Immediate_SpawnGhost     = (CostBehaviour  != PD::Build::Behaviour::Cost::EFree && ProgressBehaviour == PD::Build::Behaviour::Progress::EImmediate)
	                                    || (CostBehaviour == PD::Build::Behaviour::Cost::EFree && ProgressBehaviour != PD::Build::Behaviour::Progress::EImmediate);
	
	const bool Immediate_SpawnBuildable = CostBehaviour == PD::Build::Behaviour::Cost::EFree && ProgressBehaviour == PD::Build::Behaviour::Progress::EImmediate;

	if (NonImmediate_SpawnGhost)
	{
		SpawnFromGhost(false, true);
	}
	else if (Immediate_SpawnGhost)
	{
		SpawnFromGhost(false, false);
	}
	else if (Immediate_SpawnBuildable)
	{
		SpawnFromGhost(true);
	}
}

bool AGodHandPawn::AttemptSelectSingleUnit()
{
	// const bool ImmutableMoveInput = Value.Get<bool>();
	ARTSOController* PC = GetController<ARTSOController>();
	if (PC == nullptr || PC->IsValidLowLevelFast() == false || EntityManager->IsEntityValid(InstanceState.SelectedWorkerUnitHandle) == false)
	{
		return false;
	}

	const FVector CollisionLocation = Collision->GetComponentLocation();
	const FTransform& TargetInstanceTransform = GetEntityTransform(InstanceState.SelectedWorkerUnitHandle);
	const FVector EntityLocation = TargetInstanceTransform.GetLocation(); 
	const FVector DeltaLocation = EntityLocation - CollisionLocation;

	if (DeltaLocation.IsNearlyZero(PC->ClickToSelectErrorMin))
	{
		ResetPathParameters();

		TMap<int32, FMassEntityHandle> PassThroughHandle;
		PassThroughHandle.Emplace(InstanceState.SelectedWorkerUnitHandle.Index, InstanceState.SelectedWorkerUnitHandle);
		PC->GetMutableMarqueeSelectionMap().FindOrAdd(PC->GeneratedGroupID(), PassThroughHandle);
		PC->OnSelectionChange(false);
		PC->OnMarqueeSelectionUpdated( INDEX_NONE, {InstanceState.SelectedWorkerUnitHandle.Index});
		return true;
	}
	return false;
}

void AGodHandPawn::DispatchUnitActionWithTargets()
{
	ARTSOController* PC = GetController<ARTSOController>();
	if (PC == nullptr || PC->IsValidLowLevelFast() == false) { return; }
	
	
	const bool bIsValidFallbackLocation = true; // @todo actually do a location query here 
	
	const FGameplayTagContainer AssociatedTags = InstanceState.WorkerUnitActionTarget != nullptr && InstanceState.WorkerUnitActionTarget->Implements<UPDInteractInterface>()
		? IPDInteractInterface::Execute_GetGenericTagContainer(InstanceState.WorkerUnitActionTarget)
		: bIsValidFallbackLocation ? FGameplayTagContainer{TAG_AI_Job_WalkToTarget} : FGameplayTagContainer{}; 

	// remove the AssociatedTags.IsEmpty() and instead 
	if (AssociatedTags.IsEmpty() || EntityManager->IsEntityValid(InstanceState.SelectedWorkerUnitHandle) == false)
	{
		UE_LOG(PDLog_RTSO, Warning, TEXT("DispatchUnitActionWithTargets -- Unexpected error -- Clearing selected worker unit "))

		ResetPathParameters();
		return;
	}

	// //
	// // @todo fix path not updating

	FMassInt16Vector FallbackTargetLocation{};
	if (InstanceState.WorkerUnitActionTarget == nullptr)
	{
		FVector2D ScreenCoordinates;
		bool bFoundInputType;
		FHitResult HitResult = PC->ProjectMouseToGroundPlane(PC->DedicatedLandscapeTraceChannel, ScreenCoordinates, bFoundInputType);
		FallbackTargetLocation = HitResult.bBlockingHit ? FMassInt16Vector{HitResult.Location} : FMassInt16Vector{Collision->GetComponentLocation()};
	}
	
	const TArray<TObjectPtr<UInstancedStaticMeshComponent>>& ISMs = UPDRTSBaseSubsystem::GetMassISMs(GetWorld());
	if (ISMs.IsEmpty() == false && Cast<UPDRTSBaseUnit>(ISMs[0]))
	{
		const FVector& TestVector = FallbackTargetLocation.Get();
		UE_LOG(PDLog_RTSO, Warning, TEXT("DispatchUnitActionWithTargets -- Sending action request to entity : FallbackTargetLocation(x: %f ,y: %f ,z: %f) "), TestVector.X, TestVector.Y, TestVector.Z);
		
		FPDTargetCompound OptTarget = {PD::Mass::InvalidHandle, FallbackTargetLocation, InstanceState.WorkerUnitActionTarget};
		Cast<UPDRTSBaseUnit>(ISMs[0])->RequestAction(PC->GetActorID(), OptTarget, AssociatedTags.GetByIndex(0), InstanceState.SelectedWorkerUnitHandle);
	}

	FPDMFragment_RTSEntityBase* PermadevEntityBase = EntityManager->GetFragmentDataPtr<FPDMFragment_RTSEntityBase>(InstanceState.SelectedWorkerUnitHandle);
	if (PermadevEntityBase != nullptr)
	{
		// The initial call to the walk task will overwrite this in case it has an entry,
		// but if it is empty it will default to using a shared path instead
		PermadevEntityBase->QueuedUnitPath.Emplace(PD::Constants::INVALID_WORLD_LOC);
		PermadevEntityBase->OwnerID = PC->GetActorID();
	}	
}

//
// @thoughts: How to handle selected units?
void AGodHandPawn::ActionWorkerUnit_Completed_Implementation(const FInputActionValue& Value)
{
	// const bool ImmutableMoveInput = Value.Get<bool>();
	ARTSOController* PC = GetController<ARTSOController>();
	if (PC == nullptr || PC->IsValidLowLevelFast() == false || bJustPlacedBuildable)
	{
		// Buildable data takes priority now, but: @todo create and action/event PriorityQueue and handle each 'event' by the order of that queue!
		bJustPlacedBuildable = false;
		return;
	}

	if (PC->IsDrawingMarquee())
	{
		PC->MarqueeSelection(EMarqueeSelectionEvent::RELEASEMARQUEE);
		return;
	}
	PC->DeactivateMappingContext(TAG_CTRL_Ctxt_DragMove);
	
	// If false we failed selecting a single unit/ There was nothing to select
	if (AttemptSelectSingleUnit()) { return; }

	// Dispatch the action
	DispatchUnitActionWithTargets();

	const bool bVisualizeWorkerPaths = GetDefault<UPDRTSSubsystemSettings>()->bVisualizeWorkerPaths;
	if (bVisualizeWorkerPaths && NC_WorkerPath != nullptr) { NC_WorkerPath->DestroyInstance(); }
	
	InstanceState.bUpdatePathOnTick = false;
	InstanceState.SelectedWorkerUnitHandle = {0, 0};
	UE_LOG(PDLog_RTSO, Warning, TEXT("ActionWorkerUnit_Completed -- Clearing selected worker unit "))
}

void AGodHandPawn::ResetPathParameters()
{
	const bool bVisualizeWorkerPaths = GetDefault<UPDRTSSubsystemSettings>()->bVisualizeWorkerPaths;
	if (bVisualizeWorkerPaths && NC_WorkerPath != nullptr) { NC_WorkerPath->DestroyInstance(); }
	
	InstanceState.bUpdatePathOnTick = false;
	InstanceState.WorkUnitTargetLocation = PD::Constants::INVALID_WORLD_LOC;
	InstanceState.WorkerUnitActionTarget = nullptr;
}

void AGodHandPawn::ActionBuildMode_Implementation(const FInputActionValue& Value)
{
	// const bool ImmutableMoveInput = Value.Get<bool>(); 
}

void AGodHandPawn::ActionClearSelection_Implementation(const FInputActionValue& Value)
{
}

// DONE: Selection groups and shared navpath fragments
void AGodHandPawn::ActionMoveSelection_Implementation(const FInputActionValue& Value)
{
	FString BuildString = FString::Printf(TEXT("AGodHandPawn(%s)::ActionMoveSelection -- "), *GetName());
	// const bool ImmutableMoveInput = Value.Get<bool>();
	ARTSOController* PC = GetController<ARTSOController>();
	if (PC == nullptr || PC->IsValidLowLevelFast() == false)
	{
		// BuildString += FString::Printf(TEXT("\n PC INVALID "));
		// UE_LOG(PDLog_RTSO, Warning, TEXT("%s"), *BuildString);
		return;
	}
	
	const int32 CurrentGroupID = PC->GetCurrentGroupID();
	if (PC->GetImmutableMarqueeSelectionMap().Contains(CurrentGroupID) == false)
	{
		PC->ProcessPotentialBuildableMenu(InstanceState.HoveredActor);
		return;
	}
	PC->GetBuildableActionsWidget()->SetNewWorldActor(nullptr, FPDBuildable{});
	
	const TArray<TObjectPtr<UInstancedStaticMeshComponent>>& ISMs = UPDRTSBaseSubsystem::GetMassISMs(GetWorld());
	if (ISMs.IsEmpty() || Cast<UPDRTSBaseUnit>(ISMs[0]) == nullptr)
	{
		// BuildString += FString::Printf(TEXT("\n ISM INVALID "));
		// UE_LOG(PDLog_RTSO, Warning, TEXT("%s"), *BuildString);
		return;
	}
	
	// refresh hovered actor as target
	InstanceState.WorkerUnitActionTarget = InstanceState.HoveredActor;

	// go to mouse location is no valid target
	FMassInt16Vector SelectedLocation = PD::Mass::InvalidLoc;

	FGameplayTagContainer FallbackContainer{TAG_AI_Job_Idle};
	if (InstanceState.WorkerUnitActionTarget == nullptr)
	{
		// BuildString += FString::Printf(TEXT("\n ACTOR (todo: OR ENTITY) TARGET INVALID, TRACING TO NEW STATIC LOCATION TARGET"));
		// UE_LOG(PDLog_RTSO, Warning, TEXT("%s"), *BuildString);
		
		FVector2D ScreenCoordinates;
		bool bFoundInputType;

		FHitResult HitResult = PC->ProjectMouseToGroundPlane(PC->DedicatedLandscapeTraceChannel, ScreenCoordinates, bFoundInputType);
		if (HitResult.bBlockingHit)
		{
			FallbackContainer = FGameplayTagContainer{TAG_AI_Job_WalkToTarget};
			SelectedLocation.Set(HitResult.Location);
		}
	}
	
	//
	// Params for entity AI
	const FPDTargetCompound OptTarget = {PD::Mass::InvalidHandle, SelectedLocation, InstanceState.WorkerUnitActionTarget};
	const FGameplayTagContainer AssociatedTags = InstanceState.WorkerUnitActionTarget != nullptr && InstanceState.WorkerUnitActionTarget->Implements<UPDInteractInterface>()
		? IPDInteractInterface::Execute_GetGenericTagContainer(InstanceState.WorkerUnitActionTarget)
		: FallbackContainer;
	
	const FHitResult& Start = PC->GetLatestStartHitResult();
	const FHitResult& Center = PC->GetLatestCenterHitResult();
	const FHitResult& End    = PC->GetLatestEndHitResult();
	const FVector StartLocation =
		Center.bBlockingHit  ? Center.Location
		: Start.bBlockingHit ? Start.Location
		: End.bBlockingHit   ? StartLocation
		: PD::Constants::INVALID_WORLD_LOC;
	
	Cast<UPDRTSBaseUnit>(ISMs[0])->RequestActionMulti(
		PC->GetActorID(),
		OptTarget,
		AssociatedTags.GetByIndex(0),
		*PC->GetImmutableMarqueeSelectionMap().Find(CurrentGroupID), 
		StartLocation,
		CurrentGroupID);
}

void AGodHandPawn::ActionAssignSelectionToHotkey_Implementation(const FInputActionValue& Value)
{
}

void AGodHandPawn::ActionHotkeySelection_Implementation(const FInputActionValue& Value)
{
}

void AGodHandPawn::ActionChordedBase_Implementation(const FInputActionValue& Value)
{
}

void AGodHandPawn::OnItemUpdate(const FPDItemNetDatum& BuildableResourceDatum)
{
	UPDBuilderSubsystem* BuilderSubsystem = UPDBuilderSubsystem::Get();
	ensure(BuilderSubsystem != nullptr);

	// FILO
	AActor* TryLast = nullptr;
	TDeque<AActor*>& QueuedBuildablesToBuy = BuilderSubsystem->BuildQueues.FindOrAdd(IPDRTSBuilderInterface::GetBuilderID()).ActorInWorld;
	while (QueuedBuildablesToBuy.Num() > 0)
	{
		TryLast = QueuedBuildablesToBuy.Last();

		if (TryLast == nullptr)
		{
			QueuedBuildablesToBuy.PopLast();
			continue;
		}
		break;
	}

	// The actors in 'UPDRTSBaseSubsystem::BuildQueues' all implement IPDRTSBuildableGhostInterface
	if (TryLast != nullptr && IPDRTSBuildableGhostInterface::Execute_AttemptFinalizeGhost(TryLast))
	{
		QueuedBuildablesToBuy.PopLast();
	}

}

int32 AGodHandPawn::GetBuilderID_Implementation()
{
	AController* PC = GetController();
	if (PC == nullptr && CachedActorID == FPDPersistentID::INVALID_ID)
	{
		UE_LOG(PDLog_RTSO, Warning, TEXT("AGodHandPawn(%s)::GetBuilderID() \n PC == nullptr and CachedActorID == INDEX_NONE"), *GetName());
		return INDEX_NONE;
	}

	// If controller has become invalidated temporarily but cached index still remains, just return it so upon reconnection we can hook it back up 
	const bool bPCValid = PC != nullptr && PC->GetClass() != nullptr && PC->GetClass()->ImplementsInterface(UPDRTSBuilderInterface::StaticClass());
	const bool bRefreshActorID = bPCValid && CachedActorID == FPDPersistentID::INVALID_ID;
	CachedActorID = bRefreshActorID ? IPDRTSBuilderInterface::Execute_GetBuilderID(PC) : CachedActorID;
	return CachedActorID;
}

//
// Utility functions
FMassEntityHandle AGodHandPawn::OctreeEntityTrace_DEPRECATED(const FVector& StartLocation, const FVector& EndLocation)
{
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
		return FMassEntityHandle{0,0};
	PRAGMA_ENABLE_DEPRECATION_WARNINGS
}

AActor* AGodHandPawn::FindClosestInteractableActor() const
{
	TArray<AActor*> Overlap;
	Collision->GetOverlappingActors(Overlap);
	
	AActor* ClosestActor = nullptr;
	for (AActor* FoundActor : Overlap)
	{
		if (FoundActor == nullptr || FoundActor->GetClass()->ImplementsInterface(UPDInteractInterface::StaticClass()) == false)
		{
			continue;
		}
		if (IPDInteractInterface::Execute_GetCanInteract(FoundActor) == false)
		{
			continue;
		}

		if (ClosestActor == nullptr)
		{
			ClosestActor = FoundActor;
			continue;
		}
		
		const float LengthToOldActor = (ClosestActor->GetActorLocation() - Collision->GetComponentLocation()).Length();
		const float LengthToNewActor = (FoundActor->GetActorLocation() - Collision->GetComponentLocation()).Length();
		if (LengthToNewActor < LengthToOldActor)
		{
			ClosestActor = FoundActor; // <- find closest actor and store in this
		}
	}
	return ClosestActor;
}

const FTransform& AGodHandPawn::GetEntityTransform(const FMassEntityHandle& Handle) const
{
	// For our purposes transform fragment should never be nullptr in case the handle itself is valid
	check(EntityManager->GetFragmentDataPtr<FTransformFragment>(Handle) != nullptr);
	return EntityManager->IsEntityValid(Handle) ? EntityManager->GetFragmentDataPtr<FTransformFragment>(Handle)->GetTransform() : Collision->GetComponentTransform();
}

int32 AGodHandPawn::GetEntityOwnerID(const FMassEntityHandle& Handle) const
{
	// For our purposes transform fragment should never be nullptr in case the handle itself is valid
	check(EntityManager->GetFragmentDataPtr<FTransformFragment>(Handle) != nullptr);
	return EntityManager->IsEntityValid(Handle) ? EntityManager->GetFragmentDataPtr<FPDMFragment_RTSEntityBase>(Handle)->OwnerID : INDEX_NONE;
}


FMassEntityHandle AGodHandPawn::FindClosestMassEntity() const
{
	const UPDRTSBaseSubsystem* RTSBaseSubsystem = UPDRTSBaseSubsystem::Get();
	FQueryResult_LocAndId LocAndId = RTSBaseSubsystem->OctreeUserQuery.ReadQueryBufferFromEnd<EPDQueryGroups::QUERY_GROUP_HOVERSELECTION>(0);
	if (LocAndId.Location == FVector{})
	{
		return FMassEntityHandle{0,0};
	}

	// Temporary work-around to resolve edge-case created by fixing a data-race in the query buffer 
	// Ensure that it is not a hovered entity from a previous frame still lingering in the query buffer
	const FVector DeltaV = Collision->GetComponentLocation() - LocAndId.Location;
	if (DeltaV.IsNearlyZero(50.0) == false)
	{
		return FMassEntityHandle{0,0};
	}

	// Don't allow handling entities we do not own
	AGodHandPawn* MutableThis = const_cast<AGodHandPawn*>(this);
	if (LocAndId.OwnerID != IPDRTSBuilderInterface::Execute_GetBuilderID(MutableThis))
	{
		return FMassEntityHandle{0,0};
	}
	
	return LocAndId.EntityHandle; 
}

//
// Boilerplate - Setup

AGodHandPawn::AGodHandPawn()
{
	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
	SceneRoot  = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	InteractComponent  = CreateDefaultSubobject<UPDInteractComponent>(TEXT("InteractComponent"));
	InventoryComponent = CreateDefaultSubobject<UPDInventoryComponent>(TEXT("InventoryComponent"));
	
	Springarm = CreateDefaultSubobject<USpringArmComponent>(TEXT("Springarm"));
	Springarm->SetupAttachment(SceneRoot);
	Springarm->TargetArmLength = 800.0f;
	Springarm->SocketOffset = FVector{250.0f, 0, 0};
	Springarm->CameraLagSpeed = 5.0f;
	Springarm->bEnableCameraRotationLag = true;
	Springarm->bDoCollisionTest = false;
	
	Camera     = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(Springarm);
	Camera->FieldOfView = 25.0f;
	InteractComponent->SetCamera(Camera);
	
	CursorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CursorMesh"));
	CursorMesh->SetupAttachment(SceneRoot);
	CursorMesh->SetGenerateOverlapEvents(false);
	CursorMesh->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);

	Collision  = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	Collision->SetupAttachment(SceneRoot);
	Collision->InitSphereRadius(50.0f);
	PawnMovement = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("FloatingMovement"));
	ParticipantComponent = CreateDefaultSubobject<UConversationParticipantComponent>(TEXT("ConversationParticipantComponent"));	
}

void AGodHandPawn::BeginPlay()
{
	Super::BeginPlay();

	Collision->OnComponentBeginOverlap.AddDynamic(this, &AGodHandPawn::OnCollisionBeginOverlap);
	Collision->OnComponentEndOverlap.AddDynamic(this, &AGodHandPawn::OnCollisionEndOverlap);

	UPDRTSBaseSubsystem* RTSSubSystem = UPDRTSBaseSubsystem::Get();
	UPDBuilderSubsystem* BuilderSubsystem = UPDBuilderSubsystem::Get();
	ensure(RTSSubSystem != nullptr);
	ensure(BuilderSubsystem != nullptr);

	// Starting query data
	const FVector& QueryLocation = CursorMesh->GetComponentLocation();
	const UE::Math::TVector<double> MinimapQueryExtent{2000.0, 2000.0, 500.0};
	const double CollisionRadius = Collision->GetScaledSphereRadius(); 
	const UE::Math::TVector<double> HoverSelectionQueryExtent{CollisionRadius};

	// Query shapes
	const FPDOctreeUserQuery::QBox MinimapQueryBox          = FPDOctreeUserQuery::MakeUserQuery(QueryLocation, MinimapQueryExtent);
	const FPDOctreeUserQuery::QBox HoverSelectionQueryBox   = FPDOctreeUserQuery::MakeUserQuery(QueryLocation, HoverSelectionQueryExtent);
	const FPDOctreeUserQuery::QSphere PingSystemQuerySphere = FPDOctreeUserQuery::MakeUserQuery(QueryLocation, 10000.0); // need to update whn calling, start att 100m however

	// RTS Subsystem initialized
	RTSSubSystem->OctreeUserQuery.CreateNewQueryEntry(EPDQueryGroups::QUERY_GROUP_MINIMAP, FPDOctreeUserQuery::EBufferType::PackedData, MinimapQueryBox);
	RTSSubSystem->OctreeUserQuery.CreateNewQueryEntry(EPDQueryGroups::QUERY_GROUP_HOVERSELECTION, FPDOctreeUserQuery::EBufferType::EntityCompound, HoverSelectionQueryBox);
	RTSSubSystem->OctreeUserQuery.SetCallingUser(this);

	RTSSubSystem->BuildEntitySortComputeShader = FRTSBuildGlobalSortEntityShader::CreateLambda(
		[](FRHICommandListImmediate& RHICmdList, UTextureRenderTarget2D* RenderTarget, const TRefCountPtr<FRDGPooledBuffer>& EntityInputPooledBuffer, TArray<FLinearColor> InData, float CameraYawInRadians, FVector RegionMin, FVector RegionSize)
		{
			TShaderMapRef<FRTSMinimapSplat> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
			ComputeShader->BuildAndExecuteGraph(RHICmdList, RenderTarget, EntityInputPooledBuffer, InData, CameraYawInRadians, RegionMin, RegionSize);
		});
	RTSSubSystem->WorldInit(GetWorld());
	EntityManager = RTSSubSystem->EntityManager;
	
	// Builder Subsystem initialized
	BuilderSubsystem->OctreeBuildSystemEntityQuery.CreateNewQueryEntry(EPDQueryGroups::QUERY_GROUP_BUILDABLE_ACTORS, FPDOctreeUserQuery::EBufferType::INVALID, PingSystemQuerySphere);
	BuilderSubsystem->OctreeBuildSystemEntityQuery.SetCallingUser(this);
	BuilderSubsystem->WorldInit(GetWorld());
	
	RefreshSettingsInstance();

	UpdateMagnification();
	InitializeISMAgent();

	// There is a bug causing any modifier I am applying to the IA to be called but the modified value to be discarded,
	// I have however confirmed that the value in here at-least is correct
	// A temporary way around it maybe have to be a subsystem or other singleton which caches a stack of modifier values 	
	URTSOInputStackSubsystem* InputStackWorkaround = GetWorld()->GetSubsystem<URTSOInputStackSubsystem>();
	InputStackWorkaround->DispatchValueStackReset();
}

void AGodHandPawn::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	UPDRTSBaseSubsystem* RTSSubSystem = UPDRTSBaseSubsystem::Get();
	ensure(RTSSubSystem != nullptr);
	RTSSubSystem->WorldDeinit(GetWorld());

	Super::EndPlay(EndPlayReason);
}

void AGodHandPawn::RefreshSettingsInstance()
{
	const FString Context = FString::Printf(TEXT("(%s)::BeginPlay()"), *GetName());
	InstanceSettings = *GodHandSettingsHandle.GetRow<FRTSGodhandSettings>(Context);
}

EDataValidationResult AGodHandPawn::IsDataValid(FDataValidationContext& Context) const
{
	const FString ContextString = FString::Printf(TEXT("(%s)::IsDataValid()"), *GetName());
	const FRTSGodhandSettings* FoundData = GodHandSettingsHandle.GetRow<FRTSGodhandSettings>(ContextString);

	UE_LOG(PDLog_RTSO, Warning, TEXT("(%s)::IsDataValid() \n FoundData: %i \n FoundData->AssetData: %i"), *GetName(), FoundData != nullptr, FoundData != nullptr ? FoundData->AssetData != nullptr : 0);

	return FoundData != nullptr && FoundData->AssetData != nullptr ? Super::IsDataValid(Context) : EDataValidationResult::Invalid;
}

void AGodHandPawn::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
}

void AGodHandPawn::SelectBuildMenuEntry_Implementation(ERTSBuildMenuModules ActionMode, FGameplayTag ActionTag)
{
	IPDRTSBuilderInterface::SelectBuildMenuEntry_Implementation(ActionMode, ActionTag);

	UPDBuilderSubsystem* BuilderSubsystem = UPDBuilderSubsystem::Get();
	ensure(BuilderSubsystem != nullptr);
	
	switch (ActionMode)
	{
	case ERTSBuildMenuModules::SelectBuildable:
		CurrentBuildableData = BuilderSubsystem->GetBuildableData(ActionTag); // If null clear current
		CurrentBuildableTag = ActionTag;
		UE_LOG(PDLog_RTSO, Warning, TEXT("AGodHandPawn::NewAction - Selected Buildable (%s)"), *ActionTag.ToString())
		break;
	case ERTSBuildMenuModules::SelectContext:
		CurrentBuildContext = BuilderSubsystem->GetBuildContextEntry(ActionTag); // If null clear current
		CurrentBuildContextTag = ActionTag; 
		UE_LOG(PDLog_RTSO, Warning, TEXT("AGodHandPawn::NewAction - Selected BuildContext (%s)"), *ActionTag.ToString())
		break;
	case ERTSBuildMenuModules::DeselectBuildable:
		CurrentBuildableData = nullptr;
		CurrentBuildableTag = FGameplayTag::EmptyTag; 
		UE_LOG(PDLog_RTSO, Warning, TEXT("AGodHandPawn::NewAction - Deelected Buildable (%s)"), *ActionTag.ToString())
		break;
	case ERTSBuildMenuModules::DeselectContext:
		CurrentBuildContext = nullptr;
		CurrentBuildContextTag = FGameplayTag::EmptyTag; 
		UE_LOG(PDLog_RTSO, Warning, TEXT("AGodHandPawn::NewAction - Deselected BuildContext (%s)"), *ActionTag.ToString())
		break;
	}
}

void AGodHandPawn::PerformAction_Destroy(const TArray<uint8>& Payload)
{
	const ARTSOController* PC = GetController<ARTSOController>();
	const UPDBuildingActionsWidgetBase* BuildableActionsWidget = PC != nullptr ? PC->GetBuildableActionsWidget() : nullptr;
	if (BuildableActionsWidget == nullptr || BuildableActionsWidget->CurrentWorldActor == nullptr) { return; }

	if (BuildableActionsWidget->CurrentWorldActor->GetClass()->ImplementsInterface(UPDInteractInterface::StaticClass())
		&& GetController()->GetClass()->ImplementsInterface(UPDRTSBuilderInterface::StaticClass()))
	{
		const FGameplayTag& BuildingType = IPDInteractInterface::Execute_GetInstigatorTag(BuildableActionsWidget->CurrentWorldActor);
		const int32 InstigatorID = IPDRTSBuilderInterface::Execute_GetBuilderID(GetController());

		const FRTSOActionLogEvent NewActionEvent{TAG_ActionLog_Styling_Entry_T0, TAG_ActionLog_Styling_Timestamp_T0,
			FString::Printf(TEXT("OwnerID(%i) -- Successfully destroying building(TODO{%i}) of type %s "),
			PC->GetActorID(), INDEX_NONE, *BuildingType.GetTagName().ToString())}; 
		URTSActionLogSubsystem::DispatchEvent(InstigatorID, NewActionEvent);		
	}

	RefreshInstanceState(BuildableActionsWidget->CurrentWorldActor);
	BuildableActionsWidget->DestroyCurrentWorldActor();	
}

void AGodHandPawn::RefreshInstanceState(const AActor* DestroyedTargetActor)
{
	InstanceState.HoveredActor = InstanceState.HoveredActor == DestroyedTargetActor ? nullptr : InstanceState.HoveredActor;
	InstanceState.WorkerUnitActionTarget = InstanceState.WorkerUnitActionTarget == DestroyedTargetActor ? nullptr : InstanceState.WorkerUnitActionTarget;
	CameraInterpTarget = CameraInterpTarget == DestroyedTargetActor ? nullptr : CameraInterpTarget;
}


void AGodHandPawn::SelectActionMenuEntry_Implementation(ERTSBuildableActionMenuModules ActionMode, FGameplayTag ActionTag, const TArray<uint8>& Payload)
{
	IPDRTSBuilderInterface::SelectActionMenuEntry_Implementation(ActionMode, ActionTag, Payload);

	UPDBuilderSubsystem* BuilderSubsystem = UPDBuilderSubsystem::Get();
	ensure(BuilderSubsystem != nullptr);
	
	switch (ActionMode)
	{
		
	case ERTSBuildableActionMenuModules::SelectBuildableActionContext:
		{
			// todo: write actual impl.
			const FPDBuildActionContext** BuildActionContext = BuilderSubsystem->GrantedActionContexts_KeyedByBuildableTag.Find(ActionTag);
			if (BuildActionContext == nullptr) { break; }
			break;
		}
	case ERTSBuildableActionMenuModules::DeselectBuildableActionContext:
		{
			// todo: write actual impl.
			BuilderSubsystem->GrantedActionContexts_KeyedByBuildableTag;
			break;
		}	
	case ERTSBuildableActionMenuModules::FireBuildableAction:
		{
			const FPDBuildAction** FoundActionData = BuilderSubsystem->ActionData_WTag.Find(ActionTag);
			if (FoundActionData == nullptr) { break; }

			if (ActionTag == TAG_BUILD_Actions_DestroyBuilding)
			{
				const ARTSOController* PC = GetController<ARTSOController>();
				if (PC == nullptr) { break; }
				
				switch (GetDefault<UPDBuilderSubsystemSettings>()->DefaultBuildSystemBehaviours.DestructionBehaviour)
				{
				case EPDRTSDestroyBuildingBehaviour::RequireConfirmationDialog:
					{
						UPDBuildingActionsWidgetBase* BuildableActionWidget = PC->GetBuildableActionsWidget();
						const TSubclassOf<UPDGenericDialog>& ConfirmDialog = BuildableActionWidget->ConfirmDialogClass;
						UPDGenericDialog* Dialog = CreateWidget<UPDGenericDialog>(BuildableActionWidget, ConfirmDialog != nullptr ? ConfirmDialog.Get() : UPDGenericDialog::StaticClass());

						Dialog->DialogMessage = GetDefault<UPDBuildMessages>()->ConfirmDestroyBuilding;
						Dialog->SuccessCallback.BindUObject(this, &AGodHandPawn::PerformAction_Destroy);
						Dialog->AddToViewport(10);
						Dialog->SetupDelegates();
					}
					break;
				case EPDRTSDestroyBuildingBehaviour::ImmediateDestruction:
					{
						const TArray<uint8> DummyPayload;
						PerformAction_Destroy(DummyPayload);
					}
					break;
				}
				
				
				break;
			}

			// TAG_AI_Type_ type tags are implicitly counted as spawn actions when applied as a 'buildable action tag' in an entry in a 'FPDBuildable' datatable
			if (ActionTag.MatchesTag(TAG_AI_Type))
			{
				const ARTSOController* PC = GetController<ARTSOController>();
				if (PC == nullptr)
				{
					//@todo error message here
					return;
				}
				
				// Check if we can match it against a type in our subsystem
				const bool bIsMatchedAgainst = BuilderSubsystem->GrantedBuildContexts_WorkerTag.Contains(ActionTag);
				if (bIsMatchedAgainst == false) { return; }

				// done 0. default to a single spawn if we have no payload instructing us how many we should spawn 
				uint32 SpawnCount = Payload.IsValidIndex(3) ? RESTORE_BYTE(Payload, 0) | RESTORE_BYTE(Payload, 1) | RESTORE_BYTE(Payload, 2) | RESTORE_BYTE(Payload, 3) : 1;
				
				// @DONE 0. Ensure we afford the action
				if (InventoryComponent->CanAfford((*FoundActionData)->ActionCost), SpawnCount)
				{
					// @DONE 1. Need to select a entity config and use it to spawn
					// @DONE 2. Will need some mapping between unit-type tags and their entity configs, multiple unit-type tags may share the same configs
					// @DONE 3. Spawn here now that have validated that the worker is valid type
					// @todo 4. need a spawn queue and unit spawn rules, timings and such
					

					// @todo Need to generalize the spawn logic
					UPDRTSBaseSubsystem* RTSSubsystem = UPDRTSBaseSubsystem::Get();
					UMassSpawnerSubsystem* SpawnerSystem = UWorld::GetSubsystem<UMassSpawnerSubsystem>(GetWorld());
					if (EntityManager == nullptr) { return; }

					TMap<const FMassEntityTemplateID, ARTSOBaseGM::FEntityCompoundTuple> EntitiesToSpawn{};

					// Gather entities to spawn
					FRTSSavedWorldUnits ConstructedEntityData{};
					ConstructedEntityData.CurrentAction.ActionTag = TAG_AI_Job_Idle;
					ConstructedEntityData.EntityUnitTag = ActionTag;
					ConstructedEntityData.OwnerID = IPDRTSBuilderInterface::Execute_GetBuilderID(this);
					
					// @todo need to offset the actor location, right now it spawns in the middle of it
					ConstructedEntityData.Location = PC->GetBuildableActionsWidget()->CurrentWorldActor->GetActorLocation();
					for (uint32 SpawnIdx = 0; SpawnIdx < SpawnCount; SpawnIdx++)
					{
						ARTSOBaseGM::GatherEntityToSpawn(*GetWorld(), ConstructedEntityData, EntitiesToSpawn, RTSSubsystem, SpawnerSystem);
					}

					// Dispatch spawning of entities
					for (const TTuple<const FMassEntityTemplateID, ARTSOBaseGM::FEntityCompoundTuple>& EntityTypeCompound : EntitiesToSpawn)
					{
						ARTSOBaseGM::DispatchEntitySpawning(PC, EntityTypeCompound, EntityManager, SpawnerSystem);
						
						if (GetController()->GetClass()->ImplementsInterface(UPDRTSBuilderInterface::StaticClass()))
						{
							const int32 InstigatorID = IPDRTSBuilderInterface::Execute_GetBuilderID(GetController());
							const FRTSOActionLogEvent NewActionEvent{
								FString::Printf(TEXT("OwnerID(%i) -- Successfully Spawning entity of type %s "),
									IPDRTSBuilderInterface::Execute_GetBuilderID(this), *ActionTag.GetTagName().ToString())}; 
							URTSActionLogSubsystem::DispatchEvent(InstigatorID, NewActionEvent);		
						}
					}
				}
				else
				{
					// @done send error notification via message subsystem?
					// @todo Assign the given tag to the message datatable so this resolves to something
					UPDUserMessageSubsystem::Get()->SendMessageToUser_Server(TAG_MSG_Actions_CannotAffordAction, GetController<APlayerController>());
				}
				break;
			}

			FireAction(ActionTag); // If any other action then route it to BP
		}
		break;
	case ERTSBuildableActionMenuModules::DoNothing:
		{
			// todo: write actual impl.
			BuilderSubsystem->GrantedActionContexts_KeyedByBuildableTag;
			break;
		}		
	}
}

void AGodHandPawn::InitializeISMAgent()
{
	const FLatentActionInfo DelayInfo{0,0, TEXT("InitializeISMAgent"), this};
	if (UPDRTSBaseSubsystem::GetMassISMs(GetWorld()).IsEmpty())
	{
		UE_LOG(PDLog_RTSO, Warning, TEXT("AGodHandPawn::InitializeISMAgent - Dispatching another attempt in 1 second"))

		UKismetSystemLibrary::Delay(GetOwner(), 1.0f, DelayInfo);
		return;
	}
	
	// @todo debug why calling this invalidates the whole octree
	// UPDRTSBaseSubsystem::Get()->SetupOctreeWithNewWorld(GetWorld());
	
	InstanceState.ISMAgentComponent = Cast<UPDRTSBaseUnit>(UPDRTSBaseSubsystem::GetMassISMs(GetWorld())[0]);
	check(InstanceState.ISMAgentComponent != nullptr)
	
	InstanceState.ISMAgentComponent->SetEntityManager(EntityManager);
}

void AGodHandPawn::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	RefreshSettingsInstance();
	
	if (GetWorld() == nullptr) { return; }
	
	// There is a bug causing any modifier I am applying to the IA to be called but the modified value to be discarded,
	// I have however confirmed that the value in here at-least is correct
	// A temporary way around it maybe have to be a subsystem or other singleton which caches a stack of modifier values
	URTSOInputStackSubsystem* InputStackWorkaround = GetWorld()->GetSubsystem<URTSOInputStackSubsystem>();
	InputStackWorkaround->DispatchValueStackReset();	
}

// 
// Boilerplate - Collisions
void AGodHandPawn::OnCollisionBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult)
{
	UE_LOG(PDLog_RTSO, VeryVerbose, TEXT("AGodHandPawn::OnCollisionBeginOverlap"))
	
	// neither an ai or any interactable
	if (Cast<IPDInteractInterface>(OtherActor) == nullptr) { return; }
	InstanceState.HoveredActor = OtherActor;
}

void AGodHandPawn::ActorEndOverlapValidation()
{
	TArray<AActor*> Overlap;
	GetOverlappingActors(Overlap, AActor::StaticClass());
	InstanceState.HoveredActor = Overlap.IsEmpty() ? nullptr : InstanceState.HoveredActor;
}

void AGodHandPawn::OnCollisionEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	ActorEndOverlapValidation();
}

void AGodHandPawn::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);

	// not any interactable 
	if (Cast<IPDInteractInterface>(OtherActor) == nullptr)
	{
		InstanceState.HoveredActor = OtherActor;
		return;
	}
	
	InstanceState.ISMAgentComponent = OtherActor->GetComponentByClass<UPDRTSBaseUnit>(); 
}

void AGodHandPawn::NotifyActorEndOverlap(AActor* OtherActor)
{
	Super::NotifyActorEndOverlap(OtherActor);

	ActorEndOverlapValidation();
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
