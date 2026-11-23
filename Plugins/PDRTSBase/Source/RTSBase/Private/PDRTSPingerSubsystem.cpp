/* @author: Ario Amin @ Permafrost Development. @copyright: Full BSL(1.1) License included at bottom of the file  */

#include "PDRTSPingerSubsystem.h"
#include "PDRTSBaseSubsystem.h"

#include "MassEntitySubsystem.h"
#include "PDBuildCommon.h"
#include "PDBuilderSubsystem.h"
#include "Interfaces/PDRTSBuilderInterface.h"
#include "Pawns/PDRTSBaseUnit.h"
#include "RTSBase/Classes/PDRTSCommon.h"

struct FTransformFragment;
class UMassCrowdRepresentationSubsystem;
class AMassVisualizer;

FPDEntityPingDatum::FPDEntityPingDatum(uint32 InInstanceID)
{
	InstanceID = InInstanceID;
}

FPDEntityPingDatum::FPDEntityPingDatum(AActor* InWorldActor, const FGameplayTag& InJobTag)
	: WorldActor(InWorldActor)
	, JobTag(InJobTag)
{
	// World actor or job-tag invalid
	if ((ensure(WorldActor != nullptr) && ensure(JobTag.IsValid())) == false) { return; }

	// No Owner ID supplied in this ctor, call another ctor which supplies this to avoid this check
	check(WorldActor->GetOwner<APawn>() == nullptr && WorldActor->GetOwner<AController>() == nullptr);

	InstanceID = WorldActor->GetUniqueID();
	Unsafe_SetOwnerIDFromOwner();
}

FPDEntityPingDatum::FPDEntityPingDatum(AActor* InWorldActor, const FGameplayTag& InJobTag, int32 InAltID, double InInterval, int32 InMaxCountIntervals)
	: WorldActor(InWorldActor)
	, JobTag(InJobTag)
	, OwnerID(InAltID)
	, Interval(InInterval)
	, MaxCountIntervals(InMaxCountIntervals)
{
	// World actor or job-tag invalid
	if ((ensure(WorldActor != nullptr) && ensure(JobTag.IsValid())) == false) { return; }
	
	InstanceID = WorldActor->GetUniqueID();
	if (WorldActor->GetOwner<APawn>() == nullptr && WorldActor->GetOwner<AController>() == nullptr)
	{
		// Log error message
		return;
	}

	if (OwnerID != INDEX_NONE) { return; }

	Unsafe_SetOwnerIDFromOwner();	
}

FPDEntityPingDatum::~FPDEntityPingDatum()
{
	WorldActor = nullptr;
}

TArray<uint8> FPDEntityPingDatum::ToBytes() const
{
	const uint32 Bytes = GetTypeHash(*this);
	TArray<uint8> PingHashBytes = {MASK_BYTE(Bytes, 0), MASK_BYTE(Bytes, 1), MASK_BYTE(Bytes, 2), MASK_BYTE(Bytes, 3)};
	return PingHashBytes;
}

void FPDEntityPingDatum::FromBytes(const TArray<uint8>&& Bytes)
{
	InstanceID = RESTORE_BYTE(Bytes, 0) | RESTORE_BYTE(Bytes, 1) | RESTORE_BYTE(Bytes, 2) | RESTORE_BYTE(Bytes, 3);
}

void FPDEntityPingDatum::Unsafe_SetOwnerIDFromOwner()
{
	APawn* OwnerPawn = WorldActor->GetOwner<APawn>();
	AController* OwnerPC = OwnerPawn != nullptr ? OwnerPawn->GetController() : WorldActor->GetOwner<AController>();

	if (OwnerPawn->GetClass()->ImplementsInterface(UPDRTSBuilderInterface::StaticClass()))
	{
		OwnerID = IPDRTSBuilderInterface::Execute_GetBuilderID(OwnerPawn);
	}
	else if (OwnerPC->GetClass()->ImplementsInterface(UPDRTSBuilderInterface::StaticClass()))
	{
		OwnerID = IPDRTSBuilderInterface::Execute_GetBuilderID(OwnerPC);
	}
}

