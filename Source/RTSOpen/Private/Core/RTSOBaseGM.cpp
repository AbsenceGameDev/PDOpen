/* @author: Ario Amin @ Permafrost Development. @copyright: Full BSL(1.1) License included at bottom of the file  */
#include "Core/RTSOBaseGM.h"
#include "Core/RTSOBaseGI.h"

#include "RTSOpenCommon.h"
#include "PDInteractSubsystem.h"
#include "PDRTSBaseSubsystem.h"
#include "Actors/GodHandPawn.h"
#include "Actors/RTSOController.h"
#include "Actors/Interactables/ConversationHandlers/RTSOInteractableConversationActor.h"

#include "MassCommonFragments.h"
#include "MassEntitySubsystem.h"
#include "MassSpawnerSubsystem.h"
#include "PDBuilderSubsystem.h"
#include "Components/PDInventoryComponent.h"
#include "GameFramework/GameStateBase.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"


const FString ARTSOBaseGM::ROOTSAVE = "ROOTSAVE" ; 

ARTSOBaseGM::ARTSOBaseGM(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	DefaultPawnClass = AGodHandPawn::StaticClass();
	PlayerControllerClass = ARTSOController::StaticClass();
	// HUDClass;
	// GameStateClass;
	// PlayerStateClass;
	// SpectatorClass;
}


void ARTSOBaseGM::BeginPlay()
{
	Super::BeginPlay();

	TotalEntitiesSpawned = 0;
	GameSave = Cast<URTSOpenSaveGame>(UGameplayStatics::CreateSaveGameObject(URTSOpenSaveGame::StaticClass()));
	
	URTSOBaseGI* GI = Cast<URTSOBaseGI>(GetWorld()->GetGameInstance());
	if (GI == nullptr) { return; }
	// GI->StartTransition();

	MARK_PROPERTY_DIRTY_FROM_NAME(URTSOBaseGI, bGMReady, GI);
	GI->bGMReady = true;
}

void ARTSOBaseGM::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UPDRTSBaseSubsystem* RTSSubsystem = UPDRTSBaseSubsystem::Get();

	TArray<TSharedPtr<FOctreeElementId2>> CellIDs;
	RTSSubsystem->WorldEntityOctree.FindAllElements([&](const FPDEntityOctreeCell& EntityCell)
	{
		CellIDs.Add(EntityCell.SharedCellID);
	});

	for (TSharedPtr<FOctreeElementId2> CellIDPtr : CellIDs)
	{
		if (CellIDPtr.IsValid() == false) { continue; }

		FOctreeElementId2 CellID = *CellIDPtr.Get();
		if (RTSSubsystem->WorldEntityOctree.IsValidElementId(CellID) == false) {continue; }
		
		RTSSubsystem->WorldEntityOctree.RemoveElement(CellID);
	}

	RTSSubsystem->WorldToEntityHandler.Empty();
	CellIDs.Empty();
	
	Super::EndPlay(EndPlayReason);
}

void ARTSOBaseGM::Logout(AController* Exiting)
{
	Super::Logout(Exiting);
}

void ARTSOBaseGM::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (NewPlayer == nullptr || NewPlayer->GetClass()->ImplementsInterface(UPDRTSBuilderInterface::StaticClass()) == false)
	{
		return;
	}
	
	TArray<AActor*> Buildings;
	IPDRTSBuilderInterface::Execute_GetOwnedBuildings(NewPlayer, Buildings);
	if (Buildings.IsEmpty())
	{
		UPDBuilderSubsystem* BuilderSubsystem = UPDBuilderSubsystem::Get();
		ensure(BuilderSubsystem != nullptr);
	
		// Spawn a default base/building
		const FGameplayTag& DefaultBaseTag = IPDRTSBuilderInterface::Execute_GetDefaultBaseType(NewPlayer);
		const FPDBuildableData* BaseBuildableData =  BuilderSubsystem->GetBuildableData(DefaultBaseTag);

		if (BaseBuildableData == nullptr)
		{
			UE_LOG(
				PDLog_BuildSystem,
				Error,
				TEXT("ARTSOBaseGM::PostLogin -- "
					"UPDBuilderSubsystem could not resolve GetBuildableData(DefaultBaseTag(%s)) to an actual buildable data entry.	"
					"Please ensure the relevant data-table has an entry for this tag "), *DefaultBaseTag.GetTagName().ToString());

			return;
		}

		//@todo For newly created characters randomly we must spawn a base. Transforms must be ensured to not overlap, possibly make use of the world hashgrid
		FRotator DummyRot_RemoveAndSetupRandomizedTransformsWhichDoNotOverlap = {NewPlayer->GetControlRotation()};
		FVector  DummyLoc_RemoveAndSetupRandomizedTransformsWhichDoNotOverlap = {NewPlayer->GetFocalLocation()};

		FTransform InitialSpawnTransform{
			DummyRot_RemoveAndSetupRandomizedTransformsWhichDoNotOverlap,
			DummyLoc_RemoveAndSetupRandomizedTransformsWhichDoNotOverlap,
				FVector::OneVector};
		AActor* SpawnedBuildable = GetWorld()->SpawnActorDeferred<AActor>(BaseBuildableData->ActorToSpawn, InitialSpawnTransform, NewPlayer);
		IPDRTSBuilderInterface::Execute_SetOwnedBuilding(NewPlayer, SpawnedBuildable);
	}
}

APlayerController* ARTSOBaseGM::Login(UPlayer* NewPlayer, ENetRole InRemoteRole, const FString& Portal, const FString& Options,
	const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	return Super::Login(NewPlayer, InRemoteRole, Portal, Options, UniqueId, ErrorMessage);
}

void ARTSOBaseGM::TickActor(float DeltaTime, ELevelTick TickType, FActorTickFunction& ThisTickFunction)
{
	Super::TickActor(DeltaTime, TickType, ThisTickFunction);
	
	AutoSave.ElapsedTimeSinceSave += DeltaTime;
	if (AutoSave.ElapsedTimeSinceSave > AutoSave.TimeLimitAsSeconds)
	{
		SaveConversationActorStates();
		SaveInteractables();
		SaveAllPlayerStates();
		SaveEntities();
		SaveAllItems();
		AutoSave.ElapsedTimeSinceSave = 0.0;

		const FTimerDelegate SaveGameCallback = FTimerDelegate::CreateUObject(this, &ARTSOBaseGM::SaveGame, AutoSave.GetNextAutoSlot(), true);
		GetWorld()->GetTimerManager().SetTimer(AutoSave.SaveTimerHandle, SaveGameCallback, 4, false);		
	}
}

void ARTSOBaseGM::ClearSave_Implementation(const bool bRefreshSeed)
{
	check(GameSave != nullptr)

	GameSave->Data.GameTime = 0;
	GameSave->Data.Interactables.Empty();
	GameSave->Data.Inventories.Empty();

	if (bRefreshSeed)
	{
		GameSave->Data.Seeder = UKismetMathLibrary::MakeRandomStream(FMath::RandRange(0, INT_MAX));
	}
	
	bHasStartedSaveData = false;
}

//
// Saving

void ARTSOBaseGM::SaveGame(FString SlotNameCopy, const bool bAllowOverwrite)
{
	check(GameSave != nullptr)

	const FString SelectedSlot = SlotNameCopy == FString() ? ARTSOBaseGM::ROOTSAVE : SlotNameCopy;
	
	const bool bDoesExist = UGameplayStatics::DoesSaveGameExist(SelectedSlot,0);
	if (bDoesExist && bAllowOverwrite == false) { return; }
	
	bHasSavedDataAsync = false;
	const FAsyncSaveGameToSlotDelegate Dlgt = FAsyncSaveGameToSlotDelegate::CreateLambda(
		[This = this, SlotNameCopy, bAllowOverwrite](const FString&, int, bool bResult)
		{
			if (This == nullptr) { return; }
			This->bHasSavedDataAsync = bResult;
			This->OnSaveGame(SlotNameCopy, bAllowOverwrite);
		});
	
	UGameplayStatics::AsyncSaveGameToSlot(GameSave, SelectedSlot, 0, Dlgt);
	bHasStartedSaveData = true;	
}

