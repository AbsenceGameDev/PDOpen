/* @author: Ario Amin @ Permafrost Development. @copyright: Full BSL(1.1) License included at bottom of the file  */

#include "SaveEditor/SRTSOTagPicker.h"
#include "Misc/ConfigCacheIni.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Images/SImage.h"
#include "Styling/AppStyle.h"
#include "Widgets/SWindow.h"
#include "Misc/MessageDialog.h"
#include "GameplayTagsModule.h"
#include "ScopedTransaction.h"
#include "Textures/SlateIcon.h"
#include "Widgets/Input/SSearchBox.h"
#include "Framework/Application/SlateApplication.h"
#include "AssetRegistry/AssetData.h"
#include "Framework/Commands/UIAction.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Framework/Docking/TabManager.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Misc/EnumerateRange.h"
#include "Styling/StyleColors.h"

#define LOCTEXT_NAMESPACE "TagPicker"

void SRTSOTagPicker::Construct(const FArguments& InArgs)
{
	TagContainers = InArgs._TagContainers;
	OnRefreshTagContainers = InArgs._OnRefreshTagContainers;
	RootFilterString = InArgs._Filter;

	bDelayRefresh = false;
	MaxHeight = InArgs._MaxHeight;
	
	UGameplayTagsManager::OnEditorRefreshGameplayTagTree.AddSP(this, &SRTSOTagPicker::RefreshOnNextTick);
	UGameplayTagsManager& Manager = UGameplayTagsManager::Get();

	GetFilteredGameplayRootTags(RootFilterString, TagItems);

	if (Manager.OnFilterGameplayTag.IsBound())
	{
		for (int32 Idx = TagItems.Num() - 1; Idx >= 0; --Idx)
		{
			bool DelegateShouldHide = false;
			const FGameplayTagSource* Source = Manager.FindTagSource(TagItems[Idx]->GetFirstSourceName());
			Manager.OnFilterGameplayTag.Broadcast(UGameplayTagsManager::FFilterGameplayTagContext(RootFilterString, TagItems[Idx], Source, nullptr), DelegateShouldHide);
			if (DelegateShouldHide == false) { continue; }
			TagItems.RemoveAtSwap(Idx);
		}
	}
	
	TSharedPtr<SComboButton> SettingsCombo = SNew(SComboButton)
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Center)
		.ButtonStyle(FAppStyle::Get(), "SimpleButton")
		.HasDownArrow(false)
		.ButtonContent()
		[
			SNew(SImage)
			.Image(FAppStyle::GetBrush("Icons.Settings"))
			.ColorAndOpacity(FSlateColor::UseForeground())
		];
	SettingsCombo->SetOnGetMenuContent(FOnGetContent::CreateSP(this, &SRTSOTagPicker::MakeSettingsMenu, SettingsCombo));


	TWeakPtr<SRTSOTagPicker> WeakSelf = StaticCastWeakPtr<SRTSOTagPicker>(AsWeak());

	const TSharedRef<SWidget> Picker = 
		SNew(SBorder)
		.Padding(InArgs._Padding)
		.BorderImage(FStyleDefaults::GetNoBrush())
		[
			SNew(SVerticalBox)

			// Gameplay Tag Tree controls
			+SVerticalBox::Slot()
			.AutoHeight()
			.VAlign(VAlign_Top)
			[
				SNew(SHorizontalBox)

				// Search
				+SHorizontalBox::Slot()
				.VAlign(VAlign_Center)
				.FillWidth(1.f)
				.Padding(0,1,5,1)
				[
					SAssignNew(SearchTagBox, SSearchBox)
					.HintText(LOCTEXT("TagPicker_SearchBoxHint", "Search Gameplay Tags"))
					.OnTextChanged(this, &SRTSOTagPicker::OnFilterTextChanged)
				]

				// View settings
				+SHorizontalBox::Slot()
				.AutoWidth()
				[
					SettingsCombo.ToSharedRef()
				]
			]

			// Gameplay Tags tree
			+SVerticalBox::Slot()
			.MaxHeight(MaxHeight)
			.FillHeight(1)
			[
				SAssignNew(TagTreeContainerWidget, SBorder)
				.BorderImage(FStyleDefaults::GetNoBrush())
				[
					SAssignNew(TagTreeWidget, STreeView<TSharedPtr<FGameplayTagNode>>)
					.TreeItemsSource(&TagItems)
					.OnGenerateRow(this, &SRTSOTagPicker::OnGenerateRow)
					.OnGetChildren(this, &SRTSOTagPicker::OnGetChildren)
					.OnExpansionChanged(this, &SRTSOTagPicker::OnExpansionChanged)
					.SelectionMode(ESelectionMode::Single)
					.OnContextMenuOpening(this, &SRTSOTagPicker::OnTreeContextMenuOpening)
					.OnSelectionChanged(this, &SRTSOTagPicker::OnTreeSelectionChanged)
					.OnKeyDownHandler(this, &SRTSOTagPicker::OnTreeKeyDown)
				]
			]
		];

	if (InArgs._ShowMenuItems)
	{
		FMenuBuilder MenuBuilder(/*bInShouldCloseWindowAfterMenuSelection*/false, nullptr);

		MenuBuilder.BeginSection(FName(), LOCTEXT("SectionGameplayTag", "GameplayTag"));
		MenuBuilder.AddMenuEntry(
			LOCTEXT("TagPicker_ClearSelection", "Clear Selection"), FText::GetEmpty(), FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.X"),
			FUIAction(FExecuteAction::CreateRaw(this, &SRTSOTagPicker::OnClearAllClicked, TSharedPtr<SComboButton>()))
		);

		MenuBuilder.AddSeparator();

		const TSharedRef<SWidget> MenuContent =
			SNew(SBox)
			.WidthOverride(300.0f)
			.HeightOverride(MaxHeight)
			[
				Picker
			];
		MenuBuilder.AddWidget(MenuContent, FText::GetEmpty(), true);

		MenuBuilder.EndSection();
		
		ChildSlot
		[
			MenuBuilder.MakeWidget()
		];
	}
	else
	{
		ChildSlot
		[
			Picker
		];
	}
	
	// Force the entire tree collapsed to start
	SetTagTreeItemExpansion(/*bExpand*/false, /*bPersistExpansion*/false);
	Load();
	VerifyAssetTagValidity();
}

