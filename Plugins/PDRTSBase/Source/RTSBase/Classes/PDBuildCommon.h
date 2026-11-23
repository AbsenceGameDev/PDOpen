/* @author: Ario Amin @ Permafrost Development. @copyright: Full BSL(1.1) License included at bottom of the file  */

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "MassEntityConfigAsset.h"
#include "MassEntityTypes.h"
#include "NativeGameplayTags.h"
#include "Subsystems/EngineSubsystem.h"
#include "Containers/Deque.h"

#include "PDBuildCommon.generated.h"

class UNiagaraSystem;
DECLARE_LOG_CATEGORY_CLASS(PDLog_BuildSystem, Log, All);

/** Declaring the "AI.Type." gameplay tags. to be defined in an object-file */
PDRTSBASE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_AI_Type);
PDRTSBASE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_AI_Type_DefaultUnit);
PDRTSBASE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_AI_Type_InvalidUnit);

PDRTSBASE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_AI_Type_BuilderUnit_Novice);
PDRTSBASE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_AI_Type_BuilderUnit_Intermeditate);
PDRTSBASE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_AI_Type_BuilderUnit_Advanced);
PDRTSBASE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_AI_Type_BuilderUnit_Expert);

PDRTSBASE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_AI_Type_SoldierUnit_Novice);
PDRTSBASE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_AI_Type_SoldierUnit_Intermeditate);
PDRTSBASE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_AI_Type_SoldierUnit_Advanced);
PDRTSBASE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_AI_Type_SoldierUnit_Expert);

/** Declaring the "BUILD.ContextMenu." gameplay tags. to be defined in an object-file */
PDRTSBASE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_BUILD_ContextMenu_DefaultWorker);

PDRTSBASE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_BUILD_ContextMenu_Builder_Novice);
PDRTSBASE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_BUILD_ContextMenu_Builder_Intermediate);
PDRTSBASE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_BUILD_ContextMenu_Builder_Advanced);
PDRTSBASE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_BUILD_ContextMenu_Builder_Expert);

/** Declaring the "BUILD.Actions." gameplay tags. to be defined in an object-file */
PDRTSBASE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_BUILD_Actions_DestroyBuilding);
PDRTSBASE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_MSG_Actions_CannotAffordAction);
PDRTSBASE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_MSG_Actions_CannotPlaceBuilding_Encroachment);

// TAG_AI_Type_ tags are implicitly counted as spawn actions when applied as a 'buildable action tag' in an entry in a 'FPDBuildable'-type of datatable

/** Declaring the "BUILD.ActionContext." gameplay tags. to be defined in an object-file */
PDRTSBASE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_BUILD_ActionContext_Base0);
PDRTSBASE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_BUILD_ActionContext_Base1);
PDRTSBASE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_BUILD_ActionContext_WorkerHut0);
PDRTSBASE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_BUILD_ActionContext_WorkerHut1);
PDRTSBASE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_BUILD_ActionContext_Barracks0);
PDRTSBASE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_BUILD_ActionContext_Barracks1);
PDRTSBASE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_BUILD_ActionContext_Storage0);

/** @brief Default Build messages  */
UCLASS(Config = "Game", DefaultConfig)
class PDRTSBASE_API UPDBuildMessages : public UDeveloperSettings
{
	GENERATED_BODY()
public:
	// todo replace with string table
	UPROPERTY()
	FText ConfirmDestroyBuilding = FText::FromString("Are you sure you want to destroy this building?");
};


/** @brief Behaviours for how builds should be handled.
 * Should builds A. Start right away, B. wait for workers or C. should it call for workers,
 * the former will wait for player to dispatch builders, the latter will use the ping system attempt finding some themselves
 */
UENUM()
enum class PDBuildBehaviourProgress
{
	EImmediate      UMETA(DisplayName="BuildBehaviour::Progress::Immediate"),
	EStandBy        UMETA(DisplayName="BuildBehaviour::Progress::StandBy"),
	ECallForWorkers UMETA(DisplayName="BuildBehaviour::Progress::PingBuilders"),
};

/** @brief Behaviours for how build costs should work.
 * Should builds
 * A. Withdraw from player bank,
 * B. Withdraw from associate base's bank 
 * C. wait for workers to bring resources
 * D. be completely free, skirting past any resource requirements set for the buildable?
 */
