/* @author: Ario Amin @ Permafrost Development. @copyright: Full BSL(1.1) License included at bottom of the file  */

#pragma once

#include "CoreMinimal.h"
#include "PDRTSSharedOctree.h"
#include "AI/Mass/PDMassFragments.h"
#include "PDRTSSharedHashGrid.h"

#include "Tickable.h"
#include "MassEntityConfigAsset.h"
#include "MassEntityTypes.h"
#include "MassArchetypeTypes.h"

#include "GameplayTagContainer.h"

#include "HAL/CriticalSection.h"
#include "HAL/UnrealMemory.h"
#include "Misc/ScopeLock.h"
#include "Misc/ScopeRWLock.h"

#include "Engine/StreamableManager.h"
#include "Subsystems/EngineSubsystem.h"

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 4
#include "RenderGraphResources.h"
#endif

#include "PDRTSBaseSubsystem.generated.h"

/** fwd decl.  */
class UPDRTSBaseUnit;
class UMassEntitySubsystem;
class UTextureRenderTarget2D;
struct FPDWorkUnitDatum;

UENUM()
enum class ERTSEntityResourceCountState : uint8
{
	ENeedsAllResources,
	ENeedsSpecificCount,
	Undefined
};

USTRUCT()
struct PDRTSBASE_API FRTSEntityResourceGatherTarget
{
	GENERATED_BODY()
public:
	FRTSEntityResourceGatherTarget(const AActor* InTarget, int32 InCount) : Target(InTarget), CountNeeded(InCount) 
	{
		State = InCount < 0 ? ERTSEntityResourceCountState::ENeedsAllResources : ERTSEntityResourceCountState::ENeedsSpecificCount;
	}

	FRTSEntityResourceGatherTarget(const AActor* InTarget) : Target(InTarget), CountNeeded(INDEX_NONE) 
	{
		State = ERTSEntityResourceCountState::ENeedsAllResources;
	}

	FRTSEntityResourceGatherTarget() : Target(nullptr), CountNeeded(INDEX_NONE) 
	{
		State = ERTSEntityResourceCountState::ENeedsAllResources;
	}

	inline ERTSEntityResourceCountState GetCount(int32& OutCount) 
	{
		OutCount = CountNeeded;
		return State;
	}

	UPROPERTY()
	const AActor* Target; 
private:
	ERTSEntityResourceCountState State = ERTSEntityResourceCountState::Undefined;
	UPROPERTY()
	int32 CountNeeded = -1;
}; 

USTRUCT()
struct PDRTSBASE_API FPDRTSTSetActorWrapper
{
	GENERATED_BODY()

	UPROPERTY()
	TSet<const AActor*> Actors;
};

USTRUCT()
struct PDRTSBASE_API FPDRTSTSetTagWrapper
{
	GENERATED_BODY()

	UPROPERTY()
	TSet<FGameplayTag> Tags;	
};

USTRUCT()
struct PDRTSBASE_API FPDRTSTGatherTargetsWrapper
{
	GENERATED_BODY()


	TDeque<FRTSEntityResourceGatherTarget> Targets;	
};


DECLARE_DELEGATE_SevenParams(FRTSBuildGlobalSortEntityShader, FRHICommandListImmediate& /*RHICmdList*/, UTextureRenderTarget2D* /*RenderTarget*/, const TRefCountPtr<FRDGPooledBuffer>& /*EntityInputPooledBuffer*/, TArray<FLinearColor> /*InData*/, float /* CameraYawInRadians */ , FVector /*RegionMin*/, FVector /*RegionSize*/)

/** @brief RTS Subsystem 
 * - Handles octree size changes
 * - Acts as a manager for the entity workers, assigning jobs etc
 * - Manages sending entity data to a compute shader */
