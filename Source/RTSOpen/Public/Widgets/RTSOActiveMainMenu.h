/* @author: Ario Amin @ Permafrost Development. @copyright: Full BSL(1.1) License included at bottom of the file  */

#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "CommonButtonBase.h"
#include "CommonTabListWidgetBase.h"
#include "PDRTSSharedUI.h"
#include "RTSOpenCommon.h"
#include "UI/PDButtons.h"
#include "UI/PDDialogs.h"

#include "Widgets/Slate/SRTSOSettingsStringSelector.h"
#include "Widgets/Slate/SRTSOVectorWidgets.h"

#include "RTSOActiveMainMenu.generated.h"

class UHorizontalBox;
class UCommonTabListWidgetBase;
class UCommonTextBlock;

/** @brief Save/load game delegate, used with the pop-up dialogs defined further down this file */
DECLARE_DELEGATE_TwoParams(FRTSOSavegameDelegate, const FString&, bool)

/** @brief Save type enum*/
UENUM()
enum ERTSOSaveType
{
	SAVE,
	LOAD
};

/** @brief Game Menu - Base menu button
 * @note Moved all necessary logic to UPDGenericButton, keeping this subclassed o we don't break any uasset widgets that derive from this
 */
UCLASS(Blueprintable)
class URTSOMenuButton : public UPDGenericButton
{
	GENERATED_BODY()

public:
	/* Reserved */

	UPROPERTY()
	FName ButtonName;
};

/** @brief Game Menu - Save dialog
 * @note Moved most necessary logic to UPDGenericDialog,
 * keeping this subclassed so can override the 'DialogReplyContinue' and so we also don't break any uasset widgets that derive from this
 */
UCLASS(Blueprintable)
class URTSOSaveGameDialog : public UPDGenericDialog
{
	GENERATED_BODY()

public:

	/** @brief Calls SuccessCallback.Execute with the passthrough parameters*/
	virtual void DialogReplyContinue(const bool bSuccess) const override; // declared as ufunction in parent class

	/** @brief Pass-through for the ERTSOSaveType */
	UPROPERTY()
	TEnumAsByte<ERTSOSaveType> Type = ERTSOSaveType::LOAD;

	/** @brief Pass-through for the slotidx */
	UPROPERTY()
	int32 SlotIdx = INDEX_NONE;

	/** @brief Callback that we want to fire on success (replying yes to the dialog)*/
	FRTSOSavegameDelegate SaveSuccessCallback{};

	/** @brief Callback that we want to fire on success (replying yes to the dialog)*/
	FRTSOSavegameDelegate SaveFailCallback{};	
};

/** @brief Game Menu - Save widget*/
UCLASS(Blueprintable)
class URTSOMenuWidget_SaveGame : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	/** @brief Unimplemented. Reserved for later use */
	void ClearDelegates() const {};

	/** @brief Only Calls super for now. Reserved for later use */
	virtual void NativePreConstruct() override;
	
	/** @brief (root) Base canvas panel */
	UPROPERTY(BlueprintReadWrite, Meta=(BindWidget))
	class UCanvasPanel* BaseCanvas = nullptr;

	/** @brief The box in which the banner and the inner box should contain */
	UPROPERTY(BlueprintReadWrite, Meta=(BindWidget))
	class UVerticalBox* MainBox = nullptr;

	/** @brief Canvas panel of the widget banner, for orienting elements within the banner */
	UPROPERTY(BlueprintReadWrite, Meta=(BindWidget))
	class UCanvasPanel* BannerCanvas = nullptr;

	/** @brief Text of the widget banner */
	UPROPERTY(BlueprintReadWrite, Meta=(BindWidget))
	class UTextBlock* BannerText = nullptr;

	/** @brief Image of the widget banner */
	UPROPERTY(BlueprintReadWrite, Meta=(BindWidget))
	class UImage* BannerImage = nullptr;

	/** @brief Button that is bound (in URTSOMainMenuBase::SaveTargetWidget & URTSOMainMenuBase::LoadTargetWidget) ot close the open 'URTSOMenuWidget_SaveGame'*/
	UPROPERTY(BlueprintReadWrite, Meta=(BindWidget))
	class URTSOMenuButton* ExitButton = nullptr;

	/** @brief Box that should hold all slots of type URTSOMenuButton. @todo replace with ulistview of slots */
	UPROPERTY(BlueprintReadWrite, Meta=(BindWidget))
	class UVerticalBox* InnerBox = nullptr;

	/** @defgroup MenuButtonGroup
	 *  @todo replace with ulistview of slots */
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
	
	/** @brief Unused for now, reserve for later use */
	UPROPERTY()
	class URTSOMainMenuBase* OwningStack = nullptr;	
};

