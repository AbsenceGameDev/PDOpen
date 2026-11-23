/* @author: Ario Amin @ Permafrost Development. @copyright: Full BSL(1.1) License included at bottom of the file  */


#include "Widgets/RTSOActiveMainMenu.h"
#include "Subsystems/RTSOSettingsSubsystem.h"
#include "Widgets/Slate/SRTSOSettingsStringSelector.h"

#include "SlateFwd.h"
#include "CommonTabListWidgetBase.h"
#include "Components/CheckBox.h"
#include "Components/TextBlock.h"
#include "Components/WidgetSwitcher.h"
#include "Components/WidgetSwitcherSlot.h"


#include "Core/RTSOBaseGM.h"
#include "UI/PDNumberBoxes.h"
#include "Widgets/CommonActivatableWidgetContainer.h"
// #include "Components/CanvasPanel.h"
#include "CommonTextBlock.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/NamedSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/InputKeySelector.h"
// #include "Components/TextBlock.h"
// #include "Components/Image.h"

void URTSOSaveGameDialog::DialogReplyContinue(const bool bSuccess) const
{
	// ARTSOBaseGM* GM = GetWorld() != nullptr ? GetWorld()->GetAuthGameMode<ARTSOBaseGM>() : nullptr;
	// if (GM == nullptr) { return; }

	if (bSuccess) { SaveSuccessCallback.ExecuteIfBound(FString::FromInt(SlotIdx), true); }
	else { SaveFailCallback.ExecuteIfBound(FString::FromInt(SlotIdx), true); }
	
}

// void URTSOSaveGameDialog::SetupDelegates()
// {
// 	YesButton->Hitbox->OnReleased.AddDynamic(this, &URTSOSaveGameDialog::Reply_Yes);
// 	NoButton->Hitbox->OnReleased.AddDynamic(this, &URTSOSaveGameDialog::Reply_No);
//
// 	if (DialogContent == nullptr) { return; }
// 	DialogContent->SetText(DialogMessage);
// }

// void URTSOSaveGameDialog::Reply_Yes()
// {
// 	DialogReplyContinue();
// 	RemoveFromParent();
// }
//
// void URTSOSaveGameDialog::Reply_No()
// {
// 	RemoveFromParent();
// }

void URTSOMenuWidget_SaveGame::NativePreConstruct()
{
	// BaseCanvas->AddChildToCanvas(MainBox);
	// MainBox->AddChildToVerticalBox(BannerCanvas);
	// BannerCanvas->AddChildToCanvas(BannerText);
	// BannerCanvas->AddChildToCanvas(BannerImage);
	// BannerCanvas->AddChildToCanvas(ExitButton);
	// MainBox->AddChildToVerticalBox(InnerBox);
	// InnerBox->AddChildToVerticalBox(Slot0);
	// InnerBox->AddChildToVerticalBox(Slot1);
	// InnerBox->AddChildToVerticalBox(Slot2);
	// InnerBox->AddChildToVerticalBox(Slot3);
	// InnerBox->AddChildToVerticalBox(Slot4);
	Super::NativePreConstruct();
}

//
// String Selector 
TSharedRef<SWidget> URTSOStringSelectorBox::RebuildWidget()
{
	SRTSOStringSelector::FArguments StringArgs;
	StringArgs._Caller = Owner; 
	StringArgs._OnStringValueSelected = OnStringSelected;

	if (DataPtr != nullptr && DataPtr->GeneratedStringOptions.IsEmpty() == false)
	{
		StringArgs._OptionsArray = &DataPtr->GeneratedStringOptions;
	}
	SlateWidget = SArgumentNew(StringArgs, SRTSOStringSelector);
	return SlateWidget.ToSharedRef();
}
void URTSOStringSelectorBox::ReleaseSlateResources(bool bReleaseChildren)
{
	SlateWidget.Reset();
	Super::ReleaseSlateResources(bReleaseChildren);
}

void URTSOStringSelectorBox::Refresh()
{
	if (SlateWidget != nullptr && DataPtr != nullptr && DataPtr->GeneratedStringOptions.IsEmpty() == false)
	{
		SlateWidget->OnStringValueSelected = OnStringSelected;
		SlateWidget->UpdateOptions(&DataPtr->GeneratedStringOptions);
	}
}
void URTSOStringSelectorBox::CloseExpandableArea()
{
	if (SlateWidget != nullptr)
	{
		SlateWidget->UpdateExpandableArea(false);
	}
}

void URTSOStringSelector::Refresh()
{
	SelectedStringValue->SetText(FText::AsCultureInvariant(Data.SelectedString));
	SelectorBox->Owner = this;
	SelectorBox->DataPtr = &Data;
	SelectorBox->Refresh();
}

void URTSOStringSelector::CloseExpandableArea()
{
	SelectorBox->CloseExpandableArea();
}

void URTSOStringSelector::SetOnStringSelected(const FOnStringValueSelected& Delegate)
{
	SelectorBox->OnStringSelected = Delegate;
}