TSharedPtr<SWidget> SRTSOTagPicker::OnTreeContextMenuOpening()
{
	TArray<TSharedPtr<FGameplayTagNode>> Selection = TagTreeWidget->GetSelectedItems();
	const TSharedPtr<FGameplayTagNode> SelectedTagNode = Selection.IsEmpty() ? nullptr : Selection[0];
	return MakeTagActionsMenu(SelectedTagNode, TSharedPtr<SComboButton>(), /*bInShouldCloseWindowAfterMenuSelection*/true);
}

void SRTSOTagPicker::OnTreeSelectionChanged(TSharedPtr<FGameplayTagNode> SelectedItem, ESelectInfo::Type SelectInfo)
{
	if (SelectInfo != ESelectInfo::OnMouseClick) { return; }
	
	// Do not allow to select lines as they have not meaning, but the highlight helps navigating the list.   
	if (bInSelectionChanged) { return; }

	TGuardValue<bool> PersistExpansionChangeGuard(bInSelectionChanged, true);
	TagTreeWidget->ClearSelection();
	
	const ECheckBoxState State = IsTagChecked(SelectedItem); // Toggle selection
	if (State == ECheckBoxState::Unchecked)
	{
		OnTagChecked(SelectedItem);
	}
	else if (State == ECheckBoxState::Checked)
	{
		OnTagUnchecked(SelectedItem);
	}
}

FReply SRTSOTagPicker::OnTreeKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (FSlateApplication::Get().GetNavigationActionFromKey(InKeyEvent) == EUINavigationAction::Accept)
	{
		TArray<TSharedPtr<FGameplayTagNode>> Selection = TagTreeWidget->GetSelectedItems();
		
		TSharedPtr<FGameplayTagNode> SelectedItem;
		if (Selection.IsEmpty() == false) { SelectedItem = Selection[0]; }

		if (SelectedItem.IsValid() == false) { return SCompoundWidget::OnKeyDown(InGeometry, InKeyEvent); }
		

		TGuardValue<bool> PersistExpansionChangeGuard(bInSelectionChanged, true);
		const ECheckBoxState State = IsTagChecked(SelectedItem); // Toggle selection

		switch (State)
		{
		case ECheckBoxState::Unchecked: OnTagChecked(SelectedItem); break;
		case ECheckBoxState::Checked: OnTagUnchecked(SelectedItem); break;
		case ECheckBoxState::Undetermined: break;
		}
		
		return FReply::Handled();
	}

	return SCompoundWidget::OnKeyDown(InGeometry, InKeyEvent);
}

TSharedRef<SWidget> SRTSOTagPicker::MakeSettingsMenu(TSharedPtr<SComboButton> OwnerCombo)
{
	FMenuBuilder MenuBuilder(/*bInShouldCloseWindowAfterMenuSelection*/false, nullptr);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("TagPicker_ExpandAll", "Expand All"), FText::GetEmpty(), FSlateIcon(),
		FUIAction(FExecuteAction::CreateRaw(this, &SRTSOTagPicker::OnExpandAllClicked, OwnerCombo))
	);
	
	MenuBuilder.AddMenuEntry(
		LOCTEXT("TagPicker_CollapseAll", "Collapse All"), FText::GetEmpty(), FSlateIcon(),
		FUIAction(FExecuteAction::CreateRaw(this, &SRTSOTagPicker::OnCollapseAllClicked, OwnerCombo))
	);
	
	return MenuBuilder.MakeWidget();
}

