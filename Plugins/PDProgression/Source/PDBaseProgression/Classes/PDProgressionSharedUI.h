/* @author: Ario Amin @ Permafrost Development. @copyright: Full BSL(1.1) License included at bottom of the file  */

#pragma once

#include "CoreMinimal.h"
#include "GameplayTags.h"
#include "Subsystems/EngineSubsystem.h"

#include "Layout/Geometry.h"
#include "Input/Reply.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"

#include "PDProgressionSharedUI.generated.h"

class FPaintArgs;
class FSlateWindowElementList;
struct FSlateBrush;

/** @brief Unused. Reserved  */
namespace EPDStatListSections
{
	enum
	{
		EName = 0,
		ECategory,
		ELevel,
		EExperience,
		EValue,
		EValueOffset
	};
}

/** @brief Static helper functions for stat widgets  */
struct PDBASEPROGRESSION_API FPDStatStatics
{
public:

	/** @brief Creates a header row and fills it's column id's and labels based on the input
	 * @note - Ensures the header exists, instantiates it otherwise
	 * @note - Adds all requested columns to the table then returns the header ref */
	static TSharedRef<SHeaderRow> CreateHeaderRow(
		TSharedPtr<SHeaderRow>& ExistingHeader,
		double SectionWidth,
		double SectionCount,
		...);
	
	/** @brief Resolves the strings for the section text and value text of a dataview slot */
	static FPDStatDataViewSlotParams CreateDataViewSlotParam(
		FGameplayTag StatTag,
		double InTotalValue,
		bool IncludeParent = true);
	
};

/** @brief Keeps a 'section' text and 'value' text to be displayed in a data-view slot */
USTRUCT(Blueprintable)
struct FPDStatDataViewSlotParams
{
	GENERATED_BODY()

	/** @brief The title-/label- text of a target */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText SectionText;

	/** @brief The value-text of a target */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText ValueText;	
};

/** @brief @todo/in-progress Widget for data regarding the selected stats level and experience
 * @todo Need to have an experience-bar which also may display numbers
 * @details Show amount of tokens, and their types that will be earned upon gaining next level, and any relevant stat changes that will occur */
template<typename TViewType>
struct FPDStatViewHeaderData
{
	/** @brief Actual header slate widget  */
	TSharedPtr<SHeaderRow> Header = nullptr;
	
	/** @brief List-view that will be displayed under the header  */
	TSharedPtr<SListView<TSharedPtr<TViewType>>> ListView = nullptr;
	/** @brief Pointer to the inner data that will be displayed under the header  */
	TArray<TSharedPtr<TViewType>>* DataViewPtr = nullptr;

	/** @brief Mostly unused. Reserved for later use. Rethink any current use */
	TSharedPtr<STableRow< TSharedPtr<TViewType>>> LatestTableRow;	
};

constexpr int32 FallbackUniformSize = 800;

/** @brief Contains all settings that ended up being teh same on different widgets, keeping them together here makes things a bit more contained and easier for a human to parse the different widget class definitions */
USTRUCT(Blueprintable)
struct FPDWidgetBaseSettings
{
	GENERATED_BODY()

	FPDWidgetBaseSettings()
		: Visibility(ESlateVisibility()) {}		
	
	FPDWidgetBaseSettings(
		ESlateVisibility InVisibility)
		: Visibility(InVisibility)
		, MaxSUWidth(FallbackUniformSize)
		, MaxSUHeight(FallbackUniformSize) {}	
	
	FPDWidgetBaseSettings(
		ESlateVisibility InVisibility,
		int32 InMaxUniformSize)
		: Visibility(InVisibility)
		, MaxSUWidth(InMaxUniformSize)
		, MaxSUHeight(InMaxUniformSize) {}	
	
	FPDWidgetBaseSettings(
		ESlateVisibility InVisibility_ModifySourcesPopup,
		int32 InMaxSUWidth,
		int32 InMaxSUHeight)
		: Visibility(InVisibility_ModifySourcesPopup)
		, MaxSUWidth(InMaxSUWidth)
		, MaxSUHeight(InMaxSUHeight) {}

	//
	// Widget controls
	