void ARTSOBaseGM::ProcessChangesAndSaveGame_Implementation(const FString& Slot, const bool bAllowOverwrite)
{
	check(GameSave != nullptr)

	// Async call might be needed here and then after it has finished call SaveGame()
	SaveConversationActorStates();
	SaveInteractables();
	SaveAllPlayerStates();
	SaveEntities();
	SaveAllItems();
	
	const FTimerDelegate SaveGameCallback = FTimerDelegate::CreateUObject(this, &ARTSOBaseGM::SaveGame, Slot, bAllowOverwrite);
	GetWorld()->GetTimerManager().SetTimer(GameSave->Data.SaveThrottleHandle, SaveGameCallback, 4, false);
}

// Not needed, data updated directly after each choice injunction,
// keep here and reserve it for possible use if the data-structure we want to store data from grows larger/more complex
void ARTSOBaseGM::SaveConversationProgression_Implementation()
{
}

void ARTSOBaseGM::SaveConversationActorStates_Implementation()
{
	check(GameSave != nullptr)

	URTSOConversationActorTrackerSubsystem* Tracker =GetWorld()->GetSubsystem<URTSOConversationActorTrackerSubsystem>();
	check(Tracker != nullptr)

	for (ARTSOInteractableConversationActor* ConversationActor : Tracker->TrackedConversationActors)
	{
		if (ConversationActor == nullptr) { continue; }
		
		FPDPersistentID IDStruct =
			ConversationActor->ConversationActorPersistentID.IsValidID()
			? ConversationActor->ConversationActorPersistentID.GetID()
			: FPDPersistentID::GenerateNewPersistentID();
		
		FRTSSavedConversationActorData& State = GameSave->Data.ConversationActorState.FindOrAdd(IDStruct.GetID());
		State.Health = 1.0; // Force full health for now
		State.Location = ConversationActor->GetActorLocation();
		State.ActorClassType = TSoftClassPtr<ARTSOInteractableConversationActor>(ConversationActor->GetClass());
		
		State.ProgressionPerPlayer.Empty();


		bool bValidInstanceData = false;
		for (TTuple<FGameplayTag, FRTSOConversationMetaState>& ConversationActorState : *ConversationActor->InstanceDataPerMissionPtr)
		{
			FGameplayTag& MissionTag = ConversationActorState.Key;
			FRTSOConversationMetaState& MetaProgressState = ConversationActorState.Value;
			for (const TTuple<int32/*PlayerID*/, int32/**/>& PlayerInstanceData : MetaProgressState.ProgressionPerPlayer)
			{
				FRTSOConversationMetaProgressionDatum& MetaProgressionDatum = State.ProgressionPerPlayer.FindOrAdd(PlayerInstanceData.Key).ProgressionDataMap.FindOrAdd(MissionTag);
				MetaProgressionDatum.MissionTag = MissionTag; // Self reference
				MetaProgressionDatum.BaseProgression = PlayerInstanceData.Value;

				bValidInstanceData = true;
			}
		}
		if (bValidInstanceData == false )
		{
			UE_LOG(PDLog_RTSOInteract, Warning, TEXT("ARTSOBaseGM(%s)::SaveConversationActorStates -- 'InstanceDataPerMission' Array had no valid entries"), *GetName())
			return ;
		}
	}
}

void ARTSOBaseGM::SaveAllPlayerStates()
{
	URTSOConversationActorTrackerSubsystem* Tracker =GetWorld()->GetSubsystem<URTSOConversationActorTrackerSubsystem>();
	check(Tracker != nullptr)

	for (ARTSOController* Controller : Tracker->TrackedPlayerControllers)
	{
		SavePlayerState(Controller);
	}
}

void ARTSOBaseGM::SavePlayerState_Implementation(ARTSOController* PlayerController)
{
	check(GameSave != nullptr)

	// todo store player stats here as-well when I've finished writing the progression system 
	GameSave->Data.PlayerLocations.FindOrAdd(PlayerController->GetActorID()) = PlayerController->GetPawn()->GetActorLocation();
}

void ARTSOBaseGM::SaveInteractables_Implementation()
{
	check(GameSave != nullptr)
	check(UPDInteractSubsystem::Get() != nullptr)
	GameSave->Data.Interactables.Empty();

	FPDArrayListWrapper& SaveInfo = *UPDInteractSubsystem::Get()->WorldInteractables.Find(GetWorld());
	for (TTuple<int32, FRTSSavedInteractable>& _Actor : SaveInfo.ActorInfo)
	{
		// Update class if needed
		if (_Actor.Value.ActorInWorld == nullptr)
		{
			_Actor.Value.Usability = 0.0;
			continue;
		}

		// Update class if needed
		if (_Actor.Value.ActorClass == nullptr)
		{
			_Actor.Value.ActorClass = _Actor.Value.ActorInWorld->GetClass();
		}
		
		// update location
		_Actor.Value.Location = _Actor.Value.ActorInWorld->GetActorLocation();
	}

	TArray<FRTSSavedInteractable> InteractableArray{};
	SaveInfo.ActorInfo.GenerateValueArray(InteractableArray);
	GameSave->Data.Interactables.Append(InteractableArray);
}

void ARTSOBaseGM::SaveAllItems_Implementation()
{
	check(GameSave != nullptr)
	
	TMap<int32, AActor*>& IDToActorMap =  UPDRTSBaseSubsystem::Get()->SharedOwnerIDMappings;
	TMap<int32 /*ID*/, FRTSSavedItems> AccumulatedItems{};
	for (const TTuple<int32 /*PersistentID*/, AActor* >& ItemDataEntry : IDToActorMap)
	{
		int32 ID = ItemDataEntry.Key;
		const AController* AsController = Cast<AController>(ItemDataEntry.Value);
		if (AsController == nullptr)
		{
			UE_LOG(PDLog_RTSO, Error, TEXT("IDToActorMap.FindChecked(ID) does not point into a valid controller"));
			continue;	
		}

		const AGodHandPawn* CurrentGodHandPawn = AsController->GetPawn<AGodHandPawn>();
		if (CurrentGodHandPawn == nullptr)
		{
			UE_LOG(PDLog_RTSO, Error, TEXT("AsController does not own a valid godhand pawn"));
			continue;	
		}
		AccumulatedItems.Emplace(ID, FRTSSavedItems{CurrentGodHandPawn->InventoryComponent->ItemList.Items});
	}
	
	GameSave->Data.Inventories.Append(AccumulatedItems); // overwrite any previous value, this is slow so only allow saving resources every 4 seconds at max, limited by the latent action below 
}

