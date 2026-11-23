/* @author: Ario Amin @ Permafrost Development. @copyright: Full BSL(1.1) License included at bottom of the file  */

#pragma once
#include "CoreMinimal.h"
#include "RTSOpenCommon.h"
#include "Blueprint/UserWidget.h"
#include "Components/Widget.h"
#include "Components/WidgetSwitcher.h"
#include "Components/CircularThrobber.h"
#include "Widgets/RTSOActiveMainMenu.h"
#include "Widgets/Layout/SWrapBox.h"
#include "RTSOSaveEditorWidget.generated.h"

class SRTSOSaveEditorBase;
struct FUserMissionTagsStruct;
struct FConversationStateStruct;
struct FUserInventoriesStruct;
struct FPlayerLocationStruct;
class URTSOpenSaveGame;
class SRTSOSaveEditor;

/** @brief The inner uwidget that routes data and decides which save editor slate widget to display */
UCLASS(Blueprintable)
class URTSOSaveEditorInnerWidget : public UWidget
{
	GENERATED_BODY()

public:
	/** @brief Copies the current save-data on an async game-thread */
	void CopyData(URTSOpenSaveGame* InSaveGame);
	/** @brief Does nothing. Reserved */
	void OnFailedCopyData();
	/** @brief Caches all copied data into different arrays, to separate data for the different slate widgets beforehand  */
	void OnCompletedCopyData();
	/** @brief Outputs the copied data to log  */
	void OnCompletedCopyData_Debug();
	/** @brief Updates the inner slate editors based on the selected 'EditorType' property,
	 * @note Gives a pointer of the relevant array view to the selected slate editor  */
	void UpdateInnerEditor();
	/** @brief Ensures we have a SWrapBox then calls UpdateInnerEditor and call UpdateChildSlot on the selected save-editor slate widget */
	virtual TSharedRef<SWidget> RebuildWidget() override;
	/** @brief Ensures we release our shared pointers to the slate widgets */
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

	/** @brief Overwrites 'EditorType', resets 'SharedExistingSaveEditor' then calls 'UpdateInnerEditor()' */
	UFUNCTION() void SelectEditor(EPDSaveDataThreadSelector NewEditorType);
	/** @brief  Resets the modified copied data to the initial values in the SaveGamePtr */
	void ResetFieldData(EPDSaveDataThreadSelector SaveDataGroupSelector);

	/** @brief Wrapbox that wraps our SRTSOSaveEditorBase derived widgets */
	TSharedPtr<SWrapBox> InnerSlateWrapbox;
	/** @brief The currently selected save editor widget */
	TSharedPtr<SRTSOSaveEditorBase> SharedExistingSaveEditor;

	/** @brief The currently selected save editor type */
	EPDSaveDataThreadSelector EditorType = EPDSaveDataThreadSelector::EPlayers;
	
	/** @brief Cached player base data for the PlayerBase Save-Editor, currently only consists of userid and location */
	TArray<TSharedPtr<FPlayerLocationStruct>>    LocationsAsSharedTupleArray;
	/** @brief Cached world interactables data for the Interactables Save-Editor */
	TArray<TSharedPtr<FRTSSavedInteractable>>    InteractableAsSharedArray;
	/** @brief Cached world unit/entity data for the Entity Save-Editor */
	TArray<TSharedPtr<FRTSSavedWorldUnits>>      EntitiesAsSharedArray;
	/** @brief Cached user inventory data for the Inventory Save-Editor */
	TArray<TSharedPtr<FUserInventoriesStruct>>   AllUserInventoriesAsSharedTupleArray;
	/** @brief Cached conversation actor state data for the Conversation State Save-Editor */
	TArray<TSharedPtr<FConversationStateStruct>> ConversationStatesAsSharedArray;
	/** @brief Cached user mission progress tags, for the Mission Tags Save-Editor */
	TArray<TSharedPtr<FUserMissionTagsStruct>>   UserMissionTagsAsSharedArray;

	/** @brief Pointer to the selected savegame slot */
	UPROPERTY()
	URTSOpenSaveGame* SaveGamePtr = nullptr;
	/** @brief Copied save data that we are modifying. Only commit back tp the actual slot when finished editing */
	UPROPERTY()
	FRTSSaveData CopiedSaveData;
};

