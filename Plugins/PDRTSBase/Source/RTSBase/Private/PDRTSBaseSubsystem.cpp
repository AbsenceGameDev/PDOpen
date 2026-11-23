/* @author: Ario Amin @ Permafrost Development. @copyright: Full BSL(1.1) License included at bottom of the file  */
#include "PDRTSBaseSubsystem.h"
#include "PDRTSCommon.h"

#include "DelayAction.h"
#include "MassCommonFragments.h"
#include "MassCrowdRepresentationSubsystem.h"
#include "MassEntitySubsystem.h"
#include "MassRepresentationTypes.h"
#include "MassVisualizationComponent.h"
#include "MassVisualizer.h"
#include "NavigationSystem.h"
#include "PDBuilderSubsystem.h"
#include "Interfaces/PDRTSBuildableGhostInterface.h"
#include "Pawns/PDRTSBaseUnit.h"

#include "HAL/UnrealMemory.h"
#include "HAL/ThreadingBase.h"
#include "HAL/PlatformCrt.h"
#include "Async/Mutex.h"
#include "Async/UniqueLock.h"
#include "Misc/ScopeRWLock.h"

#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"

// Shader inc
#include "RenderGraphUtils.h"

struct FTransformFragment;
class UMassCrowdRepresentationSubsystem;
class AMassVisualizer;


float UPDRTSBaseSubsystem::PlayerYawAsRad = 0.0;
FRWLock UPDRTSBaseSubsystem::PlayerDataLock{};

void UPDRTSBaseSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	LoadAndProcessTables();

	UPDRTSSubsystemSettings* Settings = GetMutableDefault<UPDRTSSubsystemSettings>();
	ResourceGridSize = Settings->ResourceGridSize;

	WorldCleanupHandle = FWorldDelegates::OnWorldCleanup.AddUObject(this, &UPDRTSBaseSubsystem::OnWorldCleanup);	
}


void UPDRTSBaseSubsystem::Deinitialize()
{
	FWorldDelegates::OnWorldCleanup.Remove(WorldCleanupHandle);
	Super::Deinitialize();
}

bool UPDRTSBaseSubsystem::IsTrackedWorldType(const UWorld* World)
{
	return World != nullptr && 
		(World->WorldType == EWorldType::Game 
		|| World->WorldType == EWorldType::PIE 
		|| World->WorldType == EWorldType::Editor);
}

void UPDRTSBaseSubsystem::OnWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources)
{
	if (IsTrackedWorldType(World) == false) { return; }

	WorldToEntityHandler.Remove(World);
	WorldsWithOctrees.Remove(World);
	WorldEntityOctree.WorldsWithOctrees.Remove(World);
	WorldEntityOctree.TemporaryWorldCache = WorldEntityOctree.TemporaryWorldCache == World ? nullptr : WorldEntityOctree.TemporaryWorldCache;
	if (TemporaryWorldCache != World) { return; }

	TemporaryWorldCache = nullptr; // also stops ticking, see IsTickable()
	EntityManager = nullptr;
	DeleteBuffers();
}
 

void UPDRTSBaseSubsystem::LoadAndProcessTables()
{
	for (const TSoftObjectPtr<UDataTable>& TablePath : GetDefault<UPDRTSSubsystemSettings>()->WorkTables)
	{
		UDataTable* ResolvedTable = TablePath.LoadSynchronous();
		WorkTables.Emplace(ResolvedTable);

		ResolvedTable->OnDataTableChanged().AddLambda([&]() { ProcessTables(); });
	}

	GetMutableDefault<UPDRTSSubsystemSettings>()->OnSettingChanged().AddLambda(
		[&](UObject* SettingsToChange, FPropertyChangedEvent& PropertyEvent)
		{
			OnDeveloperSettingsChanged(SettingsToChange,PropertyEvent);
		});

	ProcessTables();
}

void UPDRTSBaseSubsystem::DispatchOctreeGeneration()
{
	const FLatentActionInfo DelayInfo{0,0, TEXT("SetupOctree"), this};
	FLatentActionManager& LatentActionManager = GetWorld()->GetLatentActionManager();
	if (LatentActionManager.FindExistingAction<FDelayAction>(DelayInfo.CallbackTarget, DelayInfo.UUID) == nullptr)
	{
		LatentActionManager.AddNewAction(DelayInfo.CallbackTarget, DelayInfo.UUID, new FDelayAction(10.0f, DelayInfo));
	}
}

