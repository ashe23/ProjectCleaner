// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectCleanerTypes.h"
#include "Widgets/SCompoundWidget.h"

struct FProjectCleanerScanner;

class SProjectCleanerTabNonEngineListItem final : public SMultiColumnTableRow<TSharedPtr<FProjectCleanerTabNonEngineListItem>>
{
public:
	SLATE_BEGIN_ARGS(SProjectCleanerTabNonEngineListItem)
		{
		}

		SLATE_ARGUMENT(TSharedPtr<FProjectCleanerTabNonEngineListItem>, ListItem)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InTable)
	{
		ListItem = InArgs._ListItem;

		SMultiColumnTableRow<TSharedPtr<FProjectCleanerTabNonEngineListItem>>::Construct(
			SMultiColumnTableRow<TSharedPtr<FProjectCleanerTabNonEngineListItem>>::FArguments(),
			InTable
		);
	}

	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& InColumnName) override
	{
		if (InColumnName.IsEqual(TEXT("FileName")))
		{
			return SNew(STextBlock).Text(FText::FromString(ListItem->FileName));
		}

		if (InColumnName.IsEqual(TEXT("FileExt")))
		{
			return SNew(STextBlock).Text(FText::FromString(ListItem->FileExtension));
		}

		if (InColumnName.IsEqual(TEXT("FileSize")))
		{
			return SNew(STextBlock).Text(FText::AsMemory(ListItem->FileSize));
		}

		if (InColumnName.IsEqual(TEXT("FilePath")))
		{
			return
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				  .AutoWidth()
				  .Padding(FMargin{10.0f, 0.0f, 0.0f, 0.0f})
				[
					SNew(STextBlock)
					.Justification(ETextJustify::Left)
					.Text(FText::FromString(ListItem->FilePathAbs))
				];
		}

		return SNew(STextBlock).Text(FText::FromString("No Files"));
	}

private:
	TSharedPtr<FProjectCleanerTabNonEngineListItem> ListItem;
};

class SProjectCleanerTabNonEngine final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SProjectCleanerTabNonEngine)
		{
		}

		SLATE_ARGUMENT(TSharedPtr<FProjectCleanerScanner>, Scanner)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
private:
	void ListUpdate();
	void ListSort();
	void OnListSort(EColumnSortPriority::Type SortPriority, const FName& Name, EColumnSortMode::Type SortMode);
	FText GetTotalSizeTxt() const;
	TSharedPtr<SHeaderRow> GetListHeaderRow();
	void OnListItemDblClick(TSharedPtr<FProjectCleanerTabNonEngineListItem> Item) const;
	TSharedRef<ITableRow> OnGenerateRow(
		TSharedPtr<FProjectCleanerTabNonEngineListItem> InItem,
		const TSharedRef<STableViewBase>& OwnerTable
	) const;
	TSharedPtr<SWidget> OnListContextMenu() const;

	int64 TotalSize = 0;
	FName ListSortColumn{TEXT("FileSize")};
	TEnumAsByte<EColumnSortMode::Type> ListSortMode = EColumnSortMode::Descending;
	TSharedPtr<FUICommandList> Cmds;
	TSharedPtr<FProjectCleanerScanner> Scanner;
	TArray<TSharedPtr<FProjectCleanerTabNonEngineListItem>> ListItems;
	TSharedPtr<SListView<TSharedPtr<FProjectCleanerTabNonEngineListItem>>> ListView;
};
