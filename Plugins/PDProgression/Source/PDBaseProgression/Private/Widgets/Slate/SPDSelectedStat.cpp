/* @author: Ario Amin @ Permafrost Development. @copyright: Full BSL(1.1) License included at bottom of the file  */

#include "Widgets/Slate/SPDSelectedStat.h"

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
#include "Widgets/Notifications/SProgressBar.h"

// Class picker 
#include "Components/PDProgressionComponent.h"
#include "Subsystems/PDProgressionSubsystem.h"

#define LOCTEXT_NAMESPACE "SPDStatList"


FText SPDSelectedStat_LevelData::SelectedStatLevelLabel = LOCTEXT("Stat_LevelData_Title", "LEVEL");;
FText SPDSelectedStat_LevelData::Token_SectionTitle = LOCTEXT("Stat_LevelData_TokensToGrant", "TOKENS");;
FText SPDSelectedStat_LevelData::TokenName_ColumnLabel = LOCTEXT("Stat_LevelData_TokensToGrant", "NAME");;
FText SPDSelectedStat_LevelData::TokenCount_ColumnLabel = LOCTEXT("Stat_LevelData_TokensToGrant", "COUNT");;
FText SPDSelectedStat_LevelData::OtherStats_SectionTitle = LOCTEXT("Stat_LevelData_OtherAffected", "OTHER STATS");;
FText SPDSelectedStat_LevelData::OtherStatsAffectedName_ColumnLabel = LOCTEXT("Stat_LevelData_OtherAffected", "NAME");;
FText SPDSelectedStat_LevelData::OtherStatsAffectedValue_ColumnLabel = LOCTEXT("Stat_LevelData_OtherAffected", "VALUE");;

FText SPDSelectedStat_LevelData::TokenEntryLabel = LOCTEXT("Stat_LevelData_TokensToGrant_Entry", "Token: ");;
FText SPDSelectedStat_LevelData::OtherStatsAffectedEntryLabel = LOCTEXT("Stat_LevelData_OtherAffected_Entry", "Affected Stats:");;
FText SPDSelectedStat_LevelData::ExperienceBar_Title = LOCTEXT("Stat_LevelData_ExperienceBar_Title", "EXPERIENCE");;


FText SPDSelectedStat_OffsetData::StatSources_Header_Title = LOCTEXT("StatSource_HeaderRow_Name", "OFFSETS");
FText SPDSelectedStat_OffsetData::StatSources_Header_Name = LOCTEXT("StatSource_HeaderRow_Name", "NAME");
FText SPDSelectedStat_OffsetData::StatSources_Header_Category = LOCTEXT("StatSource_HeaderRow_Name", "CATEGORY");
FText SPDSelectedStat_OffsetData::StatSources_Header_AppliedOffset = LOCTEXT("StatSource_HeaderRow_Name", "VALUE OFFSET");
FText SPDSelectedStat_OffsetData::StatSources_Header_Curves = LOCTEXT("StatSource_HeaderRow_Name", "CURVE");


//
// Selected stat - Level data


void SPDSelectedStat_LevelData::Construct(
	const FArguments& InArgs,
	int32 InOwnerID,
	const FGameplayTag& InSelectedStatTag,
	TArray<TSharedPtr<FPDSkillTokenBase>>& TokenArrayRef,
	TArray<TSharedPtr<FPDStatViewAffectedStat>>& AffectedStatsRef,
	const FPDWidgetBaseSettings& WidgetSettingsUpdate)
{
	UpdateSettings(WidgetSettingsUpdate);
	
	TArray<TSharedPtr<FPDSkillTokenBase>>*& TokenArrayPtr = HeaderDataViews.Key.DataViewPtr;
	TArray<TSharedPtr<FPDStatViewAffectedStat>>*& AffectedStatsPtr = HeaderDataViews.Value.DataViewPtr;

	OwnerID = InOwnerID;
	SelectedStatTag = InSelectedStatTag;
	TokenArrayPtr = &TokenArrayRef;
	AffectedStatsPtr = &AffectedStatsRef;

	PrepareData();
	UpdateChildSlot();
}

