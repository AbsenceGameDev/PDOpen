/* @author: Ario Amin @ Permafrost Development. @copyright: Full BSL(1.1) License included at bottom of the file  */
#pragma once

#include "CoreMinimal.h"
#include "MassEntityTypes.h"
#include "MassStateTreeTypes.h"
#include "MassEntityTypes.h"
#include "GameplayTags.h"
#include "StateTreeTypes.h"
#include "StateTreeInstanceData.h"
#include "AI/StateTree/PDMassTasks.h"
#include "PDRTSSharedHashGrid.h"
#include "RTSOMassTasks.generated.h"

struct FRTSOLightInventoryFragment;
struct FMassMoveTargetFragment;
struct FTransformFragment;
class UMassSignalSubsystem;
struct FMassMovementParameters;
class UMassEntitySubsystem;
class UPDInteractSubsystem;
struct FPDMFragment_EntityAnimation;
struct FStateTreeExecutionContext;

/**
 * @brief Interaction target data
 */
USTRUCT()
struct FRTSOTaskData_Interact
{
	GENERATED_BODY()

	/** @brief Potential Interaction Target: MassEntity */
	UPROPERTY(VisibleAnywhere, Category = Input)
	FMassEntityHandle PotentialEntityHandle;

	/** @brief Potential Interaction Target: Actor */
	UPROPERTY(VisibleAnywhere, Category = Input)
	AActor* PotentialInteractableActor;	
};

/**
 * @brief Action Log data
 */
USTRUCT()
struct FRTSOTaskData_ActionLog
{
	GENERATED_BODY()

	/** @brief Message to send to the action log */
	UPROPERTY(VisibleAnywhere, Category = Param)
	FText ActionMessage;
};

/**
 * @brief Simple Passthrough task that sends events of player-owned entities to the action log.
 * @details  
 */
USTRUCT()
struct RTSOPEN_API FRTSOTask_ActionLog : public FMassStateTreeTaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FRTSOTaskData_ActionLog;
	
	/** @brief Links external data: EntitySubsystemHandle, InventoryHandle*/
	virtual bool Link(FStateTreeLinker& Linker) override;
	/** @brief Returns the static struct of the instance data shorthand */
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	/** @brief Triggers OnInteract on the actor, (and/or entity proxy when the function is finished), if it turns out to be a valid interactable
	 * @todo write actual logic for interaction with other entity. use proxy such as the RTSBaseUnit (ISM subclass)
	 * @todo @backlog have some notes in there which I've not fully decided on, revise at some point */
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

	/** @defgroup ExternalHandles */
	TStateTreeExternalDataHandle<UMassEntitySubsystem> EntitySubsystemHandle; /**<@ingroup ExternalHandles*/
};


/**
 * @brief Interaction task.
 * @details Attempts to call Execute_OnInteract if the given target (or proxy object if the target's an entity) has an interface of type IPDInteractInterface 
 */
USTRUCT()
struct RTSOPEN_API FRTSOTask_Interact : public FMassStateTreeTaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FRTSOTaskData_Interact;
	
	/** @brief Links external data: EntitySubsystemHandle, InventoryHandle*/
	virtual bool Link(FStateTreeLinker& Linker) override;
	/** @brief Returns the static struct of the instance data shorthand */
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	/** @brief Triggers OnInteract on the actor, (and/or entity proxy when the function is finished), if it turns out to be a valid interactable
	 * @todo write actual logic for interaction with other entity. use proxy such as the RTSBaseUnit (ISM subclass)
	 * @todo @backlog have some notes in there which I've not fully decided on, revise at some point */
	template<typename TPDMassType, bool TFinishActualJobTask>
	static EStateTreeRunStatus TaskInteract(
		const TPDMassType* This,
		FStateTreeExecutionContext& Context,
		const FMassEntityHandle& OtherEntityHandle,
		const IPDInteractInterface* OtherInteractable,
		const UMassEntitySubsystem& EntitySubsystem);
	
	/** @brief Accesses linked data and calls InnerInteraction */
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

	/** @defgroup ExternalHandles */
	TStateTreeExternalDataHandle<UMassEntitySubsystem> EntitySubsystemHandle; /**<@ingroup ExternalHandles*/
	TStateTreeExternalDataHandle<UPDInteractSubsystem> InteractSubsystemHandle; /**<@ingroup ExternalHandles*/
	TStateTreeExternalDataHandle<FRTSOLightInventoryFragment> InventoryHandle; /**<@ingroup ExternalHandles*/
};


/**
 * @brief Bring back resources to bank or storage task.
 * @details Walks towards the target and attempts to call Execute_OnInteract if the given target os of storage type (or proxy object if the target's an entity) has an interface of type IPDInteractInterface
 * 
 * @note Will need a way for an entity given this task to efficiently mark some resources closeby and pick them up, then any other entity won't be pick the same resource unless the resource has enough to supply them both
 * @done Further thoughts are that is we want more then one entity to pick up from a resource it must be calculated to be enough left that it's needs are met
 * @donr Something like: struct FRTSEntityResourceGatherTarget { AActor* Target; int32 CountNeeded = -1;}; 
 */
USTRUCT()
struct RTSOPEN_API FRTSOTask_BringBackResource final : public FRTSOTask_Interact
{
	GENERATED_BODY()

	FRTSOTask_BringBackResource(){};

	DECLARE_TASK_BODY(BringBackResource)

	void OnPathSelected(FPDMFragment_RTSEntityBase& RTSData, bool bShouldUseSharedNavigation, const FVector& LastPoint) const;
	static struct FPDRTSTSetActorWrapper FindAmountOfResourceActorsNearGridCell(const FMassEntityHandle& EntityHandle, const FRTSOFindResourcesParameters& Params); // FPDGridCell GridCell, const FGameplayTag& ResourceType, int32 TargetResourceAmount);

protected:	
	/** @defgroup ExternalHandles */
	TStateTreeExternalDataHandle<UMassEntitySubsystem> EntitySubsystemHandle; /**<@ingroup ExternalHandles*/
	TStateTreeExternalDataHandle<UPDInteractSubsystem> InteractSubsystemHandle; /**<@ingroup ExternalHandles*/
	TStateTreeExternalDataHandle<FRTSOLightInventoryFragment> InventoryHandle; /**<@ingroup ExternalHandles*/
	TStateTreeExternalDataHandle<FPDMFragment_Action> ActionHandle; /**<@ingroup ExternalHandles*/

	TStateTreeExternalDataHandle<UMassSignalSubsystem> MassSignalSubsystemHandle; /**<@ingroup ExternalHandles*/
	TStateTreeExternalDataHandle<FTransformFragment> TransformHandle; /**<@ingroup ExternalHandles*/
	TStateTreeExternalDataHandle<FMassMoveTargetFragment> MoveTargetHandle; /**<@ingroup ExternalHandles*/
	TStateTreeExternalDataHandle<FMassMovementParameters> MoveParametersHandle; /**<@ingroup ExternalHandles*/
	TStateTreeExternalDataHandle<FPDMFragment_RTSEntityBase> RTSDataHandle; /**<@ingroup ExternalHandles*/

	mutable int32 CurrentPathIndex = 0;
	mutable int32 PathLimit = 0;
};


struct RTSOPEN_API FRTSOTask_MoveToTarget : public FPDMTask_MoveToTarget
{
	virtual void OnPathSelected(FPDMFragment_RTSEntityBase& RTSData, bool bShouldUseSharedNavigation, const FVector& LastPoint) const override;
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