void UPDRTSBaseSubsystem::ProcessTables()
{
	ProcessFailCounter++;
	if (WorkTables.IsEmpty())
	{
		const FString BuildString = "UPDRTSBaseSubsystem::ProcessTables -- "
		+ FString::Printf(TEXT("\n 'WorkTables' array is empty. Is not able to process data"));
		UE_LOG(PDLog_RTSBase, Error, TEXT("%s"), *BuildString);
		
		return;
	}
	
	for (const UDataTable* Table : WorkTables)
	{
		if (Table == nullptr
			|| Table->IsValidLowLevelFast() == false
			|| Table->RowStruct != FPDWorkUnitDatum::StaticStruct())
		{
			continue;
		}

		TArray<FPDWorkUnitDatum*> Rows;
		Table->GetAllRows("", Rows);
		
		TArray<FName> RowNames = Table->GetRowNames();
		for (const FName& Name : RowNames)
		{
			const FPDWorkUnitDatum* DefaultDatum = Table->FindRow<FPDWorkUnitDatum>(Name,"");
			check(DefaultDatum != nullptr) // This should never be nullptr

			const FGameplayTag& JobTag = DefaultDatum->JobTag;

			if (JobTag.IsValid() == false)
			{
				const FString BuildString = "UPDRTSBaseSubsystem::ProcessTables -- "
				+ FString::Printf(TEXT("Processing table(%s)"), *Table->GetName()) 
				+ FString::Printf(TEXT("\n Trying to add work/job on row (%s) Which does not have a valid gameplay tag. Skipping processing entry"), *Name.ToString());
				UE_LOG(PDLog_RTSBase, Error, TEXT("%s"), *BuildString);

				// @todo Write some test cases and some data validation to handle this properly, the logs will do for now  
				continue;
			}
			
			// @note If duplicates, ignore duplicate and output errors to screen and to log
			if (TagToJobMap.Contains(JobTag))
			{
				const UDataTable* RetrievedTable = TagToTable.FindRef(JobTag);
				
				const FString BuildString = "UPDInventorySubsystem::ProcessTables -- "
				+ FString::Printf(TEXT("Processing table(%s)"), *Table->GetName()) 
				+ FString::Printf(TEXT("\n Trying to add item(%s) which has already been added by previous table(%s)."),
						*JobTag.GetTagName().ToString(), RetrievedTable != nullptr ? *RetrievedTable->GetName() : *FString("INVALID TABLE"));
				UE_LOG(PDLog_RTSBase, Error, TEXT("%s"), *BuildString);

				// @todo Write some test cases and some data validation to handle this properly, the logs will do for now  
				continue;
			}
			
			TagToJobMap.Emplace(DefaultDatum->JobTag) = DefaultDatum;
			NameToTagMap.Emplace(Name) = JobTag;
			TagToNameMap.Emplace(JobTag) = Name;
			TagToTable.Emplace(JobTag) = Table;
		}
	}
	ProcessFailCounter = 0;
	bHasProcessedTables = true;
}

void UPDRTSBaseSubsystem::RequestNavpathGenerationForSelectionGroup(
	const int32 OwnerID,
	const int32 SelectionGroup,
	const FVector& SelectionCenter,
	const FPDTargetCompound& TargetCompound)
{
	if (SelectionGroup != INDEX_NONE)
	{
		const FVector TargetLocation =
			TargetCompound.ActionTargetAsActor != nullptr ? TargetCompound.ActionTargetAsActor->GetActorLocation()
			: EntityManager->IsEntityValid(TargetCompound.ActionTargetAsEntity) ? EntityManager->GetFragmentDataPtr<FTransformFragment>(TargetCompound.ActionTargetAsEntity)->GetTransform().GetLocation()
			: TargetCompound.ActionTargetAsLocation.Get();

		const UNavigationPath* Navpath = UNavigationSystemV1::FindPathToLocationSynchronously(GetWorld(), SelectionCenter, TargetLocation);
		SelectionGroupNavData.FindOrAdd(OwnerID).SelectionGroupNavData.FindOrAdd(SelectionGroup) = Navpath;
		DirtySharedData.Emplace(OwnerID, SelectionGroup);
		// bGroupPathsDirtied = true;
	}	
}

