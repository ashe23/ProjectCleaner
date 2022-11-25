// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

struct FProjectCleanerTreeItem
{
	FProjectCleanerTreeItem(const FString& PathAbs, const FString& PathRel, const FString& Name) :
		DirPathAbs(PathAbs),
		DirPathRel(PathRel),
		DirName(Name)
	{
	}

	bool operator==(const FProjectCleanerTreeItem& Other) const
	{
		return DirPathAbs.Equals(Other.DirPathAbs);
	}

	bool operator!=(const FProjectCleanerTreeItem& Other) const
	{
		return !DirPathAbs.Equals(Other.DirPathAbs);
	}

	FString DirPathAbs;
	FString DirPathRel;
	FString DirName;
	int64 SizeTotal = 0;
	int64 SizeUnused = 0;
	int32 NumTotal = 0;
	int32 NumUnused = 0;
	int32 FoldersTotal = 0;
	int32 FoldersEmpty = 0;
	TArray<TSharedPtr<FProjectCleanerTreeItem>> SubDirectories;
};

class SProjectCleanerTreeItem : public SMultiColumnTableRow<TSharedPtr<FProjectCleanerTreeItem>>
{
public:
	SLATE_BEGIN_ARGS(SProjectCleanerTreeItem)
		{
		}

		SLATE_ARGUMENT(TSharedPtr<FProjectCleanerTreeItem>, TreeItem)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& OwnerTable)
	{
		TreeItem = InArgs._TreeItem;

		SMultiColumnTableRow<TSharedPtr<FProjectCleanerTreeItem>>::Construct(
			SMultiColumnTableRow<TSharedPtr<FProjectCleanerTreeItem>>::FArguments(),
			OwnerTable
		);
	}

	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& InColumnName) override
	{
		if (InColumnName.IsEqual(TEXT("Name")))
		{
			return
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				  .AutoWidth()
				  .Padding(FMargin{5.0f})
				[
					SNew(SExpanderArrow, SharedThis(this))
					.IndentAmount(20)
					.ShouldDrawWires(true)
				]
				+ SHorizontalBox::Slot()
				  .AutoWidth()
				  .Padding(0, 0, 2, 0)
				  .VAlign(VAlign_Center)
				[
					// Folder Icon
					SNew(SImage)
					.Image(FEditorStyle::GetBrush("ContentBrowser.AssetTreeFolderOpen"))
					.ColorAndOpacity(FLinearColor::Gray)
				]
				+ SHorizontalBox::Slot()
				  .AutoWidth()
				  .Padding(FMargin{5.0f})
				[
					SNew(STextBlock).Text(FText::FromString(TreeItem->DirName))
				];
		}

		if (InColumnName.IsEqual(TEXT("FoldersTotal")))
		{
			return
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				  .FillWidth(1.0f)
				  .HAlign(HAlign_Center)
				  .VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Justification(ETextJustify::Center)
					.Text(FText::FromString(FString::Printf(TEXT("%d"), TreeItem->SubDirectories.Num())))
				];
		}

		return SNew(STextBlock).Text(FText::FromString(TEXT("")));
	}

private:
	TSharedPtr<FProjectCleanerTreeItem> TreeItem;
};


// Responsible for showing project folder structure as tree view with additional information about every folder and its content
class SProjectCleanerTreeView final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SProjectCleanerTreeView)
		{
		}

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
private:
	void TreeItemsUpdate();

	TSharedRef<ITableRow> OnTreeViewGenerateRow(TSharedPtr<FProjectCleanerTreeItem> Item, const TSharedRef<STableViewBase>& OwnerTable) const;
	TSharedRef<SHeaderRow> GetTreeViewHeaderRow() const;
	void OnTreeViewGetChildren(TSharedPtr<FProjectCleanerTreeItem> Item, TArray<TSharedPtr<FProjectCleanerTreeItem>>& OutChildren);

	TArray<TSharedPtr<FProjectCleanerTreeItem>> TreeItems;
	TSharedPtr<STreeView<TSharedPtr<FProjectCleanerTreeItem>>> TreeView;
};