UCLASS()
class PDRTSBASE_API UPDRTSBaseSubsystem 
	: public UEngineSubsystem
	, public FTickableGameObject
{
	GENERATED_BODY()
public:
	/** @brief Shorthand to get the subsystem,
	 * @note as the engine will instantiate these subsystem earlier than anything will reasonably call Get()  */
	static UPDRTSBaseSubsystem* Get();

	/** @brief ReadLocked getter for the PlayerYawAsRad */
	FORCEINLINE static float GetUserRotationCurrentTick() 
	{
		FReadScopeLock ReadLock(UPDRTSBaseSubsystem::PlayerDataLock);
		return PlayerYawAsRad;
	}
	/** @brief WriteLocked setter for the PlayerYawAsRad */
	FORCEINLINE static void SetUserRotationOnTick(float YawInRad) 
	{
		FWriteScopeLock WriteLock(UPDRTSBaseSubsystem::PlayerDataLock);
		PlayerYawAsRad = YawInRad;
	}


	// Tickable Interface
	virtual bool IsTickableWhenPaused() const final { return false; }
	virtual bool IsTickableInEditor() const final {return false;}
	virtual UWorld* GetTickableGameObjectWorld() const final{ return const_cast<UWorld*>(TemporaryWorldCache); }
	virtual void Tick( float DeltaTime ) final;
	virtual ETickableTickType GetTickableTickType() const final { return ETickableTickType::Conditional; }
	virtual bool IsTickable() const final { return TemporaryWorldCache != nullptr; }
	virtual bool IsAllowedToTick() const final { return TemporaryWorldCache != nullptr; }
	virtual TStatId GetStatId() const final;

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	void OnWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources);
	static bool IsTrackedWorldType(const UWorld* World);
	FDelegateHandle WorldCleanupHandle;
	
	/** @brief Dispatch the octree generation on a latent action, via the worlds 'LatentActionManager' */
	UFUNCTION()
	void DispatchOctreeGeneration();
	
	/** @brief Bound to the given developer setting. Resolved the paths into actual WorkTables Array */
	void OnDeveloperSettingsChanged(UObject* SettingsToChange, FPropertyChangedEvent& PropertyEvent);

	/** @brief Processes the tables in 'WorkTables' and fills a number of maps for fast lookups downstream for entity jobs and such */
	UFUNCTION()
	void ProcessTables();

	/** @brief Loads tables into 'WorkTables' and calls ProcessTables */
	UFUNCTION()
	void LoadAndProcessTables();
	
	/** @brief Requests to generate a navpath for the selection group to the given target*/
	UFUNCTION()
	virtual void RequestNavpathGenerationForSelectionGroup(
		int32 OwnerID,
		int32 SelectionGroup,
		const FVector& SelectionCenter,
		const FPDTargetCompound& TargetCompound);
	
	/** @brief Returns the default work data via it's job-tag*/
	const FPDWorkUnitDatum* GetWorkEntry(const FGameplayTag& JobTag);
	/** @brief Returns the default work data via it's rowname in the table it was sourced from*/
	const FPDWorkUnitDatum* GetWorkEntry(const FName& JobRowName);
	
	/** @brief Finding all eligible entities of a given buildable AActor* */
	static TArray<FMassEntityHandle> FindIdleEntitiesOfType(TArray<FGameplayTag> EligibleEntityTypes, const AActor* ActorToBuild, int32 OwnerID);
	
	/** @brief Associates and FMassArchetypeHandle with a config asset, so we can retrieve this info back to our save system fast when needed */
	void AssociateArchetypeWithConfigAsset(const FMassArchetypeHandle& Archetype, const TSoftObjectPtr<UMassEntityConfigAsset>& EntityConfig);

	/** @brief Retrieves the config asset */
	TSoftObjectPtr<UMassEntityConfigAsset> GetConfigAssetForArchetype(const FMassArchetypeHandle& Archetype);

	/** @brief Caches the worlds entity manager and sets up pooled buffers for a bitonic sort in a compute shader */
	void WorldInit(const UWorld* World);
	/** @brief Releases pooled buffers */
	void WorldDeinit(const UWorld* World);

	/** @brief  */
	void TryRemoveTrackedResourceEntry(const FGameplayTag& ResourceType, const AActor* TrackedActor);
	/** @brief  */
	void TryRemoveTrackedCellEntry(FPDGridCell GridCell, const AActor* TrackedActor);

private:
	/** @brief  */
	void TryRemoveTrackedResourceEntry_Unsafe(const FGameplayTag& ResourceType, const AActor* TrackedActor);
	/** @brief  */
	void TryRemoveTrackedCellEntry_Unsafe(FPDGridCell GridCell, const AActor* TrackedActor);