const FPDWorkUnitDatum* UPDRTSBaseSubsystem::GetWorkEntry(const FGameplayTag& JobTag)
{
	if (bHasProcessedTables == false && ProcessFailCounter < 2) { ProcessTables(); }
	
	return TagToJobMap.Contains(JobTag) ? TagToJobMap.FindRef(JobTag) : nullptr;
}

const FPDWorkUnitDatum* UPDRTSBaseSubsystem::GetWorkEntry(const FName& JobRowName)
{
	if (bHasProcessedTables == false && ProcessFailCounter < 2) { ProcessTables(); }
	
	const FGameplayTag& JobTag = NameToTagMap.Contains(JobRowName) ? NameToTagMap.FindRef(JobRowName) : FGameplayTag::EmptyTag;
	return GetWorkEntry(JobTag);
}

TArray<FMassEntityHandle> UPDRTSBaseSubsystem::FindIdleEntitiesOfType(TArray<FGameplayTag> EligibleEntityTypes, const AActor* ActorToBuild, int32 OwnerID)
{
	TArray<FMassEntityHandle> RetArray{};

	if (ensure(ActorToBuild != nullptr) == false)
	{
		return RetArray;
	}

	const UPDRTSBaseSubsystem* RTSBaseSubsystem = Get();
	const UPDBuilderSubsystem* BuilderSubsystem = UPDBuilderSubsystem::Get();
	const bool bIsBuildableGhost = ActorToBuild->GetClass()->ImplementsInterface(UPDRTSBuildableGhostInterface::StaticClass());
	if (bIsBuildableGhost == false || RTSBaseSubsystem->WorldToEntityHandler.Contains(ActorToBuild->GetWorld()) == false)
	{
		return RetArray;
	}

	// 1. Get cell of related octree
	// const FPDGridCell ActorCell = UPDHashGridSubsystem::GetCellIndexStatic(ActorToBuild->GetActorLocation());
	const FPDGridCell BuildingPadding = FPDGridCell::Construct(0, 0, -1); // @note @todo this is a rickety bridge, the buildingpadding offset only makes sense now because the location is in the middle of hte air, othewrwis 
	const FPDGridCell ActorCell = UPDHashGridSubsystem::StaticCell(ActorToBuild->GetActorLocation(), UPDRTSBaseSubsystem::ResourceGridSize) + BuildingPadding;

	const TDeque<FMassEntityHandle> HandlesCopy = BuilderSubsystem->CopyWorldBuildEntityHashGridHandlesDepthSearch(ActorCell, 2); 
	// 2. Iterate that cells entities, pick max 50 that are idle and eligible
	constexpr int32 MaxPingCount = 50;
	int32 PingCount = 0;

	for (const FMassEntityHandle& EntityHandle : HandlesCopy)
	{
		if (MaxPingCount < PingCount)
		{
			break;
		}

		const UWorld* World = ActorToBuild->GetWorld();
		if (RTSBaseSubsystem->EntityManager->IsEntityValid(EntityHandle) == false
			|| RTSBaseSubsystem->WorldToEntityHandler.Contains(World) == false)
		{
			continue;
		}

		for (const FGameplayTag& EligibleType : EligibleEntityTypes)
		{
			// 3. Only select those of requested type(s)
			const FPDMFragment_RTSEntityBase* EntityBase = RTSBaseSubsystem->EntityManager->GetFragmentDataPtr<FPDMFragment_RTSEntityBase>(EntityHandle);
			if (EntityBase->EntityType != EligibleType) 
			{ 
				continue; 
			}
			
			RetArray.Emplace(EntityHandle);
			PingCount++;
			break;
		}
	}
	return RetArray;
}

UPDRTSBaseSubsystem* UPDRTSBaseSubsystem::Get()
{
	return GEngine->GetEngineSubsystem<UPDRTSBaseSubsystem>();
}

void UPDRTSBaseSubsystem::AssociateArchetypeWithConfigAsset(const FMassArchetypeHandle& Archetype, const TSoftObjectPtr<UMassEntityConfigAsset>& EntityConfig)
{
	ConfigAssociations.FindOrAdd(Archetype) = EntityConfig;
}

TSoftObjectPtr<UMassEntityConfigAsset> UPDRTSBaseSubsystem::GetConfigAssetForArchetype(const FMassArchetypeHandle& Archetype)
{
	return ConfigAssociations.Contains(Archetype) ? *ConfigAssociations.Find(Archetype) : TSoftObjectPtr<UMassEntityConfigAsset>{nullptr};
}