void SRTSOTagPicker::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	if (bDelayRefresh)
	{
		RefreshTags();
		bDelayRefresh = false;
	}

	if (RequestedScrollToTag.IsValid() == false) { return; }

	// Scroll specified item into view.
	const UGameplayTagsManager& Manager = UGameplayTagsManager::Get();
	const TSharedPtr<FGameplayTagNode> Node = Manager.FindTagNode(RequestedScrollToTag);
	RequestedScrollToTag = FGameplayTag();
	
	if (Node.IsValid() == false) { return; }
	
	// Expand all the parent nodes to make sure the target node is visible.
	TSharedPtr<FGameplayTagNode> ParentNode = Node.IsValid() ? Node->GetParentTagNode() : nullptr;
	while (ParentNode.IsValid())
	{
		TagTreeWidget->SetItemExpansion(ParentNode, /*bExpand*/true);
		ParentNode = ParentNode->GetParentTagNode();
	}
	TagTreeWidget->ClearSelection();
	TagTreeWidget->SetItemSelection(Node, true);
	TagTreeWidget->RequestScrollIntoView(Node);
	
}

void SRTSOTagPicker::OnFilterTextChanged(const FText& InFilterText)
{
	FilterString = InFilterText.ToString();	
	FilterTagTree();
}

void SRTSOTagPicker::FilterTagTree()
{
	if (FilterString.IsEmpty())
	{
		TagTreeWidget->SetTreeItemsSource(&TagItems);

		for (int32 iItem = 0; iItem < TagItems.Num(); ++iItem)
		{
			SetDefaultTagNodeItemExpansion(TagItems[iItem]);
		}
	}
	else
	{
		FilteredTagItems.Empty();

		for (int32 iItem = 0; iItem < TagItems.Num(); ++iItem)
		{
			if (FilterChildrenCheck(TagItems[iItem]))
			{
				FilteredTagItems.Add(TagItems[iItem]);
				SetTagNodeItemExpansion(TagItems[iItem], true);
			}
			else
			{
				SetTagNodeItemExpansion(TagItems[iItem], false);
			}
		}

		TagTreeWidget->SetTreeItemsSource(&FilteredTagItems);
	}

	TagTreeWidget->RequestTreeRefresh();
}

bool SRTSOTagPicker::FilterChildrenCheckRecursive(TSharedPtr<FGameplayTagNode>& InItem) const
{
	for (TSharedPtr<FGameplayTagNode>& Child : InItem->GetChildTagNodes())
	{
		if (FilterChildrenCheck(Child)) { return true; }
	}
	return false;
}

bool SRTSOTagPicker::FilterChildrenCheck(TSharedPtr<FGameplayTagNode>& InItem) const
{
	if (InItem.IsValid() == false) { return false; }

	UGameplayTagsManager& Manager = UGameplayTagsManager::Get();
	bool bDelegateShouldHide = false;
	Manager.OnFilterGameplayTagChildren.Broadcast(RootFilterString, InItem, bDelegateShouldHide);
	if (bDelegateShouldHide == false && Manager.OnFilterGameplayTag.IsBound())
	{
		const FGameplayTagSource* Source = Manager.FindTagSource(InItem->GetFirstSourceName());
		Manager.OnFilterGameplayTag.Broadcast(UGameplayTagsManager::FFilterGameplayTagContext(RootFilterString, InItem, Source, nullptr), bDelegateShouldHide);
	}
	// The delegate wants to hide, see if any children need to show
	if (bDelegateShouldHide) { return FilterChildrenCheckRecursive(InItem); }

	const bool bNoFilterOrMatchedFilter = InItem->GetCompleteTagString().Contains(FilterString) || FilterString.IsEmpty();
	if (bNoFilterOrMatchedFilter) { return true; }

	return FilterChildrenCheckRecursive(InItem);
}

FText SRTSOTagPicker::GetHighlightText() const
{
	return FilterString.IsEmpty() ? FText::GetEmpty() : FText::FromString(FilterString);
}