public:
	/** @brief  */
	void TrackResource(const FGameplayTag& ResourceType, const AActor* TrackedActor); 
	/** @brief  */
	void UntrackResource(const FGameplayTag& ResourceType, const AActor* TrackedActor); 
	/** @brief  */
	void UntrackAllFromResourceActor(const FGameplayTag& ResourceType, const AActor* TrackedActor); 
	/** @brief  */
	void UntrackAllFromResourceActor_Unsafe(const FGameplayTag& ResourceType, const AActor* TrackedActor); 	
	/** @brief  */
	void UpdateResources(const AActor* TrackedActor); 


	/** @brief  */
	void AddEntityResourceTarget(FMassEntityHandle MassEntity, const FRTSEntityResourceGatherTarget& ResourceTarget); 
	/** @brief  */
	FPDRTSTGatherTargetsWrapper* GetEntityResourceTargets(FMassEntityHandle MassEntity);
	/** @brief  */
	void RemoveEntityResourceTarget(FMassEntityHandle MassEntity); 
	/** @brief  */
	void RemoveAllFEntityResourceTargets(); 


	/** @brief  */
	const FPDRTSTSetActorWrapper* GetResourceActors(const FGameplayTag& ResourceType);
	/** @brief  */
	const FPDRTSTSetActorWrapper* GetResourceActorsAtGridCell(FPDGridCell GridCell);
	/** @brief  @todo A bit inefficient, think of bulking these calls for a set ot GridCells and make a new function*/
	TSet<const AActor*> GetResourceActorsAtGridCellWithResourceType(const FRTSOFindResourcesParameters& SearchParams);

	/** @brief  @todo A bit inefficient, think of bulking these calls for a set ot GridCells and make a new function*/
	TSet<const AActor*> GetResourceActorsNearGridCellWithResourceType(const FRTSOFindResourcesParameters& SearchParams);	

private:
	const FPDRTSTSetActorWrapper* GetResourceActorsAtGridCell_Unsafe(FPDGridCell GridCell);

public:
	/** @brief  */
	void ProcessResourceActors(FSimpleDelegate ProcessDelegate);


	/** @brief Does some portable iso-approved 'hacks' to fetch the all the mass ISM's */
	static const TArray<TObjectPtr<UInstancedStaticMeshComponent>>& GetMassISMs(const UWorld* InWorld);
	
	/** @brief Reserved for other usagesm currently only calls UpdateDataTexture */
	void GenerateEntityMapData();

	UTextureRenderTarget2D* GetEntityDataTexture() { return EntityDataTexture; };

	/** @brief Finds the linked RT texture in the subsystem OR generates a transient RT texture if none is found */
	UFUNCTION(BlueprintCallable, Category = "Texture", CallInEditor)
	void CreateDataTexture();
	/** @brief Allocates a pooled buffer that we can pass into RDG */
	UFUNCTION(BlueprintCallable, Category = "Texture", CallInEditor)
	void CreateDataBuffer();
	/** @brief Copies whatever entites has been queried by the query object and send it to the RDG to pass it into the buffer and to then pass it into MinimapSplat.usf */
	UFUNCTION(BlueprintCallable, Category = "Texture", CallInEditor)
	void UpdateDataTexture();
	/** @brief Releases the buffer created for the minimap entity splatting */
	UFUNCTION(BlueprintCallable, Category = "Texture", CallInEditor)
	void DeleteBuffers(); 

	/** @brief  */
	inline static float ResourceGridSize = 20000;
	/** @brief Need to know the players camera yaw to pass it into the MinimapSplat.usf */
	static float PlayerYawAsRad;
	/** @brief Max dimension for the RT texture used in MinimapSplat.usf */
	static constexpr uint32 GMaxEntityDim = 256;
	/** @brief Max pixel count for the RT texture used in MinimapSplat.usf */
	static constexpr uint32 GMaxEntityDataSize = GMaxEntityDim * GMaxEntityDim;

