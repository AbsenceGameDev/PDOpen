/* @author: Ario Amin @ Permafrost Development. @copyright: Full BSL(1.1) License included at bottom of the file  */

#include "Widgets/PDProgressionWidgets.h"
#include "Widgets/Slate/SPDSelectedStat.h"
#include "Widgets/Slate/SPDStatList.h"
#include "PDProgressionSharedUI.h"

#include "Textures/SlateIcon.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/SCanvas.h"
#include "Framework/Commands/UIAction.h"

// Class picker 
#include "Subsystems/PDProgressionSubsystem.h"

#define LOCTEXT_NAMESPACE "PDStatList"

void UPDStatListInnerWidget::RefreshStatListOnChangedProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	const FName PropertyName = PropertyChangedEvent.GetPropertyName();
	const FName PotentialStructPropertyName = PropertyChangedEvent.GetMemberPropertyName();
	
	if (PropertyName.IsEqual(GET_MEMBER_NAME_CHECKED(UPDStatListInnerWidget, EditorTestEntries_BaseList)))
	{
		RefreshInnerStatList();
		return;
	}

	if (PropertyName.IsEqual(GET_MEMBER_NAME_CHECKED(UPDStatListInnerWidget, EditorTestEntries_LevelPopup_TokenData))
		|| PropertyName.IsEqual(GET_MEMBER_NAME_CHECKED(UPDStatListInnerWidget, EditorTestEntries_LevelPopup_AffectedStatsData)))
	{
		RefreshStatLevel_Popup();
		return;
	}

	if (PropertyName.IsEqual(GET_MEMBER_NAME_CHECKED(UPDStatListInnerWidget, EditorTestEntries_OffsetPopup_ModifySources)))
	{
		RefreshStatOffset_Popup();
		return;
	}		

	if (PotentialStructPropertyName.IsEqual(GET_MEMBER_NAME_CHECKED(UPDStatListInnerWidget, Settings_BaseStatList)))
	{
		if (PropertyName.IsEqual(GET_MEMBER_NAME_CHECKED(FPDWidgetBaseSettings, Visibility)))
		{
			SetVisibility_StatLevelData(Settings_BaseStatList.Visibility);
			return;
		}

		if (PropertyName.IsEqual(GET_MEMBER_NAME_CHECKED(FPDWidgetBaseSettings, DataViewSectionWidth)))
		{
			RefreshInnerStatList();
			return;
		}		

		if (PropertyName.IsEqual(GET_MEMBER_NAME_CHECKED(FPDWidgetBaseSettings, MaxSUWidth))
		 || PropertyName.IsEqual(GET_MEMBER_NAME_CHECKED(FPDWidgetBaseSettings, MaxSUHeight)))
		{
			SetSizeLimits_StatListDataView(Settings_BaseStatList.MaxSUWidth, Settings_BaseStatList.MaxSUHeight);
			return;
		}

		if (PropertyName.IsEqual(GET_MEMBER_NAME_CHECKED(FSlateFontInfo, FontObject)))
		{
			SetFonts_StatListDataView(Settings_BaseStatList.TitleFont, Settings_BaseStatList.SubTitleFont,
				Settings_BaseStatList.TableHeaderFont, Settings_BaseStatList.TableEntryFont);
			return;
		}
		return;
	}
	
	if (PotentialStructPropertyName.IsEqual(GET_MEMBER_NAME_CHECKED(UPDStatListInnerWidget, Settings_LevellingDataPopup)))
	{
		if (PropertyName.IsEqual(GET_MEMBER_NAME_CHECKED(FPDWidgetBaseSettings, Visibility)))
		{
			SetVisibility_StatLevelData(Settings_LevellingDataPopup.Visibility);
			return;
		}

		if (PropertyName.IsEqual(GET_MEMBER_NAME_CHECKED(FPDWidgetBaseSettings, DataViewSectionWidth)))
		{
			RefreshStatLevel_Popup();
			return;
		}				
		
		if (PropertyName.IsEqual(GET_MEMBER_NAME_CHECKED(FPDWidgetBaseSettings, MaxSUWidth))
		 || PropertyName.IsEqual(GET_MEMBER_NAME_CHECKED(FPDWidgetBaseSettings, MaxSUHeight)))
		{
			SetSizeLimits_LevelDataView(Settings_LevellingDataPopup.MaxSUWidth, Settings_LevellingDataPopup.MaxSUHeight);
			return;
		}

		if (PropertyName.IsEqual(GET_MEMBER_NAME_CHECKED(FSlateFontInfo, FontObject)))
		{
			SetFonts_LevelDataView(Settings_LevellingDataPopup.TitleFont, Settings_LevellingDataPopup.SubTitleFont,
				Settings_LevellingDataPopup.TableHeaderFont, Settings_LevellingDataPopup.TableEntryFont);
			return;
		}			
		
		return;
	}
	
	if (PotentialStructPropertyName.IsEqual(GET_MEMBER_NAME_CHECKED(UPDStatListInnerWidget, Settings_ModifySourcesPopup)))
	{
		if (PropertyName.IsEqual(GET_MEMBER_NAME_CHECKED(FPDWidgetBaseSettings, Visibility)))
		{
			SetVisibility_StatOffsets(Settings_ModifySourcesPopup.Visibility);
			return;
		}

		if (PropertyName.IsEqual(GET_MEMBER_NAME_CHECKED(FPDWidgetBaseSettings, DataViewSectionWidth)))
		{
			RefreshStatOffset_Popup();
			return;
		}					
		
		if (PropertyName.IsEqual(GET_MEMBER_NAME_CHECKED(FPDWidgetBaseSettings, MaxSUWidth))
		 || PropertyName.IsEqual(GET_MEMBER_NAME_CHECKED(FPDWidgetBaseSettings, MaxSUHeight)))
		{
			SetSizeLimits_OffsetsDataView(Settings_ModifySourcesPopup.MaxSUWidth, Settings_ModifySourcesPopup.MaxSUHeight);
			return;
		}

		if (PropertyName.IsEqual(GET_MEMBER_NAME_CHECKED(FSlateFontInfo, FontObject)))
		{
			SetFonts_OffsetsDataView(Settings_ModifySourcesPopup.TitleFont, Settings_ModifySourcesPopup.SubTitleFont,
				Settings_ModifySourcesPopup.TableHeaderFont, Settings_ModifySourcesPopup.TableEntryFont);
		}			
	}
}