//
// String Selector Widgets (UMG)
USTRUCT(Blueprintable)
struct FRTSOStringSelectorData
{
	GENERATED_BODY()
public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString SelectedString;

	TArray<TSharedPtr<FString>> GeneratedStringOptions;
};

UCLASS()
class URTSOStringSelectorBox : public UWidget
{
	GENERATED_BODY()

public:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;
	virtual void Refresh();
	virtual void CloseExpandableArea();

	TSharedPtr<class SRTSOStringSelector> SlateWidget;
	FRTSOStringSelectorData* DataPtr = nullptr;

	UPROPERTY()
	UWidget* Owner = nullptr; 

	FOnStringValueSelected OnStringSelected;
};

UCLASS(Blueprintable)
class URTSOStringSelector : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	virtual void Refresh();
	virtual void CloseExpandableArea();

	UPROPERTY(BlueprintReadWrite, Meta=(BindWidget))
	class UCommonTextBlock* SelectedStringValue;

	UPROPERTY(BlueprintReadWrite, Meta=(BindWidget))
	URTSOStringSelectorBox* SelectorBox = nullptr;	

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FRTSOStringSelectorData Data;

	void SetOnStringSelected(const FOnStringValueSelected& Delegate);
};

//
// Vector Selector Widgets (UMG)
USTRUCT(Blueprintable)
struct FRTSOVectorSelectorData
{
	GENERATED_BODY()
public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector4 SelectedValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (ValidEnumValues="Vector2, Vector3, Colour"))
	ERTSOSettingsType VectorType;
};

UCLASS()
class URTSOVectorSelectorBox : public UWidget
{
	GENERATED_BODY()

public:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;
	virtual void Refresh();
	virtual void CloseExpandableArea();

	TSharedPtr<class SRTSOVectorBase> SlateWidget;
	FRTSOVectorSelectorData* DataPtr = nullptr;

	UPROPERTY()
	UWidget* Owner = nullptr; 

	FOnVectorValueUpdated OnVectorUpdated;
};

UCLASS(Blueprintable)
class RTSOPEN_API URTSOVectorSelector : public UCommonActivatableWidget
{
	GENERATED_BODY()
public:
	virtual void Refresh();
	virtual void CloseExpandableArea();

	UPROPERTY(BlueprintReadWrite, Meta=(BindWidget))
	URTSOVectorSelectorBox* SelectorBox = nullptr;	

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FRTSOVectorSelectorData Data;

	void SetOnVectorUpdated(const FOnVectorValueUpdated& OnVectorUpdated);
};


UCLASS(Blueprintable)
class RTSOPEN_API URTSODropdownBase : public UWidget
{
	GENERATED_BODY()
public:
	virtual TSharedRef<SWidget> RebuildWidget() override 
	{
		return Super::RebuildWidget();
	}
	virtual void ReleaseSlateResources(bool bReleaseChildren) override 
	{
		Super::ReleaseSlateResources(bReleaseChildren);
	}
};

UCLASS(Blueprintable)
class RTSOPEN_API URTSODropdown : public UUserWidget
{
	GENERATED_BODY()
public:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;
	void Refresh();

	UPROPERTY(BlueprintReadOnly, Category="Widgets", Meta=(BindWidget))
	class UNamedSlot* WidgetSlot = nullptr;

	TSharedPtr<class SExpandableArea> ExpandableWidget;
	TSharedPtr<class SWidget> SlateWidget;
};


//
// Settings Entry Widgets (UMG)
DECLARE_DELEGATE_OneParam(FRTSOInputSelectorKeyUpdated, const FRTSOSettingsKeyData&)
UCLASS(Blueprintable)
class RTSOPEN_API URTSOInputSelector : public UCommonActivatableWidget
{
	GENERATED_BODY()
public:
	const FRTSOSettingsKeyData& UpdateKeyData(ERTSOSettingsKeySource KeySource, const FInputChord& InputChord, bool bAltKey);
	void UpdateAllKeyValues(const FRTSOSettingsDataSelector& ReadData);