//
// Vector Selector
TSharedRef<SWidget> URTSOVectorSelectorBox::RebuildWidget()
{
	SRTSOVectorBase::FArguments VectorArgs;
	VectorArgs._Caller = Owner; 
	VectorArgs._OnVectorUpdated = OnVectorUpdated;

	if (DataPtr != nullptr)
	{
		VectorArgs._VectorType = FPDSettingStatics::ToVectorType(DataPtr->VectorType);
	}
	SlateWidget = SArgumentNew(VectorArgs, SRTSOVectorBase);
	return SlateWidget.ToSharedRef();
}
void URTSOVectorSelectorBox::ReleaseSlateResources(bool bReleaseChildren)
{
	SlateWidget.Reset();
	Super::ReleaseSlateResources(bReleaseChildren);
}

void URTSOVectorSelectorBox::Refresh()
{
	if (SlateWidget != nullptr && DataPtr != nullptr)
	{
		SlateWidget->OnVectorUpdated = OnVectorUpdated;
		SlateWidget->UpdateType(FPDSettingStatics::ToVectorType(DataPtr->VectorType));
	}
}
void URTSOVectorSelectorBox::CloseExpandableArea()
{
	if (SlateWidget != nullptr)
	{
		SlateWidget->UpdateExpandableArea(false);
	}
}

void URTSOVectorSelector::Refresh()
{
	SelectorBox->Owner = this;
	SelectorBox->DataPtr = &Data;
	SelectorBox->Refresh();
}

void URTSOVectorSelector::CloseExpandableArea()
{
	SelectorBox->CloseExpandableArea();
}

void URTSOVectorSelector::SetOnVectorUpdated(const FOnVectorValueUpdated& OnVectorUpdated)
{
	SelectorBox->OnVectorUpdated = OnVectorUpdated;
}

TSharedRef<SWidget> URTSODropdown::RebuildWidget()
{
	if (WidgetSlot == nullptr)
	{
		return SNew(STextBlock).Text(FText::AsCultureInvariant("ERROR"));
	}

	Refresh();
	return ExpandableWidget.ToSharedRef(); 
}

void URTSODropdown::ReleaseSlateResources(bool bReleaseChildren)
{
	SlateWidget.Reset();
	ExpandableWidget.Reset();
	Super::ReleaseSlateResources(bReleaseChildren);
}

void URTSODropdown::Refresh()
{
	check(WidgetSlot)
	SlateWidget = WidgetSlot->TakeWidget().ToSharedPtr();
	ExpandableWidget = SNew(SExpandableArea)
		.InitiallyCollapsed(IsDesignTime() ? false : true)
		.HeaderContent()[SNew(STextBlock).Text(FText::AsCultureInvariant("Key selectors"))]
		.BodyContent()
		[
			SlateWidget.ToSharedRef()
		];
}

//
// Settings Entry Widgets

// Key/Input selector
const FRTSOSettingsKeyData& URTSOInputSelector::UpdateKeyData(ERTSOSettingsKeySource KeySource, const FInputChord& InputChord, bool bAltKey)
{
	LocalKeydataStore;

	if (bAltKey)
	{
		switch(KeySource)
		{
			case ERTSOSettingsKeySource::KBM: LocalKeydataStore.Alt_KeyBoardMouse = InputChord.Key; break;
			case ERTSOSettingsKeySource::GGP: LocalKeydataStore.Alt_GenericGamepad = InputChord.Key; break;
			case ERTSOSettingsKeySource::DS45: LocalKeydataStore.Alt_DS45 = InputChord.Key; break;
			case ERTSOSettingsKeySource::XSX: LocalKeydataStore.Alt_XSX = InputChord.Key; break;
		}
		return LocalKeydataStore;
	}

	switch(KeySource)
	{
		case ERTSOSettingsKeySource::KBM: LocalKeydataStore.Main_KeyBoardMouse = InputChord.Key; break;
		case ERTSOSettingsKeySource::GGP: LocalKeydataStore.Main_GenericGamepad = InputChord.Key; break;
		case ERTSOSettingsKeySource::DS45: LocalKeydataStore.Main_DS45 = InputChord.Key; break;
		case ERTSOSettingsKeySource::XSX: LocalKeydataStore.Main_XSX = InputChord.Key; break;
	}
	return LocalKeydataStore;
}

void URTSOInputSelector::UpdateAllKeyValues(const FRTSOSettingsDataSelector& ReadData)
{
	LocalKeydataStore = ReadData.AsKey;
}

