// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/ProjectCleanerTreeView.h"
#include "ProjectCleanerLibrary.h"
#include "ProjectCleanerScanSettings.h"
// Engine Headers
#include "ProjectCleaner.h"
#include "ProjectCleanerStyles.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Layout/SScrollBox.h"

void SProjectCleanerTreeView::Construct(const FArguments& InArgs)
{
	ScanSettings = GetMutableDefault<UProjectCleanerScanSettings>();
	check(ScanSettings.IsValid());

	ScanSettings->OnChange().AddLambda([&]()
	{
		UE_LOG(LogProjectCleaner, Warning, TEXT("Scan settings changed!"));
	});
	
	if (!TreeView.IsValid())
	{
		SAssignNew(TreeView, STreeView<TSharedPtr<FProjectCleanerTreeItem>>)
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
		SNew(SScrollBox)
		.ScrollWhenFocusChanges(EScrollWhenFocusChanges::NoScroll)
		.AnimateWheelScrolling(true)
		.AllowOverscroll(EAllowOverscroll::No)
		+ SScrollBox::Slot()
		[
			SNew(SVerticalBox)
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
			  .AutoHeight()
			  .Padding(FMargin{0.0f, 0.0f, 0.0f, 5.0f})
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
			  .Padding(FMargin{0.0f, 5.0f})
			[
				TreeView.ToSharedRef()
			]
		]
	];
}

void SProjectCleanerTreeView::TreeItemsUpdate()
{
	TreeItems.Reset();

	// TArray<FAssetData> Assets;
	//
	// const FString RootDir = FPaths::ProjectContentDir();
	// TSet<FString> ExcludeDirs;
	// ExcludeDirs.Add(FPaths::GameDevelopersDir()); // todo:ashe23 must be excluded by option
	// ExcludeDirs.Add(RootDir + TEXT("Collections/"));
	// // todo:ashe23 for ue5 exclude __ExternalActors__ and __ExternalObjects__ folders
	//
	//
	// TArray<TSharedPtr<FProjectCleanerTreeItem>> Stack;
	//
	// const TSharedPtr<FProjectCleanerTreeItem> RootTreeItem = MakeShareable(new FProjectCleanerTreeItem(RootDir, UProjectCleanerLibrary::PathConvertToRel(RootDir), TEXT("Content")));
	// if (!RootTreeItem.IsValid()) return;
	//
	// TSet<FString> AllSubDirs;
	// UProjectCleanerLibrary::GetSubDirectories(RootTreeItem->DirPathAbs, true, AllSubDirs, ExcludeDirs);
	//
	// TSet<FString> CachedEmptyFolders;
	// UProjectCleanerLibrary::GetEmptyDirectories(RootDir, CachedEmptyFolders, ExcludeDirs);
	// UProjectCleanerLibrary::GetAssetsInPath(UProjectCleanerLibrary::PathConvertToRel(RootDir), true, Assets);
	//
	// RootTreeItem->AssetsTotal = Assets.Num();
	// RootTreeItem->SizeTotal = UProjectCleanerLibrary::GetAssetsTotalSize(Assets);
	// RootTreeItem->FoldersTotal = AllSubDirs.Num();
	// RootTreeItem->FoldersEmpty = CachedEmptyFolders.Num();
	// RootTreeItem->bIsEmpty = CachedEmptyFolders.Contains(RootDir);
	//
	// Stack.Push(RootTreeItem);
	//
	// while (Stack.Num() > 0)
	// {
	// 	const auto CurrentItem = Stack.Pop();
	//
	// 	TSet<FString> SubDirs;
	// 	UProjectCleanerLibrary::GetSubDirectories(CurrentItem->DirPathAbs, false, SubDirs, ExcludeDirs);
	//
	// 	for (const auto& SubDir : SubDirs)
	// 	{
	// 		const TSharedPtr<FProjectCleanerTreeItem> SubDirItem = MakeShareable(
	// 			new FProjectCleanerTreeItem(
	// 				SubDir,
	// 				UProjectCleanerLibrary::PathConvertToRel(SubDir),
	// 				FPaths::GetPathLeaf(SubDir)
	// 			)
	// 		);
	// 		if (!SubDirItem.IsValid()) continue;
	//
	// 		// folders
	// 		UProjectCleanerLibrary::GetSubDirectories(SubDirItem->DirPathAbs, true, AllSubDirs, ExcludeDirs);
	// 		TSet<FString> EmptyFolders;
	// 		UProjectCleanerLibrary::GetEmptyDirectories(SubDirItem->DirPathAbs, EmptyFolders, ExcludeDirs);
	//
	// 		SubDirItem->FoldersTotal = AllSubDirs.Num();
	// 		SubDirItem->FoldersEmpty = EmptyFolders.Num();
	// 		SubDirItem->bIsEmpty = CachedEmptyFolders.Contains(SubDirItem->DirPathAbs);
	//
	// 		// assets
	// 		UProjectCleanerLibrary::GetAssetsInPath(SubDirItem->DirPathRel, true, Assets);
	// 		SubDirItem->AssetsTotal = Assets.Num();
	// 		SubDirItem->SizeTotal = UProjectCleanerLibrary::GetAssetsTotalSize(Assets);
	//
	// 		CurrentItem->SubDirectories.Add(SubDirItem);
	// 		Stack.Push(SubDirItem);
	// 	}
	// }
	//
	// TreeItems.Add(RootTreeItem);
	//
	// RootTreeItem->bExpanded = true;
	// TreeView->SetItemExpansion(RootTreeItem, true);
	// TreeView->RequestTreeRefresh();
}

