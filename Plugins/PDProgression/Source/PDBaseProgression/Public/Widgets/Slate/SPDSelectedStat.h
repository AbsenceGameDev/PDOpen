/* @author: Ario Amin @ Permafrost Development. @copyright: Full BSL(1.1) License included at bottom of the file  */

#pragma once

#include "CoreMinimal.h"
#include "PDProgressionSharedUI.h"

#include "GameplayTags.h"
#include "Net/PDProgressionNetDatum.h"
#include "Subsystems/EngineSubsystem.h"

#include "Layout/Geometry.h"
#include "Input/Reply.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"

class FPaintArgs;
class FSlateWindowElementList;
struct FSlateBrush;


// @todo (PRIO 1) Need a widget for a button to upgrade a stat and a developer setting to tell if the button should be visible or not, this is be able to cater to different players // Todo also make the inventory system
// @todo Cont. This would need to allow selection of type of token to be used, in case a upgrade allows for different types of tokens

// @todo (PRIO 3/BACKLOG) Need a widget for selecting the Category : Create a view that shows other, unlocked, stats in the same category


/** @brief @inprogress Widget for data regarding the selected stats level and experience
 * @inprogress Need to have an experience-bar which also may display numbers
 * @details Show amount of tokens, and their types that will be earned upon gaining next level, and any relevant stat changes that will occur */
