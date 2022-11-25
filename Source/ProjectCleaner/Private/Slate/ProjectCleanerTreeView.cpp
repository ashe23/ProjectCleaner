// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/ProjectCleanerTreeView.h"
#include "ProjectCleanerLibrary.h"
#include "ProjectCleaner/Public/ProjectCleanerLibrary.h"
// Engine Headers
#include "ProjectCleanerStyles.h"
#include "Widgets/Layout/SScrollBox.h"

void SProjectCleanerTreeView::Construct(const FArguments& InArgs)
{
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
			[
				SNew(STreeView<TSharedPtr<FProjectCleanerTreeItem>>)
				.TreeItemsSource(&TreeItems)
				.OnGenerateRow(this, &SProjectCleanerTreeView::OnTreeViewGenerateRow)
				.OnGetChildren(this, &SProjectCleanerTreeView::OnTreeViewGetChildren)
				.HeaderRow(GetTreeViewHeaderRow())
			]
		]
	];
}

void SProjectCleanerTreeView::TreeItemsUpdate()
{
	TreeItems.Reset();

	const FString RootDir = FPaths::ProjectContentDir();

	TSet<FString> ExcludeDirs;
	ExcludeDirs.Add(FPaths::GameDevelopersDir()); // todo:ashe23 must be excluded by option
	ExcludeDirs.Add(RootDir + TEXT("Collections/"));
	
	// todo:ashe23 for ue5 exclude __ExternalActors__ and __ExternalObjects__ folders

	TArray<TSharedPtr<FProjectCleanerTreeItem>> Stack;

	const TSharedPtr<FProjectCleanerTreeItem> RootTreeItem = MakeShareable(new FProjectCleanerTreeItem(RootDir, TEXT("/Game"), TEXT("Content")));
	if (!RootTreeItem.IsValid()) return;
	
	Stack.Push(RootTreeItem);

	while (Stack.Num() > 0)
	{
		const auto CurrentItem = Stack.Pop();

		TSet<FString> SubDirs;
		UProjectCleanerLibrary::GetSubDirectories(CurrentItem->DirPathAbs, SubDirs, ExcludeDirs);

		for (const auto& SubDir : SubDirs)
		{
			const TSharedPtr<FProjectCleanerTreeItem> SubDirItem = MakeShareable(new FProjectCleanerTreeItem(SubDir, TEXT(""), FPaths::GetPathLeaf(SubDir)));
			if (!SubDirItem.IsValid()) continue;
			
			CurrentItem->SubDirectories.Add(SubDirItem);
			Stack.Push(SubDirItem);
		}
	}
	
	TreeItems.Add(RootTreeItem);

	if (TreeView.IsValid())
	{
		TreeView->RequestTreeRefresh();
	}
}

TSharedRef<ITableRow> SProjectCleanerTreeView::OnTreeViewGenerateRow(TSharedPtr<FProjectCleanerTreeItem> Item, const TSharedRef<STableViewBase>& OwnerTable) const
{
	return SNew(SProjectCleanerTreeItem, OwnerTable).TreeItem(Item);
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
			.Font(FProjectCleanerStyles::Get().GetFontStyle("ProjectCleaner.Font.Light15"))
		]
		+ SHeaderRow::Column(TEXT("FoldersTotal"))
		.HAlignHeader(HAlign_Center)
		.VAlignHeader(VAlign_Center)
		.HeaderContentPadding(FMargin{5.0f})
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Folders")))
			.Font(FProjectCleanerStyles::Get().GetFontStyle("ProjectCleaner.Font.Light15"))
		];
}

void SProjectCleanerTreeView::OnTreeViewGetChildren(TSharedPtr<FProjectCleanerTreeItem> Item, TArray<TSharedPtr<FProjectCleanerTreeItem>>& OutChildren)
{
	OutChildren.Append(Item->SubDirectories);
}