TSharedRef<ITableRow> SProjectCleanerTreeView::OnTreeViewGenerateRow(TSharedPtr<FProjectCleanerTreeItem> Item, const TSharedRef<STableViewBase>& OwnerTable) const
{
	return SNew(SProjectCleanerTreeItem, OwnerTable).TreeItem(Item);
}

TSharedRef<SHeaderRow> SProjectCleanerTreeView::GetTreeViewHeaderRow() const
{
	constexpr uint32 HeaderRowFontSize = 12;

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
			.Font(FProjectCleanerStyles::GetFont("Light", HeaderRowFontSize))
		]
		+ SHeaderRow::Column(TEXT("FoldersTotal"))
		  .HAlignHeader(HAlign_Center)
		  .VAlignHeader(VAlign_Center)
		  .HeaderContentPadding(FMargin{5.0f})
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Folders (Total)")))
			.ColorAndOpacity(FProjectCleanerStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
			.Font(FProjectCleanerStyles::GetFont("Light", HeaderRowFontSize))
		]
		+ SHeaderRow::Column(TEXT("FoldersEmpty"))
		  .HAlignHeader(HAlign_Center)
		  .VAlignHeader(VAlign_Center)
		  .HeaderContentPadding(FMargin{5.0f})
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Folders (Empty)")))
			.ColorAndOpacity(FProjectCleanerStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
			.Font(FProjectCleanerStyles::GetFont("Light", HeaderRowFontSize))
		]
		+ SHeaderRow::Column(TEXT("AssetsTotal"))
		  .HAlignHeader(HAlign_Center)
		  .VAlignHeader(VAlign_Center)
		  .HeaderContentPadding(FMargin{5.0f})
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Assets (Total)")))
			.ColorAndOpacity(FProjectCleanerStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
			.Font(FProjectCleanerStyles::GetFont("Light", HeaderRowFontSize))
		]
		+ SHeaderRow::Column(TEXT("AssetsUnused"))
		  .HAlignHeader(HAlign_Center)
		  .VAlignHeader(VAlign_Center)
		  .HeaderContentPadding(FMargin{5.0f})
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Assets (Unused)")))
			.ColorAndOpacity(FProjectCleanerStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
			.Font(FProjectCleanerStyles::GetFont("Light", HeaderRowFontSize))
		]
		+ SHeaderRow::Column(TEXT("SizeTotal"))
		  .HAlignHeader(HAlign_Center)
		  .VAlignHeader(VAlign_Center)
		  .HeaderContentPadding(FMargin{5.0f})
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Size (Total)")))
			.ColorAndOpacity(FProjectCleanerStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
			.Font(FProjectCleanerStyles::GetFont("Light", HeaderRowFontSize))
		]
		+ SHeaderRow::Column(TEXT("SizeUnused"))
		  .HAlignHeader(HAlign_Center)
		  .VAlignHeader(VAlign_Center)
		  .HeaderContentPadding(FMargin{5.0f})
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Size (Unused)")))
			.ColorAndOpacity(FProjectCleanerStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
			.Font(FProjectCleanerStyles::GetFont("Light", HeaderRowFontSize))
		];
}

void SProjectCleanerTreeView::OnTreeViewItemMouseDblClick(TSharedPtr<FProjectCleanerTreeItem> Item) const
{
	if (!Item.IsValid()) return;

	ToggleExpansionRecursive(Item, !Item->bExpanded);
}

void SProjectCleanerTreeView::OnTreeViewGetChildren(TSharedPtr<FProjectCleanerTreeItem> Item, TArray<TSharedPtr<FProjectCleanerTreeItem>>& OutChildren) const
{
	OutChildren.Append(Item->SubDirectories);
}

void SProjectCleanerTreeView::OnTreeViewSelectionChange(TSharedPtr<FProjectCleanerTreeItem> Item, ESelectInfo::Type SelectType) const
{
	if (!Item.IsValid()) return;

	UE_LOG(LogProjectCleaner, Warning, TEXT("Selection Change"));
}

void SProjectCleanerTreeView::OnTreeViewExpansionChange(TSharedPtr<FProjectCleanerTreeItem> Item, bool bExpanded) const
{
	if (!Item.IsValid()) return;
	if (!TreeView.IsValid()) return;

	Item->bExpanded = bExpanded;

	TreeView->SetItemExpansion(Item, bExpanded);
}

void SProjectCleanerTreeView::OnTreeViewSearchBoxTextChanged(const FText& InSearchText)
{
	UE_LOG(LogProjectCleaner, Warning, TEXT("Search Text Changed: %s"), *InSearchText.ToString());
}

void SProjectCleanerTreeView::OnTreeViewSearchBoxTextCommitted(const FText& InSearchText, ETextCommit::Type InCommitType)
{
	UE_LOG(LogProjectCleaner, Warning, TEXT("Search Text Committed: %s"), *InSearchText.ToString());
}

void SProjectCleanerTreeView::ToggleExpansionRecursive(TSharedPtr<FProjectCleanerTreeItem> Item, const bool bExpanded) const
{
	if (!Item.IsValid()) return;
	if (!TreeView.IsValid()) return;

	TreeView->SetItemExpansion(Item, bExpanded);

	for (const auto& SubDir : Item->SubDirectories)
	{
		ToggleExpansionRecursive(SubDir, bExpanded);
	}
}