TSharedPtr<SHeaderRow> SPDSelectedStat_LevelData::RefreshHeaderRow(int32 HeaderRowIdx)
{
	const FPDWidgetBaseSettings& Settings = RetrieveSettings();
	
	// We want to be able to update the  header row, without redrawing the whole list
	switch (HeaderRowIdx)
	{
	default: 
	case 0:
		{
			TSharedPtr<SHeaderRow> TokenHeader = 
			FPDStatStatics::CreateHeaderRow(HeaderDataViews.Key.Header, Settings.DataViewSectionWidth, 2,
				"0", *TokenName_ColumnLabel.ToString(),
				"1", *TokenCount_ColumnLabel.ToString());
			TokenHeader->Invalidate(EInvalidateWidgetReason::Paint);

			return TokenHeader;
		}
	case 1:
		{
			TSharedPtr<SHeaderRow> AffectedHeader = 	
			FPDStatStatics::CreateHeaderRow(HeaderDataViews.Value.Header, Settings.DataViewSectionWidth, 2,
				"0", *OtherStatsAffectedName_ColumnLabel.ToString(),
				"1", *OtherStatsAffectedValue_ColumnLabel.ToString());
			AffectedHeader->Invalidate(EInvalidateWidgetReason::Paint);
			return AffectedHeader;
		}
	}
	
	return nullptr;
}

static constexpr FLinearColor TransparentGray = {0.25,0.25,0.25,0.5}; 
static constexpr FLinearColor LessTransparentGrayGreen = {0.20,0.60,0.20,0.75}; 
static FSlateBrush ProgressBarBG_Brush;
static FSlateBrush ProgressBarFill_Brush;
static FProgressBarStyle ProgressBarStyle;

void SPDSelectedStat_LevelData::UpdateChildSlot()
{
	const FPDWidgetBaseSettings& Settings = RetrieveSettings();

	InitializeFonts();

	const int32 MaxWidth = Settings.MaxSUWidth;
	const int32 MaxHeight = Settings.MaxSUHeight; 

	TArray<TSharedPtr<FPDSkillTokenBase>>*& TokenArrayPtr = HeaderDataViews.Key.DataViewPtr;
	TSharedPtr<SListView<TSharedPtr<FPDSkillTokenBase>>>& TokenArrayListView = HeaderDataViews.Key.ListView;
	const TSharedPtr<SHeaderRow> TokenHeader = RefreshHeaderRow(0);

	TArray<TSharedPtr<FPDStatViewAffectedStat>>*& AffectedStatsPtr = HeaderDataViews.Value.DataViewPtr;
	TSharedPtr<SListView<TSharedPtr<FPDStatViewAffectedStat>>>& AffectedStatsListView = HeaderDataViews.Value.ListView;
	const TSharedPtr<SHeaderRow> AffectedHeader = RefreshHeaderRow(1);
	
	TokenArrayListView =
		SNew(SListView<TSharedPtr<FPDSkillTokenBase>>)
		.HeaderRow(TokenHeader)
		.ListItemsSource(TokenArrayPtr)
		.OnGenerateRow( this, &SPDSelectedStat_LevelData::MakeListViewWidget_LinkedStat_TokensToGrant)
		.OnSelectionChanged( this, &SPDSelectedStat_LevelData::OnComponentSelected_LinkedStat_TokensToGrant );
	
	AffectedStatsListView =
		SNew(SListView<TSharedPtr<FPDStatViewAffectedStat>>)
		.HeaderRow(AffectedHeader)
		.ListItemsSource(AffectedStatsPtr)
		.OnGenerateRow( this, &SPDSelectedStat_LevelData::MakeListViewWidget_LinkedStat_AffectedStats)
		.OnSelectionChanged( this, &SPDSelectedStat_LevelData::OnComponentSelected_LinkedStat_AffectedStats );


	ProgressBarBG_Brush.ImageSize.X = MaxWidth;
	ProgressBarBG_Brush.ImageSize.Y = MaxHeight != INDEX_NONE ? MaxHeight / 75.0f : 10.0f;
	ProgressBarFill_Brush = ProgressBarBG_Brush;

	ProgressBarBG_Brush.ImageType = ESlateBrushImageType::NoImage;
	ProgressBarBG_Brush.TintColor = FSlateColor(TransparentGray);

	ProgressBarFill_Brush.ImageType = ESlateBrushImageType::Linear;
	ProgressBarFill_Brush.TintColor = FSlateColor(LessTransparentGrayGreen);
	
	ProgressBarStyle.BackgroundImage = ProgressBarBG_Brush;
	ProgressBarStyle.FillImage = ProgressBarFill_Brush;
	
	ChildSlot
	.VAlign(VAlign_Center)
	.HAlign(HAlign_Center)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.FillWidth(10)
		.MaxWidth(MaxWidth)
		[
			SetWidgetTitle(SVerticalBox, SelectedStatLevelLabel, Settings.TitleFont)
			+ SetTitleSlot(SVerticalBox, Token_SectionTitle, Settings.SubTitleFont)
				+ SetListViewSlot(SVerticalBox, MaxHeight, TokenArrayListView.ToSharedRef())
				+ SetListViewSlot(SVerticalBox, MaxHeight, AffectedStatsListView.ToSharedRef())
			
			+ SetTitleSlot(SVerticalBox, ExperienceBar_Title, Settings.SubTitleFont)
			+ SVerticalBox::Slot() 
				.Padding(FMargin{0, 0})
				.FillHeight(10)
			[
				SNew(SOverlay)
				+ SOverlay::Slot()
				.ZOrder(9)
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Fill)
				[
					// @inprogress replace with bar representing experience
	
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.MaxWidth(MaxWidth)
					[
						SNew(SProgressBar)
						.Percent(0.5)
						.Style(&ProgressBarStyle)
					]
				]
				+ SOverlay::Slot()
				.ZOrder(10)
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Fill)			
				[
					SNew(STextBlock)
					.Text(ExperienceBar_Title)
					.Justification(ETextJustify::Center)
					.Font(Settings.TableEntryFont)
					.MinDesiredWidth(MaxWidth)
					// @inprogress actual text indicating numerical value of experience
				]			
			]		
		]
	];
}

