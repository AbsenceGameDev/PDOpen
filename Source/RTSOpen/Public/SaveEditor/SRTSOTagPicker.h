/* @author: Ario Amin @ Permafrost Development. @copyright: Full BSL(1.1) License included at bottom of the file  */

#pragma once

#include "SlateFwd.h"
#include "UObject/Object.h"
#include "Layout/Visibility.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Input/Reply.h"
#include "Widgets/SWidget.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/STableViewBase.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Views/STreeView.h"
#include "GameplayTagsManager.h"
#include "EditorUndoClient.h"

class IPropertyHandle;
class SComboButton;

/** Widget allowing user to tag assets with gameplay tags */
class RTSOPEN_API SRTSOTagPicker : public SCompoundWidget, public FSelfRegisteringEditorUndoClient
{
public:

	/** Called on when tags might need refreshing (e.g. after undo/redo or when tags change). Use SetTagContainers() to set the new tags. */
	DECLARE_DELEGATE_OneParam(FOnRefreshTagContainers, SRTSOTagPicker& /*TagPicker*/)
	
	enum class ETagFilterResult
	{
		IncludeTag,
		ExcludeTag
	};

	DECLARE_DELEGATE_RetVal_OneParam(ETagFilterResult, FOnFilterTag, const TSharedPtr<FGameplayTagNode>&)

	SLATE_BEGIN_ARGS(SRTSOTagPicker)
		: _Filter()
		, _ShowMenuItems(true)
		, _MaxHeight(260.0f)
		, _Padding(FMargin(2.0f))
	{}
		// Comma delimited string of tag root names to filter by
		SLATE_ARGUMENT(FString, Filter)

		// Optional filter function called when generating the tag list
		SLATE_EVENT(FOnFilterTag, OnFilterTag)

		// If set, wraps the picker in a menu builder and adds common menu commands.
		SLATE_ARGUMENT(bool, ShowMenuItems)  

		// Tags or tag containers to modify. If MultiSelect is false, the container will contain single tags.
		// If PropertyHandle is set, the tag containers will be ignored.
		SLATE_ARGUMENT(TArray<FGameplayTagContainer>, TagContainers)

		// Caps the height of the gameplay tag tree, 0.0 means uncapped.
		SLATE_ARGUMENT(float, MaxHeight)

		// Padding inside the picker widget
		SLATE_ARGUMENT(FMargin, Padding)

		// Called on when tags might need refreshing (e.g. after undo/redo or when tags change).
		SLATE_EVENT(FOnRefreshTagContainers, OnRefreshTagContainers)
	SLATE_END_ARGS()
	
	/** Construct the actual widget */
	void Construct(const FArguments& InArgs);
	
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

	/** Updates the tag list when the filter text changes */
	void OnFilterTextChanged(const FText& InFilterText);

	/** Returns true if this TagNode has any children that match the current filter */
	bool FilterChildrenCheck(TSharedPtr<FGameplayTagNode>& InItem) const;

	/** Child recursion helper for FilterChildrenCheck() */
	bool FilterChildrenCheckRecursive(TSharedPtr<FGameplayTagNode>& InItem) const;

	/** Refreshes the tags that should be displayed by the widget */
	void RefreshTags();

	/** Forces the widget to refresh its tags on the next tick */
	void RefreshOnNextTick();

	/** Scrolls the view to specified tag. */
	void RequestScrollToView(const FGameplayTag Tag);
	
	/** Gets the widget to focus once the menu opens. */
	TSharedPtr<SWidget> GetWidgetToFocusOnOpen();

	/** Sets tag containers to edit. */
	void SetTagContainers(TConstArrayView<FGameplayTagContainer> TagContainers);
	
private:

	// FSelfRegisteringEditorUndoClient
	virtual void PostUndo(bool bSuccess) override;
	virtual void PostRedo(bool bSuccess) override;
	// ~FSelfRegisteringEditorUndoClient

	/** Verify the tags are all valid and if not prompt the user. */
	void VerifyAssetTagValidity();

	/* Filters the tree view based on the current filter text. */
	void FilterTagTree();
	
	/**
	 * Generate a row widget for the specified item node and table
	 * 
	 * @param InItem		Tag node to generate a row widget for
	 * @param OwnerTable	Table that owns the row
	 * 
	 * @return Generated row widget for the item node
	 */
	TSharedRef<ITableRow> OnGenerateRow(TSharedPtr<FGameplayTagNode> InItem, const TSharedRef<STableViewBase>& OwnerTable);