/* Save current units, their locations and actor classes*/
void ARTSOBaseGM::SaveEntities_Implementation()
{
	check(GameSave != nullptr)

	UPDRTSBaseSubsystem* RTSSubsystem = UPDRTSBaseSubsystem::Get();
	const FMassEntityManager* EntityManager = RTSSubsystem != nullptr ? RTSSubsystem->EntityManager : nullptr; 
	if (EntityManager == nullptr) { return; }

	// 1. get all our entities
	RTSSubsystem->WorldEntityOctree.FindAllElements
	(
		[&](const FPDEntityOctreeCell& Cell)
		{
			// 2. Store entities per active player
			if (EntityManager->IsEntityValid(Cell.EntityHandle) == false)
			{
				return;
			}

			FRTSSavedWorldUnits UnitDatum{};
			const FPDMFragment_RTSEntityBase& EntityBaseFragment = EntityManager->GetFragmentDataChecked<FPDMFragment_RTSEntityBase>(Cell.EntityHandle);

			// 4. Store entities active task/job data and stats
			UnitDatum.CurrentAction = EntityManager->GetFragmentDataChecked<FPDMFragment_Action>(Cell.EntityHandle);
			UnitDatum.OwnerID = EntityBaseFragment.OwnerID;
			UnitDatum.SelectionIndex = EntityBaseFragment.SelectionGroupIndex;
			UnitDatum.Location = EntityManager->GetFragmentDataChecked<FTransformFragment>(Cell.EntityHandle).GetTransform().GetLocation();

			// Get mass entity soft object pointer, RTSSubsystem will only have done tha actual association if we've spawned entities using our ARTSOMassSpawner class 
			const FMassArchetypeHandle ArchetypeHandle = EntityManager->GetArchetypeForEntity(Cell.EntityHandle);
			
			UnitDatum.InstanceIndex = Cell.EntityHandle;
			UnitDatum.EntityUnitTag = EntityBaseFragment.EntityType;
			
			GameSave->Data.EntityUnits.Emplace(UnitDatum);
		}
	);
}

//
// Loading
template<typename TInnerType = FRTSSavedWorldUnits>
void LoadArray(const TArray<TInnerType>& OldData, const TArray<TInnerType>& NewData,
	TArray<TInnerType>& DeleteContainer, // : Anything which is in OldData that isn't in NewData (Intersection of OldData and the Complement of NewData)
	TArray<TInnerType>& AddContainer,    // : Anything which is in NewData that isn't in OldData (Intersection of NewData and the Complement of OldData)
	TArray<TInnerType>& ModifyContainer) // : Anything updated from the intersection of OldData and NewData (Modified elements in the intersection of NewData and OldData)
{
	// Slow compare, needed if compare type is an array, 
	for (const TInnerType& OldDatum : OldData)
	{
		bool bOldDatumExist = false;
		bool bNeedsToBeModified = false;

		const TInnerType* DatumPtr = &OldDatum;
		
		for (const TInnerType& NewDatum : NewData)
		{
			if (OldDatum.InstanceIndex == NewDatum.InstanceIndex)
			{
				bOldDatumExist = true;
				bNeedsToBeModified = OldDatum != NewDatum;
				DatumPtr = &NewDatum;
				break;
			}
		}

		
		// Emplace copies
		if (bOldDatumExist && bNeedsToBeModified) { ModifyContainer.Emplace(*DatumPtr); }
		else if (bOldDatumExist == false) { DeleteContainer.Emplace(OldDatum); }
		
	}

	// Slow compare, needed if compare type is an array, 
	for (const TInnerType& NewDatum : NewData)
	{
		bool bOldDatumExist_DoNotAddAgain = false;
		for (const TInnerType& OldDatum : OldData)
		{
			if (OldDatum.InstanceIndex == NewDatum.InstanceIndex)
			{
				bOldDatumExist_DoNotAddAgain = true;
				break;
			}
		}

		// Emplace copy
		if (bOldDatumExist_DoNotAddAgain == false) { AddContainer.Emplace(NewDatum); }
	}
}

void ProcessLoadSavedItems(const TMap<int32, FRTSSavedItems>& OldData, const TMap<int32, FRTSSavedItems>& NewData,
	TMap<int32, FRTSSavedItems>& DeleteContainer, // : Anything which is in OldData that isn't in NewData (Intersection of OldData and the Complement of NewData)
	TMap<int32, FRTSSavedItems>& AddContainer,    // : Anything which is in NewData that isn't in OldData (Intersection of NewData and the Complement of OldData)
	TMap<int32, FRTSSavedItems>& ModifyContainer) // : Anything updated from the intersection of OldData and NewData (Modified elements in the intersection of NewData and OldData)

{
	
	// Slow inner compare, needed if compare type is an array, 
	for (const TTuple<int32, FRTSSavedItems>& OldUserDatum : OldData)
	{
		const int32 UserID = OldUserDatum.Key;
		
		TTuple<int32, FRTSSavedItems> NewUserDatum = OldUserDatum;

		const bool bUserDatumExists = NewData.Contains(UserID);
		bool bNeedsToBeModified = false; // bUserDatumExists && NewData.FindRef(OldUserDatum.Key) != OldUserDatum.Value;
		if (bUserDatumExists)
		{
			NewUserDatum = TTuple<int32, FRTSSavedItems>{UserID, NewData.FindRef(UserID)};
			
			TArray<FPDItemNetDatum> DeleteList;
			TArray<FPDItemNetDatum> AddList;
			TArray<FPDItemNetDatum> ModifyList;
			// // @todo finish
			// LoadArray<FPDItemNetDatum>(
			// 	OldUserDatum.Value.Items,
			// 	NewUserDatum.Value.Items,
			// 	DeleteList,
			// 	AddList,
			// 	ModifyList);

			for (const FPDItemNetDatum& ItemToDelete : DeleteList)
			{
				// @todo, Process for current UserID
				bNeedsToBeModified = true;
			}

			for (const FPDItemNetDatum& ItemToAdd : AddList)
			{
				// @todo, Process for current UserID
				bNeedsToBeModified = true;
			}

			for (const FPDItemNetDatum& ItemToModify : ModifyList)
			{
				// @todo, Process for current UserID
				bNeedsToBeModified = true;
			}
		}
		
		// Emplace copies
		if (bUserDatumExists && bNeedsToBeModified) { ModifyContainer.Emplace(NewUserDatum.Key, NewUserDatum.Value); }
		else if (bUserDatumExists == false) { DeleteContainer.Emplace(OldUserDatum.Key, OldUserDatum.Value); }
	}

	//
	// Compare if this is a new user from the perspective of the new data 
	for (const TTuple<int, FRTSSavedItems>& NewDatum : NewData)
	{
		const bool bUserDatumExists = OldData.Contains(NewDatum.Key);

		// Emplace copy 
		if (bUserDatumExists == false) { AddContainer.Emplace(NewDatum.Key, NewDatum.Value); }
	}
}

void ProcessLoadPlayerLocations(const TMap<int32, FVector>& OldData, const TMap<int32, FVector>& NewData,
	TMap<int32, FVector>& DeleteContainer, // : Anything which is in OldData that isn't in NewData (Intersection of OldData and the Complement of NewData)
	TMap<int32, FVector>& AddContainer,    // : Anything which is in NewData that isn't in OldData (Intersection of NewData and the Complement of OldData)
	TMap<int32, FVector>& ModifyContainer) // : Anything updated from the intersection of OldData and NewData (Modified elements in the intersection of NewData and OldData)

{
	// Slow inner compare, needed if compare type is an array, 
	for (const TTuple<int32, FVector>& OldUserDatum : OldData)
	{
		const int32 UserID = OldUserDatum.Key;
		
		TTuple<int32, FVector> NewUserDatum = OldUserDatum;

		const bool bUserDatumExists = NewData.Contains(UserID);
		bool bNeedsToBeModified = false; // bUserDatumExists && NewData.FindRef(OldUserDatum.Key) != OldUserDatum.Value;
		if (bUserDatumExists)
		{
			NewUserDatum = TTuple<int32, FVector>{UserID, NewData.FindRef(UserID)};
			bNeedsToBeModified = (OldUserDatum.Value - NewUserDatum.Value).IsNearlyZero(5.f) == false; 
		}
		
		// Emplace copies
		if (bUserDatumExists && bNeedsToBeModified) { ModifyContainer.Emplace(NewUserDatum.Key, NewUserDatum.Value); }
		else if (bUserDatumExists == false) { DeleteContainer.Emplace(OldUserDatum.Key, OldUserDatum.Value); }
		
	}

	//
	// Compare if this is a new user from the perspective of the new data 
	for (const TTuple<int, FVector>& NewDatum : NewData)
	{
		const bool bUserDatumExists = OldData.Contains(NewDatum.Key);

		// Emplace copy 
		if (bUserDatumExists == false) { AddContainer.Emplace(NewDatum.Key, NewDatum.Value); }
	}
}