void SPDSelectedStat_LevelData::PrepareData()
{
	TArray<TSharedPtr<FPDSkillTokenBase>>*& TokenArrayPtr = HeaderDataViews.Key.DataViewPtr;
	const TSharedPtr<SListView<TSharedPtr<FPDSkillTokenBase>>>& TokenArrayListView = HeaderDataViews.Key.ListView;
	TArray<TSharedPtr<FPDStatViewAffectedStat>>*& AffectedStatsPtr = HeaderDataViews.Value.DataViewPtr;
	const TSharedPtr<SListView<TSharedPtr<FPDStatViewAffectedStat>>>& AffectedStatsListView = HeaderDataViews.Value.ListView;
	
	if (TokenArrayPtr == nullptr || TokenArrayListView.IsValid() == false) { return; }
	if (AffectedStatsPtr == nullptr || AffectedStatsListView.IsValid() == false) { return; }
	
	static UPDStatSubsystem* StatSubsystem = UPDStatSubsystem::Get();
	const FPDStatsRow& SelectedStat = StatSubsystem->GetStatTypeData(SelectedStatTag);
	const TArray<FGameplayTag>& StatsThatWeAffect = StatSubsystem->StatCrossBehaviourMap.FindRef(SelectedStatTag);

	UPDStatHandler** OwnersStatHandler = StatSubsystem->StatHandlers.Find(OwnerID);
	if (OwnersStatHandler == nullptr || *OwnersStatHandler == nullptr) { return; }

	constexpr int32 DeltaLevel = 1;
	const FPDStatMapping* StatMapping = (*OwnersStatHandler)->LocalStatMappings.Find(SelectedStatTag);
	int32 SelectedStatNextLevel = DeltaLevel + (StatMapping == nullptr
		? 0
		: SelectedStatNextLevel = (*OwnersStatHandler)->StatList.Items[StatMapping->Index].CurrentLevel);

	
	for (const FGameplayTag& StatTargetTag : StatsThatWeAffect)
	{
		const FPDStatsRow* StatTargetDefaultDataPtr = StatSubsystem->GetStatTypeDataPtr(StatTargetTag);
		
		if (StatTargetDefaultDataPtr == nullptr) { continue; }

		// Find out our (SelectedStatTag) effect on target stat (Stats that are affected by us)
		double DeltaNewLevelOffset; 
		StatSubsystem->ResolveCrossBehaviourDelta(
			SelectedStatNextLevel - DeltaLevel,
			SelectedStatNextLevel,
			*StatTargetDefaultDataPtr, 
			SelectedStatTag,
			DeltaNewLevelOffset);
		
		
		TSharedPtr<FPDStatViewAffectedStat> ConstructedAffectedStatView = MakeShared<FPDStatViewAffectedStat>();
		ConstructedAffectedStatView->AffectedStat = StatTargetTag;
		ConstructedAffectedStatView->TotalAffectedDelta = DeltaNewLevelOffset;

		AffectedStatsPtr->Emplace(ConstructedAffectedStatView);
	}
	
	for (const TTuple<FGameplayTag, UCurveFloat*>& TokenCategoryCompound : SelectedStat.TokensToGrantPerLevel)
	{
		const UCurveFloat* TokenProgressCurve = TokenCategoryCompound.Value;
		if (TokenProgressCurve == nullptr)
		{
			UE_LOG(PDLog_Progression, Error,
				   TEXT("UPDStatHandler::GrantTokens "
					   "-- Iterating the token list, entry(%s) has not valid curve applied ot it"), *TokenCategoryCompound.Key.GetTagName().ToString());
			continue;
		}

		TSharedPtr<FPDSkillTokenBase> ConstructedTokenView = MakeShared<FPDSkillTokenBase>();
		ConstructedTokenView->TokenType = TokenCategoryCompound.Key;
		ConstructedTokenView->TokenValue += (TokenProgressCurve->GetFloatValue(SelectedStatNextLevel) + 0.5f);
		
		TokenArrayPtr->Emplace(ConstructedTokenView);
	}
}

