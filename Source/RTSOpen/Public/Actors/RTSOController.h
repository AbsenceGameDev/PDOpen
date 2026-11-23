/* @author: Ario Amin @ Permafrost Development. @copyright: Full BSL(1.1) License included at bottom of the file  */
#pragma once

#include "CoreMinimal.h"
#include "Interfaces/RTSOInputInterface.h"
#include "Interfaces/PDPersistenceInterface.h"
#include "Widgets/RTSOMainMenuBase.h"

#include "ConversationTypes.h"
#include "GameplayTagContainer.h"
#include "InputModifiers.h"
#include "MassEntityTypes.h"
#include "GameFramework/PlayerController.h"
#include "Interfaces/PDRTSBuilderInterface.h"
#include "Interfaces/RTSOActionLogInterface.h"
#include "RTSOController.generated.h"

struct FRTSOActionLogEvent;
class URTSOActionLogUserWidget;
class URTSOSaveEditorUserWidget;
// Fwd decl.
class URTSOConversationWidget;
class UPDBuildWidgetBase;
class URTSOInputStackSubsystem;

/** @brief 'Marquee selection event' selectors */
UENUM()
enum class EMarqueeSelectionEvent : uint8
{
	STARTMARQUEE   UMETA(DisplayName = "Start Marquee"),
	HOLDMARQUEE    UMETA(DisplayName = "Hold Marquee"),
	RELEASEMARQUEE UMETA(DisplayName = "Release Marquee"),
	ABORT          UMETA(DisplayName = "Abort Marquee"),
};

class UInputMappingContext;
class FNativeGameplayTag;
struct FInputActionValue;

/**
 * @brief The players controller class. This sets up mapping contexts, bindings and such. Expects them to be assigned in a data-BP  
 */
