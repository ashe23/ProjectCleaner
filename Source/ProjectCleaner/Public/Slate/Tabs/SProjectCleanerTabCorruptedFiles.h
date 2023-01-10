// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectCleanerSubsystem.h"
#include "ProjectCleanerTypes.h"
#include "Libs/ProjectCleanerLibPath.h"
#include "Widgets/SCompoundWidget.h"

class UProjectCleanerLibPath;
struct FProjectCleanerScanner;

class SProjectCleanerTabCorruptedListItem final : public SMultiColumnTableRow<TSharedPtr<FProjectCleanerTabCorruptedListItem>>
{
public:
	SLATE_BEGIN_ARGS(SProjectCleanerTabCorruptedListItem)
		{
		}

		SLATE_ARGUMENT(TSharedPtr<FProjectCleanerTabCorruptedListItem>, ListItem)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InTable)
	{
		ListItem = InArgs._ListItem;

		SMultiColumnTableRow::Construct(SMultiColumnTableRow::FArguments(), InTable);
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

		// if (InColumnName.IsEqual(TEXT("FilePath")))
		// {
		// 	return
		// 		SNew(SHorizontalBox)
		// 		+ SHorizontalBox::Slot()
		// 		  .AutoWidth()
		// 		  .Padding(FMargin{10.0f, 0.0f, 0.0f, 0.0f})
		// 		[
		// 			SNew(STextBlock)
		// 			.ToolTipText(FText::FromString(UProjectCleanerLibPath::ConvertToRel(ListItem->FilePathAbs)))
		// 			.Justification(ETextJustify::Left)
		// 			.Text(FText::FromString(ListItem->FilePathAbs))
		// 		];
		// }

		return SNew(STextBlock).Text(FText::FromString("No Files"));
	}

private:
	TSharedPtr<FProjectCleanerTabCorruptedListItem> ListItem;
};

class SProjectCleanerTabCorrupted final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SProjectCleanerTabCorrupted)
		{
		}

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~SProjectCleanerTabCorrupted() override;
private:
	void OnProjectScanned();
	void ListUpdate();
	void ListSort();
	void OnListSort(EColumnSortPriority::Type SortPriority, const FName& Name, EColumnSortMode::Type SortMode);
	void OnListItemDblClick(TSharedPtr<FProjectCleanerTabCorruptedListItem> Item) const;
	TSharedPtr<SHeaderRow> GetListHeaderRow();
	TSharedPtr<SWidget> OnListContextMenu() const;
	TSharedRef<ITableRow> OnListGenerateRow(TSharedPtr<FProjectCleanerTabCorruptedListItem> InItem, const TSharedRef<STableViewBase>& OwnerTable) const;
	FText GetListTextSummary() const;

	int64 TotalSize = 0;
	FName ListSortColumn{TEXT("FileSize")};
	TEnumAsByte<EColumnSortMode::Type> ListSortMode = EColumnSortMode::Descending;
	TArray<TSharedPtr<FProjectCleanerTabCorruptedListItem>> ListItems;
	TSharedPtr<SListView<TSharedPtr<FProjectCleanerTabCorruptedListItem>>> ListView;
	TSharedPtr<FUICommandList> Cmds;

	UProjectCleanerSubsystem* SubsystemPtr = nullptr;
};