FReply  SPDSelectedStat_LevelData::DesignTimeTranslation(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (bIsDesignTime == false ) { return FReply::Unhandled(); }

	// MouseEvent.IsMouseButtonDown();

	return SCompoundWidget::OnDragDetected(MyGeometry, MouseEvent); // @todo replace
}

FReply SPDSelectedStat_LevelData::OnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	FReply TranslationHandled = DesignTimeTranslation(MyGeometry, MouseEvent);
	if (TranslationHandled.IsEventHandled()) { return TranslationHandled; }
	
	return SCompoundWidget::OnDragDetected(MyGeometry, MouseEvent);
}

void SPDSelectedStat_LevelData::Refresh(
	int32 InOwnerID,
	TArray<TSharedPtr<FPDSkillTokenBase>>& TokenArrayRef,
	TArray<TSharedPtr<FPDStatViewAffectedStat>>& AffectedStatsRef,
	const FPDWidgetBaseSettings& WidgetSettingsUpdate)
{
	const bool bIsSectionWidthChanged = RetrieveSettings().DataViewSectionWidth != WidgetSettingsUpdate.DataViewSectionWidth;
	UpdateSettings(WidgetSettingsUpdate);

	OwnerID = InOwnerID;
	TArray<TSharedPtr<FPDSkillTokenBase>>*& TokenArrayPtr = HeaderDataViews.Key.DataViewPtr;
	const TSharedPtr<SListView<TSharedPtr<FPDSkillTokenBase>>>& TokenArrayListView = HeaderDataViews.Key.ListView;
	TArray<TSharedPtr<FPDStatViewAffectedStat>>*& AffectedStatsPtr = HeaderDataViews.Value.DataViewPtr;
	const TSharedPtr<SListView<TSharedPtr<FPDStatViewAffectedStat>>>& AffectedStatsListView = HeaderDataViews.Value.ListView;
	
	TokenArrayPtr = &TokenArrayRef;
	AffectedStatsPtr = &AffectedStatsRef;
	
	PrepareData();
	if (TokenArrayListView.IsValid())
	{
		if (bIsSectionWidthChanged) { RefreshHeaderRow(0); }
				
		TokenArrayListView->SetItemsSource(TokenArrayPtr);
		TokenArrayListView->RebuildList();
		TokenArrayListView->Invalidate(EInvalidateWidgetReason::Paint);
	}
	if (AffectedStatsListView.IsValid())
	{
		if (bIsSectionWidthChanged) { RefreshHeaderRow(1); }		
		
		AffectedStatsListView->SetItemsSource(AffectedStatsPtr);
		AffectedStatsListView->RebuildList();
		AffectedStatsListView->Invalidate(EInvalidateWidgetReason::Paint);
	}

	Invalidate(EInvalidateWidgetReason::Paint);
}

