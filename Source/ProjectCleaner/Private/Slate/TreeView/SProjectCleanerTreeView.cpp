// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/TreeView/SProjectCleanerTreeView.h"
#include "Slate/TreeView/SProjectCleanerTreeViewItem.h"
#include "ProjectCleaner.h"
#include "ProjectCleanerTypes.h"
#include "ProjectCleanerStyles.h"
#include "ProjectCleanerLibrary.h"
#include "ProjectCleanerScanner.h"
#include "ProjectCleanerConstants.h"
#include "ProjectCleanerScanSettings.h"
// Engine Headers
#include "ProjectCleanerCmds.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"

void SProjectCleanerTreeView::Construct(const FArguments& InArgs)
{
	Scanner = InArgs._Scanner;
	if (!Scanner.IsValid()) return;

	ScanSettings = GetMutableDefault<UProjectCleanerScanSettings>();
	if (!ScanSettings.IsValid()) return;

	Cmds = MakeShareable(new FUICommandList);
	Cmds->MapAction(
		FProjectCleanerCmds::Get().TabUnusedPathExclude,
		FUIAction(
			FExecuteAction::CreateLambda([&]()
			{
				UE_LOG(LogProjectCleaner, Warning, TEXT("Excluding paths"));
			})
		)
	);
	Cmds->MapAction(
		FProjectCleanerCmds::Get().TabUnusedPathInclude,
		FUIAction(
			FExecuteAction::CreateLambda([&]()
			{
				UE_LOG(LogProjectCleaner, Warning, TEXT("Including paths"));
			})
		)
	);
	Cmds->MapAction(
		FProjectCleanerCmds::Get().TabUnusedPathClean,
		FUIAction(
			FExecuteAction::CreateLambda([&]()
			{
				UE_LOG(LogProjectCleaner, Warning, TEXT("Cleaning paths"));
			})
		)
	);

	Scanner->OnScanFinished().AddLambda([&]()
	{
		TreeItemsUpdate();
	});

	TreeItemsUpdate();

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		  .AutoHeight()
		  .Padding(FMargin{0.0f, 0.0f, 0.0f, 10.0f})
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SImage)
				.Image(FEditorStyle::GetBrush("ContentBrowser.AssetTreeFolderOpen"))
				.ColorAndOpacity(FProjectCleanerStyles::Get().GetSlateColor("ProjectCleaner.Color.Red"))
			]
			+ SHorizontalBox::Slot()
			  .Padding(FMargin{0.0f, 2.0f, 0.0f, 0.0f})
			  .AutoWidth()
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT(" - Empty Folders")))
			]
			+ SHorizontalBox::Slot()
			  .AutoWidth()
			  .Padding(FMargin{5.0f, 0.0f, 0.0f, 0.0f})
			[
				SNew(SImage)
				.Image(FEditorStyle::GetBrush("ContentBrowser.AssetTreeFolderOpen"))
				.ColorAndOpacity(FProjectCleanerStyles::Get().GetSlateColor("ProjectCleaner.Color.Yellow"))
			]
			+ SHorizontalBox::Slot()
			  .Padding(FMargin{0.0f, 2.0f, 0.0f, 0.0f})
			  .AutoWidth()
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT(" - Excluded Folders")))
			]
		]
		+ SVerticalBox::Slot()
		  .AutoHeight()
		  .Padding(FMargin{0.0f, 0.0f, 0.0f, 5.0f})
		[
			SNew(SSeparator)
			.Thickness(5.0f)
		]
		+ SVerticalBox::Slot()
		  .AutoHeight()
		  .Padding(FMargin{0.0f, 0.0f, 0.0f, 5.0f})
		[
			SNew(SSearchBox)
			.HintText(FText::FromString(TEXT("Search Folders...")))
			.OnTextChanged(this, &SProjectCleanerTreeView::OnTreeViewSearchBoxTextChanged)
			.OnTextCommitted(this, &SProjectCleanerTreeView::OnTreeViewSearchBoxTextCommitted)
		]
		+ SVerticalBox::Slot()
		  .FillHeight(1.0f)
		  .Padding(FMargin{0.0f, 5.0f})
		[
			SNew(SScrollBox)
			.ScrollWhenFocusChanges(EScrollWhenFocusChanges::NoScroll)
			.AnimateWheelScrolling(true)
			.AllowOverscroll(EAllowOverscroll::No)
			+ SScrollBox::Slot()
			[
				TreeView.ToSharedRef()
			]
		]
		+ SVerticalBox::Slot()
		  .AutoHeight()
		  .HAlign(HAlign_Right)
		  .VAlign(VAlign_Center)
		  .Padding(FMargin{0.0f, 5.0f})
		[
			SNew(SComboButton)
			.ContentPadding(0)
			.ForegroundColor_Raw(this, &SProjectCleanerTreeView::GetTreeViewOptionsBtnForegroundColor)
			.ButtonStyle(FEditorStyle::Get(), "ToggleButton")
			.OnGetMenuContent(this, &SProjectCleanerTreeView::GetTreeViewOptionsBtnContent)
			.ButtonContent()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				  .AutoWidth()
				  .VAlign(VAlign_Center)
				[
					SNew(SImage).Image(FEditorStyle::GetBrush("GenericViewButton"))
				]
				+ SHorizontalBox::Slot()
				  .AutoWidth()
				  .Padding(2, 0, 0, 0)
				  .VAlign(VAlign_Center)
				[
					SNew(STextBlock).Text(FText::FromString(TEXT("View Options")))
				]
			]
		]
	];
}

