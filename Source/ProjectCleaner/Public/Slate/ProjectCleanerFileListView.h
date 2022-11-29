// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectCleanerTypes.h"
#include "Widgets/SCompoundWidget.h"

class SProjectCleanerFileViewItem final : public SMultiColumnTableRow<TSharedPtr<FProjectCleanerFileViewItem>>
{
public:
	SLATE_BEGIN_ARGS(SProjectCleanerFileViewItem)
		{
		}

		SLATE_ARGUMENT(TSharedPtr<FProjectCleanerFileViewItem>, ListItem)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InTable)
	{
		ListItem = InArgs._ListItem;

		SMultiColumnTableRow<TSharedPtr<FProjectCleanerFileViewItem>>::Construct(
			SMultiColumnTableRow<TSharedPtr<FProjectCleanerFileViewItem>>::FArguments(),
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
			return SNew(STextBlock).Text(FText::FromString(ListItem->FileExt));
		}

		if (InColumnName.IsEqual(TEXT("FileSize")))
		{
			return SNew(STextBlock).Text(FText::AsMemory(ListItem->FileSize));
		}

		if (InColumnName.IsEqual(TEXT("FilePath")))
		{
			return SNew(STextBlock).Text(FText::FromString(ListItem->FilePath));
		}

		return SNew(STextBlock).Text(FText::FromString("No Data"));
	}

private:
	TSharedPtr<FProjectCleanerFileViewItem> ListItem;
};

class SProjectCleanerFileListView final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SProjectCleanerFileListView)
		{
		}

		SLATE_ARGUMENT(TSet<FString>, Files)
		SLATE_ARGUMENT(FString, Title)
		SLATE_ARGUMENT(FString, Description)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
private:
	TSharedPtr<SHeaderRow> GetListHeaderRow() const;
	void OnListItemDblClick(TSharedPtr<FProjectCleanerFileViewItem> Item) const;
	TSharedRef<ITableRow> OnGenerateRow(
		TSharedPtr<FProjectCleanerFileViewItem> InItem,
		const TSharedRef<STableViewBase>& OwnerTable
	) const;

	TArray<TSharedPtr<FProjectCleanerFileViewItem>> ListItems;
	TSharedPtr<SListView<TSharedPtr<FProjectCleanerFileViewItem>>> ListView;
};
