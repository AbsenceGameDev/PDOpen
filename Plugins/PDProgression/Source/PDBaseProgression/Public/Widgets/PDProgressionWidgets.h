/* @author: Ario Amin @ Permafrost Development. @copyright: Full BSL(1.1) License included at bottom of the file  */

#pragma once

#include "CoreMinimal.h"
#include "PDProgressionSharedUI.h"
#include "Net/PDProgressionNetDatum.h"
#include "Subsystems/EngineSubsystem.h"

#include "Blueprint/UserWidget.h"
#include "Components/Widget.h"
#include "Layout/Geometry.h"
#include "Input/Reply.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"

#include "PDProgressionWidgets.generated.h"

// Don't long lists of forward declares, compacting the list into two lines, one for slatecore stuff, the other for our own stuff
class SCanvas; class SWrapBox; class FPaintArgs; class FSlateWindowElementList; struct FSlateBrush;
class SPDSelectedStat_OffsetData; class SPDSelectedStat_LevelData; class SPDStatList; struct FPDStatViewAffectedStat; struct FPDStatViewModifySource;

DECLARE_DYNAMIC_DELEGATE_RetVal(int, FOwnerIDDelegate);

/** @brief Inner uwidget which houses our 'stat' slate widgets, and exposes some editor controls to them  */
UCLASS(Blueprintable)
class PDBASEPROGRESSION_API UPDStatListInnerWidget : public UWidget
{
	GENERATED_BODY()
public:
	/** @brief UE Boiler-plate, instantiates  'InnerSlateWrapbox' (a SWrapBox) and our 'InnerStatList' which is added to a slot in the wrapbox.
	 * @return the sharedref of the 'InnerSlateWrapbox */
	virtual TSharedRef<SWidget> RebuildWidget() override;
	/** @brief UE Boiler-plate, Resets InnerSlateWrapbox and InnerStatList then calls super */
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

	/** @brief Calls RefreshInnerStatList after ensuring either 'EditorTestEntries_BaseList' or  'SectionWidth' has changed */
	void RefreshStatListOnChangedProperty(FPropertyChangedEvent& PropertyChangedEvent);
	/** @brief Calls 'RefreshStatListOnChangedProperty', passes along 'PropertyChangedEvent' */
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	/** @brief Calls 'RefreshStatListOnChangedProperty', passes along 'PropertyChangedEvent' */
	virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent) override;
	/** @brief Calls super, otherwise unused. Reserved for later use. */
	virtual void SynchronizeProperties() override;
	/** @brief Calls super, otherwise unused. Reserved for later use. */
	virtual void OnBindingChanged(const FName& Property) override;

	/** @brief Sets the stat-list visibility to 'Collapsed'. Exposing slate widget controls to BP */
	UFUNCTION(BlueprintCallable)
	void SetVisibility_StatList(ESlateVisibility NewVisibility);
	/** @brief Sets the stat-offset popup visibility to 'Collapsed'. Exposing slate widget controls to BP */
	UFUNCTION(BlueprintCallable)
	void SetVisibility_StatOffsets(ESlateVisibility NewVisibility);
	/** @brief Sets the stat-level popup visibility to 'Collapsed'. Exposing slate widget controls to BP */
	UFUNCTION(BlueprintCallable)
	void SetVisibility_StatLevelData(ESlateVisibility NewVisibility);

	/** @brief Updates the stat-list main widget size limits. Exposing slate widget controls to BP */
	UFUNCTION(BlueprintCallable)
	void SetSizeLimits_StatListDataView(int32 SlateUnitSizeX, int32 SlateUnitSizeY);
	
	/** @brief Updates the offset popup size limits. Exposing slate widget controls to BP */
	UFUNCTION(BlueprintCallable)
	void SetSizeLimits_OffsetsDataView(int32 SlateUnitSizeX, int32 SlateUnitSizeY);
	
	/** @brief Updates the stat-level popup size limits. Exposing slate widget controls to BP */
	UFUNCTION(BlueprintCallable)
	void SetSizeLimits_LevelDataView(int32 SlateUnitSizeX, int32 SlateUnitSizeY);

	/** @brief Updates the stat-list main widget fonts. Exposing slate widget controls to BP */
	UFUNCTION(BlueprintCallable)		
	void SetFonts_StatListDataView(const FSlateFontInfo& TitleFont, const FSlateFontInfo& SubTitleFont,
		const FSlateFontInfo& TableHeaderFont, const FSlateFontInfo& TableEntryFont);
	
	/** @brief Updates the stat-level popup fonts. Exposing slate widget controls to BP */
	UFUNCTION(BlueprintCallable)	
	void SetFonts_LevelDataView(const FSlateFontInfo& TitleFont, const FSlateFontInfo& SubTitleFont,
		const FSlateFontInfo& TableHeaderFont, const FSlateFontInfo& TableEntryFont);

	/** @brief Updates the offset popup fonts. Exposing slate widget controls to BP */
	UFUNCTION(BlueprintCallable)		
	void SetFonts_OffsetsDataView(const FSlateFontInfo& TitleFont, const FSlateFontInfo& SubTitleFont,
		const FSlateFontInfo& TableHeaderFont, const FSlateFontInfo& TableEntryFont);