UENUM()
enum class PDBuildBehaviourCost
{
	EPlayerBank UMETA(DisplayName="PD::Build::Behaviour::Cost::PlayerBank"),
	EBaseBank   UMETA(DisplayName="PD::Build::Behaviour::Cost::BaseBank"),  // may differ from player bank
	EStandBy    UMETA(DisplayName="PD::Build::Behaviour::Cost::StandBy"),
	EFree       UMETA(DisplayName="PD::Build::Behaviour::Cost::Free"),
};	

/** @brief  Namespace aliasing */
namespace PD::Build::Behaviour
{
	using Progress = PDBuildBehaviourProgress;
	using Cost     = PDBuildBehaviourCost;
	
}

/** @brief Ghost transition enum state */
UENUM(Blueprintable)
enum EPDRTSGhostTransition
{
	ESingleStage UMETA(DisplayName="SingleStage"),
	EMultipleStages UMETA(DisplayName="MultipleStages"),
};

/** @brief Ghost transition behaviour enum selector */
UENUM(Blueprintable)
enum class EPDRTSGhostStageBehaviour : uint8
{
	EOnStart UMETA(DisplayName="OnStart"),
	EOnEnd UMETA(DisplayName="OnEnd"),
};

/** @brief Namespace aliasing */
namespace PD::Build::Ghost::Behaviour
{
	using Transition = EPDRTSGhostTransition;
	using Stage      = EPDRTSGhostStageBehaviour;
}

/** @brief Structure that is used to pass data to the build or ping subsystem,
 * keeps an owner ID (actors persistent ID) and a pointer to that owners owned world actors (buildings)  */
struct FPDActorCompound
{
	int32 OwnerID = INDEX_NONE;
	TArray<AActor*>* WorldActorsPtr;
	bool operator==(const FPDActorCompound& Other) const
	{
		return
			this->WorldActorsPtr    != nullptr
			&& Other.WorldActorsPtr != nullptr
			&& this->OwnerID == Other.OwnerID;
	}
};

/** @brief Build queue for builds without build-stages, used when there isn¨t enough resource to buy a buildable when placing it */
USTRUCT()
struct FPDBuildQueue_WNoStageCosts
{
	GENERATED_BODY()
	
	/** @brief Actors queued to be built, checked when resources update to see if it can be afforded.
	 *  @note This will continue until the owner can afford teh build or manually removed it from the queue */
	TDeque<AActor*> ActorInWorld;
};

//
// Ghosts
/** @brief Settings for a 'ghost-able' actor */
USTRUCT(Blueprintable)
struct PDRTSBASE_API FPDRTSBuildableCollisionSettings
{
	GENERATED_BODY()

	FPDRTSBuildableCollisionSettings() = default;
	explicit FPDRTSBuildableCollisionSettings(bool bInIsSet) : bIsSet(bInIsSet) {};
	
	/** @brief Is the actor collision enabled on the owning actor, for this context  */
	UPROPERTY()
	bool bIsActorCollisionEnabled = true;

	/** @brief Map of collision enable settings, keyed by (component) name  */
	UPROPERTY()
	TMap<FName, TEnumAsByte<ECollisionEnabled::Type>>  ComponentCollisionEnabledSettings;

	/** @brief  helps us control when we want to apply the collision settings above */
	bool bIsSet = false;
};

/** @brief Data-assets for the visual effects of different build stages */
UCLASS(Blueprintable)
class UPDRTSGhostStageAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	/** @brief Mesh that we should be apply this stage */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Actor|Ghost|Settings")
	UStaticMesh* StageGhostMesh = nullptr;

	/** @brief Effects that we should be apply this stage */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Actor|Ghost|Settings")
	UNiagaraSystem* StageGhostNiagaraSystem = nullptr;

	/** @brief Set if we should loop niagara effects until stage is finished */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Actor|Ghost|Settings")
	bool bLoopNiagara = false;		
};

/** @brief Per-Stage settings for a ghost */
USTRUCT(Blueprintable)
struct PDRTSBASE_API FPDRTSGhostStageData
{
	GENERATED_BODY()

public:
	/** @brief The mesh behaviour we are expecting for this stage, (OnStartStage vs. OnEndStage) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Actor|Ghost|Settings")
	EPDRTSGhostStageBehaviour StageMesh_ApplyBehaviour = EPDRTSGhostStageBehaviour::EOnStart;	
	/** @brief The vfx behaviour we are expecting for this stage, (OnStartStage vs. OnEndStage) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Actor|Ghost|Settings")
	EPDRTSGhostStageBehaviour StageVFX_ApplyBehaviour = EPDRTSGhostStageBehaviour::EOnStart;	
	
	/** @brief The data-asset for the current stage */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Actor|Ghost|Settings")
	class UPDRTSGhostStageAsset* StageDA = nullptr;