UPDEntityPinger::UPDEntityPinger(const FPDEntityPingDatum& PingDatum)
{
	PingDataAndHandles.Emplace(PingDatum);
}

UPDEntityPinger* UPDEntityPinger::Get()
{
	return GEngine->GetEngineSubsystem<UPDEntityPinger>();	
}

TArray<uint8>  UPDEntityPinger::AddPingDatum(const FPDEntityPingDatum& PingDatum)
{
	PingDataAndHandles.Emplace(PingDatum );
	return PingDatum.ToBytes();
}

void UPDEntityPinger::RemovePingDatum(const FPDEntityPingDatum& PingDatum)
{
	PingDataAndHandles.Remove(PingDatum);
}

void UPDEntityPinger::RemovePingDatumWithHash(const FPDEntityPingDatum& PingDatum)
{
	PingDataAndHandles.RemoveByHash(GetTypeHash(PingDatum), PingDatum);
}

FTimerHandle UPDEntityPinger::GetPingHandleCopy(const TArray<uint8>& PingHashBytes)
{
	FPDEntityPingDatum BuildDatum{0};
	BuildDatum.FromBytes(std::move(PingHashBytes));

	return GetPingHandleCopy(BuildDatum.InstanceID);
}

FTimerHandle UPDEntityPinger::GetPingHandleCopy(uint32 PingHash)
{
	const FPDEntityPingDatum CompDatum{PingHash};

	if (PingDataAndHandles.ContainsByHash(PingHash, CompDatum))
	{
		return FTimerHandle{};
	}
	
	return  *PingDataAndHandles.FindByHash(PingHash, CompDatum);
}


TArray<uint8> UPDEntityPinger::EnablePing(const FPDEntityPingDatum& PingDatum)
{
	UWorld* World = PingDatum.WorldActor ? PingDatum.WorldActor->GetWorld() : nullptr;
	if (World == nullptr)
	{
		UE_LOG(PDLog_RTSBase, Error, TEXT("FPDEntityPinger::EnablePing -- World it null or not initialized yet"));
		return TArray<uint8>{};
	}
	if (World->bIsWorldInitialized == false)
	{
		UE_LOG(PDLog_RTSBase, Warning, TEXT("FPDEntityPinger::EnablePing -- World is not initialized yet"));
		return TArray<uint8>{};
	}
	UE_LOG(PDLog_RTSBase, Log, TEXT("UPDEntityPinger::Ping -- Attempt to dispatch first Ping at freq(%f.2) for datum(%s:%s)"), PingDatum.Interval, *PingDatum.WorldActor->GetName(), *PingDatum.JobTag.ToString())
	
	FTimerHandle& PingHandleRef = PingDataAndHandles.FindOrAdd(PingDatum);
	PingDataAndHandlesReverse.FindOrAdd(PingHandleRef) = PingDatum;
	const auto InnerPing =
		[&, LambdaWorld = World, LambdaDatum = PingDatum]()
		{
			// UWorld* LambdaWorld = PingDatum.WorldActor ? PingDatum.WorldActor->GetWorld() : nullptr;
			if (LambdaWorld == nullptr || LambdaWorld->IsValidLowLevelFast() == false)
			{
				UE_LOG(PDLog_RTSBase, Error, TEXT("UPDEntityPinger::Ping Delegate -- WORLD INVALID"))
				return;
			}
			UE_LOG(PDLog_RTSBase, Log, TEXT("UPDEntityPinger::Ping Delegate -- WORLD VALID. PING PREPARING TO PROCESSED"))
				
			Ping(World, LambdaDatum);
		};

	const FTimerDelegate OnPingDlgt = FTimerDelegate::CreateLambda(InnerPing);

	World->GetTimerManager().SetTimer(PingHandleRef, OnPingDlgt, PingDatum.Interval, true);
	return PingDatum.ToBytes();
}

TArray<uint8> UPDEntityPinger::EnablePingStatic(const FPDEntityPingDatum& PingDatum)
{
	return UPDEntityPinger::Get()->EnablePing(PingDatum);
}

