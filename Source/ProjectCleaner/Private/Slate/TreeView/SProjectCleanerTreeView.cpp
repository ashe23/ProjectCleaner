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
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"

void SProjectCleanerTreeView::Construct(const FArguments& InArgs)
{
	Scanner = InArgs._Scanner;
	if (!Scanner.IsValid()) return;

	ScanSettings = GetMutableDefault<UProjectCleanerScanSettings>();
	if (!ScanSettings.IsValid()) return;

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
		.HeaderRow(GetTreeViewHeaderRow())
		.OnMouseButtonDoubleClick(this, &SProjectCleanerTreeView::OnTreeViewItemMouseDblClick)
		.OnSelectionChanged(this, &SProjectCleanerTreeView::OnTreeViewSelectionChange)
		.OnExpansionChanged(this, &SProjectCleanerTreeView::OnTreeViewExpansionChange);
	}

	if (!Scanner.IsValid()) return;
	if (Scanner->GetScannerDataState() != EProjectCleanerScannerDataState::Actual) return;

	ItemsExpanded.Reset();
	ItemsSelected.Reset();
	TreeView->GetExpandedItems(ItemsExpanded);
	TreeView->GetSelectedItems(ItemsSelected);

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
		TreeView->SetItemHighlighted(RootTreeItem, true);
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

FProjectCleanerDelegatePathChanged& SProjectCleanerTreeView::OnPathChange()
{
	return DelegatePathChanged;
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

void SProjectCleanerTreeView::OnTreeViewItemMouseDblClick(TSharedPtr<FProjectCleanerTreeViewItem> Item)
{
	if (!Item.IsValid()) return;

	ToggleExpansionRecursive(Item, !Item->bExpanded);

	// TreeItemsExpanded.Reset();
	// TreeView->GetExpandedItems(TreeItemsExpanded);
}

void SProjectCleanerTreeView::OnTreeViewGetChildren(TSharedPtr<FProjectCleanerTreeViewItem> Item, TArray<TSharedPtr<FProjectCleanerTreeViewItem>>& OutChildren) const
{
	if (!Item.IsValid()) return;

	OutChildren.Append(Item->SubItems);
}

void SProjectCleanerTreeView::OnTreeViewSelectionChange(TSharedPtr<FProjectCleanerTreeViewItem> Item, ESelectInfo::Type SelectType)
{
	if (!Item.IsValid()) return;

	// LastSelectedItem.Reset();
	// LastSelectedItem = Item;

	if (DelegatePathChanged.IsBound())
	{
		DelegatePathChanged.Broadcast(Item->FolderPathRel);
	}
}

void SProjectCleanerTreeView::OnTreeViewExpansionChange(TSharedPtr<FProjectCleanerTreeViewItem> Item, bool bExpanded)
{
	if (!Item.IsValid()) return;
	if (!TreeView.IsValid()) return;

	Item->bExpanded = bExpanded;

	TreeView->SetItemExpansion(Item, bExpanded);

	// TreeItemsExpanded.Reset();
	// TreeView->GetExpandedItems(TreeItemsExpanded);
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