	/** @brief Our main title font  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Progression|Widgets|Modify Sources")
	FSlateFontInfo TitleFont;
	/** @brief Our sub-title font  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Progression|Widgets|Modify Sources")
	FSlateFontInfo SubTitleFont;
	
	/** @brief Our table-header font .
	 * @todo        This is not set up currently, SHeaderRow does not have a straight forward way to change the font afaik
	 * @todo cont.  but I will dig through the source when I have some more spare time for lower priority changes */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Progression|Widgets|Modify Sources")
	FSlateFontInfo TableHeaderFont;
	/** @brief Our table-entry font  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Progression|Widgets|Modify Sources")
	FSlateFontInfo TableEntryFont;
	
	
	/** @brief ModifySourcesPopup - BP/editor Exposed visibility controls */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Progression|Widgets|Modify Sources")
	ESlateVisibility Visibility; // = ESlateVisibility::Visible;
	/** @brief To pass down to widgets we construct within ourselves to control their sizing/limits */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Progression|Widgets|Modify Sources")
	int32 MaxSUWidth = 800;
	/** @brief To pass down to widgets we construct within ourselves to control their sizing/limits */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Progression|Widgets|Modify Sources")
	int32 MaxSUHeight = 800;

	/** @brief The (uniform) width of each section in our data-view widget(s) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Progression|Widgets|Modify Sources")
	int32 DataViewSectionWidth = 120;	
};


/** @brief DataView Widget base class, Keeps some shared functions as-well as assigning our dataview types via template parameters */
template<typename ... TViewTypes>
struct PDBASEPROGRESSION_API FPDStatWidgetBase
{
private:
	static constexpr std::size_t TViewTypeCount = sizeof...(TViewTypes); 
public:
	FPDStatWidgetBase(): OwnerID(0) { }
	FPDStatWidgetBase(int32 InOwnerID) : OwnerID(InOwnerID) {}
	virtual ~FPDStatWidgetBase() = default;

	/** @brief Initializes/Updates our font(s) if they have no valid typeface  */
	void InitializeFonts()
	{
		if (WidgetSettings.TitleFont.HasValidFont() == false)
		{
			UpdateFonts();
		}
	}
	/** @brief Retrieves an immutable reference to the widget settings */
	virtual const FPDWidgetBaseSettings& RetrieveSettings() const { return WidgetSettings; };
	

	/** @brief Implement in child classes to return our header row definition */
	virtual TSharedPtr<SHeaderRow> RefreshHeaderRow(int32 HeaderRowIdx = 0) = 0;

	/** @brief Base call, ensures we have a title-font loaded, Sets up the child slot, and passes in the data view array to an slistview wrapped in a scrollbox */
	virtual void UpdateChildSlot() = 0;

	/** @brief Must be overridden in child classes to implement translation during detected drag movements */
	virtual FReply DesignTimeTranslation(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) = 0;

	/** @brief Gets a fontstyle, and then apply different scaling to the different fonts  */
	void UpdateFonts()
	{
		// @todo Set up a custom slate styleset for the saveeditors fonts and icons 
		WidgetSettings.SubTitleFont = WidgetSettings.TitleFont = FAppStyle::GetFontStyle( TEXT("PropertyWindow.NormalFont"));
		WidgetSettings.TitleFont.Size *= 8;
		WidgetSettings.SubTitleFont.Size *= 2;
	}

protected:
	/** @brief Overwrites the widget settings, used when we have changes something in UMG land and want to update the base settings */
	virtual void UpdateSettings(const FPDWidgetBaseSettings& OverrideSettings) { WidgetSettings = OverrideSettings; };
	/** @brief Retrieves an mutable reference to the widget settings */
	virtual FPDWidgetBaseSettings& RetrieveMutableSettings() const { return const_cast<FPDWidgetBaseSettings&>(WidgetSettings); };	

public:
	
	
	/** @brief ID of player/Owner of this slate widget */
	int32 OwnerID;
	/** @brief The selected tag, needed cache for some derived classes */
	FGameplayTag SelectedStatTag;
	
	/** @brief Our data-view meta-data */
	TTuple<FPDStatViewHeaderData<TViewTypes>...> HeaderDataViews;

	/** @brief Flag that we use to tell widgets that implements us if we are in design-time or not */
	bool bIsDesignTime = true;

private:
	/** @brief The settings for our widget */
	FPDWidgetBaseSettings WidgetSettings;
};

/** @brief Modify Source data construct. Used by the 'SPDSelectedStat_OffsetData' slate widgets */
USTRUCT(Blueprintable)
struct FPDStatViewModifySource
{
	GENERATED_BODY()

	/** @brief Tag of source stat */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag StatTag{};
	
	/** @brief Offset applied from source stat */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	double AppliedStatOffset = 0.0;

	/** @brief Offset applied from source stat */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UCurveFloat* StatOffsetCurveSource = nullptr;		
};

/** @brief Stat cross-behaviour data construct. Used by the 'SPDSelectedStat_LevelData' slate widgets */
USTRUCT(Blueprintable)
struct FPDStatViewAffectedStat
{
	GENERATED_BODY()

