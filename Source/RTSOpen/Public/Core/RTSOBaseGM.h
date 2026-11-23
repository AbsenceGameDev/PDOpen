/* @author: Ario Amin @ Permafrost Development. @copyright: Full BSL(1.1) License included at bottom of the file  */
#pragma once

#include "CoreMinimal.h"
#include "MassSpawnerSubsystem.h"
#include "PDRTSBaseSubsystem.h"
#include "RTSOpenCommon.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/GameUserSettings.h"
#include "RTSOBaseGM.generated.h"

class ARTSOController;
class URTSOMainMenuBase;
class URTSOpenSaveGame;

/** @brief  Load-screen state, are we just ending or in the middle of loading */
UENUM()
enum ERTSLoadscreenState
{
	LOADING_IN,
	LOADING_OUT,
};


/**
 * @brief RTSO user auto-save settings.
 * @note on a server authoritative game this will only exist for the server,
 * and will control how often the server saves the worlds state  
 */
USTRUCT(Blueprintable, BlueprintType)
struct FRTSOAutoSaveSettings
{
	GENERATED_BODY()

	FRTSOAutoSaveSettings() : TimeLimitAsSeconds(AutoSaveInterval * 60.0) {};

	/** @brief Increments the current slot and returns the result */
	FString GetNextAutoSlot()
	{
		LatestAutoSaveSlot = (++LatestAutoSaveSlot % AutoSaveSlots); // Force range [1-10], makes sure we don't hit anything twice
		return AutoSlotBase + FString::FromInt(LatestAutoSaveSlot);
	}

	/** @brief Amount of time between each auto-save (in minutes) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 AutoSaveInterval = 60;

	/** @brief Amount of auto save slots before it starts overwriting itself.
	 * @note 0 disables the auto-save functionality */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 AutoSaveSlots = 10;

	/** @brief Active time, accumulates ticks delta-time and resets upon an auto-save */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly)
	double ElapsedTimeSinceSave = 0.0;

	/** @brief Calculated time limit as seconds */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly)
	double TimeLimitAsSeconds = AutoSaveInterval * 60;
	
	/** @brief Timer delay handle, to ensure we don't call SaveGame multiple times in a row */
	FTimerHandle SaveTimerHandle{};

private:
	/** @brief Cached latest slot, starting at INDEX_NONE but in reality this gets  */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Meta = (AllowPrivateAccess="true"))
	int32 LatestAutoSaveSlot = INDEX_NONE;
	static inline const FString AutoSlotBase = "AUTOSLOT_";
};


/** @brief Auto save - game user settings, exposed inner struct */
UCLASS(Blueprintable, BlueprintType)
class URTSOAutoSaveSettingObject : public UGameUserSettings
{
	GENERATED_BODY()

	/** @brief  Users auto save settings */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Meta = (AllowPrivateAccess="true"))
	FRTSOAutoSaveSettings Inner;
};

/**
 * @brief RTSO game mode
 */
UCLASS()
class RTSOPEN_API ARTSOBaseGM : public AGameModeBase
{
	GENERATED_UCLASS_BODY()

public:
	/** @brief  Creates a new savegame object and tells the GI that the GM is ready */
	virtual void BeginPlay() override;

	/** @brief Unload entities from the octree */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
	//
	// Saving/loading management
	/** @brief Clears save, possibly reseeds the save, and sets some flags */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void ClearSave(const bool bRefreshSeed = false);

	/* Saving functions */
	
	/** @brief Dispatches an async save */
	UFUNCTION(BlueprintCallable)
	void SaveGame(FString SlotNameCopy, const bool bAllowOverwrite = false);
	UFUNCTION(BlueprintImplementableEvent)
	void OnSaveGame(const FString& SlotName, const bool bAllowOverwrite = false);

	/** @brief Dispatches an async save */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void ProcessChangesAndSaveGame(const FString& Slot, const bool bAllowOverwrite = false);

	/** @brief Save ConversationProgression */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void SaveConversationProgression();	

	/** @brief Save ConversationProgression */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void SaveConversationActorStates();		

	/** @brief Save all player states */
	UFUNCTION(BlueprintCallable)
	void SaveAllPlayerStates();	
	
	/** @brief Save player state */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void SavePlayerState(ARTSOController* PlayerController);
	
	/** @brief Save interactables */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void SaveInteractables();

	/** @brief Save items/resources */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void SaveAllItems();

	/** @brief Save worker/units */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void SaveEntities();

	/* Loading functions */
	
	/** @brief Loads save from slot, if a save exists */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void LoadGame(const FString& Slot, const bool bDummyParam = false);

	/** @brief If making use of PCG */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void OnGeneratedLandscapeReady();