void URTSOMenuWidget_SettingsEntry::NativePreConstruct()
{
	Super::NativePreConstruct();
	
	InputValueWidgetSwitcher->SetActiveWidgetIndex(static_cast<uint8>(TargetWidgetIndex));

	switch (TargetWidgetIndex)
	{
		case ERTSOSettingsWidgetIndex::AsCheckBox:
		{
			TScriptDelegate InDelegate;
			InDelegate.BindUFunction(this, TEXT("OnCheckBoxSet"));
			AsCheckBox->OnCheckStateChanged.AddUnique(InDelegate);
		}
		break;
		case ERTSOSettingsWidgetIndex::AsKeySelector:
		{		
			TScriptDelegate MainKBMDelegate;
			MainKBMDelegate.BindUFunction(this, TEXT("OnMainKeySelected_KBM"));
			AsKeySelector->MainKeyboardMouseKey->OnKeySelected.AddUnique(MainKBMDelegate);

			TScriptDelegate MainGamepadDelegate;
			MainGamepadDelegate.BindUFunction(this, TEXT("OnMainKeySelected_GenericGamepad"));
			AsKeySelector->MainGenericGamepadKey->OnKeySelected.AddUnique(MainGamepadDelegate);
			
			TScriptDelegate MainDS45Delegate;
			MainDS45Delegate.BindUFunction(this, TEXT("OnMainKeySelected_DS45"));
			AsKeySelector->MainDS45Key->OnKeySelected.AddUnique(MainDS45Delegate);

			TScriptDelegate MainXSXDelegate;
			MainXSXDelegate.BindUFunction(this, TEXT("OnMainKeySelected_XSX"));
			AsKeySelector->MainXSXKey->OnKeySelected.AddUnique(MainXSXDelegate);

			TScriptDelegate AltKBMDelegate;
			AltKBMDelegate.BindUFunction(this, TEXT("OnAltKeySelected_KBM"));
			AsKeySelector->AltKeyboardMouseKey->OnKeySelected.AddUnique(AltKBMDelegate);

			TScriptDelegate AltGamepadDelegate;
			AltGamepadDelegate.BindUFunction(this, TEXT("OnAltKeySelected_GenericGamepad"));
			AsKeySelector->AltGenericGamepadKey->OnKeySelected.AddUnique(AltGamepadDelegate);
			
			TScriptDelegate AltDS45Delegate;
			AltDS45Delegate.BindUFunction(this, TEXT("OnAltKeySelected_DS45"));
			AsKeySelector->AltDS45Key->OnKeySelected.AddUnique(AltDS45Delegate);

			TScriptDelegate AltXSXDelegate;
			AltXSXDelegate.BindUFunction(this, TEXT("OnAltKeySelected_XSX"));
			AsKeySelector->AltXSXKey->OnKeySelected.AddUnique(AltXSXDelegate);

		}
		break;
	}
}

void URTSOMenuWidget_SettingsEntry::OnCheckBoxSet(bool bNewState)
{
	URTSOSettingsSubsystem::Get()->OnCheckBox(bNewState, SettingsTag);
}

void URTSOMenuWidget_SettingsEntry::OnKeySelected(const FRTSOSettingsKeyData& NewKeyData)
{
	URTSOSettingsSubsystem::Get()->OnKey(NewKeyData, SettingsTag);
}

void URTSOMenuWidget_SettingsEntry::OnMainKeySelected_KBM(const FInputChord& InputChord){OnKeySelected(AsKeySelector->UpdateKeyData(ERTSOSettingsKeySource::KBM, InputChord, false));}
void URTSOMenuWidget_SettingsEntry::OnMainKeySelected_GenericGamepad(const FInputChord& InputChord){OnKeySelected(AsKeySelector->UpdateKeyData(ERTSOSettingsKeySource::GGP, InputChord, false));}
void URTSOMenuWidget_SettingsEntry::OnMainKeySelected_DS45(const FInputChord& InputChord){OnKeySelected(AsKeySelector->UpdateKeyData(ERTSOSettingsKeySource::DS45, InputChord, false));}
void URTSOMenuWidget_SettingsEntry::OnMainKeySelected_XSX(const FInputChord& InputChord){OnKeySelected(AsKeySelector->UpdateKeyData(ERTSOSettingsKeySource::XSX, InputChord, false));}

void URTSOMenuWidget_SettingsEntry::OnAltKeySelected_KBM(const FInputChord& InputChord){OnKeySelected(AsKeySelector->UpdateKeyData(ERTSOSettingsKeySource::KBM, InputChord, true));}
void URTSOMenuWidget_SettingsEntry::OnAltKeySelected_GenericGamepad(const FInputChord& InputChord){OnKeySelected(AsKeySelector->UpdateKeyData(ERTSOSettingsKeySource::GGP, InputChord, true));}
void URTSOMenuWidget_SettingsEntry::OnAltKeySelected_DS45(const FInputChord& InputChord){OnKeySelected(AsKeySelector->UpdateKeyData(ERTSOSettingsKeySource::DS45, InputChord, true));}
void URTSOMenuWidget_SettingsEntry::OnAltKeySelected_XSX(const FInputChord& InputChord){OnKeySelected(AsKeySelector->UpdateKeyData(ERTSOSettingsKeySource::XSX, InputChord, true));}



void URTSOMenuWidget_SettingsEntry::NativeOnActivated()
{
	Super::NativeOnActivated();
}

void URTSOMenuWidget_SettingsCategory::NativeOnActivated()
{
	Super::NativeOnActivated();
	Refresh(EntryClass, EntryCategoryName, EntriesData);
}

void URTSOMenuWidget_SettingsCategory::NativePreConstruct()
{
	Super::NativePreConstruct();
	Refresh(EntryClass, EntryCategoryName, EntriesData);
}