void UPDRTSBaseSubsystem::WorldInit(const UWorld* World)
{
	check(World)
	TemporaryWorldCache = World;
	EntityManager = &World->GetSubsystem<UMassEntitySubsystem>()->GetEntityManager();	
	CreateDataTexture();
}

void UPDRTSBaseSubsystem::WorldDeinit(const UWorld* World)
{
	check(World)
	EntityManager = &World->GetSubsystem<UMassEntitySubsystem>()->GetEntityManager();
	TemporaryWorldCache = nullptr;
	
	DeleteBuffers();
}

void UPDRTSBaseSubsystem::TryRemoveTrackedResourceEntry(const FGameplayTag& ResourceType, const AActor* TrackedActor)
{
	FWriteScopeLock Lock(ResourceRWLock);
	TryRemoveTrackedResourceEntry_Unsafe(ResourceType, TrackedActor);
}
void UPDRTSBaseSubsystem::TryRemoveTrackedResourceEntry_Unsafe(const FGameplayTag& ResourceType, const AActor* TrackedActor)
{
	if (FPDRTSTSetActorWrapper* OldResourceTypeMapping = TrackedResourceGroups.Find(ResourceType)
		; OldResourceTypeMapping != nullptr)
	{
		OldResourceTypeMapping->Actors.Remove(TrackedActor);
		if (OldResourceTypeMapping->Actors.IsEmpty())
		{
			TrackedResourceGroups.Remove(ResourceType);
		}
	}
}
void UPDRTSBaseSubsystem::TryRemoveTrackedCellEntry(FPDGridCell GridCell, const AActor* TrackedActor)
{
	FWriteScopeLock Lock(ResourceRWLock);
	TryRemoveTrackedCellEntry_Unsafe(GridCell, TrackedActor);
}
void UPDRTSBaseSubsystem::TryRemoveTrackedCellEntry_Unsafe(FPDGridCell GridCell, const AActor* TrackedActor)
{
	if (FPDRTSTSetActorWrapper* OldResourceTypeMapping = TrackedResourceGroupsPerGridCell.Find(GridCell)
		; OldResourceTypeMapping != nullptr)
	{
		OldResourceTypeMapping->Actors.Remove(TrackedActor);
		if (OldResourceTypeMapping->Actors.IsEmpty())
		{
			TrackedResourceGroupsPerGridCell.Remove(GridCell);

			for (const FPDGridCell& Neighbour : FindValidNeighours(GridCell))
			{
				TrackedResourceGroupsPerGridCellNeighbours[Neighbour].Remove(GridCell);
			}
			TrackedResourceGroupsPerGridCellNeighbours.Remove(GridCell);
		}
	}
}

void UPDRTSBaseSubsystem::TrackResource(const FGameplayTag& ResourceType, const AActor* TrackedActor)
{
	FWriteScopeLock Lock(ResourceRWLock);
	TrackedResourceGroups.FindOrAdd(ResourceType).Actors.FindOrAdd(TrackedActor);
	
	FPDGridCell ActorGridCell = UPDHashGridSubsystem::StaticCell(TrackedActor->GetActorLocation(), ResourceGridSize);
	TrackedResourceToGridCell.FindOrAdd(TrackedActor) = ActorGridCell;
	TrackedResourceGroupsPerGridCell.FindOrAdd(ActorGridCell).Actors.FindOrAdd(TrackedActor);

	TrackedResourceGroupsPerGridCellNeighbours.FindOrAdd(ActorGridCell) = FindValidNeighours(ActorGridCell);
}
void UPDRTSBaseSubsystem::UntrackResource(const FGameplayTag& ResourceType, const AActor* TrackedActor)
{
	FWriteScopeLock Lock(ResourceRWLock);
	TryRemoveTrackedResourceEntry(ResourceType, TrackedActor);
}

void UPDRTSBaseSubsystem::UntrackAllFromResourceActor(const FGameplayTag& ResourceType, const AActor* TrackedActor)
{
	FWriteScopeLock Lock(ResourceRWLock);
	UntrackAllFromResourceActor_Unsafe(ResourceType, TrackedActor);
}

void UPDRTSBaseSubsystem::UntrackAllFromResourceActor_Unsafe(const FGameplayTag& ResourceType, const AActor* TrackedActor)
{
	FPDGridCell ActorGridCell = UPDHashGridSubsystem::StaticCell(TrackedActor->GetActorLocation(), ResourceGridSize);
	TryRemoveTrackedResourceEntry_Unsafe(ResourceType, TrackedActor);
	TryRemoveTrackedCellEntry_Unsafe(ActorGridCell, TrackedActor);
}