void UPDEntityPinger::Ping_Implementation(UWorld* World, const FPDEntityPingDatum& PingDatum)
{
	AsyncTask(ENamedThreads::GameThread,
		[ConstPingDatum = PingDatum, InWorld = World]()
		{
			UE_LOG(LogTemp, Log, TEXT("ARTSOInteractableBuildingBase::Ping"))

			const UPDBuilderSubsystem* BuilderSubsystem = UPDBuilderSubsystem::Get();
			const FGameplayTag FallBackEntityTag = TAG_AI_Type_BuilderUnit_Novice ; // @todo, pass into here from somewhere else 
			TArray<FGameplayTag> SelectedUnitTypes{FallBackEntityTag};
			if (ConstPingDatum.WorldActor == nullptr || BuilderSubsystem->Buildable_WClass.Contains(ConstPingDatum.WorldActor->GetClass()) == false)
			{
				UE_LOG(PDLog_BuildSystem, Log, TEXT("UPDEntityPinger::Ping() Was called with world actor : %s, Actor class is not a spawn type of any entry of a registered FPDBuildable data-table"), ConstPingDatum.WorldActor == nullptr ? *FString("INVALID ACTOR") : *ConstPingDatum.WorldActor->GetName() )
			}
			else
			{
				SelectedUnitTypes = BuilderSubsystem->ValidUnitTypes_PerBuildable.FindRef(BuilderSubsystem->Buildable_WClass.FindRef(ConstPingDatum.WorldActor->GetClass())->BuildableTag);
			}

			TArray<FMassEntityHandle> Handles =
				UPDRTSBaseSubsystem::FindIdleEntitiesOfType(SelectedUnitTypes, ConstPingDatum.WorldActor, ConstPingDatum.OwnerID);

			UE_LOG(LogTemp, Log, TEXT("ARTSOInteractableBuildingBase::Ping -- Found %i idle entities near ping actor"), Handles.Num())


			// Inworld refuses to survive past these two delegates, getting the world again from the world actor if it is valid
			ParallelFor(Handles.Num(),
				[InHandles = Handles, ConstPingDatum, InWorld = ConstPingDatum.WorldActor ? ConstPingDatum.WorldActor->GetWorld() : nullptr](const int32 Idx)
				{
					const UPDRTSBaseSubsystem* RTSSubsystem = UPDRTSBaseSubsystem::Get();
					
					const FMassEntityHandle& EntityHandle = InHandles[Idx];
					if (RTSSubsystem->EntityManager->IsEntityValid(EntityHandle) == false
						|| RTSSubsystem->WorldToEntityHandler.Contains(InWorld) == false)
					{
						UE_LOG(LogTemp, Log, TEXT("ARTSOInteractableBuildingBase::Ping -- FAIL(%p)"), InWorld)
						return;
					}

					const FPDTargetCompound OptTarget = {PD::Mass::InvalidHandle, ConstPingDatum.WorldActor->GetActorLocation(), ConstPingDatum.WorldActor};
					RTSSubsystem->WorldToEntityHandler.FindRef(InWorld)->RequestAction(ConstPingDatum.OwnerID, OptTarget, ConstPingDatum.JobTag, EntityHandle);
				},
				EParallelForFlags::BackgroundPriority);
		});
}

void UPDEntityPinger::DisablePing(const FPDEntityPingDatum& PingDatum)
{
	const UWorld* World = PingDatum.WorldActor ? PingDatum.WorldActor->GetWorld() : nullptr;
	if (World == nullptr)
	{
		UE_LOG(PDLog_RTSBase, Error, TEXT("FPDEntityPinger::EnablePing -- World it null or not initialized yet"));
		return;
	}
	if (World->bIsWorldInitialized == false)
	{
		UE_LOG(PDLog_RTSBase, Warning, TEXT("FPDEntityPinger::EnablePing -- World is not initialized yet"));
		return;
	}
	
	FTimerHandle& PingHandleRef = PingDataAndHandles.FindOrAdd(PingDatum);
	World->GetTimerManager().ClearTimer(PingHandleRef);
}

void UPDEntityPinger::DisablePingStatic(const FPDEntityPingDatum& PingDatum)
{
	UPDEntityPinger::Get()->DisablePing(PingDatum);
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
