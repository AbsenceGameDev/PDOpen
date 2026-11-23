/* @author: Ario Amin @ Permafrost Development. @copyright: Full BSL(1.1) License included at bottom of the file  */

#pragma once

#include "CoreMinimal.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "GameplayTagContainer.h"
#include "MassEntityTypes.h"
#include "AI/Mass/PDMassFragments.h"
#include "GameFramework/Pawn.h"
#include "Engine/StreamableManager.h"

#include "PDRTSBaseUnit.generated.h"

struct FPDWorkUnitDatum;
class UBehaviorTree;
class UFloatingPawnMovement;
class UCapsuleComponent;
class AActor;

/**
 * @brief Custom ISM which handles tracking tasks and setting entity FPDMFragment_Action values.
 * @note Used with Mass as the entity (ISM) generator .
 */
UCLASS()
class PDRTSBASE_API UPDRTSBaseUnit : public UInstancedStaticMeshComponent
{
	GENERATED_UCLASS_BODY()

public:

	/** @brief Registers the unit handler for the world this exists in with the RTSSubsystem */
	virtual void InitializeComponent() override;

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 4
	virtual TArray<int32> AddInstances(const TArray<FTransform>& InstanceTransforms, bool bShouldReturnIndices, bool bWorldSpace, bool bUpdateNavigation) override;
#else 
	virtual TArray<int32> AddInstances(const TArray<FTransform>& InstanceTransforms, bool bShouldReturnIndices, bool bWorldSpace) override;
#endif

	/** @brief Only calls Super. Reserved for later use */
	virtual TArray<int32> GetInstancesOverlappingSphere(const FVector& Center, float Radius, bool bSphereInWorldSpace) const override;
	
	/** @brief Sets job back to idle */
	void ResetState(FMassEntityHandle RequestedEntityHandle);
	
	/** @brief Sets job to the requested job on the requested entity, if possible */
	void RequestAction(
		int32 CallingOwnerID,
		const FPDTargetCompound& OptTarget,
		FGameplayTag RequestedJob,
		FMassEntityHandle RequestedEntityHandle);

	/** @brief Incomplete, this is meant to be called when dispatching a batch of tasks which arent' sharing navpath */
	void RequestActionMulti(
		int32 CallingOwnerID,
		const TArray<TTuple<
		const FPDTargetCompound& /*OptTarget*/,
		const FGameplayTag&  /*RequestedJob*/,
		FMassEntityHandle    /*RequestedEntityHandle*/>>& EntityHandleCompounds,
		int32 SelectionGroup = INDEX_NONE);	

	/**
	 * @brief Sets job to the requested job on the requested entity, if possible
	 * @note Entry Tuple<CallingActor, NewTarget, RequestedJob, RequestedEntityHandle>
	 */
	void RequestActionMulti(
		int32 CallingOwnerID,
		const FPDTargetCompound& TargetCompound,
		const FGameplayTag& RequestedJob,
		const TMap<int32, FMassEntityHandle>& EntityHandleMap,
		const FVector&                        SelectionCenter,
		int32 SelectionGroup = INDEX_NONE);
	
	/** @brief Assigns the entity manager for the world we are in, so we can refer to it and modify fragments when needed */
	FORCEINLINE void SetEntityManager(const FMassEntityManager* InEntityManager) { EntityManager = InEntityManager;}

	/** @brief Gets called when a task finished. Resets action state, optionally chains a new action unto it. */
	void OnTaskFinished(FMassEntityHandle WorkerEntity, const FGameplayTag NewOptionalJobTag = FGameplayTag{}, const FPDTargetCompound& NewOptTarget = Dummy);

	/** @brief Gets The current job of the entity. Will hit check() if entity does not exist or does not have FPDMFragment_Action */
	const FGameplayTag& GetEntityJobChecked(FMassEntityHandle WorkerEntity) const;
	/** @brief Gets The current job of the entity. Will return empty tag if entity does not exist or does not have FPDMFragment_Action */
	const FGameplayTag& GetEntityJob(FMassEntityHandle WorkerEntity) const;
	/** @return true if entities current job is TAG_AI_Job_Idle, false if entity does not exist or has another job tag */
	bool IsEntityJobIdle(FMassEntityHandle WorkerEntity) const;

protected:
	/** @brief Sets collision response channel abd assigns a dummy task. Reserved for later use */
	virtual void BeginPlay() override;

	/** @brief Sets job to the requested job on the requested entity, only called when approved */
	void AssignTask(FMassEntityHandle EntityHandle, const FGameplayTag& JobTag, const FPDTargetCompound& OptTarget);


public:
	/** @brief Only calls Super. Reserved for later use */
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
	/** @brief Active jobs. Keyed by entity index, valued by job tag */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly)
	TMap<int32 /*MassEntity index */, FGameplayTag> ActiveJobMap{};

	/** @brief Active targets. Keyed by entity index, valued by target compound */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly)
	TMap<int32 /*MassEntity index */, FPDTargetCompound> ActiveTargetMap{};

	/** @brief Cached ptr to the entity manager. */
	const FMassEntityManager* EntityManager = nullptr;

	/** @brief Dummy target compound to return when calling 'GetEntityJob'  */
	static inline FPDTargetCompound Dummy{};

	bool bAddedNewInstanceCount = false;
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