void UPDRTSBaseSubsystem::UpdateResources(const AActor* TrackedActor)
{
	FWriteScopeLock Lock(ResourceRWLock);
	FPDGridCell* OldGridCellPtr = TrackedResourceToGridCell.Find(TrackedActor);
	if (OldGridCellPtr)
	{
		TryRemoveTrackedCellEntry_Unsafe(*OldGridCellPtr, TrackedActor);
		{
			FPDGridCell NewActorGridCell = UPDHashGridSubsystem::StaticCell(TrackedActor->GetActorLocation(), ResourceGridSize);
			TrackedResourceGroupsPerGridCell.FindOrAdd(NewActorGridCell).Actors.Add(TrackedActor);
			TrackedResourceGroupsPerGridCellNeighbours.FindOrAdd(NewActorGridCell) = FindValidNeighours(NewActorGridCell);
		}		
	}
}


void UPDRTSBaseSubsystem::AddEntityResourceTarget(FMassEntityHandle MassEntity, const FRTSEntityResourceGatherTarget& ResourceTarget)
{
	FWriteScopeLock Lock(EntityResourceTaskLock);
	EntitiesCurrentResourceTargets.FindOrAdd(MassEntity).Targets.EmplaceLast(ResourceTarget);
}
FPDRTSTGatherTargetsWrapper* UPDRTSBaseSubsystem::GetEntityResourceTargets(FMassEntityHandle MassEntity)
{
	FReadScopeLock Lock(EntityResourceTaskLock);
	return EntitiesCurrentResourceTargets.Find(MassEntity);
	
}
void UPDRTSBaseSubsystem::RemoveEntityResourceTarget(FMassEntityHandle MassEntity)
{
	FWriteScopeLock Lock(EntityResourceTaskLock);
	EntitiesCurrentResourceTargets.Remove(MassEntity);

}
void UPDRTSBaseSubsystem::RemoveAllFEntityResourceTargets()
{
	FWriteScopeLock Lock(EntityResourceTaskLock);
	EntitiesCurrentResourceTargets.Empty();
}


const FPDRTSTSetActorWrapper* UPDRTSBaseSubsystem::GetResourceActors(const FGameplayTag& ResourceType)
{
	FReadScopeLock Lock(ResourceRWLock);
	const FPDRTSTSetActorWrapper* FoundEntry = TrackedResourceGroups.Find(ResourceType);
	return FoundEntry;
}
const FPDRTSTSetActorWrapper* UPDRTSBaseSubsystem::GetResourceActorsAtGridCell(FPDGridCell GridCell)
{
	FReadScopeLock Lock(ResourceRWLock);
	return GetResourceActorsAtGridCell_Unsafe(GridCell);
}

const FPDRTSTSetActorWrapper* UPDRTSBaseSubsystem::GetResourceActorsAtGridCell_Unsafe(FPDGridCell GridCell)
{
	const FPDRTSTSetActorWrapper* FoundEntry = TrackedResourceGroupsPerGridCell.Find(GridCell);
	return FoundEntry;
}

TSet<const AActor*> UPDRTSBaseSubsystem::GetResourceActorsAtGridCellWithResourceType(const FRTSOFindResourcesParameters& SearchParams)
{
	TSet<const AActor*> Intersection;
	FReadScopeLock Lock(ResourceRWLock);
	const FPDRTSTSetActorWrapper* FoundResourceMappedEntry = TrackedResourceGroups.Find(SearchParams.ResourceType);
	const FPDRTSTSetActorWrapper* FoundGridCellMappedEntry = TrackedResourceGroupsPerGridCell.Find(SearchParams.GridCell);
	if (FoundResourceMappedEntry && FoundGridCellMappedEntry)
	{
		Intersection = FoundResourceMappedEntry->Actors.Intersect(FoundGridCellMappedEntry->Actors);
	}
	
	return Intersection;
}