void ProcessLoadConversationActorsData(const TMap<int32, FRTSSavedConversationActorData>& OldData, const TMap<int32, FRTSSavedConversationActorData>& NewData,
	TMap<int32, FRTSSavedConversationActorData>& DeleteContainer, // : Anything which is in OldData that isn't in NewData (Intersection of OldData and the Complement of NewData)
	TMap<int32, FRTSSavedConversationActorData>& AddContainer,    // : Anything which is in NewData that isn't in OldData (Intersection of NewData and the Complement of OldData)
	TMap<int32, FRTSSavedConversationActorData>& ModifyContainer) // : Anything updated from the intersection of OldData and NewData (Modified elements in the intersection of NewData and OldData)

{
	// Slow inner compare, needed if compare type is an array, 
	for (const TTuple<int32, FRTSSavedConversationActorData>& OldUserDatum : OldData)
	{
		const int32 UserID = OldUserDatum.Key;
		
		TTuple<int32, FRTSSavedConversationActorData> NewUserDatum = OldUserDatum;

		const bool bUserDatumExists = NewData.Contains(UserID);
		bool bNeedsToBeModified = false; // bUserDatumExists && NewData.FindRef(OldUserDatum.Key) != OldUserDatum.Value;
		if (bUserDatumExists)
		{
			NewUserDatum = TTuple<int32, FRTSSavedConversationActorData>{UserID, NewData.FindRef(UserID)};
			bNeedsToBeModified = OldUserDatum != NewUserDatum;
		}
		
		// Emplace copies
		if (bUserDatumExists && bNeedsToBeModified) { ModifyContainer.Emplace(NewUserDatum.Key, NewUserDatum.Value); }
		else if (bUserDatumExists == false) { DeleteContainer.Emplace(OldUserDatum.Key, OldUserDatum.Value); }
		
	}

	//
	// Compare if this is a new user from the perspective of the new data 
	for (const TTuple<int, FRTSSavedConversationActorData>& NewDatum : NewData)
	{
		const bool bUserDatumExists = OldData.Contains(NewDatum.Key);

		// Emplace copy 
		if (bUserDatumExists == false) { AddContainer.Emplace(NewDatum.Key, NewDatum.Value); }
	}
}

void ProcessLoadPlayerAccumulatedTags(const TMap<int32, FGameplayTagContainer>& OldData, const TMap<int32, FGameplayTagContainer>& NewData,
	TMap<int32, FGameplayTagContainer>& DeleteContainer, // : Anything which is in OldData that isn't in NewData (Intersection of OldData and the Complement of NewData)
	TMap<int32, FGameplayTagContainer>& AddContainer,    // : Anything which is in NewData that isn't in OldData (Intersection of NewData and the Complement of OldData)
	TMap<int32, FGameplayTagContainer>& ModifyContainer) // : Anything updated from the intersection of OldData and NewData (Modified elements in the intersection of NewData and OldData)

{
	// Slow inner compare, needed if compare type is an array, 
	for (const TTuple<int32, FGameplayTagContainer>& OldUserDatum : OldData)
	{
		const int32 UserID = OldUserDatum.Key;
		
		TTuple<int32, FGameplayTagContainer> NewUserDatum = OldUserDatum;

		const bool bUserDatumExists = NewData.Contains(UserID);
		bool bNeedsToBeModified = false; // bUserDatumExists && NewData.FindRef(OldUserDatum.Key) != OldUserDatum.Value;
		if (bUserDatumExists)
		{
			NewUserDatum = TTuple<int32, FGameplayTagContainer>{UserID, NewData.FindRef(UserID)};
			bNeedsToBeModified = (OldUserDatum.Value == NewUserDatum.Value) == false; 
		}
		
		// Emplace copies
		if (bUserDatumExists && bNeedsToBeModified) { ModifyContainer.Emplace(NewUserDatum.Key, NewUserDatum.Value); }
		else if (bUserDatumExists == false) { DeleteContainer.Emplace(OldUserDatum.Key, OldUserDatum.Value); }
		
	}

	//
	// Compare if this is a new user from the perspective of the new data 
	for (const TTuple<int, FGameplayTagContainer>& NewDatum : NewData)
	{
		const bool bUserDatumExists = OldData.Contains(NewDatum.Key);

		// Emplace copy 
		if (bUserDatumExists == false) { AddContainer.Emplace(NewDatum.Key, NewDatum.Value); }
	}
}