void UPDStatListInnerWidget::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	RefreshStatListOnChangedProperty(PropertyChangedEvent);
	Super::PostEditChangeProperty(PropertyChangedEvent);
}

void UPDStatListInnerWidget::PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent)
{
	RefreshStatListOnChangedProperty(PropertyChangedEvent);
	Super::PostEditChangeChainProperty(PropertyChangedEvent);
}

void UPDStatListInnerWidget::SynchronizeProperties()
{
	Super::SynchronizeProperties();
}

void UPDStatListInnerWidget::OnBindingChanged(const FName& Property)
{
	Super::OnBindingChanged(Property);
}

//
// @todo clean up re-used functionality

void UPDStatListInnerWidget::SetVisibility_StatList(ESlateVisibility NewVisibility)
{
	if (InnerStatList.IsValid() == false) { return; }
	InnerStatList->SetVisibility(ConvertSerializedVisibilityToRuntime(NewVisibility));
	InnerStatList->Invalidate(EInvalidateWidgetReason::Paint);
}

void UPDStatListInnerWidget::SetVisibility_StatOffsets(ESlateVisibility NewVisibility)
{
	if (SelectedStatOffsetData_PopUp.IsValid() == false) { return; }
	SelectedStatOffsetData_PopUp->SetVisibility(ConvertSerializedVisibilityToRuntime(NewVisibility));	
	SelectedStatOffsetData_PopUp->Invalidate(EInvalidateWidgetReason::Paint);
}

void UPDStatListInnerWidget::SetVisibility_StatLevelData(ESlateVisibility NewVisibility)
{
	if (SelectedStatLevelData_PopUp.IsValid() == false) { return; }
	SelectedStatLevelData_PopUp->SetVisibility(ConvertSerializedVisibilityToRuntime(NewVisibility));
	SelectedStatLevelData_PopUp->Invalidate(EInvalidateWidgetReason::Paint);
}

//
// @todo clean up re-used functionality
void UPDStatListInnerWidget::SetSizeLimits_StatListDataView(int32 SlateUnitSizeX, int32 SlateUnitSizeY)
{
	if (InnerStatList.IsValid() == false) { return; }

	FPDWidgetBaseSettings& Settings = InnerStatList->RetrieveMutableSettings();
	Settings.MaxSUWidth = SlateUnitSizeX;
	Settings.MaxSUHeight = SlateUnitSizeY;

	InnerStatList->UpdateChildSlot();
	InnerStatList->Invalidate(EInvalidateWidgetReason::Paint);
}

