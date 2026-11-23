/* @author: Ario Amin @ Permafrost Development. @copyright: Full BSL(1.1) License included at bottom of the file  */
#pragma once

#include "CoreMinimal.h"
#include "MassLODCollectorProcessor.h"
#include "MassObserverProcessor.h"
#include "MassProcessor.h"
#include "MassRepresentationProcessor.h"
#include "MassVisualizationLODProcessor.h"
#include "MassSignalProcessorBase.h"
#include "PDMassProcessors.generated.h"

struct FMassVelocityFragment;
struct FPDMFragment_RTSEntityBase;
class UMassCrowdRepresentationSubsystem;
struct FMassMoveTargetFragment;
struct FPDMFragment_EntityAnimation;
class UMassSignalSubsystem;
class UNavigationSystemV1;
class UMassEntitySubsystem;

/** @brief Declare processor functions with empty defaults */
#define DECLARE_PROCESSOR_BODY \
virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override; \
virtual void ConfigureQueries() override; \
virtual void Initialize(UObject& Owner) override;

/** @brief Defines processor functions with empty defaults */
#define DEFINE_PROCESSOR_EMPTY_DEFAULTS \
virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override {}; \
virtual void ConfigureQueries() override {}; \
virtual void Initialize(UObject& Owner) override {};

/* @todo remove these, will just be confusing as to what types we are actually handling */
template<typename TFrag>
using TConstFragment = const TConstArrayView<TFrag>;
template<typename TFrag>
using TMutFragment = const TArrayView<TFrag>;
template<typename TFrag>
using TSharedFragment = const TFrag&;
template<typename TFrag>
using TMutSharedFragment = TFrag&;

/**
 * @brief Moves the entities around to a given target, tasks have the responsibility to create actual multi-point paths
 * @note Executes before avoidance
 * @note Calculates shared navpaths for selection groups which have been marked as dirty
 */
UCLASS()
class PDRTSBASE_API UPDProcessor_MoveTarget : public UMassProcessor
{
	GENERATED_BODY()

public:	
	/** @brief Sets execution order: Before Avoidance */
	UPDProcessor_MoveTarget();

	/* Macro helper to declare the required processor functions */
	DECLARE_PROCESSOR_BODY

private:
	/** @brief Processors entity query,
	 *  @requires FTransformFragment, FMassMoveTargetFragment, FMassSimulationVariableTickChunkFragment, FPDMFragment_SharedEntity*/
	FMassEntityQuery EntityQuery;
	
	/** @brief Local Signal subsystem pointer
	 * @note UPDProcessor_MoveTarget::Execute Calls SignalSubsystem->SignalEntities(UE::Mass::Signals::FollowPointPathDone, ..)
	 * when entities has finished moving to their targets */
	TObjectPtr<UMassSignalSubsystem> SignalSubsystem;
};

/**
 * @brief Initializes RTS Entities, currently only sets up possibly shared animation data
 * - The '::Execute' function refreshes the A2T data for the entities
 */
UCLASS()
class PDRTSBASE_API UPDMProcessor_InitializeEntities : public UMassObserverProcessor
{
	GENERATED_BODY()

public:
	UPDMProcessor_InitializeEntities();
	
	/* Macro helper to declare the required processor functions */
	DECLARE_PROCESSOR_BODY

protected:
	/** @brief Processors entity query,
	 *  @requires FPDMFragment_RTSEntityBase, FPDMFragment_SharedAnimData, FPDMTag_RTSEntity, FMassRepresentationFragment, FPDMFragment_EntityAnimation*/
	FMassEntityQuery EntityQuery;
};

/**
 * @brief Setup ISMs, animation, and textures for RTS Entities
 */
UCLASS()
class UPDMProcessor_EntityCosmetics : public UMassProcessor
{
	GENERATED_BODY()

public:
	UPDMProcessor_EntityCosmetics();

