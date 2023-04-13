// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PjcTypes.h"
#include "Widgets/SCompoundWidget.h"

class UPjcSubsystem;

class FPjcFileBrowserItem
{
public:
	int64 FileSize = 0;
	FString FileName;
	FString FilePathAbs;
	FString FileExtension;
	FString FileType;
};

class SPjcFileBrowserItem final : public SMultiColumnTableRow<TSharedPtr<FPjcFileBrowserItem>>
{
public:
	SLATE_BEGIN_ARGS(SPjcFileBrowserItem) {}
		SLATE_ARGUMENT(TSharedPtr<FPjcFileBrowserItem>, Item)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InTable);
	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& InColumnName) override;

private:
	TSharedPtr<FPjcFileBrowserItem> Item;
};


class SPjcFileBrowser final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SPjcFileBrowser) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	void ListItemsUpdate(const FPjcScanResult& InScanResult);
	void OnListSort(EColumnSortPriority::Type SortPriority, const FName& ColumnName, EColumnSortMode::Type SortMode);
	FText GetListSummaryText() const;
	TSharedRef<SHeaderRow> GetHeaderRow();
	TSharedRef<ITableRow> OnGenerateRow(TSharedPtr<FPjcFileBrowserItem> Item, const TSharedRef<STableViewBase>& OwnerTable) const;

	EColumnSortMode::Type ColumnFileNameSortMode = EColumnSortMode::None;
	EColumnSortMode::Type ColumnFileTypeSortMode = EColumnSortMode::None;
	EColumnSortMode::Type ColumnFileExtSortMode = EColumnSortMode::None;
	EColumnSortMode::Type ColumnFileSizeSortMode = EColumnSortMode::None;
	EColumnSortMode::Type ColumnFilePathSortMode = EColumnSortMode::None;
	TArray<TSharedPtr<FPjcFileBrowserItem>> ListItems;
	TSharedPtr<SListView<TSharedPtr<FPjcFileBrowserItem>>> ListView;
	UPjcSubsystem* SubsystemPtr = nullptr;
	FPjcScanResult ScanResult;
};
