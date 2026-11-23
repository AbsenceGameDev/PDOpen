/* @author: Ario Amin @ Permafrost Development. @copyright: Full BSL(1.1) License included at bottom of the file  */

#include "PDProgressionSharedUI.h"

#include "Textures/SlateIcon.h"
#include "Framework/Commands/UIAction.h"

#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Views/SListView.h"
#include "Styling/AppStyle.h"
#include "Fonts/FontMeasure.h"

#include "Subsystems/PDProgressionSubsystem.h"

#define LOCTEXT_NAMESPACE "SPDStatList"

// @note Limited to 10 fields
TSharedRef<SHeaderRow> FPDStatStatics::CreateHeaderRow(
	TSharedPtr<SHeaderRow>& ExistingHeader,
	double SectionWidth,
	double SectionCount,
	...)
{
	if (ExistingHeader.IsValid() == false)
	{
		ExistingHeader = SNew(SHeaderRow);
	}
	ExistingHeader->ClearColumns();

	//
	// Gather variadic arguments, expected 2 const char* per argument, one for the id, the other for the label 
	va_list Args;
	va_start(Args, SectionCount);

	TArray<TArray<FString>> Arguments;
	Arguments.SetNum(SectionCount);
	
	int32 FieldIdx = 0;
	// int32 EntryIdx = 0;
	for (; Args != nullptr && FieldIdx < SectionCount; )
	{
		TArray<FString>& InnerFields = Arguments[FieldIdx];
		InnerFields = TArray<FString>
		{
			va_arg(Args, TCHAR*),
			va_arg(Args, TCHAR*)
		};
		
		FieldIdx++;
	}
	--FieldIdx;
	va_end(Args);

	if (FieldIdx == INDEX_NONE) { return ExistingHeader.ToSharedRef(); }
	
	// Apply the gathered data and create a header column

	FieldIdx = 0;
	for (; FieldIdx < SectionCount ; FieldIdx++)
	{
		const FString& ColID = Arguments[FieldIdx][0];
		const FString& Label = Arguments[FieldIdx][1];
		
		SHeaderRow::FColumn::FArguments ColumnArguments =
			SHeaderRow::FColumn::FArguments{}
			.DefaultLabel(FText::FromString(Label))
			.FixedWidth(SectionWidth)
			.ColumnId(FName(*ColID));
		ExistingHeader->AddColumn(ColumnArguments);
	}
	
	return ExistingHeader.ToSharedRef();
}


FPDStatDataViewSlotParams FPDStatStatics::CreateDataViewSlotParam(
	FGameplayTag StatTag,
	double InTotalValue,
	const bool IncludeParent)
{
	const FString StatAndCategoryText =
		IncludeParent
		? UPDStatSubsystem::GetTagNameLeafAndParent(StatTag)
		: UPDStatSubsystem::GetTagNameLeaf(StatTag);

	const double TotalValue = InTotalValue;
	const double AbsTotalValue = FMath::Abs(TotalValue);
	const FString Sign = AbsTotalValue > 0.0 ? "+" : "-";

	return 
	FPDStatDataViewSlotParams{
		FText::FromString(StatAndCategoryText),
		FText::FromString(Sign + FString::FromInt(AbsTotalValue))};
}



//
// Stat curve
void SStatCurve::Construct( const FArguments& InArgs )
{
	MinValue = InArgs._MinValue;
	MaxValue = InArgs._MaxValue;
	FixedLabelSpacing = InArgs._FixedLabelSpacing;

	// FAppStyle::GetBrush( TEXT("PropertyWindow.Background"));
	BackgroundImage = FAppStyle::GetBrush( TEXT("ProfileVisualizer.Background"));

	Zoom = 1.0f;
	Offset = 0.0f;
}


#define DrawCurveLine(LinePoints) \
FSlateDrawElement::MakeLines( \
	OutDrawElements, \
	RetLayerId++, \
	AllottedGeometry.ToPaintGeometry(), \
	LinePoints, \
	ESlateDrawEffect::None, \
	FLinearColor::White \
	)