	/** @brief Potential max duration for a stage, this defines the space between the start stage effects and end stage effects
	 * @note If set to -1.0, this will not wait for end of stage. if set to 0.0, then this will rely on manual end of stage, this will in many cases be when a resource, or tag, quota has been met */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Actor|Ghost|Settings")
	double MaxDurationForStage = 0.0; 
};

/** @brief Stage(s) configuration for a ghost */
USTRUCT(Blueprintable)
struct FPDRTSGhostDatum
{
	GENERATED_BODY()
	
	FPDRTSGhostDatum() = default;
	
	/** @rief Will be all stages (and in order) if TransitionStageType == EPDRTSGhostTransition::EMultipleStages, can be left empty for no transition effect */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Actor|Ghost|Settings")
	TArray<FPDRTSGhostStageData> StageAssets {FPDRTSGhostStageData{}};	
};

/** @brief Ghost stat for a buildable, currently only contains the ghost stage */
USTRUCT(Blueprintable)
struct FPDRTSGhostBuildState
{
	GENERATED_BODY()

	/** @rief Will not be used in the datatable, but each buildable with a ghost should have a 'FPDRTSGhostDatum' which reflects   */
	UPROPERTY(BlueprintReadWrite, Category = "Actor|Ghost|Settings")
	int32 CurrentStageIdx = 0;	
};

/** @brief Should Move camera or not upon placing a ghost ? */
UENUM(Blueprintable)
enum class EPDRTSBuildCameraBehaviour : uint8
{
	Placement_SmoothInterp, /** @brief Immediate Ghost;  If no resources, put in waiting queue; when resources: start build automatically without workers needing to gather any resourced there */
	Place_NoCameraMovement,   /** @brief Processing: If no resources, put in waiting queue; when resources: send workers to gather to location */
};

/** @brief  Namespace aliasing */
namespace PD::Build::Behaviour
{
	using Camera = EPDRTSBuildCameraBehaviour;
}

/** @brief Building destruction behaviour, for when players "bulldoze" their own buildings  */
UENUM(Blueprintable)
enum class EPDRTSDestroyBuildingBehaviour : uint8
{
	RequireConfirmationDialog, /** @brief Require a confirmation dialog before destroying the buildable */
	ImmediateDestruction,   /** @brief Destroy immediately, for snappy feeling and in games where dialogs really mess up the flow of the game */
};

/** @brief Namespace Aliasing */
namespace PD::Build::Behaviour
{
	using Destruction = EPDRTSDestroyBuildingBehaviour;
}


//
// Buildables
/** @brief Buildable data-asset, currently only keeps our widget related data in it  */
UCLASS(Blueprintable)
class PDRTSBASE_API UPDBuildableDataAsset : public UDataAsset
{
	GENERATED_BODY()
public:
	UPDBuildableDataAsset() = default;

	/** @brief  Texture resource to apply on a buildable button (Does not take priority) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UTexture2D* Buildable_Texture = nullptr;

	/** @brief  Material resource to apply on a buildable button (Takes priority) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UMaterialInstance* Buildable_MaterialInstance = nullptr;
	
	/** @brief  Audio resource to play when clicking a menu-button */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USoundBase* Buildable_AudioLoop = nullptr;
};

/** @brief Buildable context data-asset, currently only keeps our widget related data in it  */
UCLASS(Blueprintable)
class PDRTSBASE_API UPDBuildContextDataAsset : public UDataAsset
{
	GENERATED_BODY()
public:
	/** @brief  Texture resource to apply on a buildable button (Does not take priority) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UTexture2D* BuildContext_Texture = nullptr;

	/** @brief  Material resource to apply on a buildable button (Takes priority) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UMaterialInstance* BuildContext_MaterialInstance = nullptr;
};

/** @brief Main Buildable data, no tag associated  */
USTRUCT(Blueprintable)
struct PDRTSBASE_API FPDBuildableData
{
	GENERATED_BODY()

	/** @brief Not needed, but use to display clearer name, with being set to nothing the name will be tha buildables tag converted to a name */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTSBase|WorkerUnits")
	FText ReadableName{};

	/** @brief Actor class to spawn when placing the buildable */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTSBase|WorkerUnits")
	TSubclassOf<AActor> ActorToSpawn{};
	
	/** @brief The buildables data-asset, if not spawned as ghost, or when transitioning from ghost to real building */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTSBase|WorkerUnits")
	class UPDBuildableDataAsset* DABuildAsset = nullptr;