void ARTSOBaseGM::LoadGame_Implementation(const FString& Slot, const bool bDummyParam)
{
	const FString SelectedSlot = Slot == FString() ? ARTSOBaseGM::ROOTSAVE : Slot;
	
	const bool bDoesExist = UGameplayStatics::DoesSaveGameExist(SelectedSlot,0);

	FRTSSaveData OldData = GameSave != nullptr ? GameSave->Data : FRTSSaveData{};
	if (bDoesExist)
	{
		GameSave = Cast<URTSOpenSaveGame>(UGameplayStatics::LoadGameFromSlot(SelectedSlot,0));
		return;
	}
	
	GameSave = Cast<URTSOpenSaveGame>(UGameplayStatics::CreateSaveGameObject(URTSOpenSaveGame::StaticClass()));
	const FRTSSaveData& NewData = GameSave->Data;

	// 0. Prepare 6, unbound, threaded tasks
	// 1. For each task assign the different save data types: 
	// 1a.  This means to return anything we should Delete: Anything which is in OldData that isn't in NewData (Intersection of OldData and the Complement of NewData)
	// 1b.  This means to return anything we should Add:    Anything which is in NewData that isn't in OldData (Intersection of NewData and the Complement of OldData)
	// 1c.  This means to return anything we should Modify: Anything updated from the intersection of OldData and NewData (Modified elements in the intersection of NewData and OldData)
	// 2. When all threads have finished call 'InstantiateLoadedData'
	// 2a.  @todo Need to update the 4 functions in InstantiateLoadedData: The containers for Delete/Add/Modify gathered by the threaded functions needs to be taken into account, so existing objects aren't reloaded
	// 2b.  @todo Also need a developer setting that forces a fresh (full) load, instead of the default where it only will load changes 

	for (bool& LoadThreadState : FinishedLoadThreads)
	{
		LoadThreadState = false;
	}

	bProcessingLoadData = true;
	ParallelFor(
		static_cast<int8>(EPDSaveDataThreadSelector::EEnd),
		[&, SlotCopy = Slot](int8 Step)
		{
			const EPDSaveDataThreadSelector ThreadSelector = static_cast<EPDSaveDataThreadSelector>(Step);

			switch (ThreadSelector)
			{
			default: break;
				
			case EPDSaveDataThreadSelector::EInteractables:
				{
					ProcessedLoadData& ThreadData = LoadDataInProcess[static_cast<uint8>(EPDSaveDataThreadSelector::EInteractables)];
					// (DONE) NewData.Interactables;              // TArray<FRTSSavedInteractable>
					LoadArray<FRTSSavedInteractable>(
						OldData.Interactables,
						NewData.Interactables,
						ThreadData.SavedInteracts.ToDelete,
						ThreadData.SavedInteracts.ToAdd,
						ThreadData.SavedInteracts.ToModify);
					
					OnThreadFinished_PlayerLoadDataSync(EPDSaveDataThreadSelector::EInteractables, SlotCopy);
				}
				break;
			case EPDSaveDataThreadSelector::EEntities:
				{
					ProcessedLoadData& ThreadData = LoadDataInProcess[static_cast<uint8>(EPDSaveDataThreadSelector::EEntities)];
					// (DONE) NewData.EntityUnits;                // TArray<FRTSSavedWorldUnits>
					LoadArray<FRTSSavedWorldUnits>(
						OldData.EntityUnits,
						NewData.EntityUnits,
						ThreadData.SavedUnits.ToDelete,
						ThreadData.SavedUnits.ToAdd,
						ThreadData.SavedUnits.ToModify);

					OnThreadFinished_PlayerLoadDataSync(EPDSaveDataThreadSelector::EEntities, SlotCopy);
				}	
				break;
			case EPDSaveDataThreadSelector::EInventories:
				{
					ProcessedLoadData& ThreadData = LoadDataInProcess[static_cast<uint8>(EPDSaveDataThreadSelector::EInventories)];
					// (DONE) NewData.Inventories;                // TMap<int32, FRTSSavedItems>
					ProcessLoadSavedItems(
						OldData.Inventories,
						NewData.Inventories,
						ThreadData.SavedItems.ToDelete,
						ThreadData.SavedItems.ToAdd,
						ThreadData.SavedItems.ToModify);

					OnThreadFinished_PlayerLoadDataSync(EPDSaveDataThreadSelector::EInventories, SlotCopy);
				}
				break;
			case EPDSaveDataThreadSelector::EPlayers:
				{
					ProcessedLoadData& ThreadData = LoadDataInProcess[static_cast<uint8>(EPDSaveDataThreadSelector::EPlayers)];
					// (DONE) NewData.PlayerLocations;            // TMap<int32, FVector>
					ProcessLoadPlayerLocations(
						OldData.PlayerLocations,
						NewData.PlayerLocations,
						ThreadData.SavedLocs.ToDelete,
						ThreadData.SavedLocs.ToAdd,
						ThreadData.SavedLocs.ToModify);

					OnThreadFinished_PlayerLoadDataSync(EPDSaveDataThreadSelector::EPlayers, SlotCopy);
				}
				break;
			case EPDSaveDataThreadSelector::EConversationActors:
				{
					// (DONE) NewData.ConversationActorState;     // TMap<int32, FRTSSavedConversationActorData>
					ProcessedLoadData& ThreadData = LoadDataInProcess[static_cast<uint8>(EPDSaveDataThreadSelector::EConversationActors)];
					ProcessLoadConversationActorsData(
						OldData.ConversationActorState,
						NewData.ConversationActorState,
						ThreadData.SavedConvoActors.ToDelete,
						ThreadData.SavedConvoActors.ToAdd,
						ThreadData.SavedConvoActors.ToModify);

					OnThreadFinished_PlayerLoadDataSync(EPDSaveDataThreadSelector::EConversationActors, SlotCopy);
				}
				break;
			case EPDSaveDataThreadSelector::EPlayerConversationProgress:
				{
					ProcessedLoadData& ThreadData = LoadDataInProcess[static_cast<uint8>(EPDSaveDataThreadSelector::EPlayerConversationProgress)];
					// (DONE) NewData.PlayersAndConversationTags; // TMap<int32, FGameplayTagContainer>
					ProcessLoadPlayerAccumulatedTags(
						OldData.PlayersAndConversationTags,
						NewData.PlayersAndConversationTags,
						ThreadData.SavedConvoTags.ToDelete,
						ThreadData.SavedConvoTags.ToAdd,
						ThreadData.SavedConvoTags.ToModify);

					OnThreadFinished_PlayerLoadDataSync(EPDSaveDataThreadSelector::EPlayerConversationProgress, SlotCopy);
				}
				break;
			}
		}
		, false
		, false
		);
}

void ARTSOBaseGM::FinalizeLoadReservedPlayerControllers(const TMap<int32, AActor*>& OwnerIDMappings)
{
	MProcessedLocations& ProcessedData = LoadDataInProcess[static_cast<uint8>(EPDSaveDataThreadSelector::EPlayers)].SavedLocs;
	for (TTuple<int32, FVector>& PlayerToAdd : ProcessedData.ToAdd)
	{
		const int32 UserID = PlayerToAdd.Key;

		AActor* User = OwnerIDMappings.Contains(UserID) ? OwnerIDMappings.FindRef(UserID) : nullptr;
		if (User == nullptr)
		{
			// Preload controllers in server, @todo override ::Login to route connected players to their preloaded controller
			// Preload controllers in server, @todo pass in BP subclass for spawn class! 
			APlayerController* const NewPC = PreloadedControllers.Emplace_GetRef(SpawnPlayerControllerCommon(ROLE_Authority, PlayerToAdd.Value, FRotator::ZeroRotator, ARTSOController::StaticClass()));
			NewPC->SetOwner(this); // keep owning until player connects
			continue;
		}

		User->SetActorLocation(PlayerToAdd.Value);
	}
					
	for (const TTuple<int32, FVector>& PlayerToModify : ProcessedData.ToModify)
	{
		const int32 UserID = PlayerToModify.Key;

		AActor* User = OwnerIDMappings.Contains(UserID) ? OwnerIDMappings.FindRef(UserID) : nullptr;
		if (User == nullptr)
		{
			UE_LOG(PDLog_RTSO, Error, TEXT("ARTSOBaseGM::OnThreadFinished_PlayerLoadDataSync -- Missing Expected User by ID: %i"), UserID)
			continue;
		}

		User->SetActorLocation(PlayerToModify.Value);
	}

	for (const TTuple<int32, FVector>& PlayerToDelete : ProcessedData.ToDelete)
	{
		const int32 UserID = PlayerToDelete.Key;

		AActor* User = OwnerIDMappings.Contains(UserID) ? OwnerIDMappings.FindRef(UserID) : nullptr;
		if (Owner == nullptr)
		{
			UE_LOG(PDLog_RTSO, Error, TEXT("ARTSOBaseGM::OnThreadFinished_PlayerLoadDataSync -- Missing Expected User by ID: %i"), UserID)
			continue;
		}

		User->SetOwner(nullptr);
		User->Destroy();
	}
}

