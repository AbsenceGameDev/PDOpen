/* @author: Ario Amin @ Permafrost Development. @copyright: Full BSL(1.1) License included at bottom of the file  */

#include "RTSOpenCommon.h"
#include "Widgets/Slate/SRTSOSettingsStringSelector.h"

#include "Animation/WidgetAnimation.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/Guid.h"
#include "Serialization/StructuredArchive.h"
#include "Internationalization/TextKey.h"
#include "Subsystems/RTSOSettingsSubsystem.h"
#include "UObject/UnrealType.h"

#if WITH_EDITOR
#include "Editor.h"

#include "IDetailGroup.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "PropertyCustomizationHelpers.h"
#include "IPropertyUtilities.h"

#include "Widgets/Layout/SExpandableArea.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "SGameplayTagCombo.h"
#include "SGameplayTagPicker.h"
#include "SGameplayTagWidget.h"
#include "GameplayTagsManager.h"

#include "Components/SizeBox.h"

#include "Styling/CoreStyle.h"
#endif // WITH_EDITOR


bool FRTSOSettingsKeyData::Serialize(FArchive& Ar)
{
	Ar << HiddenSettingIdToMatchAgainst;


#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 4
	for (FKey* KeyPtr : {&Main_KeyBoardMouse, &Alt_KeyBoardMouse, &Main_GenericGamepad, &Alt_GenericGamepad,
	                     &Main_DS45, &Alt_DS45, &Main_XSX, &Alt_XSX})
	{
		FName KeyName = KeyPtr->GetFName();
		Ar << KeyName;          // writes the current name when saving, reads it when loading
		*KeyPtr = FKey(KeyName); // same key when saving; rebuilds the key when loading
	}
#else	
	FBinaryArchiveFormatter ArFormatter{Ar};
	FStructuredArchive ArStructured{ArFormatter};

	FStructuredArchive::FSlot Slot = ArStructured.Open();	
	FPropertyTag Tag;
	Tag.Type = NAME_NameProperty;
	Main_KeyBoardMouse.SerializeFromMismatchedTag(Tag, Slot);
	Alt_KeyBoardMouse.SerializeFromMismatchedTag(Tag, Slot);
	Main_GenericGamepad.SerializeFromMismatchedTag(Tag, Slot);
	Alt_GenericGamepad.SerializeFromMismatchedTag(Tag, Slot);
	Main_DS45.SerializeFromMismatchedTag(Tag, Slot);
	Alt_DS45.SerializeFromMismatchedTag(Tag, Slot);
	Main_XSX.SerializeFromMismatchedTag(Tag, Slot);
	Alt_XSX.SerializeFromMismatchedTag(Tag, Slot);
	ArStructured.Close();
#endif
	return true;					
}

//
//// Value binder START
#if WITH_EDITOR
TSharedRef<IPropertyTypeCustomization> FRTSOValueBinderDetails::MakeInstance()
{
	TSharedRef<FRTSOValueBinderDetails> Shareable = MakeShareable(new FRTSOValueBinderDetails);
	return Shareable;
}
void FRTSOValueBinderDetails::CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{

}
void FRTSOValueBinderDetails::CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	if(PropertyHandle->IsValidHandle() == false || PropertyHandle->GetPropertyClass() == nullptr)
	{
		return;
	}

#if 0 // Fix later, this is not relevant now, TODO: Must come back to this however before EoY
	class FBoolProperty* EditInlinePtr = nullptr; 
	FProperty* PropertyPtr = PropertyHandle->GetProperty();
    FString EditConditionName = PropertyPtr->GetMetaData(TEXT("EditCondition"));
    if (EditConditionName.IsEmpty() == false)
    {
		UE_LOG(LogTemp, Warning, TEXT("FRTSOValueBinderDetails::CustomizeChildren - EditCondition(%s) Found"), *EditConditionName)
		
		if (UClass* OwnerClass = PropertyPtr->GetOwnerClass())
		{
			FName EditConditionFName = FName(*EditConditionName);
			EditInlinePtr = CastField<FBoolProperty>(OwnerClass->FindPropertyByName(EditConditionFName));
		}

		if (EditInlinePtr)
		{
			UE_LOG(LogTemp, Warning, TEXT("FRTSOValueBinderDetails::CustomizeChildren - EditInline Property Valid"))
			TSharedPtr<IPropertyHandle> EditInlineHandle = ChildBuilder.GetParentCategory().GetParentLayout().GetProperty(EditInlinePtr->GetFName());
			if (EditInlineHandle.IsValid())
			{
				uint32 ChildLim;
				PropertyHandle->GetNumChildren(ChildLim);
				IDetailGroup& PropertiesGroup = ChildBuilder.GetParentCategory().GetParentLayout().AddGroup(FName("SettingsEnabledTypes"), PropertyHandle->GetPropertyDisplayName());
				for (uint32 ChildIdx = 0; ChildIdx < ChildLim; ChildIdx++)
				{
					TSharedPtr<IPropertyHandle> ChildPropertyHandle = PropertyHandle->GetChildHandle(ChildIdx);
					if (ChildPropertyHandle.IsValid())
					{
						IDetailPropertyRow& InnerBinderRow = PropertiesGroup.AddPropertyRow(ChildPropertyHandle.ToSharedRef());
					}
				}

				return;
			}
		}
	}
#endif 

	FName PropertyTypeName = PropertyHandle->GetPropertyClass()->GetFName();
	UE_LOG(LogTemp, Warning, TEXT("=================================================="))
	UE_LOG(LogTemp, Warning, TEXT("FRTSOValueBinderDetails::CustomizeChildren::(%s)%s"), *PropertyHandle->GetPropertyClass()->GetName(), *PropertyHandle->GetProperty()->NamePrivate.ToString())
	TSharedPtr<IPropertyHandle> ParentHandle = PropertyHandle->GetParentHandle();
	if(ParentHandle && ParentHandle->IsValidHandle() && ParentHandle->GetPropertyClass())
	{
		UE_LOG(LogTemp, Warning, TEXT("FRTSOValueBinderDetails::CustomizeChildren::Parent::(%s)%s"), *ParentHandle->GetPropertyClass()->GetName(), *ParentHandle->GetProperty()->NamePrivate.ToString())
	}

	FStructProperty* AsStructProperty = CastField<FStructProperty>(PropertyHandle->GetProperty());
	TSharedRef<FRTSOBinderDetailRowBuilder> BinderRowBuilder = MakeShareable(new FRTSOBinderDetailRowBuilder);
	BinderRowBuilder->PropertyType = ERTSOSettingsType::None;
	if (NAME_FloatProperty == PropertyTypeName 
		| NAME_DoubleProperty == PropertyTypeName) 
	{
		BinderRowBuilder->PropertyType = ERTSOSettingsType::FloatSlider;
		UE_LOG(LogTemp, Warning, TEXT("FRTSOValueBinderDetails::CustomizeChildren - FLOAT"))
	}
	else if (NAME_IntProperty == PropertyTypeName || NAME_Int32Property == PropertyTypeName)
	{
		BinderRowBuilder->PropertyType = ERTSOSettingsType::IntegerSlider;
		UE_LOG(LogTemp, Warning, TEXT("FRTSOValueBinderDetails::CustomizeChildren - INTEGER"))
	}
	else if (NAME_ByteProperty == PropertyTypeName)
	{
		BinderRowBuilder->PropertyType = ERTSOSettingsType::EnumAsByte;
		UE_LOG(LogTemp, Warning, TEXT("FRTSOValueBinderDetails::CustomizeChildren - BYTE"))
	}	
	else if (NAME_BoolProperty == PropertyTypeName)
	{
		BinderRowBuilder->PropertyType = ERTSOSettingsType::Boolean;
		UE_LOG(LogTemp, Warning, TEXT("FRTSOValueBinderDetails::CustomizeChildren - BOOL"))
	}
	else if (NAME_StructProperty == PropertyTypeName && AsStructProperty != nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("FRTSOValueBinderDetails::CustomizeChildren - STRUCT(%s)"), *AsStructProperty->Struct->GetStructCPPName())
		
		PropertyTypeName = FName(AsStructProperty->Struct->GetStructCPPName());
		if (FName("FVector2DWrapper") == PropertyTypeName)
		{
			BinderRowBuilder->PropertyType = ERTSOSettingsType::Vector2;
			UE_LOG(LogTemp, Warning, TEXT("FRTSOValueBinderDetails::CustomizeChildren - VEC2D"))
		}
		else if (FName("FVectorWrapper") == PropertyTypeName)
		{
			BinderRowBuilder->PropertyType = ERTSOSettingsType::Vector3;
			UE_LOG(LogTemp, Warning, TEXT("FRTSOValueBinderDetails::CustomizeChildren - VEC"))
		}		
		else if (FName("FColorWrapper") == PropertyTypeName
			|| FName("FLinearColorWrapper") == PropertyTypeName)
		{
			BinderRowBuilder->PropertyType = ERTSOSettingsType::Colour;
			UE_LOG(LogTemp, Warning, TEXT("FRTSOValueBinderDetails::CustomizeChildren - COLOUR"))
		}
		else if (FName("FRTSOSettingsKeyData") == PropertyTypeName)
		{
			BinderRowBuilder->PropertyType = ERTSOSettingsType::Key;
			UE_LOG(LogTemp, Warning, TEXT("FRTSOValueBinderDetails::CustomizeChildren - KEY"))
		}
	}

	BinderRowBuilder->PropertyHandle = PropertyHandle;
	ChildBuilder.AddCustomBuilder(BinderRowBuilder);
}