TSharedRef<ITableRow> SRTSOTagPicker::OnGenerateRow(TSharedPtr<FGameplayTagNode> InItem, const TSharedRef<STableViewBase>& OwnerTable)
{
	FText TooltipText;
	FString TagSource;
	if (InItem.IsValid())
	{
		[[maybe_unused]] const UGameplayTagsManager& Manager = UGameplayTagsManager::Get();

		const FName TagName = InItem.Get()->GetCompleteTagName();
		const TSharedPtr<FGameplayTagNode> Node = Manager.FindTagNode(TagName);

		FString TooltipString = TagName.ToString();

		if (Node.IsValid())
		{
			constexpr int32 MaxSourcesToDisplay = 3; // How many sources to display before showing ellipsis (tool tip will have all sources). 

			FString AllSources;
			for (TConstEnumerateRef<FName> Source : EnumerateRange(Node->GetAllSourceNames()))
			{
				if (AllSources.Len() > 0) { AllSources += TEXT(", "); }
				AllSources += Source->ToString();
				
				if (Source.GetIndex() < MaxSourcesToDisplay)
				{
					if (TagSource.Len() > 0) { TagSource += TEXT(", "); }
					TagSource += Source->ToString();
				}
			}
			
			if (Node->GetAllSourceNames().Num() > MaxSourcesToDisplay)
			{
				TagSource += FString::Printf(TEXT(", ... (%d)"), Node->GetAllSourceNames().Num() - MaxSourcesToDisplay);
			}

			const bool bIsExplicitTag = Node->IsExplicitTag();
			TooltipString.Append(FString::Printf(TEXT("\n(%s%s)"), bIsExplicitTag ? TEXT("") : TEXT("Implicit "), *AllSources));
		}

		TooltipText = FText::FromString(TooltipString);
	}

	TSharedPtr<SComboButton> ActionsCombo = SNew(SComboButton)
		.ToolTipText(LOCTEXT("MoreActions", "More Actions..."))
		.ButtonStyle(FAppStyle::Get(), "HoverHintOnly")
		.ContentPadding(0)
		.ForegroundColor(FSlateColor::UseForeground())
		.HasDownArrow(true)
		.CollapseMenuOnParentFocus(true);

	// Create context menu with bInShouldCloseWindowAfterMenuSelection = false, or else the actions menu action will not work due the popup-menu handling order.
	ActionsCombo->SetOnGetMenuContent(FOnGetContent::CreateSP(this, &SRTSOTagPicker::MakeTagActionsMenu, InItem, ActionsCombo, /*bInShouldCloseWindowAfterMenuSelection*/false));
	
	return SNew(STableRow<TSharedPtr<FGameplayTagNode>>, OwnerTable)
		.Style(FAppStyle::Get(), "GameplayTagTreeView")
		.ToolTipText(TooltipText)
	[
		SNew(SHorizontalBox)
		// Tag Selection (selection mode only)
		+SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Center)
		[
			SNew(SCheckBox)
				.OnCheckStateChanged(this, &SRTSOTagPicker::OnTagCheckStatusChanged, InItem)
				.IsChecked(this, &SRTSOTagPicker::IsTagChecked, InItem)
				.IsEnabled(this, &SRTSOTagPicker::CanSelectTags)
				.CheckBoxContentUsesAutoWidth(false)
			[
				SNew(STextBlock)
					.HighlightText(this, &SRTSOTagPicker::GetHighlightText)
					.Text(FText::FromName(InItem->GetSimpleTagName()))
			]
		]
		// Allows non-restricted children checkbox
		+SHorizontalBox::Slot()
			.AutoWidth()
			.HAlign(HAlign_Right)
			.VAlign(VAlign_Center)
		[
			SNew(SCheckBox)
				.ToolTipText(LOCTEXT("AllowsChildren", "Does this restricted tag allow non-restricted children"))
				.IsChecked(this, &SRTSOTagPicker::IsAllowChildrenTagChecked, InItem)
				.Visibility(this, &SRTSOTagPicker::DetermineAllowChildrenVisible, InItem)
		]
		// More Actions Menu
		+SHorizontalBox::Slot()
			.AutoWidth()
			.HAlign(HAlign_Right)
			.VAlign(VAlign_Center)
		[
			ActionsCombo.ToSharedRef()
		]
	];
}

void SRTSOTagPicker::OnGetChildren(TSharedPtr<FGameplayTagNode> InItem, TArray<TSharedPtr<FGameplayTagNode>>& OutChildren)
{
	TArray<TSharedPtr<FGameplayTagNode>> FilteredChildren;
	TArray<TSharedPtr<FGameplayTagNode>> Children = InItem->GetChildTagNodes();

	for (int32 ChildIndex = 0; ChildIndex < Children.Num(); ++ChildIndex)
	{
		if (FilterChildrenCheck(Children[ChildIndex])) { FilteredChildren.Add(Children[ChildIndex]); }
	}
	OutChildren += FilteredChildren;
}

void SRTSOTagPicker::OnTagCheckStatusChanged(ECheckBoxState NewCheckState, TSharedPtr<FGameplayTagNode> NodeChanged)
{
	if (NewCheckState == ECheckBoxState::Checked) { OnTagChecked(NodeChanged); }
	else if (NewCheckState == ECheckBoxState::Unchecked) { OnTagUnchecked(NodeChanged); }
}