protected:
	/** @brief Updates our dataview with entries found in our input editor test entries array,
	 * @details this allows us to populate our inner list views in design-time for testing how elements are displayed */
	template<typename TDataViewElem>
	void UpdateDataViewWithEditorTestEntries(
		TArray<TSharedPtr<TDataViewElem>>& DataView,
		const TArray<TDataViewElem>& EditorTestEntries)
	{
#if WITH_EDITOR
		if (IsDesignTime() == false) { return; }

		DataView.Empty();
		for (const TDataViewElem& EditorEntry : EditorTestEntries)
		{
			TSharedRef<TDataViewElem> SharedNetDatum = MakeShared<TDataViewElem>(EditorEntry);
			DataView.Emplace(SharedNetDatum);			
		}
#endif // WITH_EDITOR	
	}

	
public:	
	
	/** @brief Calls Refresh on 'InnerStatList'.
	 * @note Additionally, if in design-time in editor then it also re-fills 'NetDataView' from the test data in 'EditorTestEntries_BaseList'  */
	void RefreshInnerStatList();

	/** @brief Calls Refresh on 'SelectedStatOffsetData_PopUp'.
	 * @note Additionally, if in design-time in editor then it also re-fills 'ModifyingSourcesDataView' from the test data in 'EditorTestEntries_OffsetPopup_ModifySources'  */
	void RefreshStatOffset_Popup();	
	/** @brief Calls Refresh on 'SelectedStatLevelData_PopUp'.
	 * @note Additionally, if in design-time in editor then it also re-fills 'TokenDataView' from the test data in 'EditorTestEntries_LevelPopup_TokenData'  */
	void RefreshStatLevel_Popup();

	/** @brief Setter for OwnerID. OwnerID is used for routing the widget to the correct player. If more than one player is in the same client */
	UFUNCTION(BlueprintCallable)
	virtual void UpdateOwner(int32 NewOwner);
	
	/** @deprecated !!Remove in coming commits!! @brief Wrapbox that wraps our SPDStatList derived widgets */
	TSharedPtr<SWrapBox> InnerSlateWrapbox = nullptr;
	
	/** @brief Base ptr to a SPDStatList widget */
	TSharedPtr<SPDStatList> InnerStatList = nullptr;

	/** @brief Canvas that our stat widgets are contained within */
	TSharedPtr<SCanvas> SlateCanvas = nullptr;
	
	/** @brief Popup widget that is used to display a stats levelling data
	 * @done Write Supporting code to actually open this as an interactable window */
	TSharedPtr<SPDSelectedStat_LevelData> SelectedStatLevelData_PopUp;

	/** @brief Popup widget that is used to display a stats sources of current offsets
	 * @done Write Supporting code to actually open this as an interactable window */
	TSharedPtr<SPDSelectedStat_OffsetData> SelectedStatOffsetData_PopUp;	

	/** @brief ID to route to our owner.  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 OwnerID = 0;

	/** @brief Delegate used to resolve the owner ID */
	UPROPERTY()
	FOwnerIDDelegate OwnerIDDelegate;
	
	/** @brief Editor test data. Used to test/inspect in visual design editor */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Progression|Widgets|Stat List")
	TArray<FPDStatNetDatum> EditorTestEntries_BaseList{};
	
	/** @brief Editor test data for pop-up view of offset sources. Used to test/inspect in visual design editor */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Progression|Widgets|Modify Sources")
	TArray<FPDStatViewModifySource> EditorTestEntries_OffsetPopup_ModifySources{};
	
	/** @brief Editor test data for pop-up view of levelling data -- tokens. Used to test/inspect in visual design editor */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Progression|Widgets|Level Data")
	TArray<FPDSkillTokenBase> EditorTestEntries_LevelPopup_TokenData{};
	/** @brief Editor test data for pop-up view of levelling data -- affected stats. Used to test/inspect in visual design editor */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Progression|Widgets|Level Data")
	TArray<FPDStatViewAffectedStat> EditorTestEntries_LevelPopup_AffectedStatsData{};
	
	//
	// Property bindables
	
	/** @brief Sets up editor binding callbacks, available to set up from the visual design editor */
	PROPERTY_BINDING_IMPLEMENTATION(int32, OwnerID);
	
	//
	// Widget controls

	/** @brief ModifySourcesPopup - BP/editor Exposed controls */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Progression|Widgets|Modify Sources")
	FPDWidgetBaseSettings Settings_ModifySourcesPopup;
	
	/** @brief LevellingDataPopup - BP/editor Exposed controls */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Progression|Widgets|Level Data")
	FPDWidgetBaseSettings Settings_LevellingDataPopup;	

	/** @brief BaseStatList - BP/editor Exposed controls */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Progression|Widgets|Stat List")
	FPDWidgetBaseSettings Settings_BaseStatList;		

	