TSet<const AActor*> UPDRTSBaseSubsystem::GetResourceActorsNearGridCellWithResourceType(const FRTSOFindResourcesParameters& SearchParams)
{
	UE_LOG(LogTemp, Warning, TEXT("UPDRTSBaseSubsystem::NearGridCellWithResource"));

	if (SearchParams.SearchDepth <= 0)
	{
		return GetResourceActorsAtGridCellWithResourceType(SearchParams);
	}
	

	static constexpr int32 MaxIntersectionElement = 40000; 
	TSet<const AActor*> Intersection;
	Intersection.Reserve(MaxIntersectionElement);

	FReadScopeLock Lock(ResourceRWLock);
	const FPDRTSTSetActorWrapper* FoundResourceMappedEntry = TrackedResourceGroups.Find(SearchParams.ResourceType);
	const FPDRTSTSetActorWrapper* FoundGridCellMappedEntry = TrackedResourceGroupsPerGridCell.Find(SearchParams.GridCell);
	if (FoundResourceMappedEntry && FoundGridCellMappedEntry)
	{
		Intersection.Append(FoundResourceMappedEntry->Actors.Intersect(FoundGridCellMappedEntry->Actors));
	}

	UE_LOG(LogTemp, Warning, TEXT("======== Intersection size(%i):"), Intersection.Num());
	for (const AActor* ResourceTarget : Intersection)
	{
		UE_LOG(LogTemp, Warning, TEXT("================= %s : %i"), ResourceTarget ? *ResourceTarget->GetName() : *FString("INVALID OBJECT") );
	}

	if (false == TrackedResourceGroupsPerGridCellNeighbours.Contains(SearchParams.GridCell))
	{
		return Intersection;
	}

	//
	// crude depth search
	TArray<FPDGridCell> Neighbours = TrackedResourceGroupsPerGridCellNeighbours[SearchParams.GridCell];
	TSet<const AActor*> SearchSet = FPDEntityStatics::CrudeDepthSearch<TSet<const AActor*>>(Neighbours, TrackedResourceGroupsPerGridCellNeighbours, TrackedResourceGroupsPerGridCell, SearchParams.SearchDepth, FoundResourceMappedEntry ? FoundResourceMappedEntry->Actors : TSet<const AActor*>{});
	SearchSet.Append(Intersection);

	return SearchSet;
}


void UPDRTSBaseSubsystem::ProcessResourceActors(FSimpleDelegate ProcessDelegate)
{
	FReadScopeLock Lock(ResourceRWLock);

}



void UPDRTSBaseSubsystem::OnDeveloperSettingsChanged(UObject* SettingsToChange, FPropertyChangedEvent& PropertyEvent)
{
	UPDRTSSubsystemSettings* AsSettings = Cast<UPDRTSSubsystemSettings>(SettingsToChange);
	if (AsSettings)
	{
		ResourceGridSize = AsSettings->ResourceGridSize;
	}

	const FArrayProperty* ArrayProperty = CastField<FArrayProperty>(PropertyEvent.Property);
	if (ArrayProperty == nullptr) { return; }

	const FObjectProperty* ObjectProperty = CastField<FObjectProperty>(ArrayProperty->Inner);
	if(ObjectProperty == nullptr) { return; }
	
	if(ObjectProperty->PropertyClass != UDataTable::StaticClass()) { return; }

	WorkTables.Empty(); // clear array and refill with edited properties.
	for(const TSoftObjectPtr<UDataTable>& TablePath : AsSettings->WorkTables)
	{
		UDataTable* ResolvedTable = TablePath.LoadSynchronous();
		WorkTables.Emplace(ResolvedTable);
		
		ResolvedTable->OnDataTableChanged().AddLambda([&]() { ProcessTables(); });
	}

	ProcessTables();
}


// Unseemly matters, avert your gaze
using MassVisRepType = TAccessorTypeHandler<UMassRepresentationSubsystem, TObjectPtr<AMassVisualizer>>; 
template struct TTagPrivateMember<MassVisRepType, &UMassCrowdRepresentationSubsystem::Visualizer>;

using VisualInfoTag = TAccessorTypeHandler<FMassInstancedStaticMeshInfoArrayView, TArrayView<FMassInstancedStaticMeshInfo>>; 
template struct TTagPrivateMember<VisualInfoTag, &FMassInstancedStaticMeshInfoArrayView::InstancedStaticMeshInfos>;

using ISMTag = TAccessorTypeHandler<FMassInstancedStaticMeshInfo, TArray<TObjectPtr<UInstancedStaticMeshComponent>>>; 
template struct TTagPrivateMember<ISMTag, &FMassInstancedStaticMeshInfo::InstancedStaticMeshComponents>;