//FRTSOBinderDetailRowBuilder
#define LOCTEXT_NAMESPACE "RTSOBinderDetails"
TSharedRef<SVerticalBox> FRTSOBinderDetailRowBuilder::GenerateSectionTitle(const FText& SectionText)
{
	return 
	SNew(SVerticalBox)
	+ SVerticalBox::Slot()
	[
		SNew(STextBlock)
		.Text(FText::AsCultureInvariant("Bind To Settings System:"))
	]
	+ SVerticalBox::Slot()
	[
		SNew(STextBlock)
		.Text(SectionText)
	];
}
void FRTSOBinderDetailRowBuilder::SetOnRebuildChildren(FSimpleDelegate InOnRegenerateChildren)
{
	IDetailCustomNodeBuilder::SetOnRebuildChildren(InOnRegenerateChildren);
}
void FRTSOBinderDetailRowBuilder::GenerateHeaderRowContent(FDetailWidgetRow& HeaderRow)
{
}
//
// TODO: Fix this mess. Update: fixed, but still a mess :cry:
void FRTSOBinderDetailRowBuilder::GenerateChildContent(IDetailChildrenBuilder& ChildrenBuilder)
{
	IDetailCustomNodeBuilder::GenerateChildContent(ChildrenBuilder);
	const FSlateFontInfo PropertyFontStyle = FAppStyle::GetFontStyle( TEXT("PropertyWindow.NormalFont") );

	bool bIsNestedStructData = false;
	FStructProperty* AsStructProperty = CastField<FStructProperty>(PropertyHandle->GetProperty());
	if (AsStructProperty != nullptr)
	{
		if (FName("FRTSOSettingsKeyData") == FName(AsStructProperty->Struct->GetStructCPPName())
			|| FName("FVectorWrapper") == FName(AsStructProperty->Struct->GetStructCPPName())
			|| FName("FVector2DWrapper") == FName(AsStructProperty->Struct->GetStructCPPName())
			|| FName("FLinearColorWrapper") == FName(AsStructProperty->Struct->GetStructCPPName())
			|| FName("FColorWrapper") == FName(AsStructProperty->Struct->GetStructCPPName()))
		{
			bIsNestedStructData = true;	
		}
	}

	uint32 NumChildren = 0;
	PropertyHandle->GetNumChildren(NumChildren);
	if (bIsNestedStructData && NumChildren == 0)
	{
		return;
	}

	TSharedRef<SVerticalBox> BinderBox = SNew(SVerticalBox)
	+ SVerticalBox::Slot()
	[
		SNew(STextBlock)
		.Text(FText::AsCultureInvariant("Bind Game Settings"))
		.Font(PropertyFontStyle)
	]
	+ SVerticalBox::Slot()
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.SizeParam(FAuto{})
		[
			SNew(SCheckBox)
			.OnCheckStateChanged(this, &FRTSOBinderDetailRowBuilder::OnCheckboxStateChanged)
		]
		+ SHorizontalBox::Slot()
		.SizeParam(FAuto{})
		[
			// Selected tag
			SNew(SButton)
			.OnPressed(this, &FRTSOBinderDetailRowBuilder::OnTagSelectorPressed)
			[
				SNew(STextBlock)
				.Text(this, &FRTSOBinderDetailRowBuilder::GetTagText)
				.Font(PropertyFontStyle)
			]
		]
	];

	TSharedRef<SWidget> NameContent = 
		bIsNestedStructData 
			? SNew(SHorizontalBox) 
				+ SHorizontalBox::Slot()
				[
					SNew(STextBlock)
					.Text(FText::AsCultureInvariant("Settings Binder Header"))
					.Font(PropertyFontStyle)
				] 
			: SNew(SHorizontalBox) 
				+ SHorizontalBox::Slot()[PropertyHandle->CreatePropertyNameWidget()];
	
	TSharedRef<SWidget> ValueContent = 
		bIsNestedStructData 
		? SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.Padding(FMargin(24, 0, 0, 0))
			[
				BinderBox
			]
		: SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.SizeParam(FAuto{})
			[
				PropertyHandle->CreatePropertyValueWidget()
			]
			+ SHorizontalBox::Slot()
			.Padding(FMargin(24, 0, 0, 0))
			[
				BinderBox
			];

	if (bIsNestedStructData == false)
	{
		FDetailWidgetRow& BinderRow = ChildrenBuilder.AddCustomRow(LOCTEXT("RTSOBinderDetails", "Edit Targets")).RowTag("EditTargetRow");
		BinderRow.NameContent()[NameContent];
		BinderRow.ValueContent()[ValueContent];
		return;
	}

	FText GroupName = FText::Format(FText::AsCultureInvariant("{0}(Bindable)"), PropertyHandle->GetPropertyDisplayName());
	
	IDetailGroup& PropertiesGroup = ChildrenBuilder.AddGroup(FName("SettingsEnabledTypes"), GroupName);
	FDetailWidgetRow& BinderRow = PropertiesGroup.AddWidgetRow();
	BinderRow.NameContent()[NameContent];
	BinderRow.ValueContent()[ValueContent];

	for (uint32 Index = 0; Index < NumChildren; ++Index)
	{
		TSharedPtr<IPropertyHandle> ChildPropertyHandle = PropertyHandle->GetChildHandle(Index);
		if (ChildPropertyHandle.IsValid())
		{
			IDetailPropertyRow& InnerBinderRow = PropertiesGroup.AddPropertyRow(ChildPropertyHandle.ToSharedRef());
		}
	}
}

void FRTSOBinderDetailRowBuilder::OnTagSelectorPressed()
{
	FSlateApplication& SlateApp = FSlateApplication::Get();
	const FWidgetPath WidgetPath = SlateApp.LocateWindowUnderMouse(SlateApp.GetCursorPos(), SlateApp.GetInteractiveTopLevelWindows());

	if (WidgetPath.Widgets.Num() <= 0)
	{
		return;
	}

	const FGeometry& RowWidgetGeometry = WidgetPath.Widgets.Last().Geometry;
	const float LayoutScaleMultiplier =  RowWidgetGeometry.GetAccumulatedLayoutTransform().GetScale();							
	
	TSharedRef<SVerticalBox> MenuContentRef = SpawnFilteredOptionsPickerWithSignatureBinder();
	MenuContentRef->SlatePrepass(LayoutScaleMultiplier);
	const FVector2D DesiredContentSize = MenuContentRef->GetDesiredSize(); // @todo slate: This is ignoring any w indow border size!
	const FVector2D NewPosition = FVector2D(RowWidgetGeometry.AbsolutePosition);
	
	TSharedPtr<IMenu> PopupMenu = FSlateApplication::Get().PushMenu(
		WidgetPath.Widgets.Last().Widget, 
		WidgetPath, MenuContentRef, NewPosition, FPopupTransitionEffect::ComboButton, true, DesiredContentSize, EPopupMethod::CreateNewWindow, false);
	if (PopupMenu.IsValid() == false)
	{
		UE_LOG(LogSlate, Error, TEXT(" FRTSOBinderDetailRowBuilder::OnTagSelectorPressed could not open tag menu"));
		return;
	}
	MenuContentRef->Invalidate(EInvalidateWidgetReason::Layout);
	PopupMenu->GetContent()->Invalidate(EInvalidateWidgetReason::Layout);

	PopupMenu->GetOnMenuDismissed().AddSP(this, &FRTSOBinderDetailRowBuilder::OnTagMenuDismissed);
	PopupMenuPtr = PopupMenu;	
}