void URTSOMenuWidget_SettingsCategory::Refresh(
	TSubclassOf<URTSOMenuWidget_SettingsEntry> InEntryClass,
	FString InEntryCategoryName, 
	const TMap<FGameplayTag, FRTSOSettingsDataSelector>& InEntriesData)
{
	if (InEntryClass == nullptr)
	{
		return;
	}
	
	EntryClass = InEntryClass;
	EntryCategoryName = InEntryCategoryName;
	EntriesData = InEntriesData;

	SettingsCategoryLabel->SetText(FText::FromString(EntryCategoryName));
	ItemContentBox->ClearChildren();

	for (auto& [SettingsTag, DataSelector] : InEntriesData)
	{
		URTSOMenuWidget_SettingsEntry* SettingsEntry = CreateWidget<URTSOMenuWidget_SettingsEntry>(this, EntryClass);
		const URTSOSettingsDeveloperSettings* ImmutableSettingsDevSettings = GetDefault<URTSOSettingsDeveloperSettings>();

		FText SettingsLabelText = FText::FromStringTable(ImmutableSettingsDevSettings->SettingsStringTablePath, SettingsTag.ToString());
		FText SettingsDescrText = FText::FromStringTable(ImmutableSettingsDevSettings->SettingsStringTablePath, FString::Printf(TEXT("%s_Description"), *SettingsTag.ToString()));

		SettingsEntry->SettingsEntryLabel->SetText(SettingsLabelText);
		SettingsEntry->SettingsEntryDescription->SetText(SettingsDescrText);
		SettingsEntry->SettingsTag = SettingsTag;

		SettingsEntry->OwnerSubmenu = OwnerSubmenu;

		SettingsEntry->TargetWidgetIndex = ERTSOSettingsWidgetIndex::AsRangedSelector;
		switch (DataSelector.ValueType)
		{
		case ERTSOSettingsType::FloatSelector:
			if (SettingsEntry->RangedSelector != nullptr && SettingsEntry->RangedSelector->IsValidLowLevel())
			{
				SettingsEntry->RangedSelector->CountTypeSelector = EPDSharedUICountTypeSelector::ERangedIncrementBox;
				SettingsEntry->RangedSelector->SelectedCount = DataSelector.AsDouble;
				SettingsEntry->RangedSelector->PostValueChanged.AddLambda(
					[SettingsTag](int32 NewValue) 
					{
						URTSOSettingsSubsystem::Get()->OnFloat(static_cast<double>(NewValue) / PD::Settings::FloatUIMultiplier, SettingsTag);
					});
				SettingsEntry->RangedSelector->ApplySettings(FPDRangedSliderSettings{
					static_cast<int32>(DataSelector.DoubleRange.Min),
					static_cast<int32>(DataSelector.DoubleRange.Max),
					PD::Settings::FloatUIMultiplier, 
					PD::Settings::FloatUIMultiplier / 10.0});
			}
			
			break;
		case ERTSOSettingsType::FloatSlider:
			if (SettingsEntry->RangedSelector != nullptr && SettingsEntry->RangedSelector->IsValidLowLevel())
			{
				SettingsEntry->RangedSelector->CountTypeSelector = EPDSharedUICountTypeSelector::ERangedSlider;				
				SettingsEntry->RangedSelector->SelectedCount = DataSelector.AsDouble;
				SettingsEntry->RangedSelector->PostValueChanged.AddLambda(
					[SettingsTag](int32 NewValue) 
					{
						URTSOSettingsSubsystem::Get()->OnFloat(static_cast<double>(NewValue) / PD::Settings::FloatUIMultiplier, SettingsTag);
					});
				SettingsEntry->RangedSelector->ApplySettings(FPDRangedSliderSettings{
					static_cast<int32>(DataSelector.DoubleRange.Min),
					static_cast<int32>(DataSelector.DoubleRange.Max),
					PD::Settings::FloatUIMultiplier, 
					PD::Settings::FloatUIMultiplier / 10.0});			
			}
			
			break;
		case ERTSOSettingsType::IntegerSelector:
			if (SettingsEntry->RangedSelector != nullptr && SettingsEntry->RangedSelector->IsValidLowLevel())
			{
				SettingsEntry->RangedSelector->CountTypeSelector = EPDSharedUICountTypeSelector::ERangedIncrementBox;
				SettingsEntry->RangedSelector->SelectedCount = DataSelector.AsInteger;
				SettingsEntry->RangedSelector->PostValueChanged.AddLambda(
					[SettingsTag](int32 NewValue) 
					{
						URTSOSettingsSubsystem::Get()->OnInteger(NewValue, SettingsTag);
					});
				SettingsEntry->RangedSelector->ApplySettings(FPDRangedSliderSettings{
					DataSelector.IntegerRange.Min,
					DataSelector.IntegerRange.Max,
					1.0, 
					1.0});		
			}
			
			break;
		case ERTSOSettingsType::IntegerSlider:
			if (SettingsEntry->RangedSelector != nullptr && SettingsEntry->RangedSelector->IsValidLowLevel())
			{
				SettingsEntry->RangedSelector->CountTypeSelector = EPDSharedUICountTypeSelector::ERangedSlider;
				SettingsEntry->RangedSelector->SelectedCount = DataSelector.AsInteger;
				SettingsEntry->RangedSelector->PostValueChanged.AddLambda(
					[SettingsTag](int32 NewValue) 
					{
						URTSOSettingsSubsystem::Get()->OnInteger(NewValue, SettingsTag);
					});
				SettingsEntry->RangedSelector->ApplySettings(FPDRangedSliderSettings{
					DataSelector.IntegerRange.Min,
					DataSelector.IntegerRange.Max,
					1.0, 
					1.0});
			}
			
			break;
		case ERTSOSettingsType::Vector2:
			if (SettingsEntry->AsVectorSelector != nullptr && SettingsEntry->AsVectorSelector->IsValidLowLevel())
			{
				SettingsEntry->TargetWidgetIndex = ERTSOSettingsWidgetIndex::AsVectorSelector;
				SettingsEntry->AsVectorSelector->Data.VectorType = DataSelector.ValueType;

				SettingsEntry->AsVectorSelector->Data.SelectedValue.X = DataSelector.AsVector2.X;
				SettingsEntry->AsVectorSelector->Data.SelectedValue.Y = DataSelector.AsVector2.Y;
				SettingsEntry->AsVectorSelector->SetOnVectorUpdated(FOnVectorValueUpdated::CreateLambda(
					[SettingsTag](UE::Math::TVector4<double> UpdatedValue, PD::Settings::VectorType Type, UWidget* Caller)
					{
						URTSOSettingsSubsystem::Get()->OnVector2D(FVector2D(UpdatedValue), SettingsTag);
					}));
				SettingsEntry->AsVectorSelector->Refresh();
			}
			
			break;
		case ERTSOSettingsType::Vector3:
			if (SettingsEntry->AsVectorSelector != nullptr && SettingsEntry->AsVectorSelector->IsValidLowLevel())
			{
				SettingsEntry->TargetWidgetIndex = ERTSOSettingsWidgetIndex::AsVectorSelector;
				SettingsEntry->AsVectorSelector->Data.VectorType = DataSelector.ValueType;

				SettingsEntry->AsVectorSelector->Data.SelectedValue = DataSelector.AsVector3;
				SettingsEntry->AsVectorSelector->SetOnVectorUpdated(FOnVectorValueUpdated::CreateLambda(
					[SettingsTag](UE::Math::TVector4<double> UpdatedValue, PD::Settings::VectorType Type, UWidget* Caller)
					{
						URTSOSettingsSubsystem::Get()->OnVector(FVector(UpdatedValue), SettingsTag);
					}));
				SettingsEntry->AsVectorSelector->Refresh();
			}
			
			break;
		case ERTSOSettingsType::Colour:
			if (SettingsEntry->AsVectorSelector != nullptr && SettingsEntry->AsVectorSelector->IsValidLowLevel())
			{
				SettingsEntry->TargetWidgetIndex = ERTSOSettingsWidgetIndex::AsVectorSelector;
				SettingsEntry->AsVectorSelector->Data.VectorType = DataSelector.ValueType;

				SettingsEntry->AsVectorSelector->Data.SelectedValue = FLinearColor(DataSelector.AsColour);
				SettingsEntry->AsVectorSelector->SetOnVectorUpdated(FOnVectorValueUpdated::CreateLambda(
					[SettingsTag](UE::Math::TVector4<double> UpdatedValue, PD::Settings::VectorType Type, UWidget* Caller)
					{
						URTSOSettingsSubsystem::Get()->OnColour(FLinearColor(UpdatedValue), SettingsTag);
					}));
				SettingsEntry->AsVectorSelector->Refresh();

			}
			
			break;
		case ERTSOSettingsType::Boolean:
			if (SettingsEntry->AsCheckBox != nullptr && SettingsEntry->AsCheckBox->IsValidLowLevel())
			{
				SettingsEntry->TargetWidgetIndex = ERTSOSettingsWidgetIndex::AsCheckBox;
				SettingsEntry->AsCheckBox->SetCheckedState(DataSelector.AsBool ? ECheckBoxState::Checked : ECheckBoxState::Unchecked);
			}
			break;
		case ERTSOSettingsType::EnumAsByte:
		case ERTSOSettingsType::String:
			if(SettingsEntry->AsStringSelector != nullptr && SettingsEntry->AsStringSelector->IsValidLowLevel())
			{
				SettingsEntry->TargetWidgetIndex = ERTSOSettingsWidgetIndex::AsStringSelector;
				SettingsEntry->AsStringSelector->Data.SelectedString = DataSelector.AsString;

				SettingsEntry->AsStringSelector->Data.GeneratedStringOptions = FPDSettingStatics::GenerateStringPtrArrayFromDataSelector(DataSelector);
			
				// 
				// TODO> Need to get the associated data and pass result to subsystem, let the subsystem decide what to do with it
				// TODO> Move to actual function and bind to said function
				SettingsEntry->AsStringSelector->SetOnStringSelected(FOnStringValueSelected::CreateLambda(
				[DataSelector, SettingsTag](const FString& SelectedItem, UWidget* Caller)
				{
					URTSOStringSelector* AsStringSelector = Cast<URTSOStringSelector>(Caller);
					const TArray<FString> StringList = DataSelector.StringList;
					if (AsStringSelector == nullptr || StringList.Contains(SelectedItem) == false)
					{
						return;
					}

					const int32 SelectedItemIdx = StringList.IndexOfByKey(SelectedItem);

					switch(DataSelector.AssociatedStringDataType)
					{
					case ERTSOSettingsType::FloatSlider: URTSOSettingsSubsystem::AttemptApplyDataSelectorOnString<double>(DataSelector, SelectedItemIdx, SettingsTag); break;
					case ERTSOSettingsType::IntegerSlider: URTSOSettingsSubsystem::AttemptApplyDataSelectorOnString<int32>(DataSelector, SelectedItemIdx, SettingsTag); break;
					case ERTSOSettingsType::Vector2: URTSOSettingsSubsystem::AttemptApplyDataSelectorOnString<FVector2D>(DataSelector, SelectedItemIdx, SettingsTag); break;
					case ERTSOSettingsType::Vector3: URTSOSettingsSubsystem::AttemptApplyDataSelectorOnString<FVector>(DataSelector, SelectedItemIdx, SettingsTag); break;
					case ERTSOSettingsType::Colour: URTSOSettingsSubsystem::AttemptApplyDataSelectorOnString<FColor>(DataSelector, SelectedItemIdx, SettingsTag); break;
					case ERTSOSettingsType::EnumAsByte: URTSOSettingsSubsystem::AttemptApplyDataSelectorOnString<uint8>(DataSelector, SelectedItemIdx, SettingsTag); break;
					case ERTSOSettingsType::String: URTSOSettingsSubsystem::AttemptApplyDataSelectorOnString<FString>(DataSelector, SelectedItemIdx, SettingsTag); break;
					case ERTSOSettingsType::None:
						URTSOSettingsSubsystem::Get()->OnString(SelectedItem, SettingsTag);
					break; 
					// case ERTSOSettingsType::Boolean: return;
					// case ERTSOSettingsType::Key: return;
					default: return;
					}

					AsStringSelector->Data.SelectedString = SelectedItem;
					AsStringSelector->Refresh();
					AsStringSelector->CloseExpandableArea();
				}));
				SettingsEntry->AsStringSelector->Refresh();

			}
				
			break;
		case ERTSOSettingsType::Key:
			if(SettingsEntry->AsKeySelector != nullptr && SettingsEntry->AsKeySelector->IsValidLowLevel())
			{
				SettingsEntry->TargetWidgetIndex = ERTSOSettingsWidgetIndex::AsKeySelector;
				SettingsEntry->AsKeySelector->UpdateAllKeyValues(DataSelector);
				// TODO: Key selectors aren't displaying any title, resolve ASAP
			}


			break;
		}
		

		
		ItemContentBox->AddChildToVerticalBox(SettingsEntry);
	}	
	
}