	/** @brief The buildables ghost configurations, keeps stage settings and such  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTSBase|BuildSystem")
	FPDRTSGhostDatum GhostData{};
};

/** @brief Non-critical data related to a build-menu context */
USTRUCT(Blueprintable)
struct PDRTSBASE_API FPDBuildContextData
{
	GENERATED_BODY()

	/** @brief Not needed, but use to display clearer name, with being set to nothing the name will be tha buildables tag converted to a name */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTSBase|BuildSystem")
	FText ReadableName{};
	
	/** @brief The build-context data-asset, only used for widget flair as of now  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTSBase|BuildSystem")
	UPDBuildContextDataAsset* DABuildContextAsset = nullptr;
};

/** @brief Critical data related to a buildables inner actions (For example: Spawning units, upgrading self destruction) */
USTRUCT(Blueprintable)
struct PDRTSBASE_API FPDBuildAction : public FTableRowBase
{
	GENERATED_BODY()

	FPDBuildAction() = default;
	
	/** @brief The tag of this action */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTSBase|WorkerUnits")
	FGameplayTag ActionTag{};	

	/** @brief The cost of this action, defaults to free */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTSBase|WorkerUnits")
	TMap<FGameplayTag, int32> ActionCost{};
	
	/** @brief Not needed, but use to display clearer name, with being set to nothing the name will be tha buildables tag converted to a name */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTSBase|WorkerUnits")
	FText ReadableName{};
	
	/** @brief The buildables data-asset, if not spawned as ghost, or when transitioning from ghost to real building */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTSBase|WorkerUnits")
	UPDBuildableDataAsset* DABuildAsset = nullptr;
};

/** @brief Critical data related to a buildables action context (Action contexts are grouped actions) */
USTRUCT(Blueprintable)
struct PDRTSBASE_API FPDBuildActionContext : public FTableRowBase
{
	GENERATED_BODY()

	FPDBuildActionContext() = default;
	
	/** @brief The tag of this action */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTSBase|WorkerUnits")
	FGameplayTag ContextTag{};	

	/** @brief The resource data of buildables this context provides */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTSBase|WorkerUnits", Meta = (RowType = "/Script/PDRTSBase.PDBuildAction"))
	TArray<FDataTableRowHandle> ActionData{};
	
	/** @brief The readable name of this buildable context, todo, remove, already exists within 'ContextData'  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTSBase|WorkerUnits")
	FPDBuildContextData ContextData{};
};


/** @brief Actual Buildable row structure, associates tag and it's data with a buildable  */
USTRUCT(Blueprintable)
struct PDRTSBASE_API FPDBuildable : public FTableRowBase
{
	GENERATED_BODY()

	FPDBuildable() = default;
	
	/** @brief The tag of this buildable */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTSBase|BuildSystem")
	FGameplayTag BuildableTag{};	

	/** @brief The data of this buildable */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTSBase|BuildSystem")
	FPDBuildableData BuildableData{};

	/** @brief The actions granted to this buildable when it has been built */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTSBase|BuildSystem", Meta = (RowType = "/Script/PDRTSBase.PDBuildActionContext"))
	TArray<FDataTableRowHandle> ActionContextHandles{};	
};

/** @rief Actual Build-context row structure, associates tag and it's data with a buildable  */
USTRUCT(Blueprintable)
struct PDRTSBASE_API FPDBuildContext : public FTableRowBase
{
	GENERATED_BODY()

	FPDBuildContext() = default;

	/** @brief The tags of this buildable context */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTSBase|WorkerUnits")
	FGameplayTag ContextTag{};	

	/** @brief The readable name of this buildable context, todo, remove, already exists within 'ContextData'  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTSBase|WorkerUnits")
	FText ContextReadableName{};	
	
	/** @brief The resource data of buildables this context provides */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTSBase|WorkerUnits", Meta = (RowType = "/Script/PDRTSBase.PDBuildable"))
	TArray<FDataTableRowHandle> BuildablesData{};

	/** @brief The resource data of this actual context */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTSBase|WorkerUnits")
	FPDBuildContextData ContextData{};
};

/** @rief Shared widget flair table row, for setting not specific to a given buildable or build-context but are instead applied as a whole  */
USTRUCT()
struct PDRTSBASE_API FPDSharedBuildWidgetFlair : public FTableRowBase
{
	GENERATED_BODY()

	FPDSharedBuildWidgetFlair() = default;
	
	/** @brief Tint when context is selected */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	
	FLinearColor SelectedContextTint = FLinearColor::Gray; 
	/** @brief Tint when context is not selected */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	
	FLinearColor NotSelectedContextTint = FLinearColor::Blue;
	