void FRTSOBinderDetailRowBuilder::OnTagMenuDismissed(TSharedRef<IMenu> ClosingMenu)
{
	// TODO Do some things here, not sure what yet, but at the very elast we need to unregister our lambda(s) when we've got some that needs to be unregistered
}

void FRTSOBinderDetailRowBuilder::OnCheckboxStateChanged(ECheckBoxState NewState)
{
	// TODO, Handle on check state changed
	UE_LOG(LogTemp, Warning, TEXT("FRTSOBinderDetailRowBuilder::GenerateChildContent::OnCheckStateChanged_Lambda(NewCheckboxState:%s)"), *UEnum::GetValueAsString(NewState))
}

FText FRTSOBinderDetailRowBuilder::GetTagText() const
{
	const URTSOSettingsDeveloperSettings* SettingsDevSettings = GetDefault<URTSOSettingsDeveloperSettings>();
	return FText::FromStringTable(SettingsDevSettings->SettingsStringTablePath, SelectedSettingsTag.ToString());
}

TSharedRef<SExpandableArea> FRTSOBinderDetailRowBuilder::SpawnExpandableOptionsArea()
{
	TSharedRef<SExpandableArea> ExpandableAreaRef = SNew(SExpandableArea)
	.InitiallyCollapsed(true)
	.HeaderContent()
	[
		SNew(STextBlock)
		.Text(FText::AsCultureInvariant("Options"))
	]
	.BodyContent()
	[
		SpawnFilteredOptionsPickerWithSignatureBinder()
	];

	ExpandableAreaPtr = ExpandableAreaRef.ToSharedPtr();
	return ExpandableAreaRef;
}
void FRTSOBinderDetailRowBuilder::PreprocessPropertyType()
{
	PopulateTagArray();
	PropertyCompareTypeName = FPDSettingStatics::SelectTargetFunctionParamType(PropertyType);
}
void FRTSOBinderDetailRowBuilder::PopulateTagArray()
{
	URTSOSettingsSubsystem* SettingsSubsystem = URTSOSettingsSubsystem::Get();
	TagsToShow.Empty();
	PropertyCompareTypeName = nullptr;
	switch(PropertyType)
	{
	case ERTSOSettingsType::Boolean: SettingsSubsystem->CheckBoxStates.GenerateKeyArray(TagsToShow); break;
	case ERTSOSettingsType::FloatSelector:
	case ERTSOSettingsType::FloatSlider: SettingsSubsystem->DoubleStates.GenerateKeyArray(TagsToShow); break;
	case ERTSOSettingsType::IntegerSelector:
	case ERTSOSettingsType::IntegerSlider: SettingsSubsystem->IntegerStates.GenerateKeyArray(TagsToShow); break;
	case ERTSOSettingsType::Vector2: SettingsSubsystem->Vector2dStates.GenerateKeyArray(TagsToShow); break;
	case ERTSOSettingsType::Vector3: SettingsSubsystem->VectorStates.GenerateKeyArray(TagsToShow); break;
	case ERTSOSettingsType::Colour: SettingsSubsystem->ColourStates.GenerateKeyArray(TagsToShow); break;
	case ERTSOSettingsType::String: SettingsSubsystem->StringStates.GenerateKeyArray(TagsToShow); break;
	case ERTSOSettingsType::EnumAsByte: SettingsSubsystem->ByteStates.GenerateKeyArray(TagsToShow); break;

	// Key is special case that crases when we filter out for each individual key, 
	// seemingly resulting from the generated filter string being too long
	// Current workaround is just to show the entire tag parent category, luckily for Keys these are all in one category without any other datatype
	// NOTE: This will have to be resolved as with further development the other settings states are also expected to increase to the point of hitting this 
	// string limit and then we can't use cheap tricks such as adding the tag cateogry to the filter as it will inevitably disply settingtags for other datatypes    
	case ERTSOSettingsType::Key: TagsToShow.Emplace(PD::Settings::TAG_Controls); break;
	default: break;
	}
}

TSharedRef<SGameplayTagPicker> FRTSOBinderDetailRowBuilder::SpawnFilteredOptionsPicker()
{
	FString TagsFilter;

	PopulateTagArray();
	const int32 TagLim = TagsToShow.Num();
	for (int32 TagIdx = 0; TagIdx < TagLim; TagIdx++)
	{
		const FGameplayTag& Tag = TagsToShow[TagIdx];
		TagsFilter.Append(Tag.ToString());
		if (TagIdx < TagLim - 1)
		{
			TagsFilter.Append(",");
		}
	}

	TArray<FGameplayTagContainer> TagContainers;
	TagContainers.Add(FGameplayTagContainer{PD::Settings::TAG_SettingsBase});
	TSharedRef<SGameplayTagPicker> TagPickerRef = SNew(SGameplayTagPicker)
	.ReadOnly(false)
	.ShowMenuItems(false)
	.MaxHeight(550.0f)
	.MultiSelect(false)
	.Filter(TagsFilter)
	.TagContainers(TagContainers)
	.GameplayTagPickerMode(EGameplayTagPickerMode::SelectionMode)
	.OnTagChanged_Lambda(
		[this](const TArray<FGameplayTagContainer>& TagContainers)
		{
			SelectedSettingsTag = TagContainers.IsEmpty() ? FGameplayTag() : TagContainers[0].First();
			if (PopupMenuPtr.IsValid()) 
			{
				PopupMenuPtr->Dismiss();
			}
			UE_LOG(LogTemp, Warning, TEXT("FRTSOBinderDetailRowBuilder::GenerateChildContent::OnTagChanged_Lambda(SelectedSettingsTag:%s)"), *SelectedSettingsTag.ToString())
		});

	TagPickerPtr = TagPickerRef.ToSharedPtr(); 	
	return TagPickerRef;
}

TSharedRef<SVerticalBox> FRTSOBinderDetailRowBuilder::SpawnFilteredOptionsPickerWithSignatureBinder()
{
	PropertyCompareTypeName = FPDSettingStatics::SelectTargetFunctionParamType(PropertyType);
	FProperty* ValueProperty = PropertyHandle->GetProperty();

	QualifiedFunctionPaths.Empty();
	if (ValueProperty != nullptr && ValueProperty->GetOwnerClass() != nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("================================"))

		for (TFieldIterator<UFunction> FunctionIt(ValueProperty->GetOwnerClass(), EFieldIteratorFlags::IncludeSuper);  FunctionIt != nullptr; ++FunctionIt)
		{
			if (FPDSettingStatics::IsUFunctionViableForBinding(*FunctionIt, PropertyCompareTypeName))
			{
				UE_LOG(LogTemp, Warning, TEXT("SpawnFilteredOptionsPickerWithSignatureBinder - VIABLE FUNC > Func(%s)"), *(*FunctionIt)->GetPathName())
				TSharedPtr<FString> FunctionNamePtr = MakeShared<FString>((*FunctionIt)->GetPathName());
				QualifiedFunctionPaths.Emplace(FunctionNamePtr);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("SpawnFilteredOptionsPickerWithSignatureBinder - INCOMPATIBLE FUNC > Func(%s)"), *(*FunctionIt)->GetPathName())
			}
		}
	}

	TSharedRef<SVerticalBox> CombinedBinderListRef = 
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.SizeParam(FAuto{})
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.SizeParam(FAuto{})
			[
				SNew(STextBlock)
				.Text(FText::AsCultureInvariant("BINDABLE FUNCTIONS"))
			]
			+ SVerticalBox::Slot()
			.SizeParam(FAuto{})
			[
				SNew(SListView<TSharedPtr<FString>>)
				.ListItemsSource(&QualifiedFunctionPaths)
				.OnGenerateRow(this, &FRTSOBinderDetailRowBuilder::MakeListViewWidget_QualifiedPreprocessFunctions)
				.OnSelectionChanged(this, &FRTSOBinderDetailRowBuilder::OnOptionSelected_QualifiedPreprocessFunctions)			
				.SelectionMode(ESelectionMode::Single)
				.ItemHeight(24)
			]
		]
		+ SVerticalBox::Slot()
		.SizeParam(FAuto{})
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.SizeParam(FAuto{})
			[
				SNew(STextBlock)
				.Text(FText::AsCultureInvariant("TARGET TAG SELECTOR"))
			]
			+ SVerticalBox::Slot()
			.SizeParam(FAuto{})
			[
				SpawnFilteredOptionsPicker()
			]
		];
	CombinedBinderListPtr = CombinedBinderListRef.ToSharedPtr();
	return CombinedBinderListRef;
}