	/**
	 * Get children nodes of the specified node
	 * 
	 * @param InItem		Node to get children of
	 * @param OutChildren	[OUT] Array of children nodes, if any
	 */
	void OnGetChildren(TSharedPtr<FGameplayTagNode> InItem, TArray< TSharedPtr<FGameplayTagNode> >& OutChildren);

	/**
	 * Called via delegate when the status of a check box in a row changes
	 * 
	 * @param NewCheckState	New check box state
	 * @param NodeChanged	Node that was checked/unchecked
	 */
	void OnTagCheckStatusChanged(ECheckBoxState NewCheckState, TSharedPtr<FGameplayTagNode> NodeChanged);

	/**
	 * Called via delegate to determine the checkbox state of the specified node
	 * 
	 * @param Node	Node to find the checkbox state of
	 * 
	 * @return Checkbox state of the specified node
	 */
	ECheckBoxState IsTagChecked(TSharedPtr<FGameplayTagNode> Node) const;

	/**
	 * @return true if the exact Tag provided is included in any of the tag containers the widget is editing.
	 */
	bool IsExactTagInCollection(TSharedPtr<FGameplayTagNode> Node) const;
	
	/**
	 * Called via delegate to determine the non-restricted children checkbox state of the specified node
	 *
	 * @param Node	Node to find the non-restricted children checkbox state of
	 *
	 * @return Non-restricted children heckbox state of the specified node
	 */
	ECheckBoxState IsAllowChildrenTagChecked(TSharedPtr<FGameplayTagNode> Node) const;

	/** Helper function to determine the visibility of the checkbox for allowing non-restricted children of restricted gameplay tags */
	EVisibility DetermineAllowChildrenVisible(TSharedPtr<FGameplayTagNode> Node) const;

	/**
	 * Helper function called when the specified node is checked
	 * 
	 * @param NodeChecked	Node that was checked by the user
	 */
	void OnTagChecked(TSharedPtr<FGameplayTagNode> NodeChecked);

	/**
	 * Helper function called when the specified node is unchecked
	 * 
	 * @param NodeUnchecked	Node that was unchecked by the user
	 */
	void OnTagUnchecked(TSharedPtr<FGameplayTagNode> NodeUnchecked);

	/**
	 * Recursive function to uncheck all child tags
	 * 
	 * @param NodeUnchecked	Node that was unchecked by the user
	 * @param EditableContainer The container we are removing the tags from
	 */
	void UncheckChildren(TSharedPtr<FGameplayTagNode> NodeUnchecked, FGameplayTagContainer& EditableContainer);

	/**
	 * Called via delegate to determine the text colour of the specified node
	 *
	 * @param Node	Node to find the colour of
	 *
	 * @return Text colour of the specified node
	 */
	FSlateColor GetTagTextColour(TSharedPtr<FGameplayTagNode> Node) const;
	
	/** Called when the user clicks the "Clear All" button; Clears all tags */
	void OnClearAllClicked(TSharedPtr<SComboButton> OwnerCombo);

	/** Called when the user clicks the "Expand All" button; Expands the entire tag tree */
	void OnExpandAllClicked(TSharedPtr<SComboButton> OwnerCombo);

	/** Called when the user clicks the "Collapse All" button; Collapses the entire tag tree */
	void OnCollapseAllClicked(TSharedPtr<SComboButton> OwnerCombo);

	/**
	 * Helper function to set the expansion state of the tree widget
	 * 
	 * @param bExpand If true, expand the entire tree; Otherwise, collapse the entire tree
	 * @param bPersistExpansion If true, persist the expansion state.
	 */
	void SetTagTreeItemExpansion(bool bExpand, bool bPersistExpansion = false);

	/**
	 * Helper function to set the expansion state of a specific node
	 * 
	 * @param Node		Node to set the expansion state of
	 * @param bExpand	If true, expand the node; Otherwise, collapse the node
	 * @param bPersistExpansion If true, persist the expansion state.
	 */
	void SetTagNodeItemExpansion(TSharedPtr<FGameplayTagNode> Node, bool bExpand, bool bPersistExpansion = false);

	/** Load the tags*/
	void Load();

	/** Helper function to determine the visibility of the expandable UI controls */
	EVisibility DetermineExpandableUIVisibility() const;

	/** Recursive load function to go through all tags in the tree and set the expansion*/
	void LoadTagNodeItemExpansion(TSharedPtr<FGameplayTagNode> Node );