	/** @brief Tint when a buildable is selected */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	
	FLinearColor SelectedBuildableTint = FLinearColor::Gray; 
	/** @brief Tint when a buildable is not selected */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)	
	FLinearColor NotSelectedBuildableTint = FLinearColor::Blue;	
};

/** @brief @note Will potentially be deprecated if I can't justify using it */
UCLASS(Blueprintable)
class PDRTSBASE_API UPDRTSUnitTypeDataAsset : public UDataAsset
{
	GENERATED_BODY()
public:
	UPDRTSUnitTypeDataAsset() = default;

	/** @brief The entity config we want to load this unit with */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTSBase|WorkerUnits")
	const TSoftObjectPtr<UMassEntityConfigAsset> EntityConfig = {};
};

/** @brief Worker builder definition table row, Defines what entity types are granted which build-contexts
 * @todo rename to FPDEntityUnit or something similar */
USTRUCT(Blueprintable)
struct PDRTSBASE_API FPDBuildWorker : public FTableRowBase
{
	GENERATED_BODY()

	FPDBuildWorker() = default;

	/** @brief The worker that this entry defines */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTSBase|WorkerUnits")
	FGameplayTag WorkerType{};
	
	/** @brief The buildable contexts granted to this worked */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTSBase|WorkerUnits", Meta = (RowType = "/Script/PDRTSBase.PDBuildContext"))
	TArray<FDataTableRowHandle> GrantedContexts;

	/** @brief The entity config we want to load this unit with */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTSBase|WorkerUnits")
	const UPDRTSUnitTypeDataAsset* MassEntityData = nullptr;
};


/** @brief Build system behaviour settings
 * @todo @refactor turn into datatable row type and set up a table for it, replace current default entry in developer settings
 */
USTRUCT(Blueprintable)
struct FPDRTSBuildSystemBehaviours
{
	GENERATED_BODY()

	/** @brief BuildBehaviour::Progress */
	UPROPERTY(Config, EditAnywhere, Category="Visible")
	PDBuildBehaviourProgress Progression = PD::Build::Behaviour::Progress::ECallForWorkers;

	/** @brief BuildBehaviour::Cost */
	UPROPERTY(Config, EditAnywhere, Category="Visible")
	PDBuildBehaviourCost Cost = PD::Build::Behaviour::Cost::EFree;

	/** @rief Tells us how we want the camera behaviour to be when placing a ghost  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EPDRTSBuildCameraBehaviour CameraBehaviour = PD::Build::Behaviour::Camera::Place_NoCameraMovement;

	/** @rief Tells us how we want the camera behaviour to be when placing a ghost  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EPDRTSDestroyBuildingBehaviour DestructionBehaviour = PD::Build::Behaviour::Destruction::RequireConfirmationDialog;		
};


/** @brief Subsystem (developer) settings, set the uniform bounds, the default grid cell size and the work tables for the entities to use */
UCLASS(Config = "Game", DefaultConfig)
class PDRTSBASE_API UPDBuilderSubsystemSettings : public UDeveloperSettings
{
	GENERATED_BODY()
public:

	UPDBuilderSubsystemSettings(){}

	/** @brief Build Contexts (categories) table soft objects */
	UPROPERTY(Config, EditAnywhere, Category = "Worker AI Subsystem", Meta = (RequiredAssetDataTags="RowStructure=/Script/PDRTSBase.PDBuildContext"))
	TArray<TSoftObjectPtr<UDataTable>> BuildContextTables;	

	/** @brief Build Workers (Worker types and their granted contexts) table soft objects */
	UPROPERTY(Config, EditAnywhere, Category = "Worker AI Subsystem", Meta = (RequiredAssetDataTags="RowStructure=/Script/PDRTSBase.PDBuildWorker"))
	TArray<TSoftObjectPtr<UDataTable>> BuildWorkerTables;

	/** @brief Build Workers (Worker types and their granted contexts) table soft objects */
	UPROPERTY(Config, EditAnywhere, Category = "Worker AI Subsystem", Meta = (RequiredAssetDataTags="RowStructure=/Script/PDRTSBase.PDBuildActionContext"))
	TArray<TSoftObjectPtr<UDataTable>> BuildActionContextTables;
	
	/** @brief Build Workers (Worker types and their granted contexts) table soft objects */
	UPROPERTY(Config, EditAnywhere, Category="Worker AI Subsystem|Builder|Behaviour")
	FPDRTSBuildSystemBehaviours DefaultBuildSystemBehaviours{};
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