class PDBASEPROGRESSION_API SPDSelectedStat_LevelData
	: public  SCompoundWidget
	, public FPDStatWidgetBase<FPDSkillTokenBase, FPDStatViewAffectedStat>
{
public:

	SLATE_BEGIN_ARGS(SPDSelectedStat_LevelData){}
	SLATE_END_ARGS()

	/** @brief Copies parameters into member properties then calls 'PrepareData' and 'UpdateChildSlot'.
	 * Is used by slate when a new widget is constructed */
	void Construct(
		const FArguments& InArgs,
		int32 InOwnerID,
		const FGameplayTag& InSelectedStatTag,
		TArray<TSharedPtr<FPDSkillTokenBase>>& TokenArrayRef,
		TArray<TSharedPtr<FPDStatViewAffectedStat>>& AffectedStatsRef,
		const FPDWidgetBaseSettings& WidgetSettingsUpdate);

	/** @brief Refreshes the elements in 'HeaderDataViews.Key.DataViewPtr' & 'HeaderDataViews.Value.DataViewPtr' and calls rebuild on their respective list-views  */
	void Refresh(
		int32 InOwnerID,
		TArray<TSharedPtr<FPDSkillTokenBase>>& TokenArrayRef,
		TArray<TSharedPtr<FPDStatViewAffectedStat>>& AffectedStatsRef,
		const FPDWidgetBaseSettings& WidgetSettingsUpdate);
	
	/** @brief Token entry widget. Is used to display levelling, token-related, data about a selected stat. How many tokens of a given type will be granted upon leveling the stat  */
	TSharedRef<ITableRow> MakeListViewWidget_LinkedStat_TokensToGrant(TSharedPtr<FPDSkillTokenBase> StatViewTokensToGrant, const TSharedRef<STableViewBase>& TableViewBase) const;
	/** @brief Unused. Reserved.   */
	void OnComponentSelected_LinkedStat_TokensToGrant(TSharedPtr<FPDSkillTokenBase> StatViewTokensToGrant, ESelectInfo::Type Arg) const;
	
	/** @brief Affected Stat entry widget. Represents the a stat that will be affected when another selected stat levels up, and displays the total increase or decrease of said stat value */
	TSharedRef<ITableRow> MakeListViewWidget_LinkedStat_AffectedStats(TSharedPtr<FPDStatViewAffectedStat> StatViewAffectedStats, const TSharedRef<STableViewBase>& TableViewBase) const;
	/** @brief Unused. Reserved.   */
	void OnComponentSelected_LinkedStat_AffectedStats(TSharedPtr<FPDStatViewAffectedStat> FpdStatViewAffectedStats, ESelectInfo::Type Arg) const;

	/** @brief Defines our header definition, is called on construction but is also called on table changes after construction */
	virtual TSharedPtr<SHeaderRow> RefreshHeaderRow(int32 HeaderRowIdx = 0) override;	
	
	/** @brief Builds the data-view headers and the child-slot composition (Defines the widget-layout) */
	virtual void UpdateChildSlot() override;

	/** @brief Resolves the token types and amounts we will grant upon reaching the next level, caches the results */
	void PrepareData();

	//
	// Translation
	/** @brief @todo implement translation during detected drag movements */
	virtual FReply DesignTimeTranslation(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	/** @brief Calls DesignTimeTranslation  */
	virtual FReply OnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	
	
	/** @defgroup SelectedStat_Labels */
	static FText SelectedStatLevelLabel;        /**< @ingroup SelectedStat_Labels */
	static FText Token_SectionTitle;            /**< @ingroup SelectedStat_Labels */
	static FText TokenName_ColumnLabel;         /**< @ingroup SelectedStat_Labels */
	static FText TokenCount_ColumnLabel;        /**< @ingroup SelectedStat_Labels */
	
	static FText OtherStats_SectionTitle;             /**< @ingroup SelectedStat_Labels */
	static FText OtherStatsAffectedName_ColumnLabel;  /**< @ingroup SelectedStat_Labels */
	static FText OtherStatsAffectedValue_ColumnLabel; /**< @ingroup SelectedStat_Labels */
	
	static FText TokenEntryLabel;              /**< @ingroup SelectedStat_Labels */
	static FText OtherStatsAffectedEntryLabel; /**< @ingroup SelectedStat_Labels */

	static FText ExperienceBar_Title;  /**< @ingroup SelectedStat_Labels */
	
	friend class UPDStatListInnerWidget;	
};


/** @brief A SPDSelectedStat_OffsetData displays the expected crossbehaviour value for a 'stat-B' based on Stat a given 'stat B',
 * @note This is going to be used for when selecting a stat's total crossbehaviour: To list a all sources that the crossbehviour is composed from */
class PDBASEPROGRESSION_API SPDSelectedStat_OffsetData
	: public SCompoundWidget
	, public FPDStatWidgetBase<FPDStatViewModifySource>
{
public:

	SLATE_BEGIN_ARGS(SPDSelectedStat_OffsetData){}
	SLATE_END_ARGS()

	/** @brief Copies parameters into member properties then calls 'PrepareData' and 'UpdateChildSlot'.
	 * Is used by slate when a new widget is constructed */
	void Construct(const FArguments& InArgs,
		int32 InOwnerID,
		const FGameplayTag& InSelectedStatTag,
		TArray<TSharedPtr<FPDStatViewModifySource>>& ArrayRef,
		const FPDWidgetBaseSettings& WidgetSettingsUpdate);

	/** @brief Resolves the expected cross behaviour value increase for next level, caches the results */
	void PrepareData();
	/** @brief Refreshes the elements in 'HeaderDataViews.Value.DataViewPtr' and calls rebuild on 'HeaderDataViews.Value.ListView'  */
	void Refresh(
		int32 InOwnerID,
		TArray<TSharedPtr<FPDStatViewModifySource>>& DataViewRef,
		const FPDWidgetBaseSettings& WidgetSettingsUpdate);

	/** @brief Representation of the entry widgets of the offset/modifier targets   */
	TSharedRef<ITableRow> MakeListViewWidget_LinkedStat(TSharedPtr<FPDStatViewModifySource> StatViewModifySource, const TSharedRef<STableViewBase>& OwnerTable) const;
	/** @brief Unused. Reserved  */
	void OnComponentSelected_LinkedStat(TSharedPtr<FPDStatViewModifySource> StatViewModifySource, ESelectInfo::Type Arg) const;

	/** @brief Defines our header definition, is called on construction but is also called on table changes after construction */
	virtual TSharedPtr<SHeaderRow> RefreshHeaderRow(int32 HeaderRowIdx = 0) override;	
	
	/** @brief Builds the data-view headers and the child-slot composition (Defines the widget-layout) */
	virtual void UpdateChildSlot() override;

	//
	// Translation	
	/** @brief @todo implement translation during detected drag movements */
	virtual FReply DesignTimeTranslation(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	/** @brief Calls DesignTimeTranslation  */
	virtual FReply OnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	
	/** @defgroup SelectedSources_Labels */
	static FText StatSources_Header_Title;         /**< @ingroup SelectedSources_Labels */
	static FText StatSources_Header_Name;          /**< @ingroup SelectedSources_Labels */
	static FText StatSources_Header_Category;      /**< @ingroup SelectedSources_Labels */
	static FText StatSources_Header_AppliedOffset; /**< @ingroup SelectedSources_Labels */
	static FText StatSources_Header_Curves;        /**< @ingroup SelectedSources_Labels */
	
	friend class UPDStatListInnerWidget;	
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