public:		
	/** @brief Work tables used by subsystem to organize ai-jobs */
	UPROPERTY()
	TArray<UDataTable*> WorkTables{};
	
	/** @brief Selection group navpath map, Keyed by owner ID, valued by selection group navdata container*/
	UPROPERTY()
	TMap<int32, FPDWrappedSelectionGroupNavData> SelectionGroupNavData{};
	
	/** @brief Selection group Dirty data array holding a tuple, each tuple is keyed by the owner ID and has a value of the dirtied selection group index
	 *  @todo think on a solution which marks which actual data we want to update for the group, but this for now works as a solid enough optimization
	 */
	TArray<TTuple<int32 /*OwnerID*/, int32/*Player 'Selection-group' Index */> > DirtySharedData{};
	
	/** @brief Map for fast lookups. Keyed by job-tag, valued by default data entry */
	TMap<const FGameplayTag, const FPDWorkUnitDatum*> TagToJobMap{};
	/** @brief Map for fast lookups. Keyed by Rowname, valued by job-tag */
	TMap<const FName, FGameplayTag> NameToTagMap{};
	/** @brief Map for fast lookups. Keyed by Job-tag, valued by Rowname */
	TMap<const FGameplayTag, FName> TagToNameMap{};
	/** @brief Map for fast lookups. Keyed by Job-tag, valued by source datatable */
	TMap<const FGameplayTag, const UDataTable*> TagToTable{};

	/** @brief Maps active entity handlers to their worlds */
	TMap<UWorld*, UPDRTSBaseUnit*> WorldToEntityHandler{};

	/** @brief Flag that checks if we have processed the worktables */
	uint8 bHasProcessedTables = false;
	/** @brief Counter how many times the ProcessTables() function failed internally  */
	uint16 ProcessFailCounter = 0;

	/** @brief Reserved for later use */
	FStreamableManager DataStreamer;

	/** @brief Cached entity manager ptr*/
	const FMassEntityManager* EntityManager = nullptr;

	UPROPERTY()
	/** @brief effectively unused. @todo revise if it still needed. any significant use of it has been removed since prototyping this  */
	const UWorld* TemporaryWorldCache = nullptr;

	/** @brief The actual octree our entities will make use of*/
	PD::Mass::Entity::Octree WorldEntityOctree;
	
	/** @brief Tracking worlds that has been setup with this WorldOctree.  */
	TMap<void*, bool> WorldsWithOctrees{};
	
	/** @brief Softpaths to entity configs, keyed by the archetype handle. Is set by used by our sub-classed entity spawner */
	TMap<FMassArchetypeHandle /*Archetype*/, TSoftObjectPtr<UMassEntityConfigAsset> /* EntityConfig */> ConfigAssociations{};
	
	/** @brief This will be useful on a server or shared screen environment */
	UPROPERTY(EditAnywhere)
	TMap<int32 /*OwnerID*/, AActor* /*OwningActor*/> SharedOwnerIDMappings;
	/** @brief This will be useful on a server or shared screen environment */
	UPROPERTY(EditAnywhere)
	TMap<AActor* /*OwningActor*/, int32 /*OwnerID*/> SharedOwnerIDBackMappings;

	
	/** @brief  (User) Query Shape structure */
	FPDOctreeUserQuery OctreeUserQuery{};
	
	/** @brief Delegate that we pass inot the render thread */
	FRTSBuildGlobalSortEntityShader BuildEntitySortComputeShader;
	
private:
	/** @brief */
	UPROPERTY()
	TMap<FGameplayTag /*resource/item tag*/, FPDRTSTSetActorWrapper> TrackedResourceGroups;
	TSortedMap<FPDGridCell /*Gridcell*/, FPDRTSTSetActorWrapper> TrackedResourceGroupsPerGridCell;
	
	/*
	+ + +   + + +   + + +
	+ X +   + Y +   + Z +
	+ + +   + + +   + + +
	*/
	TMap<FPDGridCell /*Gridcell*/, TArray<FPDGridCell> /*Neighbours*/> TrackedResourceGroupsPerGridCellNeighbours;
	TArray<FPDGridCell> FindValidNeighours(const FPDGridCell& GridCell)
	{
		TArray<FPDGridCell> FoundNeighbours;
		constexpr int32 MaxNeighbourDim = 3;
		FPDGridCell StartGridCell = GridCell - FPDGridCell::Construct(-1);

		// Likely inefficient in a tight loop, rewrite into flat 1dim loop whenever it becomes a problem
		for (int32 CellStepsX = 0; CellStepsX < MaxNeighbourDim; CellStepsX++)
		{
			int32 CurrentX = CellStepsX;
			for (int32 CellStepsY = 0; CellStepsY < MaxNeighbourDim; CellStepsY++)
			{
				for (int32 CellStepsZ = 0; CellStepsZ < MaxNeighbourDim; CellStepsZ++)
				{
					FPDGridCell PotentialNeighbour = StartGridCell + FPDGridCell::Construct(CellStepsX, CellStepsY, CellStepsZ);
					if(PotentialNeighbour == GridCell) {continue;}
					if (nullptr != GetResourceActorsAtGridCell_Unsafe(PotentialNeighbour)) { FoundNeighbours.Emplace(PotentialNeighbour); }
				}
			}
		}

		return FoundNeighbours;
	}

	TMap<int32, FPDGridCell> GridCellOrder;
	UPROPERTY()
	TMap<const AActor*, FPDGridCell> TrackedResourceToGridCell;
	mutable FRWLock ResourceRWLock;

	UPROPERTY()
	TMap<FMassEntityHandle, FPDRTSTGatherTargetsWrapper> EntitiesCurrentResourceTargets;



	/** @brief The RT texture we are splatting our entities unto */
	UPROPERTY()
	class UTextureRenderTarget2D* EntityDataTexture;

	/** @brief Reference to the pooled buffer of entity data */
	TRefCountPtr<FRDGPooledBuffer> EntityInputPooledBuffer;
	bool bHasCreatedPooledBuffers = false;

	/** @brief Static ReadWrite lock. Wanted to prioritize writers while letting readers not block other readers */
	static FRWLock PlayerDataLock;

	/** @brief Static ReadWrite lock. Wanted to prioritize writers while letting readers not block other readers */
	mutable FRWLock EntityResourceTaskLock;	
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