const TArray<TObjectPtr<UInstancedStaticMeshComponent>> FailDummy{};
const TArray<TObjectPtr<UInstancedStaticMeshComponent>>& UPDRTSBaseSubsystem::GetMassISMs(const UWorld* InWorld)
{
	const UMassCrowdRepresentationSubsystem* RepresentationSubsystem = UWorld::GetSubsystem<UMassCrowdRepresentationSubsystem>(InWorld);
	if (RepresentationSubsystem == nullptr)
	{
		UE_LOG(PDLog_RTSBase, Warning, TEXT("GetMassISMs Fail - No RepSubsystem"))
		return FailDummy;
	}

	const AMassVisualizer* MassVisualizer = (*RepresentationSubsystem).*TPrivateAccessor<MassVisRepType>::TypeValue;
	if (MassVisualizer == nullptr)
	{
		UE_LOG(PDLog_RTSBase, Warning, TEXT("GetMassISMs Fail - No MassVisualizer"))
		return FailDummy;
	}
	
	UMassVisualizationComponent& MassVisualization = MassVisualizer->GetVisualizationComponent();
	const FMassInstancedStaticMeshInfoArrayView VisualInfoView = MassVisualization.GetMutableVisualInfos();
	const TArrayView<FMassInstancedStaticMeshInfo>& InstancedStaticMeshInfos = VisualInfoView.*TPrivateAccessor<VisualInfoTag>::TypeValue;
	for (FMassInstancedStaticMeshInfo& MeshInfo : InstancedStaticMeshInfos)
	{
		return MeshInfo.*TPrivateAccessor<ISMTag>::TypeValue;
	}
	return FailDummy;
}

void UPDRTSBaseSubsystem::GenerateEntityMapData()
{
	UpdateDataTexture();
}

void UPDRTSBaseSubsystem::CreateDataTexture()
{
	const UPDRTSSubsystemSettings* RTSSettings = GetDefault<UPDRTSSubsystemSettings>();
	const bool bDataTextureAssigned = !RTSSettings->EntityDataTexture.IsNull(); 
	const bool bDataTextureLoaded = EntityDataTexture != nullptr && EntityDataTexture->IsValidLowLevelFast();
	EntityDataTexture = bDataTextureLoaded
		? EntityDataTexture
		: bDataTextureAssigned 
			? RTSSettings->EntityDataTexture.LoadSynchronous() 
			: NewObject<UTextureRenderTarget2D>(this);

	EntityDataTexture->bCanCreateUAV = true;

	const bool bNewlyCreatedRT = bDataTextureAssigned == false && bDataTextureLoaded == false;
	if (bNewlyCreatedRT)
	{
		EntityDataTexture->InitAutoFormat(GMaxEntityDim, GMaxEntityDim);		
		EntityDataTexture->CompressionSettings = TC_HDR_F32;
		EntityDataTexture->SRGB = false;
		EntityDataTexture->Filter = TF_Nearest; 
#if WITH_EDITORONLY_DATA
		EntityDataTexture->MipGenSettings = TMGS_NoMipmaps;
#endif
	}

	AsyncTask(ENamedThreads::GameThread, [&]()
	{
		EntityDataTexture->UpdateResource(); // recreate the RHI texture, bCanCreateUAV can thus apply to loaded assets too

		CreateDataBuffer();
	});
}

void UPDRTSBaseSubsystem::CreateDataBuffer()
{	
	ENQUEUE_RENDER_COMMAND(CreateDataBufferCommandName)
	(
		[this](FRHICommandListImmediate& RHICmdList)
		{
			uint32 ElementSize = sizeof(FLinearColor); 
			uint32 TotalSize = GMaxEntityDataSize * ElementSize;
			if (false == EntityInputPooledBuffer.IsValid())
			{
				// FRDGBufferDesc BufferDescriptor = FRDGBufferDesc::CreateBufferDesc(ElementSize, GMaxEntityDataSize);
				FRDGBufferDesc BufferDescriptor = FRDGBufferDesc::CreateStructuredDesc(ElementSize, GMaxEntityDataSize);
				EntityInputPooledBuffer = AllocatePooledBuffer(BufferDescriptor, TEXT("SortDataBuffer"), ERDGPooledBufferAlignment::PowerOfTwo);
			}
			
			bHasCreatedPooledBuffers = true;
		}
	);
}