	/* Macro helper to declare the required processor functions */
	DECLARE_PROCESSOR_BODY
	
	/** @brief Processes which vertex animation should be used based on movement and/or action */
	static bool ProcessVertexAnimation(
		int32 EntityIdx,
		TConstFragment<FMassRepresentationLODFragment>& RepresentationLODFragments,
		const FMassRepresentationFragment& Rep,
		FPDMFragment_RTSEntityBase* RTSEntityFragment,
		FPDMFragment_EntityAnimation* AnimationData,
		const FMassVelocityFragment& Velocity,
		const FMassInstancedStaticMeshInfoArrayView& MeshInfo,
		const TArrayView<FMassInstancedStaticMeshInfo>& MeshInfoInnerArray,
		const UPDMProcessor_EntityCosmetics* Self);
	/** @brief Process material instance data injection */
	static bool ProcessMaterialInstanceData(
		const FMassEntityHandle& EntityHandle,
		// const FMassRepresentationLODFragment& RepLOD,
		const FMassRepresentationFragment& Rep,
		FMassInstancedStaticMeshInfo& ISMInfo,
		FPDMFragment_RTSEntityBase* RTSEntityFragment);

	/** @brief Dispatch A2T data as batched custom data to the FMassInstancedStaticMeshInfo, passing it along to the ISM */
	void UpdateISMVertexAnimation(FMassInstancedStaticMeshInfo& ISMInfo, FPDMFragment_EntityAnimation& AnimationData,
	                              float LODSignificance, float PrevLODSignificance, int32 NumFloatsToPad) const;

protected:
	/** @brief Processors entity query,
	*  @requires FPDMTag_RTSEntity, FPDMFragment_RTSEntityBase, FPDMFragment_EntityAnimation, FPDMFragment_SharedAnimData,
	*  FMassMoveTargetFragment, FMassRepresentationFragment, FMassRepresentationLODFragment,
	*  FMassVelocityFragment, FTransformFragment, FMassVisualizationChunkFragment */
	FMassEntityQuery EntityQuery;

	/** @brief representation subsystem pointer
	 * @note UPDMProcessor_EntityCosmetics::Execute Calls RepresentationSubsystem to get the mutable InstancedStaticMeshInfo */
	TObjectPtr<UMassCrowdRepresentationSubsystem> RepresentationSubsystem;
};

UCLASS() class PDRTSBASE_API UPDMProcessor_Visualization : public UMassVisualizationProcessor
{ public: GENERATED_BODY() UPDMProcessor_Visualization() { bAutoRegisterWithProcessingPhases = true; } };

/*
 * Reserved for future use for possible crowd behaviour LOD Visualization, does nothing much right now. 
 * - Execution Order: After 'UE::Mass::ProcessorGroupNames::LODCollector'
 * - Execution Group: 'UE::Mass::ProcessorGroupNames::LOD'
 */
UCLASS(Meta=(DisplayName="(Permadev) Crowd visualization"))
class PDRTSBASE_API UPDMProcessor_LODVisualization : public UMassVisualizationLODProcessor
{
	GENERATED_BODY()
public:
	/** @brief Sets to auto-register the processor, set the execution flags and the execution order, */
	UPDMProcessor_LODVisualization();

protected:
	/* Macro helper to declare the required processor functions */
	DECLARE_PROCESSOR_BODY
};

/** @brief Lod collector subclass which override the query configuration function
 * - Execution Order: After 'UE::Mass::ProcessorGroupNames::Movement'
 */
UCLASS(Meta = (DisplayName = "Crowd LOD Collection"))
class PDRTSBASE_API UPDMProcessor_LODCollector : public UMassLODCollectorProcessor
{
	GENERATED_BODY()

	UPDMProcessor_LODCollector() { bAutoRegisterWithProcessingPhases = true; };

protected:
	/** @brief Adds tag requirement for FMassCrowdTag to the base lod collectors entity-query properties */
	virtual void ConfigureQueries() override;
};