void UPDStatListInnerWidget::SetSizeLimits_OffsetsDataView(int32 SlateUnitSizeX, int32 SlateUnitSizeY)
{
	if (SelectedStatOffsetData_PopUp.IsValid() == false) { return; }

	FPDWidgetBaseSettings& Settings = SelectedStatOffsetData_PopUp->RetrieveMutableSettings();
	Settings.MaxSUWidth = SlateUnitSizeX;
	Settings.MaxSUHeight = SlateUnitSizeY;
	
	SelectedStatOffsetData_PopUp->UpdateChildSlot();
	SelectedStatOffsetData_PopUp->Invalidate(EInvalidateWidgetReason::Paint);
}

void UPDStatListInnerWidget::SetSizeLimits_LevelDataView(int32 SlateUnitSizeX, int32 SlateUnitSizeY)
{
	if (SelectedStatLevelData_PopUp.IsValid() == false) { return; }

	
	FPDWidgetBaseSettings& Settings = SelectedStatLevelData_PopUp->RetrieveMutableSettings();
	Settings.MaxSUWidth = SlateUnitSizeX;
	Settings.MaxSUHeight = SlateUnitSizeY;

	SelectedStatLevelData_PopUp->UpdateChildSlot();
	SelectedStatLevelData_PopUp->Invalidate(EInvalidateWidgetReason::Paint);
}

void UPDStatListInnerWidget::SetFonts_StatListDataView(const FSlateFontInfo& TitleFont, const FSlateFontInfo& SubTitleFont,
	const FSlateFontInfo& TableHeaderFont, const FSlateFontInfo& TableEntryFont)
{
	if (InnerStatList.IsValid() == false) { return; }

	FPDWidgetBaseSettings& Settings = InnerStatList->RetrieveMutableSettings();
	Settings.TitleFont = TitleFont;
	Settings.SubTitleFont = SubTitleFont;
	Settings.TableHeaderFont = TableHeaderFont;
	Settings.TableEntryFont = TableEntryFont;

	
	InnerStatList->UpdateChildSlot();
	InnerStatList->Invalidate(EInvalidateWidgetReason::Paint);	
}

void UPDStatListInnerWidget::SetFonts_LevelDataView(const FSlateFontInfo& TitleFont, const FSlateFontInfo& SubTitleFont,
	const FSlateFontInfo& TableHeaderFont, const FSlateFontInfo& TableEntryFont)
{
	if (SelectedStatLevelData_PopUp.IsValid() == false) { return; }

	FPDWidgetBaseSettings& Settings = SelectedStatLevelData_PopUp->RetrieveMutableSettings();
	Settings.TitleFont = TitleFont;
	Settings.SubTitleFont = SubTitleFont;
	Settings.TableHeaderFont = TableHeaderFont;
	Settings.TableEntryFont = TableEntryFont;

	SelectedStatLevelData_PopUp->UpdateChildSlot();
	SelectedStatLevelData_PopUp->Invalidate(EInvalidateWidgetReason::Paint);
}

void UPDStatListInnerWidget::SetFonts_OffsetsDataView(const FSlateFontInfo& TitleFont, const FSlateFontInfo& SubTitleFont,
	const FSlateFontInfo& TableHeaderFont, const FSlateFontInfo& TableEntryFont)
{
	if (SelectedStatOffsetData_PopUp.IsValid() == false) { return; }
	
	FPDWidgetBaseSettings& Settings = SelectedStatOffsetData_PopUp->RetrieveMutableSettings();
	Settings.TitleFont = TitleFont;
	Settings.SubTitleFont = SubTitleFont;
	Settings.TableHeaderFont = TableHeaderFont;
	Settings.TableEntryFont = TableEntryFont;
	
	SelectedStatOffsetData_PopUp->UpdateChildSlot();
	SelectedStatOffsetData_PopUp->Invalidate(EInvalidateWidgetReason::Paint);	
}

void UPDStatListInnerWidget::RefreshInnerStatList()
{
	NetDataView.Empty();
	if (InnerStatList.IsValid())
	{
		UpdateDataViewWithEditorTestEntries(NetDataView, EditorTestEntries_BaseList);
		InnerStatList->Refresh(INDEX_NONE, NetDataView, Settings_BaseStatList);
	}
}

void UPDStatListInnerWidget::RefreshStatLevel_Popup()
{
	if (SelectedStatLevelData_PopUp.IsValid())
	{
		UpdateDataViewWithEditorTestEntries(TokenDataView, EditorTestEntries_LevelPopup_TokenData);
		UpdateDataViewWithEditorTestEntries(AffectedStatsDataView, EditorTestEntries_LevelPopup_AffectedStatsData);
		
		SelectedStatLevelData_PopUp->Refresh(INDEX_NONE, TokenDataView, AffectedStatsDataView, Settings_LevellingDataPopup);
	}
}

