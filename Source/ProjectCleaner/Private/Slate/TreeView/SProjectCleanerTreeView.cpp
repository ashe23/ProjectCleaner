// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/TreeView/SProjectCleanerTreeView.h"
#include "Slate/TreeView/SProjectCleanerTreeViewItem.h"
#include "ProjectCleaner.h"
#include "ProjectCleanerConstants.h"
#include "ProjectCleanerTypes.h"
#include "ProjectCleanerStyles.h"
#include "ProjectCleanerScanSettings.h"
#include "ProjectCleanerScanner.h"
// Engine Headers
#include "ProjectCleanerLibrary.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"

void SProjectCleanerTreeView::Construct(const FArguments& InArgs)
{
	Scanner = InArgs._Scanner;
	if (!Scanner.IsValid()) return;
	
	ScanSettings = GetMutableDefault<UProjectCleanerScanSettings>();
	if (!ScanSettings.IsValid()) return;

	ScanSettings->OnChange().AddLambda([&]()
	{
		TreeItemsUpdate();
	});
	
	if (!TreeView.IsValid())
	{
		SAssignNew(TreeView, STreeView<TSharedPtr<FProjectCleanerTreeViewItem>>)
		.TreeItemsSource(&TreeItems)
		.SelectionMode(ESelectionMode::SingleToggle)
		.OnGenerateRow(this, &SProjectCleanerTreeView::OnTreeViewGenerateRow)
		.OnGetChildren(this, &SProjectCleanerTreeView::OnTreeViewGetChildren)
		.HeaderRow(GetTreeViewHeaderRow())
		.OnMouseButtonDoubleClick(this, &SProjectCleanerTreeView::OnTreeViewItemMouseDblClick)
		.OnSelectionChanged(this, &SProjectCleanerTreeView::OnTreeViewSelectionChange)
		.OnExpansionChanged(this, &SProjectCleanerTreeView::OnTreeViewExpansionChange);
	}
	
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
	// if (!Scanner.IsValid()) return;
	//
	// TreeItems.Reset();
	//
	// // creating root item
	// const TSharedPtr<FProjectCleanerTreeViewItem> RootTreeItem = TreeItemCreate(FPaths::ProjectContentDir());
	// if (!RootTreeItem) return;
	//
	// // traversing and filling its child items
	// TArray<TSharedPtr<FProjectCleanerTreeViewItem>> Stack;
	// Stack.Push(RootTreeItem);
	// TreeItems.Add(RootTreeItem);
	//
	// while (Stack.Num() > 0)
	// {
	// 	const auto CurrentItem = Stack.Pop();
	//
	// 	TSet<FString> SubFolders;
	// 	Scanner.Get()->GetSubFolders(CurrentItem->FolderPathAbs, SubFolders);
	//
	// 	for (const auto& SubFolder : SubFolders)
	// 	{
	// 		const TSharedPtr<FProjectCleanerTreeViewItem> SubDirItem = TreeItemCreate(SubFolder);
	// 		if (!SubDirItem.IsValid()) continue;
	//
	// 		CurrentItem->SubItems.Add(SubDirItem);
	// 		Stack.Push(SubDirItem);
	// 	}
	// }
	//
	// if (TreeView.IsValid())
	// {
	// 	TreeView->SetItemExpansion(RootTreeItem, true);
	// 	TreeView->RequestTreeRefresh();
	// }
}

