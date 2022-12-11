// // Copyright Ashot Barkhudaryan. All Rights Reserved.
//
// #pragma once
//
// #include "CoreMinimal.h"
// #include "ProjectCleanerLibrary.h"
// #include "ProjectCleanerTypes.h"
// #include "Widgets/SCompoundWidget.h"
//
// struct FProjectCleanerScanner;
//
// class SProjectCleanerTabCorruptedListItem final : public SMultiColumnTableRow<TSharedPtr<FProjectCleanerTabCorruptedListItem>>
// {
// public:
// 	SLATE_BEGIN_ARGS(SProjectCleanerTabCorruptedListItem)
// 		{
// 		}
//
// 		SLATE_ARGUMENT(TSharedPtr<FProjectCleanerTabCorruptedListItem>, ListItem)
// 	SLATE_END_ARGS()
//
// 	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InTable)
// 	{
// 		ListItem = InArgs._ListItem;
//
// 		SMultiColumnTableRow<TSharedPtr<FProjectCleanerTabCorruptedListItem>>::Construct(
// 			SMultiColumnTableRow<TSharedPtr<FProjectCleanerTabCorruptedListItem>>::FArguments(),
// 			InTable
// 		);
// 	}
//
// 	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& InColumnName) override
// 	{
// 		if (InColumnName.IsEqual(TEXT("FileName")))
// 		{
// 			return SNew(STextBlock).Text(FText::FromString(ListItem->FileName));
// 		}
//
// 		if (InColumnName.IsEqual(TEXT("FileExt")))
// 		{
// 			return SNew(STextBlock).Text(FText::FromString(ListItem->FileExtension));
// 		}
//
// 		if (InColumnName.IsEqual(TEXT("FileSize")))
// 		{
// 			return SNew(STextBlock).Text(FText::AsMemory(ListItem->FileSize));
// 		}
//
// 		if (InColumnName.IsEqual(TEXT("FilePath")))
// 		{
// 			return
// 				SNew(SHorizontalBox)
// 				+ SHorizontalBox::Slot()
// 				  .AutoWidth()
// 				  .Padding(FMargin{10.0f, 0.0f, 0.0f, 0.0f})
// 				[
// 					SNew(STextBlock)
// 					.ToolTipText(FText::FromString(UProjectCleanerLibrary::PathConvertToRel(ListItem->FilePathAbs)))
// 					.Justification(ETextJustify::Left)
// 					.Text(FText::FromString(ListItem->FilePathAbs))
// 				];
// 		}
//
// 		return SNew(STextBlock).Text(FText::FromString("No Files"));
// 	}
//
// private:
// 	TSharedPtr<FProjectCleanerTabCorruptedListItem> ListItem;
// };
//
// class SProjectCleanerTabCorrupted final : public SCompoundWidget
// {
// public:
// 	SLATE_BEGIN_ARGS(SProjectCleanerTabCorrupted)
// 		{
// 		}
//
// 		SLATE_ARGUMENT(TSharedPtr<FProjectCleanerScanner>, Scanner)
// 	SLATE_END_ARGS()
//
// 	void Construct(const FArguments& InArgs);
// private:
// 	void ListUpdate();
// 	void ListSort();
// 	void OnListSort(EColumnSortPriority::Type SortPriority, const FName& Name, EColumnSortMode::Type SortMode);
// 	void OnListItemDblClick(TSharedPtr<FProjectCleanerTabCorruptedListItem> Item) const;
// 	TSharedPtr<SHeaderRow> GetListHeaderRow();
// 	TSharedRef<ITableRow> OnListGenerateRow(TSharedPtr<FProjectCleanerTabCorruptedListItem> InItem, const TSharedRef<STableViewBase>& OwnerTable) const;
// 	FText GetListTextSummary() const;
//
// 	int64 TotalSize = 0;
// 	FName ListSortColumn{TEXT("FileSize")};
// 	TEnumAsByte<EColumnSortMode::Type> ListSortMode = EColumnSortMode::Descending;
// 	TSharedPtr<FProjectCleanerScanner> Scanner;
// 	TArray<TSharedPtr<FProjectCleanerTabCorruptedListItem>> ListItems;
// 	TSharedPtr<SListView<TSharedPtr<FProjectCleanerTabCorruptedListItem>>> ListView;
// };