void URTSOMenuWidget_BaseMenu::NativeOnActivated()
{
	Super::NativeOnActivated();
	// todo
}

void URTSOMenuWidget_BaseSubmenu::NativeOnActivated()
{
	Super::NativeOnActivated();

	TArray<UWidget*> ContentBoxEntries = ContentBox->GetAllChildren();
	for (UWidget* BoxEntry : ContentBoxEntries)
	{
		if (URTSOMenuWidget_SettingsCategory* SpawnedSettingsCategory = Cast<URTSOMenuWidget_SettingsCategory>(BoxEntry)
			; SpawnedSettingsCategory != nullptr)
		{
			SpawnedSettingsCategory->OwnerSubmenu = this;
			SpawnedSettingsCategory->Refresh(SpawnedSettingsCategory->EntryClass, SpawnedSettingsCategory->EntryCategoryName, SpawnedSettingsCategory->EntriesData);
		}
	}
}

void URTSOMenuWidget_Settings::SetTabContent(const FName& TabName, const TMap<FString, TMap<FGameplayTag, FRTSOSettingsDataSelector>>& SettingsCategories)
{
	UCommonActivatableWidget** ExistingSubMenu = SpawnedSubmenus.Find(TabName);
	if (ExistingSubMenu == nullptr || (*ExistingSubMenu) == nullptr)
	{
		ExistingSubMenu = &SpawnedSubmenus.Add(TabName, SpawnSettingsCategories(SettingsCategories));
	}

	// Describe the current details 
	if (URTSOMenuWidget_BaseSubmenu* AsRTSOSubMenu = Cast<URTSOMenuWidget_BaseSubmenu>(*ExistingSubMenu))
	{
		FString TabStr = TabName.ToString();
		const URTSOSettingsDeveloperSettings* ImmutableSettingsDevSettings = GetDefault<URTSOSettingsDeveloperSettings>();
		const FText CategoryLabelText = FText::FromStringTable(ImmutableSettingsDevSettings->SettingsStringTablePath, FString::Printf(TEXT("Extended_Label_%s"), *TabStr));
		const FText CategoryDescrText = FText::FromStringTable(ImmutableSettingsDevSettings->SettingsStringTablePath, FString::Printf(TEXT("Extended_Desc_%s"), *TabStr));
		AsRTSOSubMenu->DetailsLabel->SetText(CategoryLabelText);
		AsRTSOSubMenu->DetailsDescription->SetText(CategoryDescrText);
	}

	// (*ExistingSubMenu)->ActivateWidget();
	InnerContent->SetContent(*ExistingSubMenu);
}