UCLASS()
class RTSOPEN_API ARTSOController
	: public APlayerController
	, public IRTSOInputInterface
	, public IPDRTSBuilderInterface
	, public IRTSOActionLogInterface 
{
	GENERATED_UCLASS_BODY()
	/** @brief Runs some setup, such as applying mapping contexts (TAG_CTRL_Ctxt_BaseInput, TAG_CTRL_Ctxt_WorkerUnitMode)
	 * and generates or loads a persistent ID from UPDRTSBaseSubsystem and proceeds to register it
	 * Lastly it instantiates the actual conversation widget in memory */
	virtual void BeginPlay() override;
	/** @brief Unregisters the actors persistent ID in UPDRTSBaseSubsystem */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** @brief Todo move to interface, refreshes or creates a new player ID */
	void RefreshOrAddNewID();

	virtual void SendActionEvent_Implementation(const FRTSOActionLogEvent& NewActionEvent) override;
	
	/** @brief Sets up bindings for enhanced input */
	virtual void SetupInputComponent() override;

	

	/** @brief Adds/overwrites a mapping context settings entry, keyed by a NativeGameplayTag*/
	void OverwriteMappingContextSettings(const FNativeGameplayTag& ContextTag, UInputMappingContext* NewContext);
	/** @brief Adds/overwrites a mapping context settings entry, keyed by a regular FGameplayTag*/
	void OverwriteMappingContextSettings(const FGameplayTag& ContextTag, UInputMappingContext* NewContext);
	
	/** @brief Activates a mapping context if it exists in the mapping context settings, keyed by a NativeGameplayTag*/
	void ActivateMappingContext(const FNativeGameplayTag& ContextTag);
	/** @brief Activates a mapping context if it exists in the mapping context settings, keyed by a regular GameplayTag*/
	void ActivateMappingContext(const FGameplayTag& ContextTag);
	/** @brief De-activates a mapping context if it exists in the mapping context settings, keyed by a NativeGameplayTag*/
	void DeactivateMappingContext(const FNativeGameplayTag& ContextTag);
	/** @brief De-activates a mapping context if it exists in the mapping context settings, keyed by a regular GameplayTag*/
	void DeactivateMappingContext(const FGameplayTag& ContextTag);	

	/** @brief De-activates a mapping context if it exists in the mapping context settings, keyed by a NativeGameplayTag*/
	bool IsMappingContextActive(const FNativeGameplayTag& ContextTag);
	/** @brief De-activates a mapping context if it exists in the mapping context settings, keyed by a regular GameplayTag*/
	bool IsMappingContextActive(const FGameplayTag& ContextTag);	
	
	/** @defgroup RTSOInputInterface - Start.
	 * Almost all of these only calls into the pawn in case it has the same interface */
	virtual void ActionMove_Implementation(const FInputActionValue& Value) override; /**<@ingroup RTSOInputInterface */
	virtual void ActionMagnify_Implementation(const FInputActionValue& Value) override; /**<@ingroup RTSOInputInterface */
	virtual void ActionRotate_Implementation(const FInputActionValue& Value) override; /**<@ingroup RTSOInputInterface */
	virtual void ActionDragMove_Implementation(const FInputActionValue& Value) override; /**<@ingroup RTSOInputInterface */
	virtual void ActionWorkerUnit_Triggered_Implementation(const FInputActionValue& Value) override; /**<@ingroup RTSOInputInterface */
	virtual void ActionWorkerUnit_Started_Implementation(const FInputActionValue& Value) override; /**<@ingroup RTSOInputInterface */
	virtual void ActionWorkerUnit_Cancelled_Implementation(const FInputActionValue& Value) override; /**<@ingroup RTSOInputInterface */
	virtual void ActionWorkerUnit_Completed_Implementation(const FInputActionValue& Value) override; /**<@ingroup RTSOInputInterface */
	virtual void ActionBuildMode_Implementation(const FInputActionValue& Value) override; /**<@ingroup RTSOInputInterface */
	virtual void ActionClearSelection_Implementation(const FInputActionValue& Value) override; /**<@ingroup RTSOInputInterface */
	virtual void ActionMoveSelection_Implementation(const FInputActionValue& Value) override; /**<@ingroup RTSOInputInterface */
	void ProcessPotentialBuildableMenu(const AActor* HoveredActor) const;

	/** @brief Assigns the currently selected group of entities to a selection-group */
	virtual void ActionAssignSelectionToHotkey_Implementation(const FInputActionValue& Value) override;
	/** @brief Switch between hot-keyed selection group. this is also used to read key input for conversation choices */
	virtual void ActionHotkeySelection_Implementation(const FInputActionValue& Value) override;
	/** @brief Only used as a modifier key, does not need any logic in the actual function itself but is used to control priority in our mapping contexts */
	virtual void ActionChordedBase_Implementation(const FInputActionValue& Value) override;
	virtual void ActionExitConversation_Implementation(const FInputActionValue& Value) override; /**<@ingroup RTSOInputInterface */
	virtual void ActionToggleMainMenu_Implementation(const FInputActionValue& Value) override; /**<@ingroup RTSOInputInterface */
	void HandleConversationChoiceInput(int32 ChoiceSelection) const; /**<@ingroup RTSOInputInterface */
	/* RTSOInputInterface - End */
	

	/* RTSO Menu interface - Start, @todo move to actual interface if we start making more menus*/
	UFUNCTION() void OnReleased_Resume();
	UFUNCTION() void OnReleased_QuitGame();	
	UFUNCTION() void OpenSaveEditor();	
	UFUNCTION() void CloseSaveEditor();	
	/*Handled via the buttons target widget class*/ // UFUNCTION() void OnReleased_Settings(); 
	/*Handled via the buttons target widget class*/ // UFUNCTION() void OnReleased_Save();
	/*Handled via the buttons target widget class*/ // UFUNCTION() void OnReleased_Load();
	/* RTSO Menu interface - End, @todo move to actual interface if we start making more menus*/
	void ProjectMouseToGroundPlane(
		FVector2D& ScreenCoordinates,
		FVector&   IntersectionPoint,
		bool&      bFoundInputType) const;
	void ProjectMouseToGroundPlane(
		TEnumAsByte<ECollisionChannel> OverrideTraceChannel,
		FVector2D&                     ScreenCoordinates,
		FVector&                       IntersectionPoint,
		bool&                          bFoundInputType) const;
	void ProjectMouseToGroundPlane(
		FHitResult&                    HitResult,
		TEnumAsByte<ECollisionChannel> OverrideTraceChannel,
		FVector2D&                     ScreenCoordinates,
		FVector&                       IntersectionPoint,
		bool&                          bFoundInputType) const;
	FHitResult ProjectMouseToGroundPlane(
		TEnumAsByte<ECollisionChannel> OverrideTraceChannel,
		FVector2D&                     ScreenCoordinates,
		bool&                          bFoundInputType) const;
	void DistanceFromViewportCenter(
		const FVector2D& InMoveDirection,
		FVector&         Direction,
		double&          Strength) const;
	void ClampMovementToScreen(FVector& MoveDirection, double& Strength) const;
	/* RTSO Mouse projections - Start */

	/* RTSO Marquee selection - Start */
	/** @brief Checks if the flag 'bIsDrawingMarquee' is true, this will only be the case if 'ActionWorkerUnit_Started' is triggered while not hovering over an interactable actor */
	UFUNCTION() FORCEINLINE bool IsDrawingMarquee() const { return bIsDrawingMarquee; }
	/** @brief Returns the starting screen-position for the start of the marquee */
	UFUNCTION() FORCEINLINE FVector2D GetStartMousePositionMarquee() const {return StartMousePositionMarquee;}

	/** @brief Generates a group ID outside of the hotkey range. This can be called the 'buffer-space'.
	 * @note Range loops from 1-10 */
	UFUNCTION() FORCEINLINE int32 GeneratedGroupID()
	{
		RollbackSelectionID = CurrentSelectionID;
		
		// this is the buffer-space, when we want to map it to a hotkey we move it to a key within the range of 0-9 so we can easily access it
		const int32 NewStep = ++SelectionBufferIdx %= SelectionBufferSize;
		CurrentSelectionID = MaxSelectionHotkeyIndex + NewStep;
		
		return CurrentSelectionID;
	}
	
	/** @brief Returns the current selected entity group, i.e. the current selection ID */
	UFUNCTION() FORCEINLINE int32 GetCurrentGroupID() const { return CurrentSelectionID; }

	/** @brief Get the current mouse position for the marquee, this never clears after a marquee is finished. so it holds the last value until overwritten by another marquee operation  */
	UFUNCTION() FORCEINLINE FVector2D GetCurrentMousePositionMarquee() const {return CurrentMousePositionMarquee;}

	/** @brief Event that is called when the marquee is finished and it has queried the world octree for any entities in the projected marquee volume.
	 * @param  SelectionGroup is the selectionID that was generated for this new group.
	 * @param  NewSelection is an array of MassEntity indices pointing to the entities queried from the octree */
	UFUNCTION(BlueprintNativeEvent) void OnMarqueeSelectionUpdated(int32 SelectionGroup, const TArray<int32>& NewSelection) const;

	/** @brief Gets the mouse coordinates if a mouse device is found,
	 * otherwise it returns the coordinate for the upper right corner*/
	FVector2D GetMouseScreenCoords() const;
	/** @brief Marquee drawing/selection logic */
	UFUNCTION(BlueprintCallable) void MarqueeSelection(EMarqueeSelectionEvent SelectionEvent);
	
	/** @brief Pass to widget via BP for now */
	UFUNCTION(BlueprintNativeEvent) void UpdateEntityCount(int32 TotalEntityCount) const;


	/** @brief Sets up a conversation with necessary prerequisites.
	 * @details 1. Sets camera manager view-target.
	 * 2. Activates the mapping context 'TAG_CTRL_Ctxt_ConversationMode' and deactivates 'TAG_CTRL_Ctxt_WorkerUnitMode' & 'TAG_CTRL_Ctxt_BaseInput'
	 * 3. Adds the conversation widget to viewport and sets it's initial payload*/
	UFUNCTION() void OnBeginConversation(const FClientConversationMessagePayload& Payload, AActor* PotentialCallbackActor);
	/** @brief Sets the conversations widgets new payload. This function is called when a conversation advances (see. 'BeginWaitingForChoices_Implementation' ) */
	UFUNCTION() void OnAdvanceConversation(const FClientConversationMessagePayload& Payload, AActor* PotentialCallbackActor);
	/** @brief Ends a conversation, reverts all changes regarding camera target and mapping contexts made in 'OnBeginConversation'*/
	UFUNCTION() void OnEndConversation(const FClientConversationMessagePayload& Payload, AActor* PotentialCallbackActor);

	/** @brief Returns the integer value of the FPDPersistentID 'ActorID', */
	UFUNCTION(BlueprintCallable) int32 GetActorID() const { return ActorID.GetID(); };

	/* PDRTS Builder Interface - Start */
	/** @brief Refreshes our pointer to our selected build context or selected buildable data based on the enum, and the tag finds the actual find the entry therein */
	virtual void SelectBuildMenuEntry_Implementation(ERTSBuildMenuModules ActionMode, FGameplayTag ActionTag) override;
	/** @brief Pipes through our persistent ActorID to the build system */
	virtual int32 GetBuilderID_Implementation() override { return GetActorID(); };
	
	/** @brief Passes along to pawn, which retrieves all owned buildings, copy them into OutArray. */
	UFUNCTION()
	virtual void GetOwnedBuildings_Implementation(TArray<AActor*>& OutArray) override;
	/** @brief Passes along to pawn, which will emplace the new building to their 'SpawnedBuildings' array  */
	UFUNCTION()
	virtual void SetOwnedBuilding_Implementation(AActor* NewBuilding) override;
	/* PDRTS Builder Interface - End */

	
	/** @brief Queues a draw-call into chaos' async debug draw queue */
	static void DrawBoxAndTextChaos(const FVector& BoundsCenter, const FQuat& Rotation, const FVector& DebugExtent, const FString& DebugBoxTitle, FColor LineColour = FColor::Black);
	/** @brief This function is here to fix a edge-case issue where the marquee volume is affected by large differences in z-height between the start corner and end corner world hit-results*/
	static void AdjustMarqueeHitResultsToMinimumHeight(FHitResult& StartHitResult, FHitResult& CenterHitResult, FHitResult& EndHitResult);
	/** @brief Queries the octree with the marquee selection volume and stores found handles in 'MarqueeSelectedHandles' */
	UFUNCTION() void GetEntitiesOrActorsInMarqueeSelection();
	/** @brief Reorder a selection group */
	UFUNCTION() void ReorderGroupIndex(const int32 OldID, const int32 NewID);
	/** @brief Returns immutable ref to 'MarqueeSelectedHandles'. A map keyed by the owner ID and value by a nested map keyed by selection indices and the nested value being the actual selection group handles*/
	const TMap<int32, TMap<int32, FMassEntityHandle>>& GetImmutableMarqueeSelectionMap() const { return MarqueeSelectedHandles; }
	/** @brief Returns mutable ref to 'MarqueeSelectedHandles'. A map keyed by the owner ID and value by a nested map keyed by selection indices and the nested value being the actual selection group handles*/
	TMap<int32, TMap<int32, FMassEntityHandle>>& GetMutableMarqueeSelectionMap() { return MarqueeSelectedHandles; }
	
	/** @return The conversation widget pointer, it will always be valid after begin-play in case 'ConversationWidgetClass' is pointing to a valid class */
	UFUNCTION() URTSOConversationWidget* GetConversationWidget() const { return ConversationWidget;};

	/** @brief Marquee related hit results, start corner */
	UFUNCTION() FHitResult GetLatestStartHitResult()  { return LatestStartHitResult;};
	/** @brief Marquee related hit results, center-point */
	UFUNCTION() FHitResult GetLatestCenterHitResult() { return LatestCenterHitResult;};
	/** @brief Marquee related hit results, end corner */
	UFUNCTION() FHitResult GetLatestEndHitResult()    { return LatestEndHitResult;};	


	UFUNCTION() UPDBuildingActionsWidgetBase* GetBuildableActionsWidget() const { return BuildableActionsWidget; }
	UFUNCTION() URTSOActionLogUserWidget* GetActionLogWidget() const { return ActionLogWidget; }
	
	/** @brief Dispatches a sync 'ParallelFor' that updates 'FPDMFragment_RTSEntityBase' fragments for all entities in the current selection group 
	 * @note Called when a new selection group is created or deselected. */
	void OnSelectionChange(bool bClearSelection);

protected:

	/** @brief Checks the given entity, compares with the relevant subsystem to get it's build contexts,
	 * then updates the widget based on this */
	void UpdateBuildMenuContexts(const FMassEntityHandle& CurrentEntity) const;

private:
	/** @brief Calls 'OnEndConversation' with a dummy payload.
	 * @note - Is called from 'ActionExitConversation_Implementation'
	 * @note - Is bound to 'ConversationWidget->ExitConversationButton->Hitbox->OnReleased' */
	UFUNCTION()
	void Internal_ExitConversation();
public:

	/** @brief Currently unused. Was used when initially prototyping. @todo consider removing */
	int32 LatestMenuButtonIdx = INDEX_NONE;
	
	/** @brief Adds some padding to the edge of the screen in certain screen related calculations */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Cursor|Settings")
	int32 ScreenEdgeMovementPadding = 0;

	/** @brief Use for mouse-to-world projection traces */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "RTS|Cursor|State")
	TEnumAsByte<ECollisionChannel> DedicatedLandscapeTraceChannel = ECollisionChannel::ECC_GameTraceChannel13;	
	
	/** @defgroup RTSInputActions. Assign these in editor @todo make developer settings struct to assign these from .ini configs also? */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTS|Input")
	class UInputAction* CtrlActionMove = nullptr; /**< @ingroup RTSInputActions*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTS|Input")
	class UInputAction* CtrlActionMagnify = nullptr; /**< @ingroup RTSInputActions*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTS|Input")
	class UInputAction* CtrlActionRotate = nullptr; /**< @ingroup RTSInputActions*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTS|Input")
	class UInputAction* CtrlActionDragMove = nullptr; /**< @ingroup RTSInputActions*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTS|Input")
	class UInputAction* CtrlActionWorkerUnit = nullptr; /**< @ingroup RTSInputActions*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTS|Input")
	class UInputAction* CtrlActionClearSelection = nullptr; /**< @ingroup RTSInputActions*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTS|Input")
	class UInputAction* CtrlActionMoveSelection = nullptr; /**< @ingroup RTSInputActions*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTS|Input")
	class UInputAction* CtrlActionHotkeySelection = nullptr; /**< @ingroup RTSInputActions*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTS|Input")
	class UInputAction* CtrlActionAssignSelectionToHotkey = nullptr; /**< @ingroup RTSInputActions*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTS|Input")
	class UInputAction* CtrlActionChordedBase = nullptr; /**< @ingroup RTSInputActions*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTS|Input")
	class UInputAction* CtrlActionExitConversation = nullptr; /**< @ingroup RTSInputActions*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTS|Input")
	class UInputAction* CtrlActionToggleMainMenu = nullptr; /**< @ingroup RTSInputActions*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTS|Input")
	class UInputAction* CtrlActionBuildMode = nullptr; /**< @ingroup RTSInputActions */

	/** @brief Mapping context settings, keyed by FGameplayTags. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTS|Input")
	TMap<FGameplayTag, UInputMappingContext*> MappingContexts{};

	/** @brief Selection error for when clicking to select a single entity */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTS|Input")
	double ClickToSelectErrorMin = 350.0;