	UPROPERTY(BlueprintReadWrite, Meta=(BindWidget))
	class UInputKeySelector* MainKeyboardMouseKey = nullptr;
	UPROPERTY(BlueprintReadWrite, Meta=(BindWidget))
	class UInputKeySelector* MainGenericGamepadKey = nullptr;
	UPROPERTY(BlueprintReadWrite, Meta=(BindWidget))
	class UInputKeySelector* MainDS45Key = nullptr;
	UPROPERTY(BlueprintReadWrite, Meta=(BindWidget))
	class UInputKeySelector* MainXSXKey = nullptr;	

	UPROPERTY(BlueprintReadWrite, Meta=(BindWidget))
	class UInputKeySelector* AltKeyboardMouseKey = nullptr;
	UPROPERTY(BlueprintReadWrite, Meta=(BindWidget))
	class UInputKeySelector* AltGenericGamepadKey = nullptr;
	UPROPERTY(BlueprintReadWrite, Meta=(BindWidget))
	class UInputKeySelector* AltDS45Key = nullptr;
	UPROPERTY(BlueprintReadWrite, Meta=(BindWidget))
	class UInputKeySelector* AltXSXKey = nullptr;		

	FRTSOInputSelectorKeyUpdated OnInputUpdated;
	FRTSOSettingsKeyData LocalKeydataStore;
};



UENUM(Blueprintable, BlueprintType)
enum class ERTSOSettingsWidgetIndex : uint8
{
	AsRangedSelector = 0,
	AsCheckBox,
	AsStringSelector,
	AsVectorSelector,
	AsKeySelector,
};

DECLARE_MULTICAST_DELEGATE_OneParam(FPDPostCheckBoxStateChanged, bool);
/** @brief Game Menu - Settings Entry */
UCLASS(Blueprintable)
class URTSOMenuWidget_SettingsEntry : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	/** @brief TODO */
	virtual void NativePreConstruct() override;
	/** @brief TODO */
	virtual void NativeOnActivated() override;

	/** @brief TODO */
	UFUNCTION()
	void OnCheckBoxSet(bool bNewState);

	/** @brief TODO */
	UFUNCTION()
	void OnKeySelected(const FRTSOSettingsKeyData& NewKeyData);
	UFUNCTION() void OnMainKeySelected_KBM(const FInputChord& InputChord);
	UFUNCTION() void OnMainKeySelected_GenericGamepad(const FInputChord& InputChord);
	UFUNCTION() void OnMainKeySelected_DS45(const FInputChord& InputChord);
	UFUNCTION() void OnMainKeySelected_XSX(const FInputChord& InputChord);	
	UFUNCTION() void OnAltKeySelected_KBM(const FInputChord& InputChord);
	UFUNCTION() void OnAltKeySelected_GenericGamepad(const FInputChord& InputChord);
	UFUNCTION() void OnAltKeySelected_DS45(const FInputChord& InputChord);
	UFUNCTION() void OnAltKeySelected_XSX(const FInputChord& InputChord);		
	
	/** @brief TODO */
	UPROPERTY(BlueprintReadWrite, Meta=(BindWidget))
	UCommonTextBlock* SettingsEntryLabel = nullptr;

	/** @brief TODO */
	UPROPERTY(BlueprintReadWrite, Meta=(BindWidget))
	UCommonTextBlock* SettingsEntryDescription = nullptr;
	
	/** @brief TODO */
	UPROPERTY(BlueprintReadWrite, Meta=(BindWidget))
	class UWidgetSwitcher* InputValueWidgetSwitcher = nullptr;

	/** @brief TODO */
	UPROPERTY(BlueprintReadWrite, Meta=(BindWidget))
	class URTSOStringSelector* AsStringSelector = nullptr;

	/** @brief TODO */
	UPROPERTY(BlueprintReadWrite, Meta=(BindWidget))
	class URTSOVectorSelector* AsVectorSelector = nullptr;

	/** @brief TODO */
	UPROPERTY(BlueprintReadWrite, Meta=(BindWidget))
	class URTSOInputSelector* AsKeySelector = nullptr;

	/** @brief TODO */
	UPROPERTY(BlueprintReadWrite, Meta=(BindWidget))
	class UCheckBox* AsCheckBox = nullptr;

	/** @brief TODO */
	UPROPERTY(BlueprintReadWrite, Meta=(BindWidget))
	class UPDRangedSelector* RangedSelector = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ERTSOSettingsWidgetIndex TargetWidgetIndex;

	UPROPERTY()
	class URTSOMenuWidget_BaseSubmenu* OwnerSubmenu = nullptr;

	UPROPERTY()
	FGameplayTag SettingsTag;
};