TSharedRef<ITableRow> SPDSelectedStat_LevelData::MakeListViewWidget_LinkedStat_TokensToGrant(
	TSharedPtr<FPDSkillTokenBase> StatViewTokensToGrant,
	const TSharedRef<STableViewBase>& OwnerTable) const
{
	const FPDWidgetBaseSettings& Settings = RetrieveSettings();
	
	TSharedPtr<STableRow< TSharedPtr<FPDSkillTokenBase>>> StatTable = nullptr;

	// (DONE) 1. StatViewModifySource->StatTag
	const FPDStatDataViewSlotParams& HeaderSlotParam =
		FPDStatStatics::CreateDataViewSlotParam(StatViewTokensToGrant->TokenType, StatViewTokensToGrant->TokenValue);
	
	constexpr double WidthDiscrepancy = (1.015);
	const float TrueSectionWidth = Settings.DataViewSectionWidth * WidthDiscrepancy;
	StatTable = SNew( STableRow< TSharedPtr<FPDSkillTokenBase> >, OwnerTable )
	[
		SNew(SHorizontalBox)
		+ SetListHeaderSlot(SHorizontalBox, HeaderSlotParam.SectionText, Settings.TableEntryFont, TrueSectionWidth )
		+ SetListHeaderSlot(SHorizontalBox, HeaderSlotParam.ValueText, Settings.TableEntryFont, TrueSectionWidth )
	];
	
	return StatTable.ToSharedRef();
}

TSharedRef<ITableRow> SPDSelectedStat_LevelData::MakeListViewWidget_LinkedStat_AffectedStats(
	TSharedPtr<FPDStatViewAffectedStat> CurrentAffectedStat,
	const TSharedRef<STableViewBase>& OwnerTable) const
{
	const FPDWidgetBaseSettings& Settings = RetrieveSettings();
	
	TSharedPtr<STableRow< TSharedPtr<FPDStatViewAffectedStat>>> StatTable = nullptr;

	// (DONE) 1. StatViewModifySource->StatTag
	const FPDStatDataViewSlotParams& HeaderSlotParam =
		FPDStatStatics::CreateDataViewSlotParam(CurrentAffectedStat->AffectedStat, CurrentAffectedStat->TotalAffectedDelta);

	constexpr double WidthDiscrepancy = (1.015);
	const float TrueSectionWidth = Settings.DataViewSectionWidth * WidthDiscrepancy;
	StatTable = SNew( STableRow< TSharedPtr<FPDStatViewAffectedStat> >, OwnerTable )
	[
		SNew(SHorizontalBox)
		+ SetListHeaderSlot(SHorizontalBox, HeaderSlotParam.SectionText, Settings.TableEntryFont, TrueSectionWidth )
		+ SetListHeaderSlot(SHorizontalBox, HeaderSlotParam.ValueText, Settings.TableEntryFont, TrueSectionWidth )
	];

	return StatTable.ToSharedRef();
	
}

void SPDSelectedStat_LevelData::OnComponentSelected_LinkedStat_TokensToGrant(
	TSharedPtr<FPDSkillTokenBase> StatViewTokensToGrant,
	ESelectInfo::Type Arg) const { } // @todo

void SPDSelectedStat_LevelData::OnComponentSelected_LinkedStat_AffectedStats(
	TSharedPtr<FPDStatViewAffectedStat> StatViewAffectedStats,
	ESelectInfo::Type Arg) const { } // @todo


//
// Selected Stat - Offset modify sources view
void SPDSelectedStat_OffsetData::Construct(
	const FArguments& InArgs,
	int32 InOwnerID,
	const FGameplayTag& InSelectedStatTag,
	TArray<TSharedPtr<FPDStatViewModifySource>>& ArrayRef,
	const FPDWidgetBaseSettings& WidgetSettingsUpdate)
{
	UpdateSettings(WidgetSettingsUpdate);
	TArray<TSharedPtr<FPDStatViewModifySource>>*& SelectedStatModifierSources = HeaderDataViews.Value.DataViewPtr;

	
	OwnerID = InOwnerID;
	
	SelectedStatTag = InSelectedStatTag;
	SelectedStatModifierSources = &ArrayRef;

	PrepareData();
	UpdateChildSlot();
}