FName FRTSOBinderDetailRowBuilder::GetName() const
{
	return FName("RTSOBinderDetailRowBuilder");
}

TSharedRef<ITableRow> FRTSOBinderDetailRowBuilder::MakeListViewWidget_QualifiedPreprocessFunctions(TSharedPtr<FString> InQualifiedFunctionPath, const TSharedRef<STableViewBase>& OwnerTable) const
{
	check(InQualifiedFunctionPath.Get() != nullptr)

	FString Trash;
	FString FunctionName;
	InQualifiedFunctionPath->Split(FString(":"), &Trash, &FunctionName, ESearchCase::IgnoreCase, ESearchDir::FromEnd);

	return SNew(STableRow<TSharedPtr<FString>>, OwnerTable)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		[
			SNew(SBorder)
			.Padding(FMargin{2.0f, 5.0f})
			.Content()
			[
				SNew(STextBlock)
				.Text(FText::FromString(*FunctionName))
				.Font(IDetailLayoutBuilder::GetDetailFont()) 
			]
		]
	];
}


void FRTSOBinderDetailRowBuilder::OnOptionSelected_QualifiedPreprocessFunctions(TSharedPtr<FString> InQualifiedFunctionPath, ESelectInfo::Type InSelectInfo)
{
	URTSOSettingsSubsystem* SettingsSubsystem = URTSOSettingsSubsystem::Get();

	switch(InSelectInfo)
	{
	default:
	case ESelectInfo::OnNavigation: 
	break;
      
	case ESelectInfo::OnKeyPress:
	case ESelectInfo::OnMouseClick:
	case ESelectInfo::Direct:
		SettingsSubsystem->SetProcessFunctionForSettingsBoundVariable(SelectedSettingsTag, *InQualifiedFunctionPath);
	break;
	}

	// Todo cche latest selected item perhaps? not sure yet
}

#undef LOCTEXT_NAMESPACE
#endif // WITH_EDITOR

//// Value binder END 
//

bool FPDSettingStatics::IsUFunctionViableForBinding(UFunction* Function, FString PropertyCompareTypeName)
{
	UE_LOG(LogTemp, Warning, TEXT("IsUFunctionViableForBinding - PropertyCompareTypeName(%s)"), *PropertyCompareTypeName)

	const bool bUnexpectedParamCount = Function->NumParms > 2 || 2 > Function->NumParms;
	const bool bNoReturnValue = Function->HasAnyFunctionFlags(FUNC_HasOutParms) == false;
	if (bUnexpectedParamCount || bNoReturnValue)
	{
		UE_LOG(LogTemp, Warning, TEXT("IsUFunctionViableForBinding - ERROR - Function->NumParms(%i) - bNoReturnValue(%i)"), Function->NumParms, bNoReturnValue)
		return false; // Invalid function
	}
			
	bool bPotentialFunction = true;
	for (TFieldIterator<FProperty> PropertyIt(Function); PropertyIt != nullptr; ++PropertyIt)
	{
		FProperty* ParamProperty = *PropertyIt;
		if (ParamProperty->GetCPPType() != PropertyCompareTypeName)
		{
			UE_LOG(LogTemp, Warning, TEXT("IsUFunctionViableForBinding - ERROR - ParamProperty->GetCPPType()(%s)"), *ParamProperty->GetCPPType())
			bPotentialFunction &= false;
			continue;

			// Property->GetCPPType() == FSomeStruct::StaticStruct()->GetStructCPPName()
		}
		bPotentialFunction &= true;					
	}

	return bPotentialFunction;
}

FName FPDSettingStatics::GetFirstInputPropertyName(UFunction* Function)
{
	for (TFieldIterator<FProperty> PropertyIt(Function); PropertyIt != nullptr; ++PropertyIt)
	{
		FProperty* ParamProperty = *PropertyIt;
		if (ParamProperty->HasAnyPropertyFlags(CPF_OutParm) == false)
		{
			return ParamProperty->GetFName();
		}
	}
	return NAME_None;
}

FString FPDSettingStatics::SelectTargetFunctionParamType(ERTSOSettingsType SettingsValueType)
{
	URTSOSettingsSubsystem* SettingsSubsystem = URTSOSettingsSubsystem::Get();
	FString PropertyCompareTypeName;

	switch(SettingsValueType)
	{
	case ERTSOSettingsType::Boolean: PropertyCompareTypeName = TEXT("bool"); break;
	case ERTSOSettingsType::FloatSelector:
	case ERTSOSettingsType::FloatSlider: PropertyCompareTypeName = TEXT("float"); break;
	case ERTSOSettingsType::IntegerSelector:
	case ERTSOSettingsType::IntegerSlider: PropertyCompareTypeName = TEXT("int32"); break;
	case ERTSOSettingsType::String: PropertyCompareTypeName = TEXT("FString"); break;
	case ERTSOSettingsType::EnumAsByte: PropertyCompareTypeName = TEXT("uint8"); break;
	case ERTSOSettingsType::Vector2: PropertyCompareTypeName = TEXT("FVector2D"); break;
	case ERTSOSettingsType::Vector3: PropertyCompareTypeName = TEXT("FVector"); break;
	case ERTSOSettingsType::Colour: PropertyCompareTypeName = TEXT("FColor"); break;
	case ERTSOSettingsType::Key: PropertyCompareTypeName = TEXT("FRTSOSettingsKeyData"); break;
	default: break;
	}
	return PropertyCompareTypeName;
}

TArray<TSharedPtr<FString>> FPDSettingStatics::GenerateQualityStringPtrArray()
{
	TArray<TSharedPtr<FString>> StringSource;
		
	const URTSOSettingsQualityTextLabelSettings* SettingsQualityTextLabelSettings = GetDefault<URTSOSettingsQualityTextLabelSettings>();
	StringSource.Emplace(MakeShared<FString>(SettingsQualityTextLabelSettings->LowQuality.ToString()));
	StringSource.Emplace(MakeShared<FString>(SettingsQualityTextLabelSettings->MidQuality.ToString()));
	StringSource.Emplace(MakeShared<FString>(SettingsQualityTextLabelSettings->HighQuality.ToString()));
	StringSource.Emplace(MakeShared<FString>(SettingsQualityTextLabelSettings->EpicQuality.ToString()));
	return StringSource;
}

TArray<TSharedPtr<FString>> FPDSettingStatics::GenerateStringPtrArrayFromDataSelector(const FRTSOSettingsDataSelector& DataSelector)
{
	TArray<TSharedPtr<FString>> StringSource;
	
	switch(DataSelector.ValueType)
	{
	case ERTSOSettingsType::FloatSelector:   // 'double' or 'float'
	case ERTSOSettingsType::FloatSlider:   // 'double' or 'float'
		{
			TArray<double> DoubleList = DataSelector.GetOptionsList<double>();
			for (double Entry : DoubleList)
			{
				const FString EntryString = FString::Printf(TEXT("%lf"), Entry);
				StringSource.Emplace(MakeShared<FString>(EntryString));
			}
		}
		break;
	case ERTSOSettingsType::IntegerSelector: // 'int32' or 'int64'
	case ERTSOSettingsType::IntegerSlider: // 'int32' or 'int64'
		{
			TArray<int32> IntList = DataSelector.GetOptionsList<int32>();
			for (int32 Entry : IntList)
			{
				const FString EntryString = FString::Printf(TEXT("%i"), Entry);
				StringSource.Emplace(MakeShared<FString>(EntryString));
			}
		}
		break;
	case ERTSOSettingsType::String:  // 'FString'
		{
			TArray<FString> StringList = DataSelector.GetOptionsList<FString>();
			for (const FString& Entry : StringList)
			{
				StringSource.Emplace(MakeShared<FString>(Entry));
			}
		}
		break;
	case ERTSOSettingsType::EnumAsByte: // 'Byte' // TODO Need to bind the enums to a string table, need to store some metadat about settings related enums specifically
		{
			TArray<uint8> ByteList = DataSelector.GetOptionsList<uint8>();
			for (uint8 Entry : ByteList)
			{
				const FString EntryString = FString::Printf(TEXT("BYTE: %i"), Entry);
				StringSource.Emplace(MakeShared<FString>(EntryString));
			}
		}
		break;
	}

	return StringSource;
}