/** @brief  The actual user widget that wraps out uwidget and properly exposes the widgets to UMG */
UCLASS(Blueprintable)
class URTSOSaveEditorUserWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	/** @brief Binds callbacks for our save-game slot buttons and our save editor category buttons.
	 * The latter types route into the inner save-editor which selects editor to display based on the pressed button */
	UFUNCTION()
	void BindButtonDelegates();

	/** @brief Loads the selected savegame slot and copies it to our inner save editor uwidget */
	UFUNCTION()
	void LoadSlotData(int32 SlotIdx, bool bFirstLoad = false);
	
	/** @brief Bounded to Slot0 button. Calls LoadSlotData(0)' */
	UFUNCTION()
	void LoadSlotData0();
	/** @brief Bounded to Slot1 button. Calls LoadSlotData(1)' */
	UFUNCTION()
	void LoadSlotData1();	
	/** @brief Bounded to Slot2 button. Calls LoadSlotData(2)' */
	UFUNCTION()
	void LoadSlotData2();
	/** @brief Bounded to Slot3 button. Calls LoadSlotData(3)' */
	UFUNCTION()
	void LoadSlotData3();
	/** @brief Bounded to Slot4 button. Calls LoadSlotData(4)' */
	UFUNCTION()
	void LoadSlotData4();

	/** @brief Helper to retrieve the button object based on a given selector enum */
	UButton* GetCategoryButton(EPDSaveDataThreadSelector Button) const;
	/** @brief Helper to reset a previous button state based on a given selector enum */
	void Category_ResetButtonState(EPDSaveDataThreadSelector PreviousButton) const;
	/** @brief Helper to activate a buttons state based on a given selector enum */
	void Category_ActivateButtonState(EPDSaveDataThreadSelector NewButton) const;
	/** @brief Helper that calls 'Category_ResetButtonState' & 'Category_ActivateButtonState'
	 * in the correct order while ensuring functions that are needed inbetween are called  */
	void SelectCategory(EPDSaveDataThreadSelector CategoryButton) const;
	/** @brief Calls 'SelectCategory' followed by a call to 'InvalidateLayoutAndVolatility' */
	void SelectCategoryAndInvalidateCache(EPDSaveDataThreadSelector CategoryButton);
	/** @brief Calls 'SelectCategoryAndInvalidateCache(EPDSaveDataThreadSelector::EWorldBaseData)' */
	UFUNCTION() void Category_WorldBaseData();
	/** @brief Calls 'SelectCategoryAndInvalidateCache(EPDSaveDataThreadSelector::EPlayers)' */
	UFUNCTION() void Category_PlayerBaseData();
	/** @brief Calls 'SelectCategoryAndInvalidateCache(EPDSaveDataThreadSelector::EInteractables)' */
	UFUNCTION() void Category_InteractableData();
	/** @brief Calls 'SelectCategoryAndInvalidateCache(EPDSaveDataThreadSelector::EEntities)' */
	UFUNCTION() void Category_EntityData();
	/** @brief Calls 'SelectCategoryAndInvalidateCache(EPDSaveDataThreadSelector::EInventories)' */
	UFUNCTION() void Category_PlayerInventoriesData();
	/** @brief Calls 'SelectCategoryAndInvalidateCache(EPDSaveDataThreadSelector::EConversationActors)' */
	UFUNCTION() void Category_ConversationStateData();
	/** @brief Calls 'SelectCategoryAndInvalidateCache(EPDSaveDataThreadSelector::EPlayerConversationProgress)' */
	UFUNCTION() void Category_MissionProgressTagsData();

	/** @brief Sets out widget switches to the index for loading, said index houses teh elements that our animation is using */
	virtual void OnAnimationStarted_Implementation(const UWidgetAnimation* Animation) override;
	/** @brief Sets out widget switches back from the index that it uses for loading animations, back to normal */
	virtual void OnAnimationFinished_Implementation(const UWidgetAnimation* Animation) override;
	
	/** @brief The base canvas that elements are painted on */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (BindWidget))
	UCanvasPanel* BaseCanvas = nullptr;

	/** @brief An exit button that closes the save editor */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (BindWidget))
	URTSOMenuButton* ExitButton = nullptr;	
	
	/** @brief The inner saveeditor uwidget  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (BindWidget))
	URTSOSaveEditorInnerWidget* Inner = nullptr;

	/** @brief The save editor category loading animation  */
	UPROPERTY(Transient, EditAnywhere, BlueprintReadWrite, Meta = (BindWidgetAnim))
	UWidgetAnimation* CategoryLoadingAnimation = nullptr;

	/** @brief The title textblock that displays the current data-view and which save slot it is in */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (BindWidget))
	UTextBlock* DataViewTitle = nullptr;
	
	/** @brief Widget switcher for the dataview,
	 * @note only visible when loading in large enough data that it gets a chance to run the loading animation  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (BindWidget))
	UWidgetSwitcher* LoadViewWidgetSwitch = nullptr;
	/** @brief Widget switcher for the saveeditor tabs/categories,
	 * @note only visible when loading in large enough data that it gets a chance to run the loading animation  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (BindWidget))
	UWidgetSwitcher* TabsLoadViewWidgetSwitch = nullptr;

	/** @brief  The actual overlay which houses the animation elements */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (BindWidget))
	UOverlay* DataViewLoaderAnimationLayer = nullptr;
	/** @brief  The actual overlay which houses the animation elements */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (BindWidget))
	UOverlay* TabsLoaderAnimationLayer = nullptr;
	/** @brief  The actual overlay which houses the category buttons */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (BindWidget))
	UOverlay* CategoryButtonOverlay = nullptr;
	/** @brief  The actual overlay which houses the current data-view */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (BindWidget))
	UOverlay* DataViewOverlay = nullptr;

	/** @brief Calling controller we want to bind the exit button to. Reserved for further bindings */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	AActor* ActorToBindAt = nullptr;

	/** @defgroup SaveDataEditorTabButtons  */
	UPROPERTY(BlueprintReadWrite, Meta=(BindWidget)) class URTSOMenuButton* Btn_WorldBaseData = nullptr; /**< @brief Button that invokes the WorldBase Editor Slate widget. @ingroup SaveDataEditorTabButtons */
	UPROPERTY(BlueprintReadWrite, Meta=(BindWidget)) class URTSOMenuButton* Btn_PlayerBaseData = nullptr; /**< @brief Button that invokes the PlayerBase Editor Slate Widget. @ingroup SaveDataEditorTabButtons */
	UPROPERTY(BlueprintReadWrite, Meta=(BindWidget)) class URTSOMenuButton* Btn_InteractableData = nullptr; /**< @brief Button that invokes the Interactable Editor Slate Widget. @ingroup SaveDataEditorTabButtons */
	UPROPERTY(BlueprintReadWrite, Meta=(BindWidget)) class URTSOMenuButton* Btn_EntityData = nullptr; /**< @brief Button that invokes the Entity Editor SLate Widget . @ingroup SaveDataEditorTabButtons */
	UPROPERTY(BlueprintReadWrite, Meta=(BindWidget)) class URTSOMenuButton* Btn_PlayerInventoriesData = nullptr; /**< @brief Button that invokes the Inventory Editor Slate Widget. @ingroup SaveDataEditorTabButtons */
	UPROPERTY(BlueprintReadWrite, Meta=(BindWidget)) class URTSOMenuButton* Btn_ConversationStateData = nullptr; /**< @brief Button that invokes the Conversation-State Editor Slate Widget. @ingroup SaveDataEditorTabButtons */
	UPROPERTY(BlueprintReadWrite, Meta=(BindWidget)) class URTSOMenuButton* Btn_MissionProgressTagsData = nullptr; /**< @brief Button that invokes the Mission Progress Mission Progress Slate Widget. @ingroup SaveDataEditorTabButtons */

	/** @defgroup MenuButtonGroup
	 *  @todo replace with ulistview of available slots */
	UPROPERTY(BlueprintReadWrite, Meta=(BindWidget))
	class URTSOMenuButton* Slot0 = nullptr; /**< @brief Self-explanatory. @ingroup MenuButtonGroup */
	UPROPERTY(BlueprintReadWrite, Meta=(BindWidget))
	class URTSOMenuButton* Slot1 = nullptr; /**< @brief Self-explanatory. @ingroup MenuButtonGroup */
	UPROPERTY(BlueprintReadWrite, Meta=(BindWidget))
	class URTSOMenuButton* Slot2 = nullptr; /**< @brief Self-explanatory. @ingroup MenuButtonGroup */
	UPROPERTY(BlueprintReadWrite, Meta=(BindWidget))
	class URTSOMenuButton* Slot3 = nullptr; /**< @brief Self-explanatory. @ingroup MenuButtonGroup */
	UPROPERTY(BlueprintReadWrite, Meta=(BindWidget))
	class URTSOMenuButton* Slot4 = nullptr; /**< @brief Self-explanatory. @ingroup MenuButtonGroup */

	/** @brief The currently loaded save data slot that we want to modify.
	 * @note Will never be directly operated on, only moved into when editing a slot is finished */
	UPROPERTY(VisibleInstanceOnly)
	URTSOpenSaveGame* LoadedGameSaveForModification = nullptr;
	
	/** @brief The text that represents the currently loaded slot, so there is no ambiguity in which slot the user is modifying  */
	static FText LoadedSlot_TitleText;
};

/*
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