void SPDSelectedStat_OffsetData::PrepareData()
{
	TArray<TSharedPtr<FPDStatViewModifySource>>*& SelectedStatModifierSources = HeaderDataViews.Value.DataViewPtr;
	const TSharedPtr<SListView<TSharedPtr<FPDStatViewModifySource>>>& ModifySourceListView = HeaderDataViews.Value.ListView;
	if (SelectedStatModifierSources == nullptr || ModifySourceListView.IsValid() == false)
	{
		return;
	}
	
	static UPDStatSubsystem* StatSubsystem = UPDStatSubsystem::Get();
	const FPDStatsRow& SelectedStat = StatSubsystem->GetStatTypeData(SelectedStatTag);
	const TArray<FGameplayTag>& StatsThatAffectUs = StatSubsystem->StatCrossBehaviourBackMapped.FindRef(SelectedStatTag);

	UPDStatHandler** OwnersStatHandler = StatSubsystem->StatHandlers.Find(OwnerID);
	if (OwnersStatHandler == nullptr || *OwnersStatHandler == nullptr)
	{
		return;
	}
	
	for (const FGameplayTag& StatSourceTag : StatsThatAffectUs)
	{
		FPDStatsCrossBehaviourRules CrossBehaviourRules;

		const FPDStatMapping* StatMapping = (*OwnersStatHandler)->LocalStatMappings.Find(StatSourceTag);
		int32 StatSourceLevel = (StatMapping == nullptr
			? 0
			: StatSourceLevel = (*OwnersStatHandler)->StatList.Items[StatMapping->Index].CurrentLevel);		
		
		double CrossBehaviourOffset_Normalized;
		StatSubsystem->ResolveCrossBehaviours(
			StatSourceLevel,
			SelectedStat, 
			StatSourceTag,
			CrossBehaviourRules,
			CrossBehaviourOffset_Normalized);

		TSharedPtr<FPDStatViewModifySource> ConstructedModifySourceView = MakeShared<FPDStatViewModifySource>();
		ConstructedModifySourceView->StatTag = StatSourceTag;
		ConstructedModifySourceView->AppliedStatOffset = CrossBehaviourOffset_Normalized;
		ConstructedModifySourceView->StatOffsetCurveSource = CrossBehaviourRules.RuleSetLevelCurveMultiplier; 

		// @todo, need to write a shader that visualizes this curve for us, and where on the curve we are at
		
		SelectedStatModifierSources->Emplace(ConstructedModifySourceView);
	}
	
}

void SPDSelectedStat_OffsetData::Refresh(
	int32 InOwnerID,
	TArray<TSharedPtr<FPDStatViewModifySource>>& DataViewRef,
	const FPDWidgetBaseSettings& WidgetSettingsUpdate)
{
	const bool bIsSectionWidthChanged = RetrieveSettings().DataViewSectionWidth != WidgetSettingsUpdate.DataViewSectionWidth;
	UpdateSettings(WidgetSettingsUpdate);
	
	TArray<TSharedPtr<FPDStatViewModifySource>>*& SelectedStatModifierSources = HeaderDataViews.Value.DataViewPtr;
	const TSharedPtr<SListView<TSharedPtr<FPDStatViewModifySource>>>& ModifySourceListView = HeaderDataViews.Value.ListView;
	
	OwnerID = InOwnerID;
	SelectedStatModifierSources = &DataViewRef;
	
	if (ModifySourceListView.IsValid() == false) { return; }

	PrepareData();
	if (bIsSectionWidthChanged) { RefreshHeaderRow(0); }	
	
	ModifySourceListView->RebuildList();
	ModifySourceListView->Invalidate(EInvalidateWidgetReason::Paint);
	Invalidate(EInvalidateWidgetReason::Paint);
}

TSharedPtr<SHeaderRow> SPDSelectedStat_OffsetData::RefreshHeaderRow(int32 HeaderRowIdx)
{
	const FPDWidgetBaseSettings& Settings = RetrieveSettings();

	TSharedPtr<SHeaderRow> Header = 
	FPDStatStatics::CreateHeaderRow(HeaderDataViews.Value.Header, Settings.DataViewSectionWidth, 4,
		"0", *StatSources_Header_Name.ToString(),
		"1", *StatSources_Header_Category.ToString(),
		"2", *StatSources_Header_AppliedOffset.ToString(),
		"3", *StatSources_Header_Curves.ToString());
	Header->Invalidate(EInvalidateWidgetReason::Paint);
	return Header;
}

