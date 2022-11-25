// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectCleanerTypes.h"
#include "Widgets/SCompoundWidget.h"

class UProjectCleanerStatTreeItem;

class SProjectCleanerStatTreeItem : public SMultiColumnTableRow<TWeakObjectPtr<UProjectCleanerStatTreeItem>>
{
public:
	SLATE_BEGIN_ARGS(SProjectCleanerStatTreeItem)
		{
		}

		SLATE_ARGUMENT(TWeakObjectPtr<UProjectCleanerStatTreeItem>, ListItem)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView)
	{
		ListItem = InArgs._ListItem;

		SMultiColumnTableRow<TWeakObjectPtr<UProjectCleanerStatTreeItem>>::Construct(
			SMultiColumnTableRow<TWeakObjectPtr<UProjectCleanerStatTreeItem>>::FArguments(),
			InOwnerTableView
		);
	}

	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& InColumnName) override
	{
		if (InColumnName.IsEqual(TEXT("Path")))
		{
			return
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SExpanderArrow, SharedThis(this))
					.IndentAmount(20)
					.ShouldDrawWires(true)
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(STextBlock).Text(FText::FromString(ListItem->Path))
				];
		}

		if (InColumnName.IsEqual(TEXT("Size")))
		{
			return SNew(STextBlock).Text(FText::FromString(ListItem->Size));
		}

		return SNew(STextBlock).Text(FText::FromString(""));
	}

private:
	TWeakObjectPtr<UProjectCleanerStatTreeItem> ListItem;
};

class SProjectCleanerStats : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SProjectCleanerStats)
		{
		}

	SLATE_END_ARGS()

	void Construct(const FArguments& Args);

private:
	TSharedRef<ITableRow> OnGenerateRow(TWeakObjectPtr<UProjectCleanerStatTreeItem> Item, const TSharedRef<STableViewBase>& OwnerTable) const;
	void OnGetChildren(TWeakObjectPtr<UProjectCleanerStatTreeItem> Item, TArray<TWeakObjectPtr<UProjectCleanerStatTreeItem>>& OutChildren);

	TArray<TWeakObjectPtr<UProjectCleanerStatTreeItem>> TreeItems;
	TSharedPtr<STreeView<TSharedPtr<UProjectCleanerStatTreeItem>>> TreeView;
};