void SRTSOTagPicker::OnTagChecked(TSharedPtr<FGameplayTagNode> NodeChecked)
{
	FScopedTransaction Transaction(LOCTEXT("TagPicker_SelectTags", "Select Gameplay Tags"));

	[[maybe_unused]] UGameplayTagsManager& TagsManager = UGameplayTagsManager::Get();

	for (FGameplayTagContainer& Container : TagContainers)
	{
		TSharedPtr<FGameplayTagNode> CurNode(NodeChecked);

		bool bRemoveParents = false;

		while (CurNode.IsValid())
		{
			FGameplayTag GameplayTag = CurNode->GetCompleteTag();

			if (bRemoveParents == false)
			{
				bRemoveParents = true;
				Container.Reset();
				Container.AddTag(GameplayTag);
			}
			else
			{
				Container.RemoveTag(GameplayTag);
			}

			CurNode = CurNode->GetParentTagNode();
		}
	}
}

void SRTSOTagPicker::OnTagUnchecked(TSharedPtr<FGameplayTagNode> NodeUnchecked)
{
	FScopedTransaction Transaction(LOCTEXT("TagPicker_RemoveTags", "Remove Gameplay Tags"));
	if (NodeUnchecked.IsValid() == false) { return; }
	
	[[maybe_unused]] UGameplayTagsManager& TagsManager = UGameplayTagsManager::Get();

	for (FGameplayTagContainer& Container : TagContainers)
	{
		FGameplayTag GameplayTag = NodeUnchecked->GetCompleteTag();
		Container.RemoveTag(GameplayTag);
		TSharedPtr<FGameplayTagNode> ParentNode = NodeUnchecked->GetParentTagNode();
		if (ParentNode.IsValid())
		{
			// Check if there are other siblings before adding parent
			bool bOtherSiblings = false;
			for (auto It = ParentNode->GetChildTagNodes().CreateConstIterator(); It; ++It)
			{
				GameplayTag = It->Get()->GetCompleteTag();
				if (Container.HasTagExact(GameplayTag))
				{
					bOtherSiblings = true;
					break;
				}
			}
			// Add Parent
			if (bOtherSiblings == false)
			{
				GameplayTag = ParentNode->GetCompleteTag();
				Container.AddTag(GameplayTag);
			}
		}
		
		// Uncheck Children
		for (const auto& ChildNode : NodeUnchecked->GetChildTagNodes())
		{
			UncheckChildren(ChildNode, Container);
		}
	}
}

void SRTSOTagPicker::UncheckChildren(TSharedPtr<FGameplayTagNode> NodeUnchecked, FGameplayTagContainer& EditableContainer)
{
	[[maybe_unused]] UGameplayTagsManager& TagsManager = UGameplayTagsManager::Get();

	const FGameplayTag GameplayTag = NodeUnchecked->GetCompleteTag();
	EditableContainer.RemoveTag(GameplayTag);

	// Uncheck Children
	for (const auto& ChildNode : NodeUnchecked->GetChildTagNodes())
	{
		UncheckChildren(ChildNode, EditableContainer);
	}
}

ECheckBoxState SRTSOTagPicker::IsTagChecked(TSharedPtr<FGameplayTagNode> Node) const
{
	int32 NumValidAssets = 0;
	int32 NumAssetsTagIsAppliedTo = 0;

	if (Node.IsValid())
	{
		[[maybe_unused]] UGameplayTagsManager& TagsManager = UGameplayTagsManager::Get();

		for (const FGameplayTagContainer& Container : TagContainers)
		{
			NumValidAssets++;
			const FGameplayTag GameplayTag = Node->GetCompleteTag();
			if (GameplayTag.IsValid() && Container.HasTag(GameplayTag))
			{
				++NumAssetsTagIsAppliedTo;
			}
		}
	}

	if (NumAssetsTagIsAppliedTo == 0) { return ECheckBoxState::Unchecked; }
	if (NumAssetsTagIsAppliedTo == NumValidAssets) { return ECheckBoxState::Checked; }
	return ECheckBoxState::Undetermined;
}

bool SRTSOTagPicker::IsExactTagInCollection(TSharedPtr<FGameplayTagNode> Node) const
{
	if (Node.IsValid() == false) { return false; }
	
	for (const FGameplayTagContainer& Container : TagContainers)
	{
		FGameplayTag GameplayTag = Node->GetCompleteTag();
		if (GameplayTag.IsValid() && Container.HasTagExact(GameplayTag))
		{
			return true;
		}
	}

	return false;
}

