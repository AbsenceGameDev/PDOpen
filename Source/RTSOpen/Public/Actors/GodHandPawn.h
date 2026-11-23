/* @author: Ario Amin @ Permafrost Development. @copyright: Full BSL(1.1) License included at bottom of the file  */

#pragma once

#include "CoreMinimal.h"
#include "PDItemCommon.h"
#include "GameplayTagContainer.h"
#include "InputActionValue.h"
#include "MassEntityTypes.h"
#include "RTSOController.h"
#include "RTSOpenCommon.h"
#include "Containers/Deque.h"
#include "GameFramework/Pawn.h"
#include "Interfaces/PDRTSBuilderInterface.h"
#include "Interfaces/RTSOConversationInterface.h"
#include "Interfaces/RTSOInputInterface.h"
#include "GodHandPawn.generated.h"

class APDInteractActor;
class UPDInventoryComponent;


//
// Godhand functionality

/**
 * @brief The players main means of action. This is the RTS god-hand. This tracks and controls your units.  
 */
UCLASS()
class RTSOPEN_API AGodHandPawn
	: public APawn
	, public IPDRTSBuilderInterface
	, public IRTSOInputInterface
	, public IRTSOConversationInterface
{
	GENERATED_BODY()

public:
	/** @brief Sets up subobjects such as a scene root, the camera, spring arm, the cursor mesh, the collision component,
	 * the interact component, the inventory component, the movement component and lastly the participant component */
	AGodHandPawn();

	
	/** @brief Calls 'TrackMovement(), HoverTick(), RotationTick()'
	 * and if bUpdatePathOnTick is true then call RefreshPathingEffects()*/
	virtual void Tick(float DeltaTime) override;

	/** @brief If we have a CameraInterpTarget then move towards that as first priority, when reached clear the target */
	void CameraInterpTick(float DeltaTime);

	/** @brief Rotates towards the requested direction, rotates in 90 degree increments, snaps to worlds cardinal directions*/
	void RotationTick(float& DeltaTime);
	
	/** @brief Updated the current buildable ghost in the world-grid */
	void BuildableGhostTick(float DeltaTime);
	
	/** @brief Offset own actor location based on tracked actor, to keep in view */
	void TrackUnitMovement();

	/** @brief Binds collision delegates, updates zoom/magnify levels, setup ISM component,
	 * @todo backlog: when the design issues with enhanced input have been resolved by epic then remove the reset value stack of the input subsystem */
	virtual void BeginPlay() override;

	/** @brief Deinit subsystems if needed */
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

	/** @brief Refreshes the instance settings from the settings table handle */
	void RefreshSettingsInstance();
	
	/** @brief Validates that the instance settings table row handle has been set */
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;

	/** @brief Only calls super. Reserved for later use */
	virtual void OnConstruction(const FTransform& Transform) override;

	/** @brief Only calls super. Reserved for later use */
	UFUNCTION(BlueprintImplementableEvent)
	void FireAction(FGameplayTag ActionTag);
	
	/* PDRTS Builder Interface - Start */
	/** @brief Refreshes our pointer to our selected build context or selected buildable data based on the enum, and the tag finds the actual find the entry therein */
	virtual void SelectBuildMenuEntry_Implementation(ERTSBuildMenuModules ActionMode, FGameplayTag ActionTag) override;

	/** @brief  Destroys the currently selected buildable actor */
	UFUNCTION()
	void PerformAction_Destroy(const TArray<uint8>& Payload);
	
	UFUNCTION()
	void RefreshInstanceState(const AActor* DestroyedTargetActor);


	/** @brief Selects a buildable action entry, may it be to spawn workers or to destroy the associated buildable */
	virtual void SelectActionMenuEntry_Implementation(ERTSBuildableActionMenuModules ActionMode, FGameplayTag ActionTag, const TArray<uint8>& Payload) override;
	/** @brief Passes the call to the controller and retrieves our persistent ID */
	virtual int32 GetBuilderID_Implementation() override;

	/** @brief Copies our 'SpawnedBuildings' array to the outarray */
	UFUNCTION()
	virtual void GetOwnedBuildings_Implementation(TArray<AActor*>& OutArray) override
	{
		OutArray = SpawnedBuildings;
	}

	/** @brief Adds a new building to our 'SpawnedBuildings' array */
	UFUNCTION()
	virtual void SetOwnedBuilding_Implementation(AActor* NewBuilding) override
	{
		SpawnedBuildings.Emplace(NewBuilding);
	}
	
	/* PDRTS Builder Interface - End */

	/** @brief Retrieves the ISM Agent component from UPDRTSBaseSubsystem and sets its entity manager*/
	UFUNCTION()
	void InitializeISMAgent();

	/** @brief Updates the magnify value based on our scroll input.
	 * Sources the magnify values from either a an actual float curve asset or if not available a curve defined as a math expression */
	void UpdateMagnification();
	/** @brief Updates the cursor location and 'snaps' to the currently hovered target, if any*/
	void UpdateCursorLocation(float DeltaTime);
	/** @brief Calls super and clears the input value stack, so any remaining values on teh input stack isn't being applied to the new pawn*/
	virtual void PossessedBy(AController* NewController) override;
	
	/** @brief If the other actor has an interact interface, then it assigns 'OtherActor' parameter to the 'HoveredActor' property */
	UFUNCTION()
	void OnCollisionBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor*              OtherActor,
		UPrimitiveComponent* OtherComp,
		int32                OtherBodyIndex,
		bool                 bFromSweep,
		const                FHitResult& SweepResult);
	/** @brief Clears 'HoveredActor' */
	void ActorEndOverlapValidation();

	/** @brief Calls 'ActorEndOverlapValidation()' */
	UFUNCTION()
	void OnCollisionEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor*              OtherActor,
		UPrimitiveComponent* OtherComp,
		int32                OtherBodyIndex);
	/** @brief Sets the ISM agent component. @todo This is likely not needed anymore, will likely be removed in a coming commit */
	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;
	/** @brief Calls 'ActorEndOverlapValidation()' @todo This might not needed anymore. revise if should be removed*/
	virtual void NotifyActorEndOverlap(AActor* OtherActor) override;

	/** @brief Deprecated, remove next commit: 'Traces' against mass entities via a bounds-query of the octree */
	UE_DEPRECATED(5.3, "Entities are queried using the RTS subsystem. Call AGodHandPawn::FindClosestMassEntity to retrieve any entitiy overlapping the player cursor.")
	virtual FMassEntityHandle OctreeEntityTrace_DEPRECATED(const FVector& StartLocation, const FVector& EndLocation);
	
	/** @brief Call into the UPDRTSBaseSubsystem and check if it's OctreeUserQuery.CurrentBuffer has anything entries for us to read */
	FMassEntityHandle FindClosestMassEntity() const;
	/** @brief Does an overlap check and picks the closes actor inheriting from IPDInteractInterface, if any*/
	AActor* FindClosestInteractableActor() const;
	/** @brief Helper to return the value of the given entity's transform fragment, if entity is valid. */
	const FTransform& GetEntityTransform(const FMassEntityHandle& Handle) const;
	int32 GetEntityOwnerID(const FMassEntityHandle& Handle) const;
	/** @brief Calls 'FindClosestMassEntity()' and 'FindClosestInteractableActor()' */
	void HoverTick(float DeltaTime);
	
	/* RTSO Input Interface - Start */
	/** @brief Adds movement input to the input vector scaled by 100 */
	virtual void ActionMove_Implementation(const FInputActionValue& Value) override;
	/** @brief Sets Magnification strength the the value of the input action, in this case bound to scroll */
	virtual void ActionMagnify_Implementation(const FInputActionValue& Value) override;
	/** @brief Adds a rotation direction to the rotation queue. actual rotation updates in RotationTick() where it snaps the target to a cardinal direction*/
	virtual void ActionRotate_Implementation(const FInputActionValue& Value) override;
	/** @brief Drags and moves the viewport, currently unused / unset input */
	virtual void ActionDragMove_Implementation(const FInputActionValue& Value) override;
	bool ClickPotentialBuildable(ARTSOController* PC);

	/** @brief If a valid mass entity or target actor hovered then update niagara effect, otherwise initiate drawing a marquee.*/
	virtual void ActionWorkerUnit_Started_Implementation(const FInputActionValue& Value) override;
	/** @brief Called while the input action is being held down, updates 'WorkerUnitActionTarget' with 'HoveredActor'.
	 * In case we are drawing a marquee, then it also updates the marquee*/
	virtual void ActionWorkerUnit_Triggered_Implementation(const FInputActionValue& Value) override;
	/** @brief Calls into IRTSOInputInterface::Execute_ActionWorkerUnit_Completed. Reserve for later use */
	virtual void ActionWorkerUnit_Cancelled_Implementation(const FInputActionValue& Value) override;
	
	/** @brief Spawns a ghost or buildable from a ghost, if ghost then it may need to wait for build times. */
	void SpawnFromGhost(bool bBuildable, bool bRequiresWorkersToBuild = false);
	
	/** @brief Aborts marquee selection and either spawns a ghost or spawns a buildable, based on buildable placement behaviour
	 * @todo @refactor might have a bug here, look into it when refactoring */
	void ProcessPlaceBuildable(ARTSOController* PC);
	bool AttemptSelectSingleUnit();
	void DispatchUnitActionWithTargets();

	/** @brief Resets data that is otherwise used for visually pathing a workers potential walk-path */
	void ResetPathParameters();
	
	/** @brief If we were NOT drawing a marquee: then dispatch ai job to entity, if we had a valid entity.
	 * If we were drawing a marquee: call MarqueeSelection(EMarqueeSelectionEvent::RELEASEMARQUEE).*/
	virtual void ActionWorkerUnit_Completed_Implementation(const FInputActionValue& Value) override;

	/** @brief Empty for now. Reserved for later use*/
	virtual void ActionBuildMode_Implementation(const FInputActionValue& Value) override;
	/** @brief Empty for now. Reserved for later use*/
	virtual void ActionClearSelection_Implementation(const FInputActionValue& Value) override;
	/** @brief This action dispatches any selected group of entities to do a given job for them, the job depends on the target itself */
	virtual void ActionMoveSelection_Implementation(const FInputActionValue& Value) override;
	/** @brief Empty for now. Reserved for later use */
	virtual void ActionAssignSelectionToHotkey_Implementation(const FInputActionValue& Value) override;
	/** @brief Empty for now. Reserved for later use */
	virtual void ActionHotkeySelection_Implementation(const FInputActionValue& Value) override;
	/** @brief Empty for now. Reserved for later use */
	virtual void ActionChordedBase_Implementation(const FInputActionValue& Value) override;
	
	/* RTSO Input Interface - End */

	/** @brief Empty for now. Reserved for later use */
	UFUNCTION()
	void OnItemUpdate(const FPDItemNetDatum& BuildableResourceDatum);