protected:
	/** @brief Main menu widget base class, has a widget stack for supplying different stacked widgets */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Menu|Startscreen")
	TSubclassOf<URTSOMainMenuBase> MMWidgetClass{};
	/** @brief Instantiated main menu widget */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Menu|Startscreen")
	URTSOMainMenuBase* MainMenuWidget = nullptr;
	
	/** @brief Conversation widget base class */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<URTSOConversationWidget> ConversationWidgetClass = nullptr;
	/** @brief Instantiated conversation widget */
	UPROPERTY(VisibleInstanceOnly)
	URTSOConversationWidget* ConversationWidget = nullptr;

	/** @brief Conversation widget base class */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<URTSOSaveEditorUserWidget> SaveEditorWidgetClass = nullptr;
	/** @brief Instantiated conversation widget */
	UPROPERTY(VisibleInstanceOnly)
	URTSOSaveEditorUserWidget* SaveEditorWidget = nullptr;
	
	
	/** @brief Build menu widget base class */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UPDBuildWidgetBase> BuildMenuWidgetClass = nullptr;
	/** @brief Instantiated conversation widget */
	UPROPERTY(VisibleInstanceOnly)
	UPDBuildWidgetBase* BuildMenuWidget = nullptr;

	/** @brief Build menu widget base class */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UPDBuildingActionsWidgetBase> BuildableActionsWidgetClass = nullptr;
	/** @brief Instantiated conversation widget */
	UPROPERTY(VisibleInstanceOnly, Getter)
	UPDBuildingActionsWidgetBase* BuildableActionsWidget = nullptr;

	/** @brief Build menu widget base class */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<URTSOActionLogUserWidget> ActionLogWidgetClass = nullptr;
	/** @brief Instantiated conversation widget */
	UPROPERTY(VisibleInstanceOnly, Getter)
	URTSOActionLogUserWidget* ActionLogWidget = nullptr;
	
	
	/** @brief Marquee related hit results, start corner */
	UPROPERTY(VisibleInstanceOnly)
	FHitResult LatestStartHitResult{};
	/** @brief Marquee related hit results, center-point */
	UPROPERTY(VisibleInstanceOnly)
	FHitResult LatestCenterHitResult{};
	/** @brief Marquee related hit results, end corner */	
	UPROPERTY(VisibleInstanceOnly)
	FHitResult LatestEndHitResult{};

	/** @brief Actors persistent ID, is generated upon BeginPlay if none already exists in the UPDRTSBaseSubsystem */
	UPROPERTY(VisibleInstanceOnly)
	FPDPersistentID ActorID{};
	
	/** @brief Currently unused. Last selection ID after updating it, if we need it for rollback purposes */
	UPROPERTY(VisibleInstanceOnly)
	int32 RollbackSelectionID = INDEX_NONE;
	/** @brief The index we are at in the 'buffer-space' of the selection IDs*/
	UPROPERTY(VisibleInstanceOnly)
	int32 SelectionBufferIdx = INDEX_NONE;	
	/** @brief The size of the 'buffer-space' of the selection IDs*/
	UPROPERTY(VisibleInstanceOnly)
	int32 SelectionBufferSize = 20;
	/** @brief Max hot-keyable index for selection IDs*/
	UPROPERTY(VisibleInstanceOnly)
	int32 MaxSelectionHotkeyIndex = 9;
	/** @brief Current/Latest Selection ID*/
	UPROPERTY(VisibleInstanceOnly)
	int32 CurrentSelectionID = INDEX_NONE;
	
	/** @brief Marquee - active state */
	uint8 bIsDrawingMarquee : 1;
	/** @brief Marquee - (Start) screen position */
	FVector2D StartMousePositionMarquee{};
	/** @brief Marquee - (Current) screen position */
	FVector2D CurrentMousePositionMarquee{};
	
	/** @brief Selection groups that have been hot-keyed */
	TSet<int32> HotKeyedSelectionGroups{};	
	/** @brief All selection groups.
	 * - Keyed by selection ID.
	 * - Value is a nested map keyed by entity ID and it's inner value being the actual entity handle
	 * - The nested map could be replaced with a set but search complexity will virtually be the same. Or rather might be slower as we a hash of FMassEntityHandle could have collisions where int32 would have 0 collisions 
	 * Space complexity will however be three times in size but even at a selection group of 10.000 entities this is still less than 120KB working memory 
	 */
	TMap<int32, TMap<int32, FMassEntityHandle>> MarqueeSelectedHandles{};

	/** @brief Empty key array, returned as dummy when we can't find a selection group */
	static inline const TArray<int32> EmptyKeys = {};
};