ECheckBoxState SRTSOTagPicker::IsAllowChildrenTagChecked(TSharedPtr<FGameplayTagNode> Node) const
{
	return Node->GetAllowNonRestrictedChildren() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

EVisibility SRTSOTagPicker::DetermineAllowChildrenVisible(TSharedPtr<FGameplayTagNode> Node) const
{
	return EVisibility::Visible;
}

void SRTSOTagPicker::OnClearAllClicked(TSharedPtr<SComboButton> OwnerCombo)
{
	FScopedTransaction Transaction(LOCTEXT("TagPicker_RemoveAllTags", "Remove All Gameplay Tags") );

	for (FGameplayTagContainer& Container : TagContainers) { Container.Reset(); }

	if (OwnerCombo.IsValid()) { OwnerCombo->SetIsOpen(false); }
}

FSlateColor SRTSOTagPicker::GetTagTextColour(TSharedPtr<FGameplayTagNode> Node) const
{
	static const FLinearColor DefaultTextColour = FLinearColor::White;
	return DefaultTextColour;
}

void SRTSOTagPicker::OnExpandAllClicked(TSharedPtr<SComboButton> OwnerCombo)
{
	SetTagTreeItemExpansion(/*bExpand*/true, /*bPersistExpansion*/true);
	if (OwnerCombo.IsValid())
	{
		OwnerCombo->SetIsOpen(false);
	}
}

void SRTSOTagPicker::OnCollapseAllClicked(TSharedPtr<SComboButton> OwnerCombo)
{
	SetTagTreeItemExpansion(/*bExpand*/false, /*bPersistExpansion*/true);
	if (OwnerCombo.IsValid()) { OwnerCombo->SetIsOpen(false); }
}

TSharedRef<SWidget> SRTSOTagPicker::MakeTagActionsMenu(TSharedPtr<FGameplayTagNode> InTagNode, TSharedPtr<SComboButton> ActionsCombo, bool bInShouldCloseWindowAfterMenuSelection)
{
	if (InTagNode.IsValid() == false) { return SNullWidget::NullWidget;}

	[[maybe_unused]] UGameplayTagsManager& Manager = UGameplayTagsManager::Get();
	
	// Do not close menu after selection. The close deletes this widget before action is executed leading to no action being performed.
	// Occurs when SRTSOTagPicker is being used as a menu item itself (Details panel of blueprint editor for example).
	FMenuBuilder MenuBuilder(bInShouldCloseWindowAfterMenuSelection, nullptr);
	
	MenuBuilder.AddSeparator();
	
	// Search for References
	if (FEditorDelegates::OnOpenReferenceViewer.IsBound())
	{
		MenuBuilder.AddMenuEntry(LOCTEXT("TagPicker_SearchForReferences", "Search For References"),
		FText::Format(LOCTEXT("TagPicker_SearchForReferencesTooltip", "Find references to the tag {0}"), FText::AsCultureInvariant(InTagNode->GetCompleteTagString())),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Search"),
			FUIAction(FExecuteAction::CreateSP(this, &SRTSOTagPicker::OnSearchForReferences, InTagNode, ActionsCombo)));
	}

	// Copy Name to Clipboard
	MenuBuilder.AddMenuEntry(LOCTEXT("TagPicker_CopyNameToClipboard", "Copy Name to Clipboard"),
	FText::Format(LOCTEXT("TagPicker_CopyNameToClipboardTooltip", "Copy tag {0} to clipboard"), FText::AsCultureInvariant(InTagNode->GetCompleteTagString())),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "GenericCommands.Copy"),
		FUIAction(FExecuteAction::CreateSP(this, &SRTSOTagPicker::OnCopyTagNameToClipboard, InTagNode, ActionsCombo)));

	return MenuBuilder.MakeWidget();
}

void SRTSOTagPicker::OnSearchForReferences(TSharedPtr<FGameplayTagNode> InTagNode, TSharedPtr<SComboButton> OwnerCombo)
{
	if (InTagNode.IsValid())
	{
		TArray<FAssetIdentifier> AssetIdentifiers;
		AssetIdentifiers.Add(FAssetIdentifier(FGameplayTag::StaticStruct(), InTagNode->GetCompleteTagName()));
		FEditorDelegates::OnOpenReferenceViewer.Broadcast(AssetIdentifiers, FReferenceViewerParams());
	}
	
	if (OwnerCombo.IsValid()) { OwnerCombo->SetIsOpen(false); }
}

void SRTSOTagPicker::OnCopyTagNameToClipboard(TSharedPtr<FGameplayTagNode> InTagNode, TSharedPtr<SComboButton> OwnerCombo)
{
	if (InTagNode.IsValid())
	{
		const FString TagName = InTagNode->GetCompleteTagString();
		FPlatformApplicationMisc::ClipboardCopy(*TagName);
	}
	
	if (OwnerCombo.IsValid()) { OwnerCombo->SetIsOpen(false); }
}