/** @brief Game Menu - Base Submenu*/
UCLASS(Blueprintable)
class URTSOMenuWidget_SettingsCategory : public UCommonActivatableWidget
{
	GENERATED_BODY()
public:
	/** @brief TODO */
	virtual void NativeOnActivated() override;
	/** @brief TODO */
	virtual void NativePreConstruct() override;
	/** @brief TODO */
	void Refresh(TSubclassOf<URTSOMenuWidget_SettingsEntry> InEntryClass, FString InEntryCategoryName, const TMap<FGameplayTag, FRTSOSettingsDataSelector>& InEntriesData);
	
	/** @brief TODO */
	UPROPERTY(BlueprintReadWrite, Meta=(BindWidget))
	UCommonTextBlock* SettingsCategoryLabel = nullptr;

	/** @brief TODO */
	UPROPERTY(BlueprintReadWrite, Meta=(BindWidget))
	UVerticalBox* ItemContentBox  = nullptr;
	
	/** @brief TODO */
	UPROPERTY()
	TSubclassOf<URTSOMenuWidget_SettingsEntry> EntryClass;
	/** @brief TODO */
	UPROPERTY()
	FString EntryCategoryName;
	/** @brief TODO */
	UPROPERTY()
	TMap<FGameplayTag, FRTSOSettingsDataSelector> EntriesData;

	UPROPERTY()
	class URTSOMenuWidget_BaseSubmenu* OwnerSubmenu = nullptr;  	
};

/** @brief Game Menu - Base Menu*/
UCLASS(Blueprintable)
class URTSOMenuWidget_BaseMenu : public UCommonActivatableWidget
{
	GENERATED_BODY()

protected:
	/** @brief TODO */
	virtual void NativePreConstruct() override;
	/** @brief TODO */
	virtual void NativeOnActivated() override;
	/** @brief TODO */
	// virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const;
public:
	
	/** @brief TODO */
	UPROPERTY(BlueprintReadWrite, Meta=(BindWidget))
	UCanvasPanel* BaseCanvasPanel  = nullptr;	

	/** @brief TODO */
	UPROPERTY(BlueprintReadWrite, Meta=(BindWidget))
	UBorder* MenuTitleBorder = nullptr;
	/** @brief TODO */
	UPROPERTY(BlueprintReadWrite, Meta=(BindWidget))
	UCommonTextBlock* MenuTitleLabel = nullptr;

	/** @brief TODO */
	UPROPERTY(BlueprintReadWrite, Meta=(BindWidget))
	UHorizontalBox* SubmenuSelector = nullptr;

	/** @brief TODO */
	UPROPERTY(BlueprintReadWrite, Meta=(BindWidget))
	UNamedSlot* InnerContent = nullptr;

};


/** @brief Game Menu - Base Submenu*/
UCLASS(Blueprintable)
class URTSOMenuWidget_BaseSubmenu : public UCommonActivatableWidget
{
	GENERATED_BODY()

	virtual void NativeOnActivated() override;

public:
	/** @brief TODO */
	UPROPERTY(BlueprintReadWrite, Meta=(BindWidget))
	UCanvasPanel* BaseCanvasPanel  = nullptr;	

	/** @brief TODO */
	UPROPERTY(BlueprintReadWrite, Meta=(BindWidget))
	UHorizontalBox* ViewSplit  = nullptr;
	/** @brief TODO */
	UPROPERTY(BlueprintReadWrite, Meta=(BindWidget))
	UBorder* ContentBorder = nullptr;
	/** @brief TODO */
	UPROPERTY(BlueprintReadWrite, Meta=(BindWidget))
	UVerticalBox* ContentBox  = nullptr;
	