/** @brief Octree cell processing / entity tracking
 * - ExecutionOrder: After 'UE::Mass::ProcessorGroupNames::Movement'
 */
UCLASS()
class PDRTSBASE_API UPDOctreeProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	/** @brief Sets execution order and execution flags */
	UPDOctreeProcessor();
	/** @brief Draws all octree cells using async draw via Chaos.
	 * Attempts to force enable chaos debugging if it is not enabled already */
	void DebugDrawCells();

protected:	
	/* Macro helper to declare the required processor functions */
	DECLARE_PROCESSOR_BODY

public:	
	/** @brief Processors entity query,
	*   @requires FMassVelocityFragment, FTransformFragment, FPDOctreeFragment, FPDInOctreeGridTag */
	FMassEntityQuery UpdateOctreeElementsQuery;
	
	/** @brief Local pointer to the RTSSubsystem, to source the octree from */
	UPROPERTY()
	class UPDRTSBaseSubsystem* RTSSubsystem = nullptr;

	/** @brief Declaration of console variable for draw cells command */
	static TAutoConsoleVariable<bool> CVarDrawCells;

	/** @brief Flag to make sure we aren't excessively calling GEngine->HandleDeferCommand */
	bool bSentChaosCommand = false;
};

/** @brief Adding of octree cells upon observer add operation (EMassObservedOperation::Add) */
UCLASS()
class PDRTSBASE_API UPDOctreeEntityObserver : public UMassObserverProcessor
{
	GENERATED_BODY()

public:
	UPDOctreeEntityObserver();

protected:	
	/* Macro helper to declare the required processor functions */
	DECLARE_PROCESSOR_BODY

public:	
	/** @brief Processors entity query,
	 *  @requires FPDOctreeFragment, FTransformFragment, FAgentRadiusFragment */
	FMassEntityQuery EntityQuery;

	/** @brief Local pointer to the RTSSubsystem, to source the octree from */
	UPROPERTY()
	class UPDRTSBaseSubsystem* RTSSubsystem = nullptr;

	/** @brief Element cell size, calculated from agent radius if available, otherwise assigned by default settings */
	UPROPERTY()
	float ElementCellSize = 0.0;
};

/** @brief Removal of octree cells upon observer remove operation (EMassObservedOperation::Remove) */
UCLASS()
class PDRTSBASE_API UPDGridCellDeinitObserver : public UMassObserverProcessor
{
	GENERATED_BODY()
public:
	
	UPDGridCellDeinitObserver();

protected:	
	/* Macro helper to declare the required processor functions */
	DECLARE_PROCESSOR_BODY

public:	
	/** @brief Processors entity query,
	 *  @requires FPDOctreeFragment */	
	FMassEntityQuery EntityQuery;

	/** @brief Local pointer to the RTSSubsystem, to source the octree from */
	UPROPERTY()
	class UPDRTSBaseSubsystem* RTSSubsystem = nullptr;
};

/** @brief Entity on entity 'collision' processor,
 *  @todo Finish testing the bounds check
 *  - Execution Order : After 'UPDOctreeProcessor' & After 'UE::Mass::ProcessorGroupNames::Movement'
 */
UCLASS()
class PDRTSBASE_API UPDCollisionSignalProcessor : public UMassSignalProcessorBase
{
	GENERATED_BODY()

public:
	UPDCollisionSignalProcessor();

protected:	
	/* Macro helper to declare the required processor functions */
	DECLARE_PROCESSOR_BODY
	
public:	
	/** @brief Processors entity query,
	 *  @requires FMassVelocityFragment, FTransformFragment, FPDOctreeQueryTag, UMassSignalSubsystem */
	FMassEntityQuery WorldOctreeEntityQuery{};

	/** @brief Local pointer to the RTSSubsystem, to source the octree from */
	UPROPERTY()
	class UPDRTSBaseSubsystem* RTSSubsystem = nullptr;
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