	/** @brief Tag of source stat */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag AffectedStat;

	/** @brief The total effect from another stat on this stat */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	double TotalAffectedDelta;
	
};

/** A timeline widget.*/
class PDBASEPROGRESSION_API SStatCurve : public SCompoundWidget
{

public:
	SLATE_BEGIN_ARGS( SStatCurve )
		: _MinValue( 0.0f )
		, _MaxValue( 1.0f )
		, _FixedLabelSpacing(1.0)
		{}

		/** Min. value on the curve */
		SLATE_ARGUMENT( float, MinValue )

		/** Max. value on the curve */
		SLATE_ARGUMENT( float, MaxValue )

		/** fixed pixel spacing between centers of labels */
		SLATE_ARGUMENT( float, FixedLabelSpacing )

	SLATE_END_ARGS()

	/** @brief Sets min/max calues, label spacing and our 'BackgroundImage' brush. Resets our 'Zoom' and 'Offset'  */
	void Construct( const FArguments& InArgs );

	// SWidget interface
	virtual int32 OnPaint( const FPaintArgs& Args,  const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const override;
	virtual void Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime ) override;
	virtual FReply OnMouseButtonDown( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) override;
	virtual FReply OnMouseMove( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) override;
	FVector2D ComputeDesiredSize(float) const override;
	// End of SWidget interface

	/** @brief Sets zoom, clamps it to a minimum of 1.0 */
	void SetZoom(float InZoom) { Zoom = FMath::Max(InZoom, 1.0f); }

	/** @brief Returns the current zoom value */
	float GetZoom() const { return Zoom; }
	
	/** @brief Update our offset */
	void SetOffset(float InOffset) { Offset = InOffset; } 

	/** @brief Get our current offset */
	float GetOffset() const { return Offset; }

	/** @brief Override our min/max values */
	void SetMinMaxValues(float InMin, float InMax)
	{
		MinValue = InMin;
		MaxValue = InMax;
	}

	/** @brief Return our min/max values */
	void GetMinMaxValues(float &InMin, float &InMax)
	{
		InMin = MinValue;
		InMax = MaxValue;
	}

	/** @brief Override our draw geom. */
	void SetDrawingGeometry(const FGeometry& Geometry) { DrawingGeometry = Geometry; }

	/** @brief Retrieve our draw geom. */
	FGeometry GetDrawingGeometry() const { return DrawingGeometry; }

private:

	/** Background image to use for the graph bar */
	const FSlateBrush* BackgroundImage = nullptr;

	/** Minimum value on the timeline */
	float MinValue = 0;

	/** Maximum value on the timeline */
	float MaxValue = 1;

	/** fixed pixel spacing between centers of labels */
	float FixedLabelSpacing = 1;

	/** Current zoom of the graph */
	float Zoom = 1;

	/** Current offset of the graph */
	float Offset = 0;

	/** @brief Offset our Draws on x */
	float DrawingOffsetX = 0;

	/** @brief Our drawing geometry */
	FGeometry DrawingGeometry;
};


//
// Stat Widget Base
#define SetTitleSlot(TSlateType, Title, TitleFont) \
		TSlateType::Slot() \
		  .Padding(FMargin{0, 0}) \
		  .AutoHeight() \
		[ \
			SNew(STextBlock) \
				.Font(TitleFont) \
				.Justification(ETextJustify::Center) \
				.Text(Title) \
		]


#define SetListViewSlot(TSlateType, SlotMaxSize, TargetListView) \
TSlateType::Slot() \
	.Padding(FMargin{0, 0}) \
	.FillHeight(10) \
[ \
	SNew(SScrollBox) \
	.ScrollBarAlwaysVisible(true) \
	.ScrollBarVisibility(EVisibility::Visible) \
	.ScrollBarThickness(UE::Slate::FDeprecateVector2DParameter(10)) \
	.Orientation(EOrientation::Orient_Vertical) \
	+SScrollBox::Slot() \
		.AutoSize() \
		.MaxSize(SlotMaxSize) \
	[ \
		TargetListView \
	] \
]

#define SetListHeaderSlot(TSlateType, TextContent, InFont, SectionWidth) \
SHorizontalBox::Slot() \
.MaxWidth(SectionWidth) \
[ \
	SNew(STextBlock) \
	.Text(TextContent) \
	.Font(InFont) \
	.MinDesiredWidth(SectionWidth) \
]

#define SetWidgetTitle(TSlateType, Title, TitleFont) \
	SNew(TSlateType) \
		+ SetTitleSlot(TSlateType, Title, TitleFont)


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