	/** @brief Reserved. Only calls super for now */
	virtual void Logout(AController* Exiting) override;
	/** @brief  Spawns a default base for our player in case they do no have buildings in the world already. */
	virtual void PostLogin(APlayerController* NewPlayer) override;
	/** @brief @todo Load the player and ensure we associate teh player with their preloaded data, this also requires finishing up the preloaders */
	virtual APlayerController* Login(UPlayer* NewPlayer, ENetRole InRemoteRole, const FString& Portal, const FString& Options, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;
	
	/** @brief Load worker/units from current save data*/
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void LoadEntities(const TArray<FRTSSavedWorldUnits>& OverrideEntityUnits);	

	/** @brief Runs configured auto-saver */
	virtual void TickActor(float DeltaTime, ELevelTick TickType, FActorTickFunction& ThisTickFunction) override;

	/** @brief Spawns player controllers and keeps them reserved until player with mathcing ID logs in, @todo at which point they need to be assigned to the incoming connection with matching ID*/
	void FinalizeLoadReservedPlayerControllers(const TMap<int32, AActor*>& OwnerIDMappings);
	void FinalizeLoadInteractableData();
	void FinalizeLoadEntityData();
	void FinalizeLoadInventoryData(const TMap<int32, AActor*>& OwnerIDMappings);
	void FinalizeLoadConversationActors();
	void FinalizeLoadPlayerMissionTags(const TMap<int32, AActor*>& OwnerIDMappings);
	void OnThreadFinished_PlayerLoadDataSync(EPDSaveDataThreadSelector FinishedThread, const FString& SlotCopy);

	/** @brief Alias for an array view of unit/entity data */
	using FInnerEntityData = TArray<const FRTSSavedWorldUnits*>;
	/** @brief Alias for a tuple that associates a user and their world units/entities  */
	using FEntityCompoundTuple = TTuple<int32, FInnerEntityData>;

	/** @brief  Stores the given entity from 'EntityData' in the referenced 'EntityPool' */
	static void GatherEntityToSpawn(
		const UWorld& WorldRef,
		const FRTSSavedWorldUnits& EntityData,
		TMap<const FMassEntityTemplateID, FEntityCompoundTuple>& EntityPool,
		UPDRTSBaseSubsystem* RTSSubsystem,
		UMassSpawnerSubsystem* SpawnerSystem);

	/** @brief  Dispatches the given entity from 'EntityTypeCompound' via the UMassSpawnerSubsystem */
	static void DispatchEntitySpawning(
		const class ARTSOController* Caller,
		const TTuple<const FMassEntityTemplateID, FEntityCompoundTuple>& EntityTypeCompound,
		const FMassEntityManager* EntityManager,
		UMassSpawnerSubsystem* SpawnerSystem);
	
public:
	
	/** @brief Actual game save-data */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	URTSOpenSaveGame* GameSave = nullptr;

	/** @brief Auto Save Settings, @todo load from GameUser Settings */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FRTSOAutoSaveSettings AutoSave{};
	
	/** @brief Flag, has started async */
	bool bHasStartedSaveData = false;
	/** @brief Flag, has finished async */
	bool bHasSavedDataAsync = false;
	
	/** @brief  */
	inline static int32 TotalEntitiesSpawned = 0;

	/** @brief Reserved use, for when inventory items lists grow large we want to avoid costly search operations */
	const TMap<int32, FRTSSavedItems>* MapPointer = nullptr;

	/** @brief Root save slot name, fallback if none is selected */
	static const FString ROOTSAVE;

private:
	
	/** @brief We preload controller upon startup, when the player has connected we assign the controller to them
	 *  @todo Finish supporting code for preloaded controllers */
	UPROPERTY()
	TArray<APlayerController*> PreloadedControllers{};

	#define TPDPreprocessLoadData(Name, TContainer,  ...) \
	typedef struct Name  \
	{ \
	public:\
		TContainer<__VA_ARGS__> ToDelete; \
		TContainer<__VA_ARGS__> ToAdd; \
		TContainer<__VA_ARGS__> ToModify; \
	} _##Name;
	
	/** @brief  Helpers for processing savedata changes, only overwrite what has been modified, only add/remove what is new/old */
	TPDPreprocessLoadData(MProcessedInteractData, TArray, FRTSSavedInteractable);
	/** @brief  Helpers for processing savedata changes, only overwrite what has been modified, only add/remove what is new/old */
	TPDPreprocessLoadData(MProcessedUnits, TArray, FRTSSavedWorldUnits);
	/** @brief  Helpers for processing savedata changes, only overwrite what has been modified, only add/remove what is new/old */
	TPDPreprocessLoadData(MProcessedItems, TMap, int32, FRTSSavedItems);
	/** @brief  Helpers for processing savedata changes, only overwrite what has been modified, only add/remove what is new/old */
	TPDPreprocessLoadData(MProcessedLocations, TMap, int32, FVector);
	/** @brief  Helpers for processing savedata changes, only overwrite what has been modified, only add/remove what is new/old */
	TPDPreprocessLoadData(MProcessedConvos, TMap, int32, FRTSSavedConversationActorData);
	/** @brief  Helpers for processing savedata changes, only overwrite what has been modified, only add/remove what is new/old */
	TPDPreprocessLoadData(MProcessedTags, TMap, int32, FGameplayTagContainer);

	/** @brief Union that helps us keep a smaller memory footprint when dispatching data chunks for threaded calculations */
	union ProcessedLoadData
	{
		ProcessedLoadData() {}
		~ProcessedLoadData() {}
		MProcessedInteractData SavedInteracts;
		MProcessedUnits     SavedUnits;
		MProcessedItems     SavedItems;
		MProcessedLocations SavedLocs;
		MProcessedConvos    SavedConvoActors;
		MProcessedTags      SavedConvoTags;		
	};
	
	/** @brief State flag, so we don't prematurely finalize the loaded data until all threads are finished */
	bool bProcessingLoadData = false;
	/** @brief List of finished threads */
	TStaticArray<bool, static_cast<uint8>(EPDSaveDataThreadSelector::EEnd), 8> FinishedLoadThreads;
	/** @brief List of active/in-process threads */
	TStaticArray<ProcessedLoadData, static_cast<uint8>(EPDSaveDataThreadSelector::EEnd), 8> LoadDataInProcess;
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
