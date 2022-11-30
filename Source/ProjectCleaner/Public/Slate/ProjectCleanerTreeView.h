// // Copyright Ashot Barkhudaryan. All Rights Reserved.
//
// #pragma once
//
// #include "CoreMinimal.h"
// #include "ProjectCleanerScanner.h"
// #include "ProjectCleanerStyles.h"
// #include "Widgets/SCompoundWidget.h"
//
// class UProjectCleanerScanSettings;
//
// struct FProjectCleanerTreeItem
// {
// 	bool operator==(const FProjectCleanerTreeItem& Other) const
// 	{
// 		return DirPathAbs.Equals(Other.DirPathAbs);
// 	}
//
// 	bool operator!=(const FProjectCleanerTreeItem& Other) const
// 	{
// 		return !DirPathAbs.Equals(Other.DirPathAbs);
// 	}
//
// 	FString DirPathAbs;
// 	FString DirPathRel;
// 	FString DirName;
// 	int64 SizeTotal = 0;
// 	int64 SizeUnused = 0;
// 	int32 AssetsTotal = 0;
// 	int32 AssetsUnused = 0;
// 	int32 FoldersTotal = 0;
// 	int32 FoldersEmpty = 0;
// 	bool bExpanded = false;
// 	bool bIsEmpty = false;
// 	TArray<TSharedPtr<FProjectCleanerTreeItem>> SubDirectories;
// };
//
// class SProjectCleanerTreeItem final : public SMultiColumnTableRow<TSharedPtr<FProjectCleanerTreeItem>>
// {
// public:
// 	SLATE_BEGIN_ARGS(SProjectCleanerTreeItem)
// 		{
// 		}
//
// 		SLATE_ARGUMENT(TSharedPtr<FProjectCleanerTreeItem>, TreeItem)
// 	SLATE_END_ARGS()
//
// 	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& OwnerTable)
// 	{
// 		TreeItem = InArgs._TreeItem;
//
// 		SMultiColumnTableRow<TSharedPtr<FProjectCleanerTreeItem>>::Construct(
// 			SMultiColumnTableRow<TSharedPtr<FProjectCleanerTreeItem>>::FArguments(),
// 			OwnerTable
// 		);
// 	}
//
// 	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& InColumnName) override
// 	{
// 		if (InColumnName.IsEqual(TEXT("Name")))
// 		{
// 			const FString FolderIcon = TreeItem->bExpanded ? TEXT("ContentBrowser.AssetTreeFolderOpen") : TEXT("ContentBrowser.AssetTreeFolderClosed");
//
// 			return
// 				SNew(SHorizontalBox)
// 				+ SHorizontalBox::Slot()
// 				  .AutoWidth()
// 				  .Padding(FMargin{2.0f})
// 				[
// 					SNew(SExpanderArrow, SharedThis(this))
// 					.IndentAmount(20)
// 					.ShouldDrawWires(true)
// 				]
// 				+ SHorizontalBox::Slot()
// 				  .AutoWidth()
// 				  .Padding(0, 0, 2, 0)
// 				  .VAlign(VAlign_Center)
// 				[
//
// 					// Folder Icon
// 					SNew(SImage)
// 					.Image(this, &SProjectCleanerTreeItem::GetFolderIcon)
// 					.ColorAndOpacity(this, &SProjectCleanerTreeItem::GetFolderColor)
// 				]
// 				+ SHorizontalBox::Slot()
// 				  .AutoWidth()
// 				  .Padding(FMargin{5.0f})
// 				[
// 					SNew(STextBlock).Text(FText::FromString(TreeItem->DirName))
// 				];
// 		}
//
// 		if (InColumnName.IsEqual(TEXT("FoldersTotal")))
// 		{
// 			return
// 				SNew(SHorizontalBox)
// 				+ SHorizontalBox::Slot()
// 				  .FillWidth(1.0f)
// 				  .HAlign(HAlign_Center)
// 				  .VAlign(VAlign_Center)
// 				[
// 					SNew(STextBlock)
// 					.Justification(ETextJustify::Center)
// 					.Text(FText::FromString(FString::Printf(TEXT("%d"), TreeItem->FoldersTotal)))
// 				];
// 		}
//
// 		if (InColumnName.IsEqual(TEXT("FoldersEmpty")))
// 		{
// 			return
// 				SNew(SHorizontalBox)
// 				+ SHorizontalBox::Slot()
// 				  .FillWidth(1.0f)
// 				  .HAlign(HAlign_Center)
// 				  .VAlign(VAlign_Center)
// 				[
// 					SNew(STextBlock)
// 					.Justification(ETextJustify::Center)
// 					.Text(FText::FromString(FString::Printf(TEXT("%d"), TreeItem->FoldersEmpty)))
// 				];
// 		}
//
// 		if (InColumnName.IsEqual(TEXT("AssetsTotal")))
// 		{
// 			return
// 				SNew(SHorizontalBox)
// 				+ SHorizontalBox::Slot()
// 				  .FillWidth(1.0f)
// 				  .HAlign(HAlign_Center)
// 				  .VAlign(VAlign_Center)
// 				[
// 					SNew(STextBlock)
// 					.Justification(ETextJustify::Center)
// 					.Text(FText::FromString(FString::Printf(TEXT("%d"), TreeItem->AssetsTotal)))
// 				];
// 		}
//
// 		if (InColumnName.IsEqual(TEXT("AssetsUnused")))
// 		{
// 			return
// 				SNew(SHorizontalBox)
// 				+ SHorizontalBox::Slot()
// 				  .FillWidth(1.0f)
// 				  .HAlign(HAlign_Center)
// 				  .VAlign(VAlign_Center)
// 				[
// 					SNew(STextBlock)
// 					.Justification(ETextJustify::Center)
// 					.Text(FText::FromString(FString::Printf(TEXT("%d"), TreeItem->AssetsUnused)))
// 				];
// 		}
//
// 		if (InColumnName.IsEqual(TEXT("SizeTotal")))
// 		{
// 			return
// 				SNew(SHorizontalBox)
// 				+ SHorizontalBox::Slot()
// 				  .FillWidth(1.0f)
// 				  .HAlign(HAlign_Center)
// 				  .VAlign(VAlign_Center)
// 				[
// 					SNew(STextBlock)
// 					.Justification(ETextJustify::Center)
// 					.Text(FText::AsMemory(TreeItem->SizeTotal))
// 				];
// 		}
//
// 		if (InColumnName.IsEqual(TEXT("SizeUnused")))
// 		{
// 			return
// 				SNew(SHorizontalBox)
// 				+ SHorizontalBox::Slot()
// 				  .FillWidth(1.0f)
// 				  .HAlign(HAlign_Center)
// 				  .VAlign(VAlign_Center)
// 				[
// 					SNew(STextBlock)
// 					.Justification(ETextJustify::Center)
// 					.Text(FText::AsMemory(TreeItem->SizeUnused))
// 				];
// 		}
//
// 		return SNew(STextBlock).Text(FText::FromString(TEXT("")));
// 	}
//
// private:
// 	const FSlateBrush* GetFolderIcon() const
// 	{
// 		// todo:ashe23 handle developer folder icon separate
// 		return FEditorStyle::GetBrush(TreeItem->bExpanded ? TEXT("ContentBrowser.AssetTreeFolderOpen") : TEXT("ContentBrowser.AssetTreeFolderClosed"));
// 	}
//
// 	FSlateColor GetFolderColor() const
// 	{
// 		return TreeItem->bIsEmpty ? FProjectCleanerStyles::Get().GetSlateColor("ProjectCleaner.Color.Red") : FLinearColor::Gray;
// 	}
//
// 	TSharedPtr<FProjectCleanerTreeItem> TreeItem;
// };
//
//
// // Responsible for showing project folder structure as tree view with additional information about every folder and its content
// class SProjectCleanerTreeView final : public SCompoundWidget
// {
// public:
// 	SLATE_BEGIN_ARGS(SProjectCleanerTreeView)
// 		{
// 		}
//
// 		SLATE_ARGUMENT(TSharedPtr<FProjectCleanerScanner>, Scanner)
// 	SLATE_END_ARGS()
//
// 	void Construct(const FArguments& InArgs);
// private:
// 	void TreeItemsUpdate();
//
// 	TSharedRef<ITableRow> OnTreeViewGenerateRow(TSharedPtr<FProjectCleanerTreeItem> Item, const TSharedRef<STableViewBase>& OwnerTable) const;
// 	TSharedRef<SHeaderRow> GetTreeViewHeaderRow() const;
// 	void OnTreeViewItemMouseDblClick(TSharedPtr<FProjectCleanerTreeItem> Item) const;
// 	void OnTreeViewGetChildren(TSharedPtr<FProjectCleanerTreeItem> Item, TArray<TSharedPtr<FProjectCleanerTreeItem>>& OutChildren) const;
// 	void OnTreeViewSelectionChange(TSharedPtr<FProjectCleanerTreeItem> Item, ESelectInfo::Type SelectType) const;
// 	void OnTreeViewExpansionChange(TSharedPtr<FProjectCleanerTreeItem> Item, bool bExpanded) const;
// 	void OnTreeViewSearchBoxTextChanged(const FText& InSearchText);
// 	void OnTreeViewSearchBoxTextCommitted(const FText& InSearchText, ETextCommit::Type InCommitType);
// 	void ToggleExpansionRecursive(TSharedPtr<FProjectCleanerTreeItem> Item, const bool bExpanded) const;
//
//
// 	TWeakObjectPtr<UProjectCleanerScanSettings> ScanSettings;
// 	TArray<TSharedPtr<FProjectCleanerTreeItem>> TreeItems;
// 	TSharedPtr<STreeView<TSharedPtr<FProjectCleanerTreeItem>>> TreeView;
// 	TSharedPtr<FProjectCleanerScanner> Scanner;
// };