void URTSOMenuWidget_Settings::OnCategoryPressed_Gameplay()
{
	TMap<FString, TMap<FGameplayTag, FRTSOSettingsDataSelector>> SettingsCategories{};
	SettingsCategories.Add("Camera") = PD::Settings::FGameplaySettingsDefaults::Camera;
	SettingsCategories.Add("Action Log") = PD::Settings::FGameplaySettingsDefaults::ActionLog;
	SettingsCategories.Add("Difficulty") = PD::Settings::FGameplaySettingsDefaults::Difficulty;

	FName TabName = PD::Settings::FAllSettingsDefaults::GetTabName(PD::Settings::ERTSOSettingsGroups::Gameplay);	
	MenuTitleLabel->SetText(FText::FromString(FString::Printf(TEXT("Settings - %s"), *TabName.ToString())));
	SetTabContent(TabName, SettingsCategories);
}

void URTSOMenuWidget_Settings::OnCategoryPressed_Video()
{
	TMap<FString, TMap<FGameplayTag, FRTSOSettingsDataSelector>> SettingsCategories{};
	SettingsCategories.Add("Display") = PD::Settings::FVideoSettingsDefaults::Display;
	SettingsCategories.Add("Effects") = PD::Settings::FVideoSettingsDefaults::Effects;
	SettingsCategories.Add("Graphics") = PD::Settings::FVideoSettingsDefaults::Graphics;
	
	FName TabName = PD::Settings::FAllSettingsDefaults::GetTabName(PD::Settings::ERTSOSettingsGroups::Video);
	MenuTitleLabel->SetText(FText::FromString(FString::Printf(TEXT("Settings - %s"), *TabName.ToString())));
	SetTabContent(TabName, SettingsCategories);
}	
void URTSOMenuWidget_Settings::OnCategoryPressed_Audio()
{
	TMap<FString, TMap<FGameplayTag, FRTSOSettingsDataSelector>> SettingsCategories{};
	SettingsCategories.Add("Base") = PD::Settings::FAudioSettingsDefaults::Base;
	
	FName TabName = PD::Settings::FAllSettingsDefaults::GetTabName(PD::Settings::ERTSOSettingsGroups::Audio);	
	MenuTitleLabel->SetText(FText::FromString(FString::Printf(TEXT("Settings - %s"), *TabName.ToString())));
	SetTabContent(TabName, SettingsCategories);
}	
void URTSOMenuWidget_Settings::OnCategoryPressed_Controls()
{
	TMap<FString, TMap<FGameplayTag, FRTSOSettingsDataSelector>> SettingsCategories{};
	SettingsCategories.Add("Game") = PD::Settings::FControlsSettingsDefaults::Game;
	SettingsCategories.Add("UI") = PD::Settings::FControlsSettingsDefaults::UI;
	
	FName TabName = PD::Settings::FAllSettingsDefaults::GetTabName(PD::Settings::ERTSOSettingsGroups::Controls);	
	MenuTitleLabel->SetText(FText::FromString(FString::Printf(TEXT("Settings - %s"), *TabName.ToString())));
	SetTabContent(TabName, SettingsCategories);
}	
void URTSOMenuWidget_Settings::OnCategoryPressed_Interface()
{
	TMap<FString, TMap<FGameplayTag, FRTSOSettingsDataSelector>> SettingsCategories{};
	SettingsCategories.Add("Base") = PD::Settings::FInterfaceSettingsDefaults::Base;
	
	FName TabName = PD::Settings::FAllSettingsDefaults::GetTabName(PD::Settings::ERTSOSettingsGroups::Interface);	
	MenuTitleLabel->SetText(FText::FromString(FString::Printf(TEXT("Settings - %s"), *TabName.ToString())));
	SetTabContent(TabName, SettingsCategories);
}	

