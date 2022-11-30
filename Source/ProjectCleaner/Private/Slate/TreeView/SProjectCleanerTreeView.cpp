// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/TreeView/SProjectCleanerTreeView.h"
#include "Slate/TreeView/SProjectCleanerTreeViewItem.h"
#include "ProjectCleaner.h"
#include "ProjectCleanerConstants.h"
#include "ProjectCleanerTypes.h"
#include "ProjectCleanerStyles.h"
// Engine Headers
#include "ProjectCleanerLibrary.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"

void SProjectCleanerTreeView::Construct(const FArguments& InArgs)
{
	if (InArgs._RootFolder.IsEmpty()) return;

	RootFolder = InArgs._RootFolder;
	ForbiddenFolders = InArgs._ForbiddenFolders;

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

	TreeView->RequestTreeRefresh();

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
	if (ForbiddenFolders.Contains(RootFolder))
	{
		UE_LOG(LogProjectCleaner, Warning, TEXT("Root folder cant be in forbidden list"));
		return;
	}
		
	TreeItems.Reset();

	// creating root item
	const TSharedPtr<FProjectCleanerTreeViewItem> RootTreeItem = MakeShareable(new FProjectCleanerTreeViewItem);
	if (!RootTreeItem) return;

	RootTreeItem->DirPathAbs = RootFolder;
	RootTreeItem->DirPathRel = UProjectCleanerLibrary::PathConvertToRel(RootFolder);
	RootTreeItem->DirName = RootFolder.Equals(FPaths::ProjectContentDir()) ? TEXT("Content") : FPaths::GetPathLeaf(RootFolder);
	RootTreeItem->FoldersTotal = UProjectCleanerLibrary::GetSubFoldersNum(RootFolder, true);
	RootTreeItem->FoldersEmpty = UProjectCleanerLibrary::GetEmptyFoldersNum(RootFolder);
	RootTreeItem->bDeveloperFolder = RootFolder.Equals(FPaths::ProjectContentDir() / TEXT("Developers"));
	RootTreeItem->bEmpty = UProjectCleanerLibrary::IsEmptyFolder(RootFolder);
	RootTreeItem->bExpanded = true;

	// traversing and filling its child items
	TArray<TSharedPtr<FProjectCleanerTreeViewItem>> Stack;
	Stack.Push(RootTreeItem);
	TreeItems.Add(RootTreeItem);

	while (Stack.Num() > 0)
	{
		const auto CurrentItem = Stack.Pop();

		TSet<FString> SubFolders;
		UProjectCleanerLibrary::GetSubFolders(CurrentItem->DirPathAbs, false, SubFolders);

		for (const auto& SubFolder : SubFolders)
		{
			if (ForbiddenFolders.Contains(SubFolder)) continue;
			
			const TSharedPtr<FProjectCleanerTreeViewItem> SubDirItem = MakeShareable(new FProjectCleanerTreeViewItem());
			if (!SubDirItem.IsValid()) continue;

			SubDirItem->DirPathAbs = SubFolder;
			SubDirItem->DirPathRel = UProjectCleanerLibrary::PathConvertToRel(SubFolder);
			SubDirItem->DirName = FPaths::GetPathLeaf(SubFolder);
			SubDirItem->FoldersTotal = UProjectCleanerLibrary::GetSubFoldersNum(SubFolder, true);
			SubDirItem->FoldersEmpty = UProjectCleanerLibrary::GetEmptyFoldersNum(SubFolder);
			SubDirItem->bDeveloperFolder = SubFolder.Equals(FPaths::ProjectContentDir() / TEXT("Developers"));
			SubDirItem->bEmpty = UProjectCleanerLibrary::IsEmptyFolder(SubFolder);

			CurrentItem->SubDirectories.Add(SubDirItem);
			Stack.Push(SubDirItem);
		}
	}

	TreeView->SetItemExpansion(RootTreeItem, true);
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
	return SNew(SProjectCleanerTreeViewItem, OwnerTable).ToolTipText(FText::FromString(Item->DirPathRel)).TreeItem(Item);
}

void SProjectCleanerTreeView::OnTreeViewItemMouseDblClick(TSharedPtr<FProjectCleanerTreeViewItem> Item) const
{
	if (!Item.IsValid()) return;

	ToggleExpansionRecursive(Item, !Item->bExpanded);
}

void SProjectCleanerTreeView::OnTreeViewGetChildren(TSharedPtr<FProjectCleanerTreeViewItem> Item, TArray<TSharedPtr<FProjectCleanerTreeViewItem>>& OutChildren) const
{
	OutChildren.Append(Item->SubDirectories);
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

	TreeView->SetItemExpansion(Item, bExpanded);

	for (const auto& SubDir : Item->SubDirectories)
	{
		ToggleExpansionRecursive(SubDir, bExpanded);
	}
}