static TAutoConsoleVariable<int32> CVarRTSMinimapSplat(
	TEXT("rts.MinimapSplat"), 1, 
	TEXT("0 disables the minimap entity compute pass (crash isolation)."));

void UPDRTSBaseSubsystem::UpdateDataTexture()
{
	if (false == bHasCreatedPooledBuffers || CVarRTSMinimapSplat.GetValueOnAnyThread() == 0)
	{
		// Potential TODO: Log error? Assert?
		 return; 
	}
	
	UTextureRenderTarget2D* RenderTargetParam = EntityDataTexture;
	TRefCountPtr<FRDGPooledBuffer> EntityInputPooledBufferCopy = EntityInputPooledBuffer;
	FRTSBuildGlobalSortEntityShader BuildEntitySortComputeShaderCopy = BuildEntitySortComputeShader;

	TArray<FLinearColor> EntitiesInBound;
	OctreeUserQuery.MemCpyQueryBuffer<EPDQueryGroups::QUERY_GROUP_MINIMAP>(EntitiesInBound);
	FPDOctreeUserQuery::FBoundsSimple QueryBounds = OctreeUserQuery.GetLatestQueryBounds<EPDQueryGroups::QUERY_GROUP_MINIMAP>();
	

#define DEBUG false
#if DEBUG

	UE_LOG(PDLog_RTSBase, Warning, TEXT("==================== UPDRTSBaseSubsystem::UpdateDataTexture ===================="))
	int32 EntStep = 0;
	for (const FLinearColor& EntityDatum : EntitiesInBound)
	{
		FVector2D EntityWorldPos = FVector2D{EntityDatum.R, EntityDatum.G};
		FVector2D PixelPos = (EntityWorldPos - FVector2D(QueryBounds.Min)) / FVector2D(QueryBounds.Size) * 255.0;


		float CameraYawLookDirection = UPDRTSBaseSubsystem::GetUserRotationCurrentTick();
		FVector2D Centered = PixelPos - FVector2D(128.0, 128.0);
		FVector2D RotatedCoord;
		RotatedCoord.X = (Centered.X * cos(CameraYawLookDirection)) - (Centered.Y * sin(CameraYawLookDirection));
		RotatedCoord.Y = (Centered.X * sin(CameraYawLookDirection)) + (Centered.Y * cos(CameraYawLookDirection));
		RotatedCoord += FVector2D(128.0, 128.0);
		

		UE_LOG(PDLog_RTSBase, Warning, TEXT("=== Entity(%i)"), EntStep)
		UE_LOG(PDLog_RTSBase, Warning, TEXT("======= Mapped PixelPos(%f, %f)"), PixelPos.X, PixelPos.Y)
		UE_LOG(PDLog_RTSBase, Warning, TEXT("======= Rotated Mapped PixelPos(%f, %f)"), RotatedCoord.X, RotatedCoord.Y)
		UE_LOG(PDLog_RTSBase, Warning, TEXT("======= Yaw look Direction: %f PI"), CameraYawLookDirection / PI)
	
	}
	UE_LOG(PDLog_RTSBase, Warning, TEXT("========================================"))
#endif


	ENQUEUE_RENDER_COMMAND(UpdateDataTextureCommandName)(
	[
		BuildEntitySortComputeShaderCopy,
		RenderTargetParam,
		EntityInputPooledBufferCopy,
		EntitiesInBound,
		QueryBounds
	](FRHICommandListImmediate& RHICmdList)
	{
		BuildEntitySortComputeShaderCopy.ExecuteIfBound(
			RHICmdList,
			RenderTargetParam,
			EntityInputPooledBufferCopy,
			EntitiesInBound,
			UPDRTSBaseSubsystem::GetUserRotationCurrentTick(),
			QueryBounds.Min,
			QueryBounds.Size
		);
	});
}


void UPDRTSBaseSubsystem::DeleteBuffers()
{
	ENQUEUE_RENDER_COMMAND(DeleteBuffersCommandName)
	(
		[this](FRHICommandListImmediate& RHICmdList)
		{			
			bHasCreatedPooledBuffers = false;
			if (EntityInputPooledBuffer.IsValid()) {EntityInputPooledBuffer.SafeRelease();}
		}	
	);
}

// Tickable interface
void UPDRTSBaseSubsystem::Tick(float DeltaTime) 
{
	// Resered for potential future use
}

TStatId UPDRTSBaseSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UPDRTSBaseSubsystem, STATGROUP_Tickables);
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