void SPDSelectedStat_OffsetData::UpdateChildSlot()
{
	const FPDWidgetBaseSettings& Settings = RetrieveSettings();
	
	InitializeFonts();
	const int32 MaxWidth = Settings.MaxSUWidth;
	const int32 MaxHeight = Settings.MaxSUHeight; 
	
	TArray<TSharedPtr<FPDStatViewModifySource>>*& SelectedStatModifierSources = HeaderDataViews.Value.DataViewPtr;
	TSharedPtr<SListView<TSharedPtr<FPDStatViewModifySource>>>& ModifySourceListView = HeaderDataViews.Value.ListView;

	const TSharedPtr<SHeaderRow> Header = RefreshHeaderRow();
	ModifySourceListView = SNew(SListView<TSharedPtr<FPDStatViewModifySource>>)
		.HeaderRow(Header)
		.ListItemsSource(SelectedStatModifierSources)
		.OnGenerateRow( this, &SPDSelectedStat_OffsetData::MakeListViewWidget_LinkedStat)
		.OnSelectionChanged( this, &SPDSelectedStat_OffsetData::OnComponentSelected_LinkedStat );	
	
	ChildSlot
	.HAlign(HAlign_Center)
	.VAlign(VAlign_Center)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.FillWidth(10)
		.MaxWidth(MaxWidth)
		[
			SetWidgetTitle(SVerticalBox, StatSources_Header_Title, Settings.TitleFont)
			
			+ SetListViewSlot(SVerticalBox, MaxWidth, ModifySourceListView.ToSharedRef())
		]
	];
}

FReply SPDSelectedStat_OffsetData::DesignTimeTranslation(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (bIsDesignTime == false) { return FReply::Unhandled(); }

	// MouseEvent. .IsMouseButtonDown();

	return SCompoundWidget::OnDragDetected(MyGeometry, MouseEvent); // @todo replace
}

FReply SPDSelectedStat_OffsetData::OnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	FReply TranslationHandled = DesignTimeTranslation(MyGeometry, MouseEvent);
	if (TranslationHandled.IsEventHandled()) { return TranslationHandled; }
	
	return SCompoundWidget::OnDragDetected(MyGeometry, MouseEvent);
}

TSharedRef<ITableRow> SPDSelectedStat_OffsetData::MakeListViewWidget_LinkedStat(
	TSharedPtr<FPDStatViewModifySource> StatViewModifySource,
	const TSharedRef<STableViewBase>& OwnerTable) const
{
	const FPDWidgetBaseSettings& Settings = RetrieveSettings();
	
	TSharedPtr<STableRow< TSharedPtr<FPDStatViewModifySource>>> StatTable = nullptr;

	const FPDStatDataViewSlotParams& HeaderSlotParam_RepresentedStat =
		FPDStatStatics::CreateDataViewSlotParam(StatViewModifySource->StatTag, StatViewModifySource->AppliedStatOffset, false);

	const FPDStatDataViewSlotParams& HeaderSlotParam_Parent =
		FPDStatStatics::CreateDataViewSlotParam(StatViewModifySource->StatTag.RequestDirectParent(), INDEX_NONE, false);	
	
	// @todo 3. StatViewModifySource->StatOffsetCurveSource;
	const FText PlaceholderCurveString = LOCTEXT("Placeholder93u490803","TODO - IMPLEMENT CURVE DISPLAY/VIEW");
	
	constexpr double WidthDiscrepancy = (1.015);
	const float TrueSectionWidth = Settings.DataViewSectionWidth * WidthDiscrepancy;
	StatTable =
		SNew( STableRow< TSharedPtr<FPDStatViewModifySource> >, OwnerTable )
		[
			SNew(SHorizontalBox)

			// Name & Category Fields			
			+ SetListHeaderSlot(SHorizontalBox, HeaderSlotParam_RepresentedStat.SectionText, Settings.TableEntryFont, TrueSectionWidth )
			+ SetListHeaderSlot(SHorizontalBox, HeaderSlotParam_Parent.SectionText, Settings.TableEntryFont, TrueSectionWidth )

			// Value Field		
			+ SetListHeaderSlot(SHorizontalBox, HeaderSlotParam_RepresentedStat.ValueText, Settings.TableEntryFont, TrueSectionWidth )
			
			// @todo 3. StatViewModifySource->StatOffsetCurveSource;
			// Curve display, Showing the Applied offset			
			+ SetListHeaderSlot(SHorizontalBox, PlaceholderCurveString, Settings.TableEntryFont, TrueSectionWidth )
		];


	return StatTable.ToSharedRef();
}

void SPDSelectedStat_OffsetData::OnComponentSelected_LinkedStat(TSharedPtr<FPDStatViewModifySource> FpdStatViewModifySource,
	ESelectInfo::Type Arg) const
{
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