	/** Recursive function to go through all tags in the tree and set the expansion to default*/
	void SetDefaultTagNodeItemExpansion(TSharedPtr<FGameplayTagNode> Node);

	/** Expansion changed callback */
	void OnExpansionChanged(TSharedPtr<FGameplayTagNode> InItem, bool bIsExpanded);

	FText GetHighlightText() const;
	
	/** Creates a dropdown menu to provide additional functionality for tags (renaming, deletion, search for references, etc.) */
	TSharedRef<SWidget> MakeTagActionsMenu(TSharedPtr<FGameplayTagNode> InTagNode, TSharedPtr<SComboButton> OwnerCombo, const bool bInShouldCloseWindowAfterMenuSelection);

	/** Creates settings menu content. */
	TSharedRef<SWidget> MakeSettingsMenu(TSharedPtr<SComboButton> OwnerCombo);
	
	/** Searches for all references for the selected tag */
	void OnSearchForReferences(TSharedPtr<FGameplayTagNode> InTagNode, TSharedPtr<SComboButton> OwnerCombo);

	/** Copies individual tag's name to clipboard. */
	void OnCopyTagNameToClipboard(TSharedPtr<FGameplayTagNode> InTagNode, TSharedPtr<SComboButton> OwnerCombo);
	
	/** Returns true if the user can select tags from the widget */
	bool CanSelectTags() const { return true; };
	
	/** Populate tag items from the gameplay tags manager. */
	void GetFilteredGameplayRootTags(const FString& InFilterString, TArray<TSharedPtr<FGameplayTagNode>>& OutNodes) const;

	/** Called to create context menu for the tag tree items. */
	TSharedPtr<SWidget> OnTreeContextMenuOpening();

	/** Called when the tag tree selection changes. */
	void OnTreeSelectionChanged(TSharedPtr<FGameplayTagNode> SelectedItem, ESelectInfo::Type SelectInfo);

	/** Called to handle key presses in the tag tree. */
	FReply OnTreeKeyDown(const FGeometry& Geometry, const FKeyEvent& Key);

	/* Filter string used during search box */
	FString FilterString;

	/** root filter (passed in on creation) */
	FString RootFilterString;

	/** User specified filter function. */
	FOnFilterTag TagFilter; 

	/** If true, refreshes tags on the next frame */
	bool bDelayRefresh = false;

	/** The maximum height of the gameplay tag tree. If 0, the height is unbound. */
	float MaxHeight = 260.0f;

	/* Array of all tags */
	TArray<TSharedPtr<FGameplayTagNode>> TagItems;

	/* Array of tags filtered to be displayed in the TreeView */
	TArray<TSharedPtr<FGameplayTagNode>> FilteredTagItems;

	/** Container widget holding the tag tree */
	TSharedPtr<SBorder> TagTreeContainerWidget;

	/** Tree widget showing the gameplay tag library */
	TSharedPtr<STreeView<TSharedPtr<FGameplayTagNode>>> TagTreeWidget;

	/** Allows for the user to find a specific gameplay tag in the tree */
	TSharedPtr<SSearchBox> SearchTagBox;

	/** Containers to modify, ignored if PropertyHandle is set. */
	TArray<FGameplayTagContainer> TagContainers;

	/** Called on when tags might need refreshing (e.g. after undo/redo or when tags change). */
	FOnRefreshTagContainers OnRefreshTagContainers;

	/** Tag to scroll to, cleared after scroll is requested. */
	FGameplayTag RequestedScrollToTag;
	
	/** Guard value used to prevent feedback loops in selection handling. */
	bool bInSelectionChanged = false;

	/** Guard value used define if expansion code should persist the expansion operations. */
	bool bPersistExpansionChange = true;

	/** Expanded items cached from settings */
	TSet<TSharedPtr<FGameplayTagNode>> CachedExpandedItems;

	bool bNewTagWidgetVisible = false;
};


struct FRTSOTagWindowArgs
{
	FText Title;
	FString Filter; // Comma delimited string of tag root names to filter by
	FGameplayTag HighlightedTag; // Tag to highlight when window is opened. 
};

namespace PD::SaveEditor::Tags
{
	RTSOPEN_API TSharedRef<SWidget> CreateGameplayTagWindow(const FRTSOTagWindowArgs& Args);
	RTSOPEN_API void CloseGameplayTagWindow(TWeakPtr<SRTSOTagPicker> TagWidget);
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