void UPDStatListInnerWidget::RefreshStatOffset_Popup()
{
	if (SelectedStatOffsetData_PopUp.IsValid())
	{
		UpdateDataViewWithEditorTestEntries(ModifyingSourcesDataView, EditorTestEntries_OffsetPopup_ModifySources);
		SelectedStatOffsetData_PopUp->Refresh(INDEX_NONE, ModifyingSourcesDataView,  Settings_ModifySourcesPopup);
	}
}

TSharedRef<SWidget> UPDStatListInnerWidget::RebuildWidget()
{
	if (InnerSlateWrapbox.IsValid() == false)
	{
		InnerSlateWrapbox = SNew(SWrapBox);
	}
	if (InnerStatList.IsValid() == false)
	{
		UpdateDataViewWithEditorTestEntries(NetDataView, EditorTestEntries_BaseList);
		InnerStatList = SNew(SPDStatList, OwnerID, NetDataView, Settings_BaseStatList);
	}
	SetVisibility_StatList(Settings_BaseStatList.Visibility);

	static const FGameplayTag DummyStarterTag = FGameplayTag{};
	if (SelectedStatLevelData_PopUp.IsValid()  == false)
	{
		UpdateDataViewWithEditorTestEntries(TokenDataView, EditorTestEntries_LevelPopup_TokenData);
		UpdateDataViewWithEditorTestEntries(AffectedStatsDataView, EditorTestEntries_LevelPopup_AffectedStatsData);
		SelectedStatLevelData_PopUp = SNew(SPDSelectedStat_LevelData, OwnerID, DummyStarterTag ,TokenDataView, AffectedStatsDataView, Settings_LevellingDataPopup);
	}
	SetVisibility_StatLevelData(Settings_LevellingDataPopup.Visibility);

	if (SelectedStatOffsetData_PopUp.IsValid()  == false)
	{
		UpdateDataViewWithEditorTestEntries(ModifyingSourcesDataView, EditorTestEntries_OffsetPopup_ModifySources);
		SelectedStatOffsetData_PopUp = SNew(SPDSelectedStat_OffsetData, OwnerID, DummyStarterTag, ModifyingSourcesDataView, Settings_ModifySourcesPopup);
	}
	SetVisibility_StatOffsets(Settings_ModifySourcesPopup.Visibility);
	
	
	InnerSlateWrapbox->ClearChildren();
	SWrapBox::FScopedWidgetSlotArguments StatList_CanvasSlot = InnerSlateWrapbox->AddSlot();
	StatList_CanvasSlot.AttachWidget(InnerStatList.ToSharedRef());

	SWrapBox::FScopedWidgetSlotArguments SelectedStatLevelDataPopup_CanvasSlot = InnerSlateWrapbox->AddSlot();
	SelectedStatLevelDataPopup_CanvasSlot.AttachWidget(SelectedStatLevelData_PopUp.ToSharedRef());

	SWrapBox::FScopedWidgetSlotArguments AffectedStatsPopup_CanvasSlot = InnerSlateWrapbox->AddSlot();
	AffectedStatsPopup_CanvasSlot.AttachWidget(SelectedStatOffsetData_PopUp.ToSharedRef());

	return InnerSlateWrapbox.ToSharedRef();
}

void UPDStatListInnerWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	InnerSlateWrapbox.Reset();
	InnerStatList.Reset();
	SelectedStatLevelData_PopUp.Reset();
	SelectedStatOffsetData_PopUp.Reset();
	
	Super::ReleaseSlateResources(bReleaseChildren);
}

void UPDStatListInnerWidget::UpdateOwner(int32 NewOwner)
{
	OwnerID = NewOwner;
}

void UPDStatListUserWidget::NativePreConstruct()
{
	if (InnerStatList != nullptr)
	{
		InnerStatList->UpdateOwner(GetOwnerID());
	}
	
	Super::NativePreConstruct();
}

int32 UPDStatListUserWidget::GetOwnerID()
{
	if (OwnerIDDelegate.IsBound() == false)
	{
		UE_LOG(PDLog_Progression, Warning, TEXT("UPDStatListUserWidget(%s)::GetOwnerID -- Failed -- OwnerIDDelegate is not bound!"), *GetName())

		return 0;
	}
	
	return OwnerIDDelegate.Execute();
}


#undef LOCTEXT_NAMESPACE

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