void SRTSOTagPicker::SetTagTreeItemExpansion(bool bExpand, bool bPersistExpansion)
{
	TArray<TSharedPtr<FGameplayTagNode>> TagArray;
	UGameplayTagsManager::Get().GetFilteredGameplayRootTags(TEXT(""), TagArray);
	for (int32 TagIdx = 0; TagIdx < TagArray.Num(); ++TagIdx)
	{
		SetTagNodeItemExpansion(TagArray[TagIdx], bExpand, bPersistExpansion);
	}
}

void SRTSOTagPicker::SetTagNodeItemExpansion(TSharedPtr<FGameplayTagNode> Node, bool bExpand, bool bPersistExpansion)
{
	TGuardValue<bool> PersistExpansionChangeGuard(bPersistExpansionChange, bPersistExpansion);
	if (Node.IsValid() && TagTreeWidget.IsValid())
	{
		TagTreeWidget->SetItemExpansion(Node, bExpand);

		const TArray<TSharedPtr<FGameplayTagNode>>& ChildTags = Node->GetChildTagNodes();
		for (int32 ChildIdx = 0; ChildIdx < ChildTags.Num(); ++ChildIdx)
		{
			SetTagNodeItemExpansion(ChildTags[ChildIdx], bExpand, bPersistExpansion);
		}
	}
}

void SRTSOTagPicker::Load()
{
	[[maybe_unused]] const UGameplayTagsManager& TagsManager = UGameplayTagsManager::Get();
	
	CachedExpandedItems.Reset();
	TArray<TSharedPtr<FGameplayTagNode>> TagArray;
	
	TagsManager.GetFilteredGameplayRootTags(TEXT(""), TagArray);
	for (int32 TagIdx = 0; TagIdx < TagArray.Num(); ++TagIdx)
	{
		LoadTagNodeItemExpansion(TagArray[TagIdx]);
	}
}

void SRTSOTagPicker::SetDefaultTagNodeItemExpansion(TSharedPtr<FGameplayTagNode> Node)
{
	TGuardValue<bool> PersistExpansionChangeGuard(bPersistExpansionChange, false);
	if (Node.IsValid() && TagTreeWidget.IsValid())
	{
		const bool bIsExpanded = CachedExpandedItems.Contains(Node) || IsTagChecked(Node) == ECheckBoxState::Checked;
		TagTreeWidget->SetItemExpansion(Node, bIsExpanded);
		
		const TArray<TSharedPtr<FGameplayTagNode>>& ChildTags = Node->GetChildTagNodes();
		for (int32 ChildIdx = 0; ChildIdx < ChildTags.Num(); ++ChildIdx)
		{
			SetDefaultTagNodeItemExpansion(ChildTags[ChildIdx]);
		}
	}
}

void SRTSOTagPicker::LoadTagNodeItemExpansion(TSharedPtr<FGameplayTagNode> Node)
{
	TGuardValue<bool> PersistExpansionChangeGuard(bPersistExpansionChange, false);
	if (Node.IsValid() == false || TagTreeWidget.IsValid() == false) { return; }
	
	// @todo allow not selecting
	if (Node->GetChildTagNodes().IsEmpty() == false)
	{
		TagTreeWidget->SetItemExpansion(Node, true);
		CachedExpandedItems.Add(Node);
	}
	else if (IsTagChecked(Node) == ECheckBoxState::Checked) 
	{
		TagTreeWidget->SetItemExpansion(Node, true);
	}
	const TArray<TSharedPtr<FGameplayTagNode>>& ChildTags = Node->GetChildTagNodes();
	for (int32 ChildIdx = 0; ChildIdx < ChildTags.Num(); ++ChildIdx)
	{
		LoadTagNodeItemExpansion(ChildTags[ChildIdx]);
	}
}

void SRTSOTagPicker::OnExpansionChanged(TSharedPtr<FGameplayTagNode> InItem, bool bIsExpanded)
{
	if (bPersistExpansionChange == false) { return; }
	if (bIsExpanded) { CachedExpandedItems.Add(InItem); }
	else { CachedExpandedItems.Remove(InItem); }
}

void SRTSOTagPicker::RefreshTags()
{
	UGameplayTagsManager& Manager = UGameplayTagsManager::Get();
	Manager.GetFilteredGameplayRootTags(RootFilterString, TagItems);
	
	if (Manager.OnFilterGameplayTag.IsBound())
	{
		for (int32 Idx = TagItems.Num() - 1; Idx >= 0; --Idx)
		{
			bool DelegateShouldHide = false;
			const FGameplayTagSource* Source = Manager.FindTagSource(TagItems[Idx]->GetFirstSourceName());
			Manager.OnFilterGameplayTag.Broadcast(UGameplayTagsManager::FFilterGameplayTagContext(RootFilterString, TagItems[Idx], Source, nullptr), DelegateShouldHide);
			if (DelegateShouldHide) { TagItems.RemoveAtSwap(Idx); }
		}
	}

	// Restore expansion state.
	CachedExpandedItems.Reset();
	for (int32 TagIdx = 0; TagIdx < TagItems.Num(); ++TagIdx)
	{
		LoadTagNodeItemExpansion(TagItems[TagIdx]);
	}

	FilterTagTree();
	OnRefreshTagContainers.ExecuteIfBound(*this);
}

EVisibility SRTSOTagPicker::DetermineExpandableUIVisibility() const
{
	[[maybe_unused]] const UGameplayTagsManager& Manager = UGameplayTagsManager::Get();
	
	return EVisibility::Collapsed;
}


void SRTSOTagPicker::RefreshOnNextTick()
{
	bDelayRefresh = true;
}

void SRTSOTagPicker::RequestScrollToView(const FGameplayTag RequestedTag)
{
	RequestedScrollToTag = RequestedTag;
}

TSharedPtr<SWidget> SRTSOTagPicker::GetWidgetToFocusOnOpen()
{
	return SearchTagBox;
}

void SRTSOTagPicker::SetTagContainers(TConstArrayView<FGameplayTagContainer> InTagContainers)
{
	TagContainers = InTagContainers;
}

void SRTSOTagPicker::PostUndo(bool bSuccess)
{
	OnRefreshTagContainers.ExecuteIfBound(*this);
}

void SRTSOTagPicker::PostRedo(bool bSuccess)
{
	OnRefreshTagContainers.ExecuteIfBound(*this);
}

void SRTSOTagPicker::VerifyAssetTagValidity()
{
	[[maybe_unused]] UGameplayTagsManager& TagsManager = UGameplayTagsManager::Get();

	// Find and remove any tags on the asset that are no longer in the library
	for (FGameplayTagContainer& Container : TagContainers)
	{
		// Use a set instead of a container so we can find and remove None tags
		TSet<FGameplayTag> InvalidTags;

		for (auto It = Container.CreateConstIterator(); It; ++It)
		{
			const FGameplayTag TagToCheck = *It;

			const bool bValidTag = UGameplayTagsManager::Get().RequestGameplayTag(TagToCheck.GetTagName(), false).IsValid() == false;
			if (bValidTag == false) { InvalidTags.Add(*It); } 
		}

		if (InvalidTags.IsEmpty()) { return; }
		
		FString InvalidTagNames;
		for (auto InvalidIter = InvalidTags.CreateConstIterator(); InvalidIter; ++InvalidIter)
		{
			Container.RemoveTag(*InvalidIter);
			InvalidTagNames += InvalidIter->ToString() + TEXT("\n");
		}
		FFormatNamedArguments Arguments;
		Arguments.Add(TEXT("Objects"), FText::FromString(InvalidTagNames));
		FText DialogText = FText::Format(LOCTEXT("TagPicker_InvalidTags", "Invalid Tags that have been removed: \n\n{Objects}"), Arguments);
		FMessageDialog::Open(EAppMsgType::Ok, DialogText, LOCTEXT("TagPicker_Warning", "Warning"));
	}
}

void SRTSOTagPicker::GetFilteredGameplayRootTags(const FString& InFilterString, TArray<TSharedPtr<FGameplayTagNode>>& OutNodes) const
{
	OutNodes.Empty();
	const UGameplayTagsManager& Manager = UGameplayTagsManager::Get();

	if (TagFilter.IsBound() == false)
	{
		Manager.GetFilteredGameplayRootTags(InFilterString, OutNodes);
		return;
	}
	
	TArray<TSharedPtr<FGameplayTagNode>> UnfilteredItems;
	Manager.GetFilteredGameplayRootTags(InFilterString, UnfilteredItems);
	for (const TSharedPtr<FGameplayTagNode>& Node : UnfilteredItems)
	{
		if (TagFilter.Execute(Node) != ETagFilterResult::IncludeTag) { continue; }
		OutNodes.Add(Node);
	}
}

namespace PD::SaveEditor::Tags
{
	static TWeakPtr<SRTSOTagPicker> GlobalTagWidget;
	static TWeakPtr<SWindow> GlobalTagWidgetWindow;

	TSharedRef<SWidget> CreateGameplayTagWindow(const FRTSOTagWindowArgs& Args)
	{
		return SNew(SRTSOTagPicker)
			.Filter(Args.Filter)
			.TagContainers({FGameplayTagContainer{Args.HighlightedTag}});
	}
	
	void CloseGameplayTagWindow(TWeakPtr<SRTSOTagPicker> TagWidget)
	{
		if (GlobalTagWidget.IsValid()
			&& GlobalTagWidgetWindow.IsValid()
			&& (TagWidget.IsValid() == false || TagWidget == GlobalTagWidget))
		{
			GlobalTagWidgetWindow.Pin()->RequestDestroyWindow();
		}
		
		GlobalTagWidgetWindow = nullptr;
		GlobalTagWidget = nullptr;
	}

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