void UPDTransitionWidget::StartTransition_Implementation()
{
	PlayAnimation(TransitionAnimation);
}

void UPDTransitionWidget::EndTransition_Implementation()
{
	PlayAnimation(TransitionAnimation, 0 ,1, EUMGSequencePlayMode::Reverse);

	const FLatentActionInfo DelayInfo{0,0, TEXT("RemoveFromParent"), this};
	UKismetSystemLibrary::Delay(this, TransitionAnimation->GetEndTime(), DelayInfo);
}

FRTSOSettingsKeyData::FRTSOSettingsKeyData(FGuid IdToMatchAgainst, FKey InMain_KeyBoardMouse) : Main_KeyBoardMouse(InMain_KeyBoardMouse)
{
	HiddenSettingIdToMatchAgainst = IdToMatchAgainst;
}
FRTSOSettingsKeyData::FRTSOSettingsKeyData(FGuid IdToMatchAgainst, FKey InMain_KeyBoardMouse, FKey InMain_GenericGamepad, FKey InMain_DS45, FKey InMain_XSX) 
		: Main_KeyBoardMouse(InMain_KeyBoardMouse), Main_GenericGamepad(InMain_GenericGamepad), Main_DS45(InMain_DS45), Main_XSX(InMain_XSX)
{
	HiddenSettingIdToMatchAgainst = IdToMatchAgainst;
}	
FRTSOSettingsKeyData::FRTSOSettingsKeyData(FGuid IdToMatchAgainst, 
		FKey InMain_KeyBoardMouse, FKey InMain_GenericGamepad, FKey InMain_DS45, FKey InMain_XSX, 
		FKey InAlt_KeyBoardMouse, FKey InAlt_GenericGamepad, FKey InAlt_DS45, FKey InAlt_XSX) 
		: Main_KeyBoardMouse(InMain_KeyBoardMouse), Main_GenericGamepad(InMain_GenericGamepad), Main_DS45(InMain_DS45), Main_XSX(InMain_XSX) 
		, Alt_KeyBoardMouse(InAlt_KeyBoardMouse), Alt_GenericGamepad(InAlt_GenericGamepad), Alt_DS45(InAlt_DS45), Alt_XSX(InAlt_XSX)		
{
	HiddenSettingIdToMatchAgainst = IdToMatchAgainst;
}

FRTSOSettingsKeyData::FRTSOSettingsKeyData(FKey InMain_KeyBoardMouse) : Main_KeyBoardMouse(InMain_KeyBoardMouse)
{
	HiddenSettingIdToMatchAgainst = FGuid::NewGuid();
}
FRTSOSettingsKeyData::FRTSOSettingsKeyData(FKey InMain_KeyBoardMouse, FKey InMain_GenericGamepad, FKey InMain_DS45, FKey InMain_XSX) 
		: Main_KeyBoardMouse(InMain_KeyBoardMouse), Main_GenericGamepad(InMain_GenericGamepad), Main_DS45(InMain_DS45), Main_XSX(InMain_XSX)
{
	HiddenSettingIdToMatchAgainst = FGuid::NewGuid();
}	
FRTSOSettingsKeyData::FRTSOSettingsKeyData(
		FKey InMain_KeyBoardMouse, FKey InMain_GenericGamepad, FKey InMain_DS45, FKey InMain_XSX, 
		FKey InAlt_KeyBoardMouse, FKey InAlt_GenericGamepad, FKey InAlt_DS45, FKey InAlt_XSX) 
		: Main_KeyBoardMouse(InMain_KeyBoardMouse), Main_GenericGamepad(InMain_GenericGamepad), Main_DS45(InMain_DS45), Main_XSX(InMain_XSX) 
		, Alt_KeyBoardMouse(InAlt_KeyBoardMouse), Alt_GenericGamepad(InAlt_GenericGamepad), Alt_DS45(InAlt_DS45), Alt_XSX(InAlt_XSX)		
{
	HiddenSettingIdToMatchAgainst = FGuid::NewGuid();
}

namespace PD::Settings
{
	UE_DEFINE_GAMEPLAY_TAG(TAG_SettingsBase, "Settings")

	//
	// Settings - Gameplay categories
	UE_DEFINE_GAMEPLAY_TAG(TAG_Camera, "Settings.Gameplay.Camera")
	UE_DEFINE_GAMEPLAY_TAG(TAG_ActionLog, "Settings.Gameplay.ActionLog")
	UE_DEFINE_GAMEPLAY_TAG(TAG_Difficulty, "Settings.Gameplay.Difficulty")

	// Settings - Video categories
	UE_DEFINE_GAMEPLAY_TAG(TAG_Display, "Settings.Video.Display")
	UE_DEFINE_GAMEPLAY_TAG(TAG_Effects, "Settings.Video.Effects") 
	UE_DEFINE_GAMEPLAY_TAG(TAG_Graphics, "Settings.Video.Graphics")

	// Settings - Audio categories
	UE_DEFINE_GAMEPLAY_TAG(TAG_Audio, "Settings.Audio")
	UE_DEFINE_GAMEPLAY_TAG(TAG_Audio_Volume, "Settings.Audio.Volume")

	// Settings - Control categories
	UE_DEFINE_GAMEPLAY_TAG(TAG_Controls, "Settings.Controls")
	UE_DEFINE_GAMEPLAY_TAG(TAG_Controls_UI, "Settings.Controls.UI") 

	// Settings - Interface categories
	UE_DEFINE_GAMEPLAY_TAG(TAG_Interface, "Settings.Interface") 

	namespace Gameplay::Camera
	{
		UE_DEFINE_GAMEPLAY_TAG(TAG_Camera_RotationRateModifier, "Settings.Gameplay.Camera.RotationRateModifier") // (Slider, Limit to range [40, 100])
		UE_DEFINE_GAMEPLAY_TAG(TAG_Camera_TargetInterpSpeed, "Settings.Gameplay.Camera.TargetInterpSpeed") // (Slider, Limit to range [1, 10])
		UE_DEFINE_GAMEPLAY_TAG(TAG_Camera_ScrollSpeed, "Settings.Gameplay.Camera.ScrollSpeed") // (Slider, Limit to range [1, 10])
		UE_DEFINE_GAMEPLAY_TAG(TAG_Camera_DoF, "Settings.Gameplay.Camera.DOF") // - Depth of Field (Slider)
		UE_DEFINE_GAMEPLAY_TAG(TAG_Camera_FoV, "Settings.Gameplay.Camera.FOV") // - Field of View (Slider)
	}
	namespace Gameplay::ActionLog
	{
		UE_DEFINE_GAMEPLAY_TAG(TAG_ActionLog_Show, "Settings.Gameplay.ActionLog.Show") // Checkbox
		UE_DEFINE_GAMEPLAY_TAG(TAG_ActionLog_ColorHightlight0, "Settings.Gameplay.ActionLog.ColorHightlight0") // FColor picker needed
		UE_DEFINE_GAMEPLAY_TAG(TAG_ActionLog_ColorHightlight1, "Settings.Gameplay.ActionLog.ColorHightlight1") // FColor picker needed		
	}
	namespace Gameplay::Difficulty
	{
		UE_DEFINE_GAMEPLAY_TAG(TAG_Difficulty_Combat, "Settings.Gameplay.Difficulty.Combat") // - Combat difficulty (AI Behaviour, Overall damage, NO health increased)
		UE_DEFINE_GAMEPLAY_TAG(TAG_Difficulty_World, "Settings.Gameplay.Difficulty.World") // - World difficulty (FogOfWar, Puzzles?, Resource costs)
		// NOTE: Difficulty modifiers should affect AI behavior and overall damage, but never increase AI or player health.
		// NOTE: We want to avoid bullet sponges. Make the player die fast but the enemies die faster.
		// NOTE: Difficulty modifiers should also affect things such a fog of war clearing distance and such.		
	}
	
	namespace Video::Display
	{
		UE_DEFINE_GAMEPLAY_TAG(TAG_Display_Mode, "Settings.Video.Display.Mode") // Mode (List of options (Windowed, Borderless, Fullscreen))
		UE_DEFINE_GAMEPLAY_TAG(TAG_Display_Resolution, "Settings.Video.Display.Resolution") // Resolution (List of available resolutions)
		UE_DEFINE_GAMEPLAY_TAG(TAG_Display_ResolutionScale, "Settings.Video.Display.ResolutionScale") // Resolution (List of available resolutions)
		UE_DEFINE_GAMEPLAY_TAG(TAG_Display_FPSLimit, "Settings.Video.Display.FPSLimit") // FPS Limit (30, 60, 120, 144)
		UE_DEFINE_GAMEPLAY_TAG(TAG_Display_VSyncEnabled, "Settings.Video.Display.VSync") // V-Sync (Checkbox)
		UE_DEFINE_GAMEPLAY_TAG(TAG_Display_Gamma, "Settings.Video.Display.Gamma") // Gamma (Slider)
		UE_DEFINE_GAMEPLAY_TAG(TAG_Display_AntiAliasing, "Settings.Video.Display.AntiAliasing") // - Anti-aliasing (List of AA options)
	}

	namespace Video::Effects
	{
		UE_DEFINE_GAMEPLAY_TAG(TAG_Effects_AmbientOcclusion, "Settings.Video.Effects.AmbientOcclusion") // - Ambient Occlusion (CheckBox)
		UE_DEFINE_GAMEPLAY_TAG(TAG_Effects_MotionBlur, "Settings.Video.Effects.MotionBlur") // - Motion blur (CheckBox)
		UE_DEFINE_GAMEPLAY_TAG(TAG_Effects_FilmGrain, "Settings.Video.Effects.FilmGrain") // - Film grain (CheckBox)
	}

	namespace Video::Graphics
	{
		UE_DEFINE_GAMEPLAY_TAG(TAG_Graphics_TextureQuality, "Settings.Video.Graphics.TextureQuality") // Texture quality (List of Quality options)
		UE_DEFINE_GAMEPLAY_TAG(TAG_Graphics_ShadowQuality, "Settings.Video.Graphics.ShadowQuality") // Shadow quality (List of Quality options)
		UE_DEFINE_GAMEPLAY_TAG(TAG_Graphics_ReflectionQuality, "Settings.Video.Graphics.ReflectionQuality") // Reflection quality ? (List of Quality options)
		UE_DEFINE_GAMEPLAY_TAG(TAG_Graphics_GIQuality, "Settings.Video.Graphics.GIQuality") // Global Illumination quality ? (List of Quality options)
		UE_DEFINE_GAMEPLAY_TAG(TAG_Graphics_ParticleEffect, "Settings.Video.Graphics.ParticleQuality") // Particle Effect Quality (List of Quality options)
		UE_DEFINE_GAMEPLAY_TAG(TAG_Graphics_FoliageQuality, "Settings.Video.Graphics.FoliageQuality") // Mesh Quality (List of Quality options)
		UE_DEFINE_GAMEPLAY_TAG(TAG_Graphics_ResolutionQuality, "Settings.Video.Graphics.ResolutionQuality") // Animation Quality (List of Quality options)
		UE_DEFINE_GAMEPLAY_TAG(TAG_Graphics_PostProcessQuality, "Settings.Video.Graphics.PostProcessQuality") // Post process Quality (List of Quality options)
		UE_DEFINE_GAMEPLAY_TAG(TAG_Graphics_ViewDistance, "Settings.Video.Graphics.ViewDistance") // View Distance (List of Quality options)
	}
	
	namespace Audio
	{
		UE_DEFINE_GAMEPLAY_TAG(TAG_Audio_MasterVolume, "Settings.Audio.Volume.Master") // . Master Volume (Slider)
		UE_DEFINE_GAMEPLAY_TAG(TAG_Audio_SFXVolume, "Settings.Audio.Volume.SFX")    // - SFX volume (Slider)
		UE_DEFINE_GAMEPLAY_TAG(TAG_Audio_MusicVolume, "Settings.Audio.Volume.Music")  // - Music Volume (Slider)
		UE_DEFINE_GAMEPLAY_TAG(TAG_Audio_AmbientVolume, "Settings.Audio.Volume.Ambient")  // - Ambient Volume (Slider)
		UE_DEFINE_GAMEPLAY_TAG(TAG_Audio_VoiceVolume, "Settings.Audio.Volume.Voice")  // - Voice volume (Slider)
		UE_DEFINE_GAMEPLAY_TAG(TAG_Audio_Subtitles, "Settings.Audio.Subtitles") // -- Subtitles (List of available language options)		
	}

	namespace Controls::Game
	{
		UE_DEFINE_GAMEPLAY_TAG(TAG_Controls_MoveForward, "Settings.Controls.MoveForward") // - Discrete Key selector, with two alts for each input type (explicit tags for each alt is not needed)
		UE_DEFINE_GAMEPLAY_TAG(TAG_Controls_MoveBackward, "Settings.Controls.MoveBackward") // - Discrete Key selector, with two alts for each input type (explicit tags for each alt is not needed)
		UE_DEFINE_GAMEPLAY_TAG(TAG_Controls_MoveLeftward, "Settings.Controls.MoveLeftward") // - Discrete Key selector, with two alts for each input type (explicit tags for each alt is not needed)
		UE_DEFINE_GAMEPLAY_TAG(TAG_Controls_MoveRightward, "Settings.Controls.MoveRightward") // - Discrete Key selector, with two alts for each input type (explicit tags for each alt is not needed)
		UE_DEFINE_GAMEPLAY_TAG(TAG_Controls_RotateLeft, "Settings.Controls.RotateLeft") // - Discrete Key selector, with two alts for each input type (explicit tags for each alt is not needed)
		UE_DEFINE_GAMEPLAY_TAG(TAG_Controls_RotateRight, "Settings.Controls.RotateRight") // - Discrete Key selector, with two alts for each input type (explicit tags for each alt is not needed)
		UE_DEFINE_GAMEPLAY_TAG(TAG_Controls_Interact, "Settings.Controls.Interact") // - Discrete Key selector, with two alts for each input type (explicit tags for each alt is not needed)
		UE_DEFINE_GAMEPLAY_TAG(TAG_Controls_PauseMenu, "Settings.Controls.PauseMenu") // - Discrete Key selector, with two alts for each input type (explicit tags for each alt is not needed)
		UE_DEFINE_GAMEPLAY_TAG(TAG_Controls_GameMenu, "Settings.Controls.GameMenu") // - Discrete Key selector, with two alts for each input type (explicit tags for each alt is not needed)
		UE_DEFINE_GAMEPLAY_TAG(TAG_Controls_GameMenu_Inventory, "Settings.Controls.GameMenu.Inventory") // - Discrete Key selector, with two alts for each input type (explicit tags for each alt is not needed)
		UE_DEFINE_GAMEPLAY_TAG(TAG_Controls_GameMenu_Map, "Settings.Controls.GameMenu.Map") // - Discrete Key selector, with two alts for each input type (explicit tags for each alt is not needed)
		UE_DEFINE_GAMEPLAY_TAG(TAG_Controls_GameMenu_QuestLog, "Settings.Controls.GameMenu.QuestLog") // - Discrete Key selector, with two alts for each input type (explicit tags for each alt is not needed)
		UE_DEFINE_GAMEPLAY_TAG(TAG_Controls_Zoom, "Settings.Controls.Zoom") // - Analog key selector (or two buttoned key selector), with two alts for each input type (explicit tags for each alt is not needed)
		
	}
	namespace Controls::UI
	{
		UE_DEFINE_GAMEPLAY_TAG(TAG_Controls_UI_AcceptOrEnter, "Settings.Controls.UI.AcceptOrEnter") // - Discrete Key selector, with two alts for each input type (explicit tags for each alt is not needed)
		UE_DEFINE_GAMEPLAY_TAG(TAG_Controls_UI_CancelOrBack, "Settings.Controls.UI.CancelOrBack") // - Discrete Key selector, with two alts for each input type (explicit tags for each alt is not needed)
		UE_DEFINE_GAMEPLAY_TAG(TAG_Controls_UI_NextTab, "Settings.Controls.UI.NextTab") // - Discrete Key selector, with two alts for each input type (explicit tags for each alt is not needed)
		UE_DEFINE_GAMEPLAY_TAG(TAG_Controls_UI_PreviousTab, "Settings.Controls.UI.PreviousTab") // - Discrete Key selector, with two alts for each input type (explicit tags for each alt is not needed)
		UE_DEFINE_GAMEPLAY_TAG(TAG_Controls_UI_InnerNextTab, "Settings.Controls.UI.InnerNextTab") // - Discrete Key selector, with two alts for each input type (explicit tags for each alt is not needed)
		UE_DEFINE_GAMEPLAY_TAG(TAG_Controls_UI_InnerPreviousTab, "Settings.Controls.UI.InnerPreviousTab") // - Discrete Key selector, with two alts for each input type (explicit tags for each alt is not needed)
		UE_DEFINE_GAMEPLAY_TAG(TAG_Controls_UI_NavigateUp, "Settings.Controls.UI.NavigateUp") // - Discrete Key selector, with two alts for each input type (explicit tags for each alt is not needed)
		UE_DEFINE_GAMEPLAY_TAG(TAG_Controls_UI_NavigateDown, "Settings.Controls.UI.NavigateDown") // - Discrete Key selector, with two alts for each input type (explicit tags for each alt is not needed)
		UE_DEFINE_GAMEPLAY_TAG(TAG_Controls_UI_NavigateLeft, "Settings.Controls.UI.NavigateLeft") // - Discrete Key selector, with two alts for each input type (explicit tags for each alt is not needed)
		UE_DEFINE_GAMEPLAY_TAG(TAG_Controls_UI_NavigateRight, "Settings.Controls.UI.NavigateRight") // - Discrete Key selector, with two alts for each input type (explicit tags for each alt is not needed)
	}
	
	namespace Interface
	{
		UE_DEFINE_GAMEPLAY_TAG(TAG_Interface_UIScaling, "Settings.Interface.UIScaling") // - UIScaling slider (Range [0.5 , 4])
		UE_DEFINE_GAMEPLAY_TAG(TAG_Interface_ShowObjectiveMarkers, "Settings.Interface.ShowObjectiveMarkers") // - CheckBox
	}

	//
	// Gameplay
	using Entry = TTuple<FGameplayTag, FRTSOSettingsDataSelector>;
	TMap<FGameplayTag, FRTSOSettingsDataSelector> FGameplaySettingsDefaults::Camera =
		{
			Entry{Gameplay::Camera::TAG_Camera_RotationRateModifier, FRTSOSettingsDataSelector{ERTSOSettingsType::IntegerSlider, 40, FRTSOMinMax<int32>{40, 100}}},
			Entry{Gameplay::Camera::TAG_Camera_TargetInterpSpeed, FRTSOSettingsDataSelector{ERTSOSettingsType::FloatSlider, 2.0 * FloatUIMultiplier, FRTSOMinMax<double>{FloatUIMultiplier, 10 * FloatUIMultiplier}}},
			Entry{Gameplay::Camera::TAG_Camera_ScrollSpeed, FRTSOSettingsDataSelector{ERTSOSettingsType::FloatSlider, 2.0 * FloatUIMultiplier , FRTSOMinMax<double>{FloatUIMultiplier, 10 * FloatUIMultiplier}}},
			Entry{Gameplay::Camera::TAG_Camera_DoF, FRTSOSettingsDataSelector{ERTSOSettingsType::FloatSlider, 2.0 * FloatUIMultiplier, FRTSOMinMax<double>{FloatUIMultiplier, 10 * FloatUIMultiplier}}},
			Entry{Gameplay::Camera::TAG_Camera_FoV, FRTSOSettingsDataSelector{ERTSOSettingsType::IntegerSlider, 90, FRTSOMinMax<int32>{0, 180}}}
		};

	TMap<FGameplayTag, FRTSOSettingsDataSelector> FGameplaySettingsDefaults::ActionLog =
		{
			Entry{Gameplay::ActionLog::TAG_ActionLog_Show, FRTSOSettingsDataSelector{ERTSOSettingsType::Boolean, true}},
			Entry{Gameplay::ActionLog::TAG_ActionLog_ColorHightlight0, FRTSOSettingsDataSelector{ERTSOSettingsType::Colour, FColor::Orange}},
			Entry{Gameplay::ActionLog::TAG_ActionLog_ColorHightlight1, FRTSOSettingsDataSelector{ERTSOSettingsType::Colour, FColor::Silver}}
		};

	TMap<FGameplayTag, FRTSOSettingsDataSelector> FGameplaySettingsDefaults::Difficulty =
		{
			Entry{Gameplay::Difficulty::TAG_Difficulty_Combat, FRTSOSettingsDataSelector{ERTSOSettingsType::String, DifficultyDefaultStrings[0], FRTSOMinMax<FString>{}, DifficultyDefaultStrings}},
			Entry{Gameplay::Difficulty::TAG_Difficulty_World, FRTSOSettingsDataSelector{ERTSOSettingsType::String, DifficultyDefaultStrings[0], FRTSOMinMax<FString>{}, DifficultyDefaultStrings}},
		};

	//
	// Video
	TMap<FGameplayTag, FRTSOSettingsDataSelector> FVideoSettingsDefaults::Display =
		{
			Entry{Video::Display::TAG_Display_Gamma, FRTSOSettingsDataSelector{ERTSOSettingsType::IntegerSlider, 50, FRTSOMinMax<int32>{0, 100}}},
			Entry{Video::Display::TAG_Display_Mode, FRTSOSettingsDataSelector{ERTSOSettingsType::EnumAsByte, ERTSOResolutionMode::Windowed}},
			Entry{Video::Display::TAG_Display_Resolution, FRTSOSettingsDataSelector{ERTSOSettingsType::String, Resolution_Strings[1], FRTSOMinMax<FString>{}, Resolution_Strings, Resolution_Values}},
			Entry{Video::Display::TAG_Display_ResolutionScale, FRTSOSettingsDataSelector{ERTSOSettingsType::IntegerSlider, 100, FRTSOMinMax<int32>{10, 100}}},
			Entry{Video::Display::TAG_Display_AntiAliasing, FRTSOSettingsDataSelector{ERTSOSettingsType::String, FString("4"), FRTSOMinMax<FString>{}, TArray<FString>{"0", "1", "2", "3", "4", "5", "6"}, TArray{0, 1, 2, 3, 4, 5, 6}}},
			Entry{Video::Display::TAG_Display_VSyncEnabled, FRTSOSettingsDataSelector{ERTSOSettingsType::Boolean, false}},
			Entry{Video::Display::TAG_Display_FPSLimit, FRTSOSettingsDataSelector{ERTSOSettingsType::String, FString("60"), FRTSOMinMax<FString>{}, TArray<FString>{"0", "30", "60", "120", "144"}, TArray{0, 30, 60, 120, 144}}},
		};

	TMap<FGameplayTag, FRTSOSettingsDataSelector> FVideoSettingsDefaults::Effects =
		{
			Entry{Video::Effects::TAG_Effects_AmbientOcclusion, FRTSOSettingsDataSelector{ERTSOSettingsType::Boolean, false}},
			Entry{Video::Effects::TAG_Effects_MotionBlur, FRTSOSettingsDataSelector{ERTSOSettingsType::Boolean, false}},
			Entry{Video::Effects::TAG_Effects_FilmGrain, FRTSOSettingsDataSelector{ERTSOSettingsType::Boolean, false}},
		};

	TMap<FGameplayTag, FRTSOSettingsDataSelector> FVideoSettingsDefaults::Graphics =
		{
			Entry{Video::Graphics::TAG_Graphics_TextureQuality, FRTSOSettingsDataSelector{ERTSOSettingsType::String, QualityDefaultStrings[0], FRTSOMinMax<FString>{}, QualityDefaultStrings}},
			Entry{Video::Graphics::TAG_Graphics_ShadowQuality, FRTSOSettingsDataSelector{ERTSOSettingsType::String, QualityDefaultStrings[0], FRTSOMinMax<FString>{}, QualityDefaultStrings}},
			Entry{Video::Graphics::TAG_Graphics_ReflectionQuality, FRTSOSettingsDataSelector{ERTSOSettingsType::String, QualityDefaultStrings[0], FRTSOMinMax<FString>{}, QualityDefaultStrings}},
			Entry{Video::Graphics::TAG_Graphics_GIQuality, FRTSOSettingsDataSelector{ERTSOSettingsType::String, QualityDefaultStrings[0], FRTSOMinMax<FString>{}, QualityDefaultStrings}},
			Entry{Video::Graphics::TAG_Graphics_ParticleEffect, FRTSOSettingsDataSelector{ERTSOSettingsType::String, QualityDefaultStrings[0], FRTSOMinMax<FString>{}, QualityDefaultStrings}},
			Entry{Video::Graphics::TAG_Graphics_FoliageQuality, FRTSOSettingsDataSelector{ERTSOSettingsType::String, QualityDefaultStrings[0], FRTSOMinMax<FString>{}, QualityDefaultStrings}},
			Entry{Video::Graphics::TAG_Graphics_ResolutionQuality, FRTSOSettingsDataSelector{ERTSOSettingsType::String, QualityDefaultStrings[0], FRTSOMinMax<FString>{}, QualityDefaultStrings}},
			Entry{Video::Graphics::TAG_Graphics_PostProcessQuality, FRTSOSettingsDataSelector{ERTSOSettingsType::String, QualityDefaultStrings[0], FRTSOMinMax<FString>{}, QualityDefaultStrings}},
			Entry{Video::Graphics::TAG_Graphics_ViewDistance, FRTSOSettingsDataSelector{ERTSOSettingsType::String, QualityDefaultStrings[0], FRTSOMinMax<FString>{}, QualityDefaultStrings}},
		};

	//
	// Audio
	TMap<FGameplayTag, FRTSOSettingsDataSelector> FAudioSettingsDefaults::Base =
		{
			Entry{Audio::TAG_Audio_MasterVolume, FRTSOSettingsDataSelector{ERTSOSettingsType::FloatSlider, 70.0 * FloatUIMultiplier, FRTSOMinMax<double>{0, 100 * FloatUIMultiplier}}},
			Entry{Audio::TAG_Audio_MusicVolume, FRTSOSettingsDataSelector{ERTSOSettingsType::FloatSlider, 70.0 * FloatUIMultiplier, FRTSOMinMax<double>{0, 100 * FloatUIMultiplier}}},
			Entry{Audio::TAG_Audio_AmbientVolume, FRTSOSettingsDataSelector{ERTSOSettingsType::FloatSlider, 70.0 * FloatUIMultiplier, FRTSOMinMax<double>{0, 100 * FloatUIMultiplier}}},
			Entry{Audio::TAG_Audio_VoiceVolume, FRTSOSettingsDataSelector{ERTSOSettingsType::FloatSlider, 70.0 * FloatUIMultiplier, FRTSOMinMax<double>{0, 100 * FloatUIMultiplier}}},
			Entry{Audio::TAG_Audio_SFXVolume, FRTSOSettingsDataSelector{ERTSOSettingsType::FloatSlider, 70.0 * FloatUIMultiplier, FRTSOMinMax<double>{0, 100 * FloatUIMultiplier}}},
			Entry{Audio::TAG_Audio_Subtitles, FRTSOSettingsDataSelector{ERTSOSettingsType::Boolean, true}},
		};	

	//
	// Controls
	TMap<FGameplayTag, FRTSOSettingsDataSelector> FControlsSettingsDefaults::Game =
		{
			Entry{Controls::Game::TAG_Controls_MoveForward, FRTSOSettingsDataSelector{ERTSOSettingsType::Key, FRTSOSettingsKeyData(FKey{EKeys::W})}},
			Entry{Controls::Game::TAG_Controls_MoveBackward, FRTSOSettingsDataSelector{ERTSOSettingsType::Key, FRTSOSettingsKeyData{FKey{EKeys::S}}}},
			Entry{Controls::Game::TAG_Controls_MoveLeftward, FRTSOSettingsDataSelector{ERTSOSettingsType::Key, FRTSOSettingsKeyData{FKey{EKeys::A}}}},
			Entry{Controls::Game::TAG_Controls_MoveRightward, FRTSOSettingsDataSelector{ERTSOSettingsType::Key, FRTSOSettingsKeyData{FKey{EKeys::D}}}},
			Entry{Controls::Game::TAG_Controls_RotateLeft, FRTSOSettingsDataSelector{ERTSOSettingsType::Key, FRTSOSettingsKeyData{FKey{EKeys::Q}}}},
			Entry{Controls::Game::TAG_Controls_RotateRight, FRTSOSettingsDataSelector{ERTSOSettingsType::Key, FRTSOSettingsKeyData{FKey{EKeys::E}}}},
			Entry{Controls::Game::TAG_Controls_Interact, FRTSOSettingsDataSelector{ERTSOSettingsType::Key, FRTSOSettingsKeyData{FKey{EKeys::LeftMouseButton}}}},
			Entry{Controls::Game::TAG_Controls_PauseMenu, FRTSOSettingsDataSelector{ERTSOSettingsType::Key, FRTSOSettingsKeyData{FKey{EKeys::Escape}}}},
			Entry{Controls::Game::TAG_Controls_GameMenu, FRTSOSettingsDataSelector{ERTSOSettingsType::Key, FRTSOSettingsKeyData{FKey{EKeys::Tab}}}},
			Entry{Controls::Game::TAG_Controls_GameMenu_Inventory, FRTSOSettingsDataSelector{ERTSOSettingsType::Key, FRTSOSettingsKeyData{FKey{EKeys::I}}}},
			Entry{Controls::Game::TAG_Controls_GameMenu_Map, FRTSOSettingsDataSelector{ERTSOSettingsType::Key, FRTSOSettingsKeyData{FKey{EKeys::M}}}},
			Entry{Controls::Game::TAG_Controls_GameMenu_QuestLog, FRTSOSettingsDataSelector{ERTSOSettingsType::Key, FRTSOSettingsKeyData{FKey{EKeys::L}}}},
			Entry{Controls::Game::TAG_Controls_Zoom, FRTSOSettingsDataSelector{ERTSOSettingsType::Key, FRTSOSettingsKeyData{FKey{EKeys::MouseScrollUp}}}},
		};	
	TMap<FGameplayTag, FRTSOSettingsDataSelector> FControlsSettingsDefaults::UI =
		{
			Entry{Controls::UI::TAG_Controls_UI_AcceptOrEnter, FRTSOSettingsDataSelector{ERTSOSettingsType::Key, FRTSOSettingsKeyData{FKey{EKeys::Enter}, FKey{EKeys::SpaceBar}}}},
			Entry{Controls::UI::TAG_Controls_UI_CancelOrBack, FRTSOSettingsDataSelector{ERTSOSettingsType::Key, FRTSOSettingsKeyData{FKey{EKeys::Escape}, FKey{EKeys::BackSpace}}}},
			Entry{Controls::UI::TAG_Controls_UI_NextTab, FRTSOSettingsDataSelector{ERTSOSettingsType::Key, FRTSOSettingsKeyData{FKey{EKeys::One}}}},
			Entry{Controls::UI::TAG_Controls_UI_PreviousTab, FRTSOSettingsDataSelector{ERTSOSettingsType::Key, FRTSOSettingsKeyData{FKey{EKeys::Three}}}},
			Entry{Controls::UI::TAG_Controls_UI_InnerNextTab, FRTSOSettingsDataSelector{ERTSOSettingsType::Key, FRTSOSettingsKeyData{FKey{EKeys::Q}}}},
			Entry{Controls::UI::TAG_Controls_UI_InnerPreviousTab, FRTSOSettingsDataSelector{ERTSOSettingsType::Key, FRTSOSettingsKeyData{FKey{EKeys::E}}}},
			Entry{Controls::UI::TAG_Controls_UI_NavigateUp, FRTSOSettingsDataSelector{ERTSOSettingsType::Key, FRTSOSettingsKeyData{FKey{EKeys::W}, FKey{EKeys::Up}}}},
			Entry{Controls::UI::TAG_Controls_UI_NavigateDown, FRTSOSettingsDataSelector{ERTSOSettingsType::Key, FRTSOSettingsKeyData{FKey{EKeys::S}, FKey{EKeys::Down}}}},
			Entry{Controls::UI::TAG_Controls_UI_NavigateLeft, FRTSOSettingsDataSelector{ERTSOSettingsType::Key, FRTSOSettingsKeyData{FKey{EKeys::A}, FKey{EKeys::Left}}}},
			Entry{Controls::UI::TAG_Controls_UI_NavigateRight, FRTSOSettingsDataSelector{ERTSOSettingsType::Key, FRTSOSettingsKeyData{FKey{EKeys::D}, FKey{EKeys::Right}}}},
		};

	//
	// Interface
	TMap<FGameplayTag, FRTSOSettingsDataSelector> FInterfaceSettingsDefaults::Base =
		{
			Entry{Interface::TAG_Interface_UIScaling, FRTSOSettingsDataSelector{ERTSOSettingsType::FloatSlider,  FloatUIMultiplier, FRTSOMinMax<double>{0.5 * FloatUIMultiplier, 4 * FloatUIMultiplier}}},
			Entry{Interface::TAG_Interface_ShowObjectiveMarkers, FRTSOSettingsDataSelector{ERTSOSettingsType::Boolean, true}},
		};
	
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