UCommonActivatableWidget* URTSOMenuWidget_Settings::SpawnSettingsCategories(const TMap<FString, TMap<FGameplayTag, FRTSOSettingsDataSelector>>& SettingsCategories)
{
	URTSOMenuWidget_BaseSubmenu* Submenu = CreateWidget<URTSOMenuWidget_BaseSubmenu>(this, SubMenuClass);
	check(Submenu)

	for (auto&[SettingsCategoryName, CategoryData] : SettingsCategories)
	{
		UVerticalBoxSlot* ContentSlot = Submenu->ContentBox->AddChildToVerticalBox(CreateWidget<URTSOMenuWidget_SettingsCategory>(this, SettingsCategoryClass));
		if (URTSOMenuWidget_SettingsCategory* SpawnedSettingsCategory = Cast<URTSOMenuWidget_SettingsCategory>(ContentSlot->Content)
			; SpawnedSettingsCategory != nullptr)
		{
			SpawnedSettingsCategory->OwnerSubmenu = Submenu;
			SpawnedSettingsCategory->Refresh(SettingsEntryClass, SettingsCategoryName, CategoryData);
		}
	}
	return Submenu;
}


void URTSOMenuWidget_BaseMenu::NativePreConstruct()
{
	Super::NativePreConstruct();
	// todo
}

