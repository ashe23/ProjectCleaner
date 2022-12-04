// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectCleanerTypes.h"
#include "Widgets/SCompoundWidget.h"

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
			return SNew(STextBlock).Text(FText::FromString(ListItem->FilePathAbs));
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
		
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
private:
	FText GetTotalSizeTxt() const;
	TSharedPtr<SHeaderRow> GetListHeaderRow() const;
	void OnListItemDblClick(TSharedPtr<FProjectCleanerTabNonEngineListItem> Item) const;
	TSharedRef<ITableRow> OnGenerateRow(
		TSharedPtr<FProjectCleanerTabNonEngineListItem> InItem,
		const TSharedRef<STableViewBase>& OwnerTable
	) const;

	int64 TotalSize = 0;
	TArray<TSharedPtr<FProjectCleanerTabNonEngineListItem>> ListItems;
	TSharedPtr<SListView<TSharedPtr<FProjectCleanerTabNonEngineListItem>>> ListView;
};