protected:	
	//
	// Dataviews
	/** @brief Data-view of our netdatums */
	TArray<TSharedPtr<FPDStatNetDatum>> NetDataView{};

	/** @brief Data-view of our sources of offsets */
	TArray<TSharedPtr<FPDStatViewModifySource>> ModifyingSourcesDataView{};
	
	/** @brief Data-view of our expected tokens next level */
	TArray<TSharedPtr<FPDSkillTokenBase>> TokenDataView;
	
	/** @brief Data-view of our affected stats and effect amount when we reach  next level */
	TArray<TSharedPtr<FPDStatViewAffectedStat>> AffectedStatsDataView;
	
};

/** @brief User facing widget. Mainly just a wrapper for above UWidget */
UCLASS(Blueprintable)
class PDBASEPROGRESSION_API UPDStatListUserWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	DECLARE_DELEGATE_RetVal(int, FOwnerIDDelegate)
	
	/** @brief Updates the owner of 'InnerStatList' */
	UFUNCTION()
	virtual void NativePreConstruct() override;

	/** @brief Executes 'OwnerIDDelegate' if bound. Logs a warning int is yet to be bound */
	UFUNCTION()
	virtual int32 GetOwnerID();

	/** @brief Bind ot a function of choice which resolves the owner to som reliably persistent ID */
	FOwnerIDDelegate OwnerIDDelegate;
	
	/** @brief Enforces creation of a 'UPDStatListInnerWidget' via BindWidget meta specifier*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (BindWidget))
	UPDStatListInnerWidget* InnerStatList;
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