void URTSOMenuWidget_Settings::NativeConstruct()
{
	Super::NativeConstruct();
	Refresh();
}
void URTSOMenuWidget_Settings::NativeDestruct()
{
	SpawnedSubmenus.Empty();
	Super::NativeDestruct();
}
void URTSOMenuWidget_Settings::NativePreConstruct()
{
	Super::NativePreConstruct();
	Refresh();
}
void URTSOMenuWidget_Settings::NativeOnActivated()
{
	Super::NativeOnActivated();
	Refresh();
}
void URTSOMenuWidget_Settings::Refresh()
{
	if (SubmenuSelector != nullptr && SubmenuSelector->IsValidLowLevelFast())
	{
		SubmenuSelector->ClearChildren();
	}

	if (SubMenuClass == nullptr || SettingsCategoryClass == nullptr || TabButtonClass == nullptr || SettingsEntryClass == nullptr
		|| SubMenuClass->IsValidLowLevelFast() == false || SettingsCategoryClass->IsValidLowLevelFast() == false
		|| TabButtonClass->IsValidLowLevelFast() == false || SettingsEntryClass->IsValidLowLevelFast() == false) 
	{
		return;
	}

	for (PD::Settings::ERTSOSettingsGroups TopLevelSetting : PD::Settings::FAllSettingsDefaults::TopLevelSettingTypes)
	{
		UHorizontalBoxSlot* CategorySlot = SubmenuSelector->AddChildToHorizontalBox(CreateWidget<UPDGenericButton>(this, TabButtonClass));
		const FName TabName = PD::Settings::FAllSettingsDefaults::GetTabName(TopLevelSetting);

		if (URTSOMenuButton* AsSpawnedChild = Cast<URTSOMenuButton>(CategorySlot->Content)
			; AsSpawnedChild != nullptr)
		{
			AsSpawnedChild->ButtonName = TabName;
			AsSpawnedChild->TextBlock->SetText(FText::FromString(TabName.ToString()));

			switch(TopLevelSetting)
			{
			case PD::Settings::ERTSOSettingsGroups::Gameplay: 
				AsSpawnedChild->Hitbox->OnPressed.AddDynamic(this, &ThisClass::OnCategoryPressed_Gameplay);
				if (TopLevelSetting == DefaultView){OnCategoryPressed_Gameplay();}
				break;
			case PD::Settings::ERTSOSettingsGroups::Video: 
				AsSpawnedChild->Hitbox->OnPressed.AddDynamic(this, &ThisClass::OnCategoryPressed_Video);
				if (TopLevelSetting == DefaultView){OnCategoryPressed_Video();}
				break;
			case PD::Settings::ERTSOSettingsGroups::Audio: 
				AsSpawnedChild->Hitbox->OnPressed.AddDynamic(this, &ThisClass::OnCategoryPressed_Audio);
				if (TopLevelSetting == DefaultView){OnCategoryPressed_Audio();}
				break;
			case PD::Settings::ERTSOSettingsGroups::Controls: 
				AsSpawnedChild->Hitbox->OnPressed.AddDynamic(this, &ThisClass::OnCategoryPressed_Controls);
				if (TopLevelSetting == DefaultView){OnCategoryPressed_Controls();}
				break;				
			case PD::Settings::ERTSOSettingsGroups::Interface: 
				AsSpawnedChild->Hitbox->OnPressed.AddDynamic(this, &ThisClass::OnCategoryPressed_Interface);
				if (TopLevelSetting == DefaultView){OnCategoryPressed_Interface();}
				break;								
			}
		}
	}

}

void URTSOMenuWidget::ClearDelegates() const
{
	ResumeButton->Hitbox->OnPressed.Clear();
	ResumeButton->Hitbox->OnReleased.Clear();

	SettingsButton->Hitbox->OnPressed.Clear();
	SettingsButton->Hitbox->OnReleased.Clear();

	SaveButton->Hitbox->OnPressed.Clear();
	SaveButton->Hitbox->OnReleased.Clear();

	LoadButton->Hitbox->OnPressed.Clear();
	LoadButton->Hitbox->OnReleased.Clear();

	QuitButton->Hitbox->OnPressed.Clear();
	QuitButton->Hitbox->OnReleased.Clear();

	SaveEditor->Hitbox->OnPressed.Clear();
	SaveEditor->Hitbox->OnReleased.Clear();
}

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