	/** @brief TODO */
	UPROPERTY(BlueprintReadWrite, Meta=(BindWidget))
	UBorder* DetailsBorder = nullptr;
	/** @brief TODO */
	UPROPERTY(BlueprintReadWrite, Meta=(BindWidget))
	UCommonTextBlock* DetailsLabel = nullptr;
	/** @brief TODO */
	UPROPERTY(BlueprintReadWrite, Meta=(BindWidget))
	UCommonTextBlock* DetailsDescription = nullptr;

};

/** @brief TODO */
UCLASS(Blueprintable)
class URTSOMenuWidget_Settings : public URTSOMenuWidget_BaseMenu
{
	GENERATED_BODY()

public:
	/** @brief 
	 *  @note */
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativePreConstruct() override;
	virtual void NativeOnActivated() override;
	virtual void Refresh();
	
	UCommonActivatableWidget* SpawnSettingsCategories(const TMap<FString, TMap<FGameplayTag, FRTSOSettingsDataSelector>>& SettingsCategories);
	
	FORCEINLINE void SetTabContent(const FName& TabName, const TMap<FString, TMap<FGameplayTag, FRTSOSettingsDataSelector>>& SettingsCategories);
	UFUNCTION() void OnCategoryPressed_Gameplay();
	UFUNCTION() void OnCategoryPressed_Video();	
	UFUNCTION() void OnCategoryPressed_Audio();	
	UFUNCTION() void OnCategoryPressed_Controls();	
	UFUNCTION() void OnCategoryPressed_Interface();		

	
	/** @brief TODO */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UPDGenericButton> TabButtonClass;
	
	/** @brief TODO */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<URTSOMenuWidget_BaseSubmenu> SubMenuClass;
	
	/** @brief TODO */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<URTSOMenuWidget_SettingsCategory> SettingsCategoryClass;
	/** @brief TODO */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<URTSOMenuWidget_SettingsEntry> SettingsEntryClass;

	/** @brief TODO */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ERTSOSettings DefaultView = ERTSOSettings::Gameplay;

	UPROPERTY()
	TMap<FName, UCommonActivatableWidget*> SpawnedSubmenus;



	// UPROPERTY()
	// TMap<FString, TMap<FGameplayTag, FRTSOSettingsDataSelector>> SettingsData;
};

/** @brief Game Start Menu base widget*/
UCLASS(Blueprintable)
class URTSOMenuWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	/** @brief Clears delegates on the buttons hitboxes */
	void ClearDelegates() const;
	
	/** @brief The resume button,
	 *  @note is bound by the OwningMenuBaseWidget to to a function which removes the menu and resumes the game */
	UPROPERTY(BlueprintReadWrite, Meta=(BindWidget))
	class URTSOMenuButton* ResumeButton = nullptr;

	/** @brief The settings button,
	 *  @note will add the settings menu to the widget stack of the owning menu widget base */
	UPROPERTY(BlueprintReadWrite, Meta=(BindWidget))
	class URTSOMenuButton* SettingsButton = nullptr;

	/** @brief Invoking the save-game editor,
	 *  @note will add the save menu editor to the widget stack of the owning menu widget base */
	UPROPERTY(BlueprintReadWrite, Meta=(BindWidget))
	class URTSOMenuButton* SaveEditor = nullptr;	
	
	/** @brief The save-game button,
	 *  @note will add the save menu to the widget stack of the owning menu widget base */
	UPROPERTY(BlueprintReadWrite, Meta=(BindWidget))
	class URTSOMenuButton* SaveButton = nullptr;

	/** @brief The load-game button,
	 *  @brief add the load menu to the widget stack of the owning menu widget base */
	UPROPERTY(BlueprintReadWrite, Meta=(BindWidget))
	class URTSOMenuButton* LoadButton = nullptr;

	/** @brief The quit-game button quits the actual game
	 *  @todo set up dialog widget to make sure the player wants to quit if they have not saved the game */
	UPROPERTY(BlueprintReadWrite, Meta=(BindWidget))
	class URTSOMenuButton* QuitButton = nullptr;

	/** @brief Contains the buttons in a vertical direction */
	UPROPERTY(BlueprintReadWrite, Meta=(BindWidget))
	class UVerticalBox* VerticalBoxContainer = nullptr;

	/** @brief Unused for now, reserve for later use */
	UPROPERTY()
	class URTSOMainMenuBase* OwningMenuBaseWidget = nullptr;	
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