void ARTSOBaseGM::FinalizeLoadInteractableData()
{
	MProcessedInteractData& ProcessedData = LoadDataInProcess[static_cast<uint8>(EPDSaveDataThreadSelector::EInteractables)].SavedInteracts;
	// @DONE iterate and spawn all interactables from ProcessedData.ToAdd
	for (FRTSSavedInteractable& InteractableToSpawn : ProcessedData.ToAdd)
	{
		FRotator DummyRot{0};
		APDInteractActor* InteractActor = Cast<APDInteractActor>(GetWorld()->SpawnActor(InteractableToSpawn.ActorClass.Get(), &InteractableToSpawn.Location, &DummyRot));
		if (InteractActor == nullptr)
		{
			UE_LOG(PDLog_RTSO, Error, TEXT("ARTSOBaseGM::OnThreadFinished_PlayerLoadDataSync -- Loaded interactable was not inheriting from 'APDInteractActor'"))
			continue;
		}

		InteractableToSpawn.ActorInWorld = InteractActor;
		InteractActor->Usability = InteractableToSpawn.Usability;
	}
					
	// @DONE iterate and delete all interactables from ProcessedData.ToDelete
	for (const FRTSSavedInteractable& InteractableToRemove : ProcessedData.ToDelete)
	{
		const FPDArrayListWrapper& InteractableListWrapper = UPDInteractSubsystem::Get()->WorldInteractables.FindRef(GetWorld());
		AActor* ActorInWorld = InteractableListWrapper.ActorInfo.FindRef(InteractableToRemove.InstanceIndex).ActorInWorld;
		if (ActorInWorld == nullptr) { continue; }
		ActorInWorld->Destroy();
	}
					
	// @DONE iterate and modify all interactables from ProcessedData.ToModify
	for (const FRTSSavedInteractable& InteractableToModify : ProcessedData.ToModify)
	{
		const FPDArrayListWrapper& InteractableListWrapper = UPDInteractSubsystem::Get()->WorldInteractables.FindRef(GetWorld());
		AActor* ActorInWorld = InteractableListWrapper.ActorInfo.FindRef(InteractableToModify.InstanceIndex).ActorInWorld;
		APDInteractActor* InteractActor = Cast<APDInteractActor>(ActorInWorld);
		if (InteractActor == nullptr) { continue; }

		InteractActor->Usability = InteractableToModify.Usability;
		ActorInWorld->SetActorLocation(InteractableToModify.Location);
	}
}

void ARTSOBaseGM::FinalizeLoadEntityData()
{
	MProcessedUnits& ProcessedData = LoadDataInProcess[static_cast<uint8>(EPDSaveDataThreadSelector::EEntities)].SavedUnits;
	// @DONE spawn all entities from ProcessedData.ToAdd
	LoadEntities(ProcessedData.ToAdd);

	UMassEntitySubsystem* SpawnerSystem = UWorld::GetSubsystem<UMassEntitySubsystem>(GetWorld());
	const UMassEntitySubsystem* EntitySubsystem = UWorld::GetSubsystem<UMassEntitySubsystem>(GetWorld());
	FMassEntityManager* EntityManager = EntitySubsystem != nullptr ? const_cast<FMassEntityManager*>(&EntitySubsystem->GetEntityManager()) : nullptr;
	if (EntityManager == nullptr) { return; }
					
	// @DONE iterate and delete all entities from ProcessedData.ToDelete
	for (const FRTSSavedWorldUnits& EntityToRemove : ProcessedData.ToDelete)
	{
		if (EntityManager->IsEntityValid(EntityToRemove.InstanceIndex) == false)
		{
			continue;
		}
		EntityManager->DestroyEntity(EntityToRemove.InstanceIndex);
	}
					
	// @DONE iterate and modify all entities from ProcessedData.ToModify
	for (const FRTSSavedWorldUnits& EntityToModify : ProcessedData.ToModify)
	{
		if (EntityManager->IsEntityValid(EntityToModify.InstanceIndex) == false)
		{
			continue;
		}

		FPDMFragment_Action* EntityAction = EntityManager->GetFragmentDataPtr<FPDMFragment_Action>(EntityToModify.InstanceIndex);
		if (EntityAction != nullptr)
		{
			*EntityAction = EntityToModify.CurrentAction; 
		}

		FTransformFragment* EntityTransformFragment = EntityManager->GetFragmentDataPtr<FTransformFragment>(EntityToModify.InstanceIndex);
		if (EntityTransformFragment != nullptr)
		{
			EntityTransformFragment->GetMutableTransform().SetLocation(EntityToModify.Location); 
		}

		FPDMFragment_RTSEntityBase* EntityBaseFragment = EntityManager->GetFragmentDataPtr<FPDMFragment_RTSEntityBase>(EntityToModify.InstanceIndex);
		if (EntityBaseFragment != nullptr)
		{
			EntityBaseFragment->EntityType = EntityToModify.EntityUnitTag; 
			EntityBaseFragment->OwnerID = EntityToModify.OwnerID; 
			EntityBaseFragment->SelectionGroupIndex = EntityToModify.SelectionIndex; 
		}
	}
}

void ARTSOBaseGM::FinalizeLoadInventoryData(const TMap<int32, AActor*>& OwnerIDMappings)
{
	MProcessedItems& ProcessedData = LoadDataInProcess[static_cast<uint8>(EPDSaveDataThreadSelector::EInventories)].SavedItems;
	// @DONE iterate and spawn all inventories from ProcessedData.ToAdd
	for (TTuple<int32, FRTSSavedItems>& InvToSpawn : ProcessedData.ToAdd)
	{
		const int32 UserID = InvToSpawn.Key;
		const FRTSSavedItems& ItemsToAdd = InvToSpawn.Value;

		const AActor* User = OwnerIDMappings.Contains(UserID) ? OwnerIDMappings.FindRef(UserID) : nullptr;
		if (User == nullptr) { continue; }

		UPDInventoryComponent* Inv = User->GetComponentByClass<UPDInventoryComponent>();
		if (Inv == nullptr) { continue; }

		for (const FPDItemNetDatum& AddedItem : ItemsToAdd.Items)
		{
			Inv->RequestUpdateItem(EPDItemNetOperation::ADDNEW, AddedItem.ItemTag, AddedItem.TotalItemCount);
		}
	}
					
	// @DONE iterate and delete all inventories from ProcessedData.ToDelete
	for (const TTuple<int32, FRTSSavedItems>& InvToRemove : ProcessedData.ToDelete)
	{
		const int32 UserID = InvToRemove.Key;
		const FRTSSavedItems& ItemsToRemove = InvToRemove.Value;

		const AActor* User = OwnerIDMappings.Contains(UserID) ? OwnerIDMappings.FindRef(UserID) : nullptr;
		if (User == nullptr) { continue; }

		UPDInventoryComponent* Inv = User->GetComponentByClass<UPDInventoryComponent>();
		if (Inv == nullptr) { continue; }

		for (const FPDItemNetDatum& RemoveItem : ItemsToRemove.Items)
		{
			Inv->RequestUpdateItem(EPDItemNetOperation::REMOVEALL, RemoveItem.ItemTag, 0);
		}
						
	}
					
	// @DONE iterate and modify all inventories from ProcessedData.ToModify
	for (const TTuple<int32, FRTSSavedItems>& InvToModify : ProcessedData.ToModify)
	{
		const int32 UserID = InvToModify.Key;
		const FRTSSavedItems& ItemsToUpdate = InvToModify.Value;

		const AActor* User = OwnerIDMappings.Contains(UserID) ? OwnerIDMappings.FindRef(UserID) : nullptr;
		if (User == nullptr) { continue; }

		UPDInventoryComponent* Inv = User->GetComponentByClass<UPDInventoryComponent>();
		if (Inv == nullptr) { continue; }

		for (const FPDItemNetDatum& ChangedItem : ItemsToUpdate.Items)
		{
			const FPDItemNetDatum& ExistingItem = Inv->ItemList.Items[Inv->ItemList.ItemToIndexMapping.FindRef(ChangedItem.ItemTag)];
			Inv->RequestUpdateItem(EPDItemNetOperation::CHANGE, ChangedItem.ItemTag, ChangedItem.TotalItemCount - ExistingItem.TotalItemCount);
		}						
	}
}