private:
	/** @brief Refreshes the niagara effect and updates it's visualized navpath */
	void RefreshPathingEffects();
	/** @brief Updates collision and cursor mesh to be where the mouse trace intersects with objects in the level*/
	void TrackMovement(float DeltaTime);

public:
	/* Gameplay system components */	
	/** @brief This component performs our interaction traces and is able to interact with 'IPDInteractInterface's */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category ="RTS|Pawn|GameplayComponents")
	class UPDInteractComponent* InteractComponent = nullptr;
	/** @brief Inventory component to keep track of our items, stacks and such.
	 * Allows trading between out custom mass entity inventory fragments and actor inventory components*/
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category ="RTS|Pawn|GameplayComponents")
	class UPDInventoryComponent* InventoryComponent = nullptr;
	/** @brief Conversation participant component for our listener participant (the players own pawn)*/
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category ="RTS|Pawn|GameplayComponents")
	class UConversationParticipantComponent* ParticipantComponent = nullptr;
	
	/* Niagara */
	/** @briefThe actual niagara effect we want to apply for the pathing visualization */
	UPROPERTY(EditAnywhere, Category = "RTS|Pawn|Effects")
	class UNiagaraSystem* NS_WorkerPath;
	/** @brief The actual niagara component which will run the effect */
	UPROPERTY(VisibleInstanceOnly, Category = "RTS|Pawn|Effects")
	class UNiagaraComponent* NC_WorkerPath;
	
	/* Scene/actor components */	
	/** @brief Actors root component */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Pawn|Base")
	class USceneComponent* SceneRoot = nullptr;
	/** @brief Spring arm fro the camera, we also modify the camera focal distance based on springarm length */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Pawn|Base")
	class USpringArmComponent* Springarm = nullptr;
	/** @brief Actual camera component. Do not interfere with directly, is handled by the camera manager */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Pawn|Base")
	class UCameraComponent* Camera = nullptr;
	/** @brief Actual 'cursor/godhand mesh' we want to display */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Pawn|Base")
	class UStaticMeshComponent* CursorMesh = nullptr;
	/** @brief Collision component for overlaps and overlap checks with regular actors */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Pawn|Base")
	class USphereComponent* Collision = nullptr;
	/** @brief Pawn (floating) movement component, to enable som basic movement with acceleration and such */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Pawn|Base")
	class UFloatingPawnMovement* PawnMovement = nullptr;
	
	/** @brief  Godhand settings, datatable row handle meant to link to an  'FRTSGodhandSettings' datatable entry */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (RowType = "/Script/RTSOpen.RTSGodhandSettings"))
	FDataTableRowHandle GodHandSettingsHandle;

	/** @brief The actual instanced Godhand settings, sourced from 'GodHandSettingsHandle' */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "RTS|Pawn|Cursor|Settings")
	FRTSGodhandSettings InstanceSettings;

	/** @brief Godhand state, keeps stateful runtime data */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "RTS|Pawn|Cursor|Settings")
	FRTSGodhandState InstanceState;
	
	/** @brief The currently selected build context */
	const FPDBuildContext* CurrentBuildContext;

	/** @brief The currently selected buildable data */
	const FPDBuildableData* CurrentBuildableData;

	/** @brief The ghost mesh of the currently selected buildable */
	UPROPERTY()
	AActor* CurrentGhost = nullptr;

	/** @brief Spawned buildings, may be ghosts or completed buildings */
	UPROPERTY()
	TArray<AActor*> SpawnedBuildings{};
	/** @brief Currently selected buildable type by tag, will be a ghost until placed, then placement behaviour settings dictate what happens */
	FGameplayTag CurrentBuildableTag{};
	/** @brief Currently selected build-context by tag, Reserved */
	FGameplayTag CurrentBuildContextTag{};

	/** @brief Cached owner ID, to avoid having to call the interface multiple times in fast paths @todo @refactor Set default value*/
	UPROPERTY()
	int32 CachedActorID = FPDPersistentID::INVALID_ID;

	/** @brief Camera interpolation target */
	UPROPERTY()
	AActor* CameraInterpTarget = nullptr;
	
	/** @brief Flag to tell us when we just have placed a buildable, @todo @efactor move somewhere more appropriate */
	bool bJustPlacedBuildable = false;

	
	/** @brief Poor mans double click detection - Amount of clicks */
	UPROPERTY()
	int32 HitValidBuildableCounter = 0;
	/** @brief Poor mans double click detection - Time since last click */
	UPROPERTY()
	float TimeBetweenClicks = 0.0;	
	/** @brief Poor mans double click detection - Used to ensure our double clicks hit the same actor */
	UPROPERTY()
	AActor* LastClickedBuildable = nullptr;

	
	/** @brief Fallback comparison value for a closest distance search when the vector was not valid */
	static inline constexpr double  InvalidDistance{UE_MAX_FLT * 2};
	/** @brief Fallback comparison value for a closest distance search when the vector was not valid */
	static inline const FVector InvalidVector{InvalidDistance};

	/** @brief Active entity manager, exists in the mass entity subsystem */
	const FMassEntityManager* EntityManager = nullptr; 
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