/**
 * @brief Scalar modifier pass-through hack. Scales input by a set factor per axis
 */
UCLASS(NotBlueprintable, MinimalAPI, meta = (DisplayName = "IntegerPassthrough"))
class UInputModifierIntegerPassthrough : public UInputModifier
{
	GENERATED_BODY()

public:

#if WITH_EDITOR
	/** @brief Always returns true */
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif
protected:
	/** @bug There is a bug with EI where it discards any output of 'ModifyRaw' if the given 'UInputModifier' has been applied to a input mapping context. THis poses severe limitations of the usability and modularity of mapping contexts so causes a major issue
	 * @workaround Small subsystem which works as an input stack, stores the output and we query peek and pop the top of the stack when the input event is fired */
	virtual FInputActionValue ModifyRaw_Implementation(const UEnhancedPlayerInput* PlayerInput, FInputActionValue CurrentValue, float DeltaTime) override;

public:
	/** @brief Pass-through value, set in mapping context where it makes sense to have multiple numbered keys on an action */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category=Settings)
	int32 IntegerPassthrough = INDEX_NONE;
	
	/** @brief Input stack subsystem pointer, so we don't have to fetch it every time the passthrough input modifier is called */
	UPROPERTY()
	URTSOInputStackSubsystem* InputStackWorkaround = nullptr;

	/** @brief unused, leftover from prototyping. Remove by next commit */
	FInputActionValue CachedLastValue;
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