void ARTSOBaseGM::FinalizeLoadConversationActors()
{
	UPDInteractSubsystem* InteractSubsystem = UPDInteractSubsystem::Get();
	FPDArrayListWrapper& CurrentWorldInteractables = InteractSubsystem->WorldInteractables.FindOrAdd(GetWorld());
					
	MProcessedConvos& ProcessedData = LoadDataInProcess[static_cast<uint8>(EPDSaveDataThreadSelector::EConversationActors)].SavedConvoActors;
	// @done iterate and set all conversation interactable actors from ProcessedData.ToAdd
	for (TTuple<int32, FRTSSavedConversationActorData>& ConversationToAdd : ProcessedData.ToAdd)
	{
		const int32 ConvActorID = ConversationToAdd.Key;
		const FRTSSavedConversationActorData& ConvActorData = ConversationToAdd.Value;

						
		// if it was already spawned in when parsing interactables to spawn 
		bool bAddActor = true;
		if (CurrentWorldInteractables.ActorInfo.Contains(ConvActorID))
		{
			ARTSOInteractableConversationActor* ExistingActor = Cast<ARTSOInteractableConversationActor>(CurrentWorldInteractables.ActorInfo.FindRef(ConvActorID).ActorInWorld);
			if (ExistingActor == nullptr)
			{
				CurrentWorldInteractables.ActorInfo.Remove(ConvActorID);
								
				continue;
			}

			bAddActor = false;
							
			ExistingActor->SetActorLocation(ConvActorData.Location);
			for (const TTuple<int, FRTSOConversationMetaProgressionListWrapper>& PlayerConvTuple : ConvActorData.ProgressionPerPlayer)
			{
				const int32 UserID = PlayerConvTuple.Key;
				const TMap<FGameplayTag, FRTSOConversationMetaProgressionDatum>& UserProgressionMap = PlayerConvTuple.Value.ProgressionDataMap;

				for (const TTuple<FGameplayTag, FRTSOConversationMetaProgressionDatum>& UserProgressionTuple : UserProgressionMap)
				{
					const FRTSOConversationMetaProgressionDatum& MetaProgressionDatum = UserProgressionTuple.Value;

					// @todo this is nasty, fix! 
					FRTSOConversationMetaState& MissionInstanceData = ExistingActor->InstanceDataPerMission.FindOrAdd(MetaProgressionDatum.MissionTag); 
					MissionInstanceData.MissionTag = MetaProgressionDatum.MissionTag;
					MissionInstanceData.PhaseRequiredTags = MetaProgressionDatum.PhaseRequiredTags;
					MissionInstanceData.ProgressionPerPlayer.FindOrAdd(UserID) = MetaProgressionDatum.BaseProgression;
				}
			}
		}

		if (bAddActor)
		{
			FRTSSavedInteractable& NewActorInfo = CurrentWorldInteractables.ActorInfo.FindOrAdd(ConvActorID);
							
			NewActorInfo.Location = ConvActorData.Location;
			NewActorInfo.Usability = ConvActorData.Health;
			NewActorInfo.ActorClass = ConvActorData.ActorClassType;
			NewActorInfo.InstanceIndex = ConvActorID;

			FRotator DummyRot = FRotator::ZeroRotator; // @todo supply actual rotation
			NewActorInfo.ActorInWorld = GetWorld()->SpawnActor(NewActorInfo.ActorClass.LoadSynchronous(),&NewActorInfo.Location, &DummyRot);
			NewActorInfo.ActorInWorld->SetOwner(this);
		}
						
	}
					
	// @done iterate and delete all conversation interactable actors from ProcessedData.ToDelete
	for (const TTuple<int32, FRTSSavedConversationActorData>& ConversationToRemove : ProcessedData.ToDelete)
	{
		const int32 ConvActorID = ConversationToRemove.Key;
		// if it was already spawned in when parsing interactables to spawn 
		if (CurrentWorldInteractables.ActorInfo.Contains(ConvActorID))
		{
			ARTSOInteractableConversationActor* ExistingActor = Cast<ARTSOInteractableConversationActor>(CurrentWorldInteractables.ActorInfo.FindRef(ConvActorID).ActorInWorld);
			if (ExistingActor == nullptr) { continue; }

			ExistingActor->SetOwner(nullptr);
			ExistingActor->Destroy();
		}					
	}
					
	// @done iterate and modify all conversation interactable actors from ProcessedData.ToModify
	for (const TTuple<int32, FRTSSavedConversationActorData>& ConversationToModify : ProcessedData.ToModify)
	{
		const int32 ConvActorID = ConversationToModify.Key;
		const FRTSSavedConversationActorData& ConvActorData = ConversationToModify.Value;

		// if it was already spawned in when parsing interactables to spawn 
		if (CurrentWorldInteractables.ActorInfo.Contains(ConvActorID))
		{
			ARTSOInteractableConversationActor* ExistingActor = Cast<ARTSOInteractableConversationActor>(CurrentWorldInteractables.ActorInfo.FindRef(ConvActorID).ActorInWorld);
			if (ExistingActor == nullptr)
			{
				UE_LOG(PDLog_RTSO, Error, TEXT("ARTSOBaseGM::OnThreadFinished_PlayerLoadDataSync -- Missing Expected ConversationActor by ConversationActor ID: %i"), ConvActorID)
				continue;
			}
							
			ExistingActor->SetActorLocation(ConvActorData.Location);
			for (const TTuple<int, FRTSOConversationMetaProgressionListWrapper>& PlayerConvTuple : ConvActorData.ProgressionPerPlayer)
			{
				const int32 UserID = PlayerConvTuple.Key;
				const TMap<FGameplayTag, FRTSOConversationMetaProgressionDatum>& UserProgressionMap = PlayerConvTuple.Value.ProgressionDataMap;

				for (const TTuple<FGameplayTag, FRTSOConversationMetaProgressionDatum>& UserProgressionTuple : UserProgressionMap)
				{
					const FRTSOConversationMetaProgressionDatum& MetaProgressionDatum = UserProgressionTuple.Value;

					// @todo this is nasty with self referencing tags, fix  
					FRTSOConversationMetaState& MissionInstanceData = ExistingActor->InstanceDataPerMission.FindOrAdd(MetaProgressionDatum.MissionTag); 
					MissionInstanceData.MissionTag = MetaProgressionDatum.MissionTag;
					MissionInstanceData.PhaseRequiredTags = MetaProgressionDatum.PhaseRequiredTags;
					MissionInstanceData.ProgressionPerPlayer.FindOrAdd(UserID) = MetaProgressionDatum.BaseProgression;
				}
			}
		}
	}
}

void ARTSOBaseGM::FinalizeLoadPlayerMissionTags(const TMap<int32, AActor*>& OwnerIDMappings)
{
	MProcessedTags& ProcessedData = LoadDataInProcess[static_cast<uint8>(EPDSaveDataThreadSelector::EPlayerConversationProgress)].SavedConvoTags;

	// @Note, no need to iterate ProcessedData.ToModify, it will always be empty
					
	// @done iterate and set all conversation/mission tags from ProcessedData.ToAdd
	for (TTuple<int32, FGameplayTagContainer>& TagsToAdd : ProcessedData.ToAdd)
	{
		const int32 UserID = TagsToAdd.Key;

		const AActor* User = OwnerIDMappings.Contains(UserID) ? OwnerIDMappings.FindRef(UserID) : nullptr;
		const ARTSOController* PC = Cast<ARTSOController>(User);
		if (PC == nullptr) { continue; }

		IRTSOConversationInterface::Execute_AddProgressionTagContainer(PC->GetPawn(), TagsToAdd.Value);
	}
					
	// @done iterate and delete all conversation/mission tags from ProcessedData.ToDelete
	for (const TTuple<int32, FGameplayTagContainer>& TagsToRemove : ProcessedData.ToDelete)
	{
		const int32 UserID = TagsToRemove.Key;

		const AActor* User = OwnerIDMappings.Contains(UserID) ? OwnerIDMappings.FindRef(UserID) : nullptr;
		const ARTSOController* PC = Cast<ARTSOController>(User);
		if (PC == nullptr) { continue; }
		
		IRTSOConversationInterface::Execute_RemoveProgressionTagContainer(PC->GetPawn(), TagsToRemove.Value);
	}
}

void ARTSOBaseGM::OnThreadFinished_PlayerLoadDataSync(EPDSaveDataThreadSelector FinishedThread, const FString& SlotCopy)
{
	const TMap<int32, AActor*>& OwnerIDMappings = GEngine->GetEngineSubsystem<UPDRTSBaseSubsystem>()->SharedOwnerIDMappings;
	
	FinishedLoadThreads[static_cast<uint8>(FinishedThread)] = true;
	bool bAllThreadsLoaded = true;
	for (const bool& LoadThreadState : FinishedLoadThreads)
	{
		bAllThreadsLoaded &= LoadThreadState ;
	}

	if (bAllThreadsLoaded && bProcessingLoadData)
	{
		bProcessingLoadData = false;
		FinalizeLoadReservedPlayerControllers(OwnerIDMappings);					
		FinalizeLoadInteractableData();
		FinalizeLoadEntityData();
		FinalizeLoadInventoryData(OwnerIDMappings);
		FinalizeLoadConversationActors();
		FinalizeLoadPlayerMissionTags(OwnerIDMappings);

		
		for (const TTuple<int32, AActor*>& OwnerTuple : OwnerIDMappings)
		{
			const FRTSOActionLogEvent NewActionEvent{
				FString::Printf(TEXT("OwnerID(%i) -- Loaded save slot: %s "), OwnerTuple.Key, *SlotCopy)}; 
			URTSActionLogSubsystem::DispatchEvent(OwnerTuple.Key, NewActionEvent);		
		}
	}
}

void ARTSOBaseGM::GatherEntityToSpawn(
	const UWorld& WorldRef,
	const FRTSSavedWorldUnits& EntityData,
	TMap<const FMassEntityTemplateID, FEntityCompoundTuple>& EntityPool,
	UPDRTSBaseSubsystem* RTSSubsystem,
	UMassSpawnerSubsystem* SpawnerSystem)
{
	const FPDBuildWorker* Unit = UPDBuilderSubsystem::GetWorkerDataStatic(EntityData.EntityUnitTag);
	if (Unit == nullptr)
	{
		UE_LOG(PDLog_RTSO, Error, TEXT("ARTSOBaseGM::GatherEntityToSpawn -- Found no worker data for worker of type :%s"), *EntityData.EntityUnitTag.GetTagName().ToString())
		return;
	}
						
	const UMassEntityConfigAsset* LoadedConfig = Unit->MassEntityData->EntityConfig.LoadSynchronous();
	const FMassEntityTemplate& Template = LoadedConfig->GetOrCreateEntityTemplate(WorldRef);
				
	// Make sure the subsystem associates the archetypes and configs in-case we
	// load entities from a save file and not via an ARTSOMassSpawner
	const FMassArchetypeHandle& Archetype = SpawnerSystem->GetMassEntityTemplate(Template.GetTemplateID())->GetArchetype();
	RTSSubsystem->AssociateArchetypeWithConfigAsset(Archetype, Unit->MassEntityData->EntityConfig);

	const FMassEntityTemplateID TemplateID = Template.GetTemplateID();
	if (EntityPool.Contains(TemplateID) == false)
	{
		FInnerEntityData TempDataArray;
		TempDataArray.Emplace(&EntityData);
				
		FEntityCompoundTuple TempData {1, TempDataArray};
		EntityPool.Emplace(Template.GetTemplateID(), TempData);
	}
	else
	{
		FEntityCompoundTuple& FoundEntityCompound = EntityPool.FindChecked(TemplateID);
		FoundEntityCompound.Key += 1;
		FoundEntityCompound.Value.Emplace(&EntityData);
	}
						
	// @todo load other fragment data, there are plenty still not being stored	
}

void ARTSOBaseGM::DispatchEntitySpawning(
	const ARTSOController* Caller,
	const TTuple<const FMassEntityTemplateID, FEntityCompoundTuple>& EntityTypeCompound,
	const FMassEntityManager* EntityManager,
	UMassSpawnerSubsystem* SpawnerSystem)
{
	TArray<FMassEntityHandle> OutHandles;
	SpawnerSystem->SpawnEntities(*SpawnerSystem->GetMassEntityTemplate(EntityTypeCompound.Key), EntityTypeCompound.Value.Key, OutHandles);
	
	int32 Step = 0;
	for (const FMassEntityHandle& NewEntityHandle : OutHandles)
	{
		const FInnerEntityData& EntityDataArray = EntityTypeCompound.Value.Value;
		const FRTSSavedWorldUnits* WorldEntityData = EntityDataArray[Step];
			
		// Load entities owner data
		FPDMFragment_RTSEntityBase* EntityBaseFragment = EntityManager->GetFragmentDataPtr<FPDMFragment_RTSEntityBase>(NewEntityHandle);
		if (EntityBaseFragment != nullptr)
		{
			EntityBaseFragment->OwnerID = WorldEntityData->OwnerID;
			EntityBaseFragment->SelectionGroupIndex = WorldEntityData->SelectionIndex;
		}
			
		// Load entities active task/job data and stats
		FPDMFragment_Action* CurrentAction = EntityManager->GetFragmentDataPtr<FPDMFragment_Action>(NewEntityHandle);
		if (CurrentAction != nullptr)
		{
			*CurrentAction = WorldEntityData->CurrentAction;
		}
			
		// Update entities transform to the loaded position
		FTransformFragment* EntityTransform = EntityManager->GetFragmentDataPtr<FTransformFragment>(NewEntityHandle);
		if (EntityTransform != nullptr)
		{
			EntityTransform->SetTransform(FTransform{WorldEntityData->Location});
		}
			
		++Step;
			
		// // @todo make use of or remove
		// WorldEntityData.Health;	
	}

	TotalEntitiesSpawned += Step;
	if (nullptr != Caller)
	{
		Caller->UpdateEntityCount(TotalEntitiesSpawned);
	}
}


void ARTSOBaseGM::OnGeneratedLandscapeReady_Implementation()
{
	// Load file data if it exists
}

void ARTSOBaseGM::LoadEntities_Implementation(const TArray<FRTSSavedWorldUnits>& OverrideEntityUnits)
{
	check(GameSave != nullptr)

	UPDRTSBaseSubsystem* RTSSubsystem = UPDRTSBaseSubsystem::Get();
	const FMassEntityManager* EntityManager = RTSSubsystem != nullptr ? RTSSubsystem->EntityManager : nullptr;
	UMassSpawnerSubsystem* SpawnerSystem = UWorld::GetSubsystem<UMassSpawnerSubsystem>(GetWorld());
	if (EntityManager == nullptr) { return; }

	using FInnerEntityData = TArray<const FRTSSavedWorldUnits*>;
	using FEntityCompoundTuple = TTuple<int32, FInnerEntityData>;
	TMap<const FMassEntityTemplateID, FEntityCompoundTuple> EntitiesToSpawn{};
	
	for (const FRTSSavedWorldUnits& EntityData : OverrideEntityUnits.IsEmpty() ? GameSave->Data.EntityUnits : OverrideEntityUnits)
	{
		GatherEntityToSpawn(*GetWorld(), EntityData, EntitiesToSpawn, RTSSubsystem, SpawnerSystem);
	}

	for (const TTuple<const FMassEntityTemplateID, FEntityCompoundTuple>&
		EntityTypeCompound :  EntitiesToSpawn)
	{
		DispatchEntitySpawning(Cast<ARTSOController>(GetWorld()->GetFirstPlayerController()), EntityTypeCompound, EntityManager, SpawnerSystem);
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