TSharedPtr<FProjectCleanerTreeViewItem> SProjectCleanerTreeView::TreeItemCreate(const FString& InFolderPathAbs) const
{
	return {};
	// if (UProjectCleanerLibrary::IsUnderAnyFolder(InFolderPathAbs, Scanner.Get()->GetForbiddenFoldersToScan())) return {};
	//
	// const TSharedPtr<FProjectCleanerTreeViewItem> TreeItem = MakeShareable(new FProjectCleanerTreeViewItem());
	// if (!TreeItem.IsValid()) return {};
	//
	// const bool bIsProjectContentFolder = InFolderPathAbs.Equals(FPaths::ProjectContentDir());
	// const bool bIsProjectDeveloperFolder = InFolderPathAbs.Equals(FPaths::ProjectContentDir() / TEXT("Developers"));
	//
	// TreeItem->FolderPathAbs = InFolderPathAbs;
	// TreeItem->FolderPathRel = UProjectCleanerLibrary::PathConvertToRel(InFolderPathAbs);
	// TreeItem->FolderName = bIsProjectContentFolder ? TEXT("Content") : FPaths::GetPathLeaf(InFolderPathAbs);
	// TreeItem->FoldersTotal = Scanner.Get()->GetFoldersTotalNum(InFolderPathAbs);
	// TreeItem->FoldersEmpty = Scanner.Get()->GetFoldersEmptyNum(InFolderPathAbs);
	// TreeItem->AssetsTotal = Scanner.Get()->GetAssetsUnusedNum(InFolderPathAbs);
	// TreeItem->AssetsUnused = Scanner.Get()->GetAssetsUnusedNum(InFolderPathAbs);
	// TreeItem->SizeTotal = Scanner.Get()->GetSizeTotal(InFolderPathAbs);
	// TreeItem->SizeUnused = Scanner.Get()->GetSizeUnused(InFolderPathAbs);
	// TreeItem->bDevFolder = bIsProjectDeveloperFolder;
	// TreeItem->bEmpty = Scanner.Get()->IsEmptyFolder(InFolderPathAbs);
	// TreeItem->bExpanded = bIsProjectContentFolder;
	// TreeItem->bExcluded = Scanner.Get()->IsExcludedFolder(InFolderPathAbs);;
	//
	// return TreeItem;
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
			.ColorAndOpacity(FProjectCleanerStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
			.Font(FProjectCleanerStyles::GetFont("Light", ProjectCleanerConstants::HeaderRowFontSize))
		]
		+ SHeaderRow::Column(TEXT("FoldersTotal"))
		  .HAlignHeader(HAlign_Center)
		  .VAlignHeader(VAlign_Center)
		  .HeaderContentPadding(FMargin{5.0f})
		[
			SNew(STextBlock)
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
			.Text(FText::FromString(TEXT("Size (Unused)")))
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
	return SNew(SProjectCleanerTreeViewItem, OwnerTable).ToolTipText(FText::FromString(Item->FolderPathRel)).TreeItem(Item);
}

void SProjectCleanerTreeView::OnTreeViewItemMouseDblClick(TSharedPtr<FProjectCleanerTreeViewItem> Item) const
{
	if (!Item.IsValid()) return;

	ToggleExpansionRecursive(Item, !Item->bExpanded);
}

void SProjectCleanerTreeView::OnTreeViewGetChildren(TSharedPtr<FProjectCleanerTreeViewItem> Item, TArray<TSharedPtr<FProjectCleanerTreeViewItem>>& OutChildren) const
{
	if (!Item.IsValid()) return;

	OutChildren.Append(Item->SubItems);
}

void SProjectCleanerTreeView::OnTreeViewSelectionChange(TSharedPtr<FProjectCleanerTreeViewItem> Item, ESelectInfo::Type SelectType) const
{
	if (!Item.IsValid()) return;

	UE_LOG(LogProjectCleaner, Warning, TEXT("Selection Change"));
}

void SProjectCleanerTreeView::OnTreeViewExpansionChange(TSharedPtr<FProjectCleanerTreeViewItem> Item, bool bExpanded) const
{
	if (!Item.IsValid()) return;
	if (!TreeView.IsValid()) return;

	Item->bExpanded = bExpanded;

	TreeView->SetItemExpansion(Item, bExpanded);
}

void SProjectCleanerTreeView::ToggleExpansionRecursive(TSharedPtr<FProjectCleanerTreeViewItem> Item, const bool bExpanded) const
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