void SProjectCleanerTreeView::TreeItemsUpdate()
{
	if (!TreeView.IsValid())
	{
		SAssignNew(TreeView, STreeView<TSharedPtr<FProjectCleanerTreeViewItem>>)
		.TreeItemsSource(&TreeItems)
		.SelectionMode(ESelectionMode::Multi)
		.OnGenerateRow(this, &SProjectCleanerTreeView::OnTreeViewGenerateRow)
		.OnGetChildren(this, &SProjectCleanerTreeView::OnTreeViewGetChildren)
		.OnContextMenuOpening_Raw(this, &SProjectCleanerTreeView::GetTreeItemContextMenu)
		.HeaderRow(GetTreeViewHeaderRow())
		.OnMouseButtonDoubleClick(this, &SProjectCleanerTreeView::OnTreeViewItemMouseDblClick)
		.OnSelectionChanged(this, &SProjectCleanerTreeView::OnTreeViewSelectionChange)
		.OnExpansionChanged(this, &SProjectCleanerTreeView::OnTreeViewExpansionChange);
	}

	if (!Scanner.IsValid()) return;
	// if (Scanner->GetScannerDataState() != EProjectCleanerScannerDataState::Actual) return;

	// caching expanded and selected items in order to keep them , when we updating data
	ItemsExpanded.Reset();
	ItemsSelected.Reset();
	TreeView->GetExpandedItems(ItemsExpanded);
	TreeView->GetSelectedItems(ItemsSelected);
	TreeView->ClearHighlightedItems();

	TreeItems.Reset();

	// creating root item
	const TSharedPtr<FProjectCleanerTreeViewItem> RootTreeItem = TreeItemCreate(FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir()));
	if (!RootTreeItem) return;

	// traversing and filling its child items
	TArray<TSharedPtr<FProjectCleanerTreeViewItem>> Stack;
	Stack.Push(RootTreeItem);
	TreeItems.Add(RootTreeItem);

	if (ItemsExpanded.Num() == 0)
	{
		TreeView->SetItemExpansion(RootTreeItem, true);
	}

	if (ItemsSelected.Num() == 0)
	{
		TreeView->SetItemSelection(RootTreeItem, true);
	}

	while (Stack.Num() > 0)
	{
		const auto CurrentItem = Stack.Pop();

		TSet<FString> SubFolders;
		Scanner.Get()->GetSubFolders(CurrentItem->FolderPathAbs, SubFolders);

		for (const auto& SubFolder : SubFolders)
		{
			const TSharedPtr<FProjectCleanerTreeViewItem> SubDirItem = TreeItemCreate(SubFolder);
			if (!SubDirItem.IsValid()) continue;

			CurrentItem->SubItems.Add(SubDirItem);
			Stack.Push(SubDirItem);
		}
	}

	TreeView->RequestTreeRefresh();
}

FProjectCleanerDelegatePathSelected& SProjectCleanerTreeView::OnPathSelected()
{
	return DelegatePathSelected;
}

FProjectCleanerDelegatePathExcluded& SProjectCleanerTreeView::OnPathExcluded()
{
	return DelegatePathExcluded;
}

FProjectCleanerDelegatePathIncluded& SProjectCleanerTreeView::OnPathIncluded()
{
	return DelegatePathIncluded;
}

FProjectCleanerDelegatePathCleaned& SProjectCleanerTreeView::OnPathCleaned()
{
	return DelegatePathCleaned;
}

TSharedPtr<FProjectCleanerTreeViewItem> SProjectCleanerTreeView::TreeItemCreate(const FString& InFolderPathAbs) const
{
	if (UProjectCleanerLibrary::PathIsUnderFolders(InFolderPathAbs, Scanner.Get()->GetFoldersBlacklist())) return {};

	const TSharedPtr<FProjectCleanerTreeViewItem> TreeItem = MakeShareable(new FProjectCleanerTreeViewItem());
	if (!TreeItem.IsValid()) return {};

	const bool bIsProjectContentFolder = InFolderPathAbs.Equals(FPaths::ProjectContentDir());
	const bool bIsProjectDeveloperFolder = InFolderPathAbs.Equals(FPaths::ProjectContentDir() / ProjectCleanerConstants::FolderDevelopers.ToString());

	TreeItem->FolderPathAbs = FPaths::ConvertRelativePathToFull(InFolderPathAbs);
	TreeItem->FolderPathRel = UProjectCleanerLibrary::PathConvertToRel(InFolderPathAbs);
	TreeItem->FolderName = bIsProjectContentFolder ? ProjectCleanerConstants::FolderContent.ToString() : FPaths::GetPathLeaf(InFolderPathAbs);
	TreeItem->FoldersTotal = Scanner.Get()->GetFoldersTotalNum(InFolderPathAbs);
	TreeItem->FoldersEmpty = Scanner.Get()->GetFoldersEmptyNum(InFolderPathAbs);
	TreeItem->AssetsTotal = Scanner.Get()->GetAssetTotalNum(InFolderPathAbs);
	TreeItem->AssetsUnused = Scanner.Get()->GetAssetUnusedNum(InFolderPathAbs);
	TreeItem->SizeTotal = Scanner.Get()->GetSizeTotal(InFolderPathAbs);
	TreeItem->SizeUnused = Scanner.Get()->GetSizeUnused(InFolderPathAbs);
	TreeItem->bDevFolder = bIsProjectDeveloperFolder;
	TreeItem->bEmpty = Scanner.Get()->IsFolderEmpty(InFolderPathAbs);
	TreeItem->bExcluded = Scanner.Get()->IsFolderExcluded(InFolderPathAbs);
	TreeItem->PercentUnused = TreeItem->AssetsTotal == 0 ? 0.0f : TreeItem->AssetsUnused * 100.0f / TreeItem->AssetsTotal;
	TreeItem->PercentUnusedNormalized = FMath::GetMappedRangeValueClamped(FVector2D{0.0f, 100.0f}, FVector2D{0.0f, 1.0f}, TreeItem->PercentUnused);
	TreeItem->bExpanded = bIsProjectContentFolder;

	for (const auto& ExpandedItem : ItemsExpanded)
	{
		if (ExpandedItem->FolderPathAbs.Equals(TreeItem->FolderPathAbs))
		{
			TreeItem->bExpanded = true;
			TreeView->SetItemExpansion(TreeItem, true);
			break;
		}
	}

	for (const auto& SelectedItem : ItemsSelected)
	{
		if (SelectedItem->FolderPathAbs.Equals(TreeItem->FolderPathAbs))
		{
			TreeView->SetItemSelection(TreeItem, true);
			TreeView->SetItemHighlighted(TreeItem, true);
			break;
		}
	}

	return TreeItem;
}

TSharedRef<SHeaderRow> SProjectCleanerTreeView::GetTreeViewHeaderRow() const
{
	return
		SNew(SHeaderRow)
		+ SHeaderRow::Column(TEXT("Name"))
		  .HAlignHeader(HAlign_Center)
		  .VAlignHeader(VAlign_Center)
		  .HeaderContentPadding(FMargin{5.0f})
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Name")))
			.ToolTipText(FText::FromString(TEXT("Folder Name")))
			.ColorAndOpacity(FProjectCleanerStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
			.Font(FProjectCleanerStyles::GetFont("Light", ProjectCleanerConstants::HeaderRowFontSize))
		]
		+ SHeaderRow::Column(TEXT("FoldersTotal"))
		  .HAlignHeader(HAlign_Center)
		  .VAlignHeader(VAlign_Center)
		  .HeaderContentPadding(FMargin{5.0f})
		[
			SNew(STextBlock)
			.ToolTipText(FText::FromString(TEXT("Total number of folders")))
			.Text(FText::FromString(TEXT("Folders (Total)")))
			.ColorAndOpacity(FProjectCleanerStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
			.Font(FProjectCleanerStyles::GetFont("Light", ProjectCleanerConstants::HeaderRowFontSize))
		]
		+ SHeaderRow::Column(TEXT("FoldersEmpty"))
		  .HAlignHeader(HAlign_Center)
		  .VAlignHeader(VAlign_Center)
		  .HeaderContentPadding(FMargin{5.0f})
		[
			SNew(STextBlock)
			.ToolTipText(FText::FromString(TEXT("Total number of empty folders")))
			.Text(FText::FromString(TEXT("Folders (Empty)")))
			.ColorAndOpacity(FProjectCleanerStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
			.Font(FProjectCleanerStyles::GetFont("Light", ProjectCleanerConstants::HeaderRowFontSize))
		]
		+ SHeaderRow::Column(TEXT("AssetsTotal"))
		  .HAlignHeader(HAlign_Center)
		  .VAlignHeader(VAlign_Center)
		  .HeaderContentPadding(FMargin{5.0f})
		[
			SNew(STextBlock)
			.ToolTipText(FText::FromString(TEXT("Total number of assets")))
			.Text(FText::FromString(TEXT("Assets (Total)")))
			.ColorAndOpacity(FProjectCleanerStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
			.Font(FProjectCleanerStyles::GetFont("Light", ProjectCleanerConstants::HeaderRowFontSize))
		]
		+ SHeaderRow::Column(TEXT("AssetsUnused"))
		  .HAlignHeader(HAlign_Center)
		  .VAlignHeader(VAlign_Center)
		  .HeaderContentPadding(FMargin{5.0f})
		[
			SNew(STextBlock)
			.ToolTipText(FText::FromString(TEXT("Total number of unused assets")))
			.Text(FText::FromString(TEXT("Assets (Unused)")))
			.ColorAndOpacity(FProjectCleanerStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
			.Font(FProjectCleanerStyles::GetFont("Light", ProjectCleanerConstants::HeaderRowFontSize))
		]
		+ SHeaderRow::Column(TEXT("SizeTotal"))
		  .HAlignHeader(HAlign_Center)
		  .VAlignHeader(VAlign_Center)
		  .HeaderContentPadding(FMargin{5.0f})
		[
			SNew(STextBlock)
			.ToolTipText(FText::FromString(TEXT("Total size of all assets")))
			.Text(FText::FromString(TEXT("Size (Total)")))
			.ColorAndOpacity(FProjectCleanerStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
			.Font(FProjectCleanerStyles::GetFont("Light", ProjectCleanerConstants::HeaderRowFontSize))
		]
		+ SHeaderRow::Column(TEXT("SizeUnused"))
		  .HAlignHeader(HAlign_Center)
		  .VAlignHeader(VAlign_Center)
		  .HeaderContentPadding(FMargin{5.0f})
		[
			SNew(STextBlock)
			.ToolTipText(FText::FromString(TEXT("Total size of unused assets")))
			.Text(FText::FromString(TEXT("Size (Unused)")))
			.ColorAndOpacity(FProjectCleanerStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
			.Font(FProjectCleanerStyles::GetFont("Light", ProjectCleanerConstants::HeaderRowFontSize))
		]
		+ SHeaderRow::Column(TEXT("Percent"))
		  .HAlignHeader(HAlign_Center)
		  .VAlignHeader(VAlign_Center)
		  .HeaderContentPadding(FMargin{5.0f})
		[
			SNew(STextBlock)
			.ToolTipText(FText::FromString(TEXT("Percent of unused number relative to total number of assets in folder")))
			.Text(FText::FromString(TEXT("% of Unused")))
			.ColorAndOpacity(FProjectCleanerStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
			.Font(FProjectCleanerStyles::GetFont("Light", ProjectCleanerConstants::HeaderRowFontSize))
		];
}

TSharedRef<SWidget> SProjectCleanerTreeView::GetTreeViewOptionsBtnContent() const
{
	const TSharedPtr<FExtender> Extender;
	FMenuBuilder MenuBuilder(true, nullptr, Extender, true);
	MenuBuilder.AddMenuSeparator(NAME_None);
	MenuBuilder.AddMenuEntry(
		FText::FromString(TEXT("Show Lines")),
		FText::FromString(TEXT("Show Lines in tree view")),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateLambda([&]
			{
				if (!ViewOptionsComboButton.IsValid()) return;

				ViewOptionsComboButton->SetEnabled(!ViewOptionsComboButton->IsEnabled());
				UE_LOG(LogProjectCleaner, Warning, TEXT("Show lines btn clicked"));
			}),
			FCanExecuteAction::CreateLambda([] { return true; })
			// FIsActionChecked::CreateSP(this, &SOutputLog::IsWordWrapEnabled)
		),
		NAME_None,
		EUserInterfaceActionType::ToggleButton
	);
	MenuBuilder.AddMenuSeparator(NAME_None);
	MenuBuilder.AddMenuEntry(
		FText::FromString(TEXT("Show Empty Folders")),
		FText::FromString(TEXT("Show Empty Folders in tree view")),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateLambda([&]
			{
				// if (!ViewOptionsComboButton.IsValid()) return;
				//
				// ViewOptionsComboButton->SetEnabled(!ViewOptionsComboButton->IsEnabled());
				UE_LOG(LogProjectCleaner, Warning, TEXT("Show empty folders btn clicked"));
				// This is a toggle, hence that it is inverted
				// SetWordWrapEnabled(IsWordWrapEnabled() ? ECheckBoxState::Unchecked : ECheckBoxState::Checked);
			}),
			FCanExecuteAction::CreateLambda([] { return true; })
			// FIsActionChecked::CreateSP(this, &SOutputLog::IsWordWrapEnabled)
		),
		NAME_None,
		EUserInterfaceActionType::ToggleButton
	);
	MenuBuilder.AddMenuEntry(
		FText::FromString(TEXT("Show Excluded Folders")),
		FText::FromString(TEXT("Show Excluded Folders in tree view")),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateLambda([&]
			{
				// if (!ViewOptionsComboButton.IsValid()) return;
				//
				// ViewOptionsComboButton->SetEnabled(!ViewOptionsComboButton->IsEnabled());
				UE_LOG(LogProjectCleaner, Warning, TEXT("Show Excluded folders btn clicked"));
				// This is a toggle, hence that it is inverted
				// SetWordWrapEnabled(IsWordWrapEnabled() ? ECheckBoxState::Unchecked : ECheckBoxState::Checked);
			}),
			FCanExecuteAction::CreateLambda([] { return true; })
			// FIsActionChecked::CreateSP(this, &SOutputLog::IsWordWrapEnabled)
		),
		NAME_None,
		EUserInterfaceActionType::ToggleButton
	);

	MenuBuilder.AddMenuEntry(
		FText::FromString(TEXT("Show Folders NonEngine")),
		FText::FromString(TEXT("Show Folders that contains non engine files in it")),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateLambda([&]
			{
				// if (!ViewOptionsComboButton.IsValid()) return;
				//
				// ViewOptionsComboButton->SetEnabled(!ViewOptionsComboButton->IsEnabled());
				UE_LOG(LogProjectCleaner, Warning, TEXT("Show NonEngine folders btn clicked"));
				// This is a toggle, hence that it is inverted
				// SetWordWrapEnabled(IsWordWrapEnabled() ? ECheckBoxState::Unchecked : ECheckBoxState::Checked);
			}),
			FCanExecuteAction::CreateLambda([] { return true; })
			// FIsActionChecked::CreateSP(this, &SOutputLog::IsWordWrapEnabled)
		),
		NAME_None,
		EUserInterfaceActionType::ToggleButton
	);

	MenuBuilder.AddMenuEntry(
		FText::FromString(TEXT("Show Folders Corrupted")),
		FText::FromString(TEXT("Show Folders that contains corrupted files in it")),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateLambda([&]
			{
				// if (!ViewOptionsComboButton.IsValid()) return;
				//
				// ViewOptionsComboButton->SetEnabled(!ViewOptionsComboButton->IsEnabled());
				UE_LOG(LogProjectCleaner, Warning, TEXT("Show Corrupted folders btn clicked"));
				// This is a toggle, hence that it is inverted
				// SetWordWrapEnabled(IsWordWrapEnabled() ? ECheckBoxState::Unchecked : ECheckBoxState::Checked);
			}),
			FCanExecuteAction::CreateLambda([] { return true; })
			// FIsActionChecked::CreateSP(this, &SOutputLog::IsWordWrapEnabled)
		),
		NAME_None,
		EUserInterfaceActionType::ToggleButton
	);

	return MenuBuilder.MakeWidget();
}

FSlateColor SProjectCleanerTreeView::GetTreeViewOptionsBtnForegroundColor() const
{
	static const FName InvertedForegroundName("InvertedForeground");
	static const FName DefaultForegroundName("DefaultForeground");

	if (!ViewOptionsComboButton.IsValid()) return FEditorStyle::GetSlateColor(DefaultForegroundName);

	return ViewOptionsComboButton->IsHovered() ? FEditorStyle::GetSlateColor(InvertedForegroundName) : FEditorStyle::GetSlateColor(DefaultForegroundName);
}

void SProjectCleanerTreeView::OnTreeViewSearchBoxTextChanged(const FText& InSearchText)
{
	UE_LOG(LogProjectCleaner, Warning, TEXT("Search Text Changed: %s"), *InSearchText.ToString());
}

void SProjectCleanerTreeView::OnTreeViewSearchBoxTextCommitted(const FText& InSearchText, ETextCommit::Type InCommitType)
{
	UE_LOG(LogProjectCleaner, Warning, TEXT("Search Text Committed: %s"), *InSearchText.ToString());
}

TSharedRef<ITableRow> SProjectCleanerTreeView::OnTreeViewGenerateRow(TSharedPtr<FProjectCleanerTreeViewItem> Item, const TSharedRef<STableViewBase>& OwnerTable) const
{
	return SNew(SProjectCleanerTreeViewItem, OwnerTable).TreeItem(Item);
}

TSharedPtr<SWidget> SProjectCleanerTreeView::GetTreeItemContextMenu() const
{
	FMenuBuilder MenuBuilder{true, Cmds};
	MenuBuilder.BeginSection(TEXT("Actions"), FText::FromString(TEXT("Path Actions")));
	{
		MenuBuilder.AddMenuEntry(FProjectCleanerCmds::Get().TabUnusedPathExclude);
		MenuBuilder.AddMenuEntry(FProjectCleanerCmds::Get().TabUnusedPathInclude);
		MenuBuilder.AddMenuEntry(FProjectCleanerCmds::Get().TabUnusedPathClean);
	}
	MenuBuilder.EndSection();

	return MenuBuilder.MakeWidget();
}

void SProjectCleanerTreeView::OnTreeViewItemMouseDblClick(TSharedPtr<FProjectCleanerTreeViewItem> Item)
{
	if (!Item.IsValid()) return;

	ToggleExpansionRecursive(Item, !Item->bExpanded);
}

void SProjectCleanerTreeView::OnTreeViewGetChildren(TSharedPtr<FProjectCleanerTreeViewItem> Item, TArray<TSharedPtr<FProjectCleanerTreeViewItem>>& OutChildren) const
{
	if (!Item.IsValid()) return;

	OutChildren.Append(Item->SubItems);
}

void SProjectCleanerTreeView::OnTreeViewSelectionChange(TSharedPtr<FProjectCleanerTreeViewItem> Item, ESelectInfo::Type SelectType)
{
	if (!Item.IsValid()) return;

	// todo:ashe23 optimize this, callback called multiple times, because of tree items update
	const auto SelectedItems = TreeView->GetSelectedItems();
	TSet<FString> SelectedPaths;
	SelectedPaths.Reserve(SelectedItems.Num());
	for (const auto& SelectedItem : SelectedItems)
	{
		SelectedPaths.Add(SelectedItem->FolderPathRel);
	}

	if (DelegatePathSelected.IsBound())
	{
		DelegatePathSelected.Broadcast(SelectedPaths);
	}
}

void SProjectCleanerTreeView::OnTreeViewExpansionChange(TSharedPtr<FProjectCleanerTreeViewItem> Item, bool bExpanded)
{
	if (!Item.IsValid()) return;
	if (!TreeView.IsValid()) return;

	Item->bExpanded = bExpanded;

	TreeView->SetItemExpansion(Item, bExpanded);
}

void SProjectCleanerTreeView::ToggleExpansionRecursive(TSharedPtr<FProjectCleanerTreeViewItem> Item, const bool bExpanded)
{
	if (!Item.IsValid()) return;
	if (!TreeView.IsValid()) return;
	if (!Scanner.IsValid()) return;

	TreeView->SetItemExpansion(Item, bExpanded);

	for (const auto& SubDir : Item->SubItems)
	{
		ToggleExpansionRecursive(SubDir, bExpanded);
	}
}