int32 SStatCurve::OnPaint( const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const
{
	FSlateFontInfo MyFont = FCoreStyle::GetDefaultFontStyle("Regular", 10);
	
	// Used to track the layer ID we will return.
	int32 RetLayerId = LayerId;
	const TSharedRef< FSlateFontMeasure > FontMeasureService = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
	
	// Paint inside the border only. 
	const FVector2D BorderPadding = FAppStyle::GetVector( TEXT("ProfileVisualizer.ProgressBar.BorderPadding"));

	const float OffsetX = DrawingOffsetX; // BorderPadding.X
	const float Width = DrawingGeometry.Size.X; // AllottedGeometry.Size.X - - 2.0f * BorderPadding.X


	// Create line points
	const float ValueScale = MaxValue - MinValue;
	const int32 NumValues = FMath::FloorToInt( AllottedGeometry.Size.X * Zoom / FixedLabelSpacing );

	TArray< FVector2D > LinePoints;
	LinePoints.AddUninitialized( 2 );

	// Scoped
	{
		LinePoints[0] = FVector2D( OffsetX, BorderPadding.Y + 1.0f );
		LinePoints[1] = FVector2D( OffsetX + Width, BorderPadding.Y + 1.0f );

		// Draw lines
		DrawCurveLine(LinePoints);
	}

	const FVector2D TextDrawSize = FontMeasureService->Measure(TEXT("0.00"), MyFont);
	const float LineHeight = AllottedGeometry.Size.Y - BorderPadding.Y - TextDrawSize.Y - 2.0f;
	
	for( int32 LineIndex = 0; LineIndex <= NumValues; LineIndex++ )
	{
		const float NormalizedX = static_cast<float>(LineIndex) / NumValues;
		const float LineX = Offset + NormalizedX * Zoom;
		if( LineX < 0.0f || LineX > 1.0f ) { continue; }

		const float LineXPos =  OffsetX + Width * LineX;
		LinePoints[0] = FVector2D( LineXPos, BorderPadding.Y );
		LinePoints[1] = FVector2D( LineXPos, LineHeight );

		// Draw lines
		DrawCurveLine(LinePoints);
		
		FString ValueText( FString::Printf( TEXT("%.2f"), MinValue + NormalizedX * ValueScale ) );
		FVector2D DrawSize = FontMeasureService->Measure(ValueText, MyFont);
		FVector2D TextPos( LineXPos - DrawSize.X * 0.5f, LineHeight );

		if( TextPos.X < 0.0f )
		{
			TextPos.X = 0.0f;
		}
		else if( (TextPos.X + DrawSize.X) > AllottedGeometry.Size.X )
		{
			TextPos.X = OffsetX + Width - DrawSize.X;
		}

		FSlateDrawElement::MakeText( 
			OutDrawElements,
			RetLayerId,
			AllottedGeometry.ToOffsetPaintGeometry( TextPos ),
			ValueText,
			MyFont,
			ESlateDrawEffect::None,
			FLinearColor::White
			);
	}

	// Always draw lines at the start and at the end of the timeline
	{
		LinePoints[0] = FVector2D( OffsetX, BorderPadding.Y );
		LinePoints[1] = FVector2D( OffsetX, LineHeight );

		// Draw lines
		DrawCurveLine(LinePoints);

	}

	{
		LinePoints[0] = FVector2D( OffsetX + Width, BorderPadding.Y );
		LinePoints[1] = FVector2D( OffsetX + Width, LineHeight );

		// Draw lines
		DrawCurveLine(LinePoints);
	}

	return RetLayerId - 1;
}

void SStatCurve::Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime )
{
	DrawingOffsetX = DrawingGeometry.AbsolutePosition.X - AllottedGeometry.AbsolutePosition.X;
}

FReply SStatCurve::OnMouseButtonDown( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	return SCompoundWidget::OnMouseButtonDown( MyGeometry, MouseEvent );
}

FReply SStatCurve::OnMouseMove( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	return SCompoundWidget::OnMouseMove( MyGeometry, MouseEvent );
}

FVector2D SStatCurve::ComputeDesiredSize( float ) const
{
	return FVector2D( 8.0f, 8.0f );
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

