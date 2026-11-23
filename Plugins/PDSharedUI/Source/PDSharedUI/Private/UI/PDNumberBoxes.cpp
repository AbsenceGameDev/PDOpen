/* @author: Ario Amin @ Permafrost Development. @copyright: Full BSL(1.1) License included at bottom of the file  */

#include "UI/PDNumberBoxes.h"

#include "PDUIBaseDefinitions.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"

//
// Legal according to ISO due to explicit template instantiation bypassing access rules: https://eel.is/c++draft/temp.friend

// Creates a static data member that will store a of type Tag::TType in which to store the address of a private member.
template <class Tag> struct TPrivateAccessor { static typename Tag::TType TypeValue; }; 
template <class Tag> typename Tag::TType TPrivateAccessor<Tag>::TypeValue;

// Generate a static data member whose constructor initializes TPrivateAccessor<Tag>::TypeValue.
// This type will only be named in an explicit instantiation, where it is legal to pass the address of a private member.
template <class Tag, typename Tag::TType TypeValue>
struct TTagPrivateMember
{
	TTagPrivateMember() { TPrivateAccessor<Tag>::TypeValue = TypeValue; }
	static TTagPrivateMember PrivateInstance;
};
template <class Tag, typename Tag::TType x> 
TTagPrivateMember<Tag,x> TTagPrivateMember<Tag,x>::PrivateInstance;

// A templated tag type for a given private member.  Each distinct private member you need to access should have its own tag.
// Each tag should contain a nested ::TType that is the corresponding pointer-to-member type.'
template<typename TAccessorType, typename TAccessorValue>
struct TAccessorTypeHandler { typedef TAccessorValue(TAccessorType::*TType); };

//
// Ranged Editable number box

void UPDRangedNumberBox::SetupDelegates()
{
	if (ValidatedNumberBox->OnTextCommitted.IsBound())
	{
		ValidatedNumberBox->OnTextCommitted.Clear();
	}

	ValidatedNumberBox->OnTextCommitted.AddDynamic(this, &UPDRangedNumberBox::OnTextBoxCommitted);
}

// Unseemly matters, avert your gaze. Note: This is done because 'Text' is marked as deprecated and
// will be moved into private access level at some point, I want to avoid a broke build when that happens
using EditableTextType = TAccessorTypeHandler<UEditableTextBox, FText>; 
template struct TTagPrivateMember<EditableTextType, &UEditableTextBox::Text>; // UHT complains as it does not realize this is a template 'tag', this is not a common way to access data 

void UPDRangedNumberBox::OnTextBoxCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
	switch (CommitMethod)
	{
	case ETextCommit::OnCleared:
		return; // Exit function if we've just cleared the box
	default:
		break; // In all other cases
	}
	
	const int32 CommittedInt = FCString::Atoi(*Text.ToString());
	ValidateNewValue(CommittedInt);
}

void UPDRangedNumberBox::ValidateNewValue(int32 InCount)
{
	UE_LOG(PDLog_SharedUI, VeryVerbose, TEXT("UPDRangedNumberBox::ValidateNewValue -- InCount: %i "), InCount);
	SelectedCount = FMath::Clamp(InCount, MinimumCount, MaximumCount);
	if (OnValueChanged.ExecuteIfBound(SelectedCount) == false)
	{
		// @todo log warning
	}
	
	constexpr ETextIdenticalModeFlags ComparisonFlags = ETextIdenticalModeFlags::DeepCompare | ETextIdenticalModeFlags::LexicalCompareInvariants;
	FText& InnerText = ValidatedNumberBox->*TPrivateAccessor<EditableTextType>::TypeValue; // nasty ISO-valid hack, please forgive

	const FText NewText = FText::FromString(FString::FromInt(SelectedCount));
	if (InnerText.IdenticalTo(NewText, ComparisonFlags) == false)
	{
		InnerText = NewText;
		ValidatedNumberBox->BroadcastFieldValueChanged(UEditableTextBox::FFieldNotificationClassDescriptor::Text);
	}
}

//
// Ranged Incremental number box
void UPDRangedIncrementBox::NativePreConstruct()
{
	Super::NativePreConstruct();
	SetupDelegates();
}
void UPDRangedIncrementBox::SetupDelegates()
{
	IncrementTextBlock->SetText(IncrementText);
	DecrementTextBlock->SetText(DecrementText);

	FSlateFontInfo ModifiedIncButtonFont = IncrementTextBlock->GetFont();
	FSlateFontInfo ModifiedDecButtonFont = DecrementTextBlock->GetFont();
	FSlateFontInfo ModifiedValueFont = NumberTextBlock->GetFont();
	ModifiedIncButtonFont.Size = ModifiedDecButtonFont.Size = ButtonTextFontSize;
	ModifiedValueFont.Size = ValueTextFontSize;	
	
	IncrementTextBlock->SetFont(ModifiedIncButtonFont);
	DecrementTextBlock->SetFont(ModifiedDecButtonFont);
	NumberTextBlock->SetFont(ModifiedValueFont);

	if (HitboxIncrement->OnPressed.IsBound()){HitboxIncrement->OnPressed.Clear();}
	if (HitboxDecrement->OnPressed.IsBound()){HitboxDecrement->OnPressed.Clear();}

	HitboxIncrement->OnPressed.AddDynamic(this, &UPDRangedIncrementBox::OnIncrement);
	HitboxDecrement->OnPressed.AddDynamic(this, &UPDRangedIncrementBox::OnDecrement);
}

void UPDRangedIncrementBox::ValidateNewValue(int32 InCount)
{
	UE_LOG(PDLog_SharedUI, VeryVerbose, TEXT("UPDRangedIncrementBox::ValidateNewValue -- InCount: %i "), InCount);

	SelectedCount = FMath::Clamp(InCount, MinimumCount, MaximumCount);
	const FText NewText = FText::FromString(FString::FromInt(SelectedCount));

	if (OnValueChanged.ExecuteIfBound(SelectedCount) == false)
	{
		// @todo log warning
	}
	
	NumberTextBlock->SetText(NewText);
}

void UPDRangedIncrementBox::OnIncrement()
{
	ValidateNewValue(++SelectedCount);
}

void UPDRangedIncrementBox::OnDecrement()
{
	ValidateNewValue(--SelectedCount);
}

void UPDRangedSelector::NativePreConstruct()
{
	Super::NativePreConstruct();
	ApplySettings(SliderSettings);
}

void UPDRangedSelector::NativeDestruct()
{
	Super::NativeDestruct();

	if (RangedSlider == nullptr || RangedSlider->IsValidLowLevelFast() == false
		|| RangedNumberBox == nullptr || RangedNumberBox->IsValidLowLevelFast() == false
		|| RangedIncrementBox == nullptr || RangedIncrementBox->IsValidLowLevelFast() == false)
	{
		return;
	}

	RangedSlider->OnValueChanged.Clear();
	RangedNumberBox->OnValueChanged.Unbind();
	RangedIncrementBox->OnValueChanged.Unbind();
}

void UPDRangedSelector::ApplySettings(FPDRangedSliderSettings InSliderSettings)
{	
	SliderSettings = InSliderSettings;
	MinimumCount = FMath::Min(SliderSettings.MinimumCount, SliderSettings.MaximumCount); 
	MaximumCount = FMath::Max(SliderSettings.MinimumCount, SliderSettings.MaximumCount); 

	if (RangedSlider == nullptr || RangedSlider->IsValidLowLevelFast() == false
		|| RangedNumberBox == nullptr || RangedNumberBox->IsValidLowLevelFast() == false
		|| RangedIncrementBox == nullptr || RangedIncrementBox->IsValidLowLevelFast() == false)
	{
		return;
	}

	RangedSliderTextBlock->SetVisibility(ESlateVisibility::Hidden);
	RangedSlider->SetVisibility(ESlateVisibility::Hidden);
	RangedNumberBox->SetVisibility(ESlateVisibility::Hidden);
	RangedIncrementBox->SetVisibility(ESlateVisibility::Hidden);

	EPDSharedUICountTypeSelector SelectedCountType = bUseGlobalUICountTypeSelector 
		? GetDefault<UPDSharedUISettings>()->UICountTypeSelector 
		: CountTypeSelector;

	// @todo remember last used value?
	switch (SelectedCountType)
	{
	case EPDSharedUICountTypeSelector::ERangedSlider:
		if (RangedSlider->OnValueChanged.Contains(this, STATIC_FUNCTION_FNAME(TEXT("UPDRangedSelector::OnSliderValueChanged"))) == false)
		{
			RangedSlider->OnValueChanged.AddDynamic(this, &UPDRangedSelector::OnSliderValueChanged);
		}
		
		RangedSlider->SetValue(SelectedCount);
		RangedSlider->SetStepSize(SliderSettings.StepSize);
		RangedSliderTextBlock->SetText(FText::AsCultureInvariant(FString::Printf(TEXT("%lf"), static_cast<double>(SelectedCount) / SliderSettings.FloatUIMultiplier)));

		RangedSlider->SetMinValue(MinimumCount);
		RangedSlider->SetMaxValue(MaximumCount);
		
		RangedSlider->SetVisibility(ESlateVisibility::Visible);
		RangedSliderTextBlock->SetVisibility(ESlateVisibility::Visible);
		break;
	case EPDSharedUICountTypeSelector::ERangedEditableNumber:
		RangedNumberBox->SetupDelegates();

		if (RangedNumberBox->OnValueChanged.IsBoundToObject(this) == false)
		{
			RangedNumberBox->OnValueChanged.BindUObject(this, &UPDRangedSelector::OnNumberBoxChanged);
		};
		RangedNumberBox->ApplySettings(SelectedCount, MinimumCount, MaximumCount);
		RangedNumberBox->SetVisibility(ESlateVisibility::Visible);
		break;
	case EPDSharedUICountTypeSelector::ERangedIncrementBox:
		RangedIncrementBox->ButtonTextFontSize = ButtonTextFontSize;
		RangedIncrementBox->ValueTextFontSize = ValueTextFontSize;
		RangedIncrementBox->SetupDelegates();

		if (RangedIncrementBox->OnValueChanged.IsBoundToObject(this) == false)
		{
			RangedIncrementBox->OnValueChanged.BindUObject(this, &UPDRangedSelector::OnNumberBoxChanged);
		}

		RangedIncrementBox->ApplySettings(SelectedCount, MinimumCount, MaximumCount);
		RangedIncrementBox->SetVisibility(ESlateVisibility::Visible);
		break;
	}
}

void UPDRangedSelector::OnRangeUpdated(int32 NewMin, int32 NewMax)
{
	MinimumCount = NewMin != INDEX_NONE ? NewMin : MinimumCount;
	MaximumCount = NewMax != INDEX_NONE ? FMath::Max(MinimumCount, NewMax) : MaximumCount; // don't allow Max to be below Min

	EPDSharedUICountTypeSelector SelectedCountType = bUseGlobalUICountTypeSelector 
		? GetDefault<UPDSharedUISettings>()->UICountTypeSelector 
		: CountTypeSelector;
	
	switch (SelectedCountType)
	{
	case EPDSharedUICountTypeSelector::ERangedSlider:
		RangedSlider->SetMinValue(MinimumCount);
		RangedSlider->SetMaxValue(MaximumCount);
		break;
	case EPDSharedUICountTypeSelector::ERangedEditableNumber:
		RangedNumberBox->ApplySettings(SelectedCount, MinimumCount, MaximumCount);
		break;
	case EPDSharedUICountTypeSelector::ERangedIncrementBox:
		RangedIncrementBox->ApplySettings(SelectedCount, MinimumCount, MaximumCount);
		break;
	}
}

void UPDRangedSelector::OnSliderValueChanged(float NewValue)
{
	OnNumberBoxChanged(static_cast<int32>(NewValue + 0.5f)); // Halving the speed of the slider
	RangedSliderTextBlock->SetText(FText::FromString(FString::Printf(TEXT("%f"), SelectedCount / SliderSettings.FloatUIMultiplier))); 
}

void UPDRangedSelector::OnNumberBoxChanged(int32 NewValue)
{
	SelectedCount = NewValue;

	PostValueChanged.Broadcast(SelectedCount);
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
