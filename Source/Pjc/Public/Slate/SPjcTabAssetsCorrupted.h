// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

struct FPjcCorruptedAssetItem;

class SPjcTabAssetsCorrupted final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SPjcTabAssetsCorrupted) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

protected:
	void ListUpdateData();
	void ListUpdateView();
	void OnListSort(EColumnSortPriority::Type SortPriority, const FName& ColumnName, EColumnSortMode::Type InSortMode);
	void OnSearchTextChanged(const FText&);
	void OnSearchTextCommitted(const FText&, ETextCommit::Type);
	TSharedRef<SHeaderRow> GetListHeaderRow();
	TSharedRef<ITableRow> OnListGenerateRow(TSharedPtr<FPjcCorruptedAssetItem> Item, const TSharedRef<STableViewBase>& OwnerTable) const;
	TSharedPtr<SWidget> OnContextMenuOpening() const;
	TSharedRef<SWidget> CreateToolbar() const;
	FText GetTxtSummary() const;

private:
	FText SearchText;
	int32 NumFilesTotal = 0;
	int64 SizeFilesTotal = 0;
	TSharedPtr<FUICommandList> Cmds;
	TArray<TSharedPtr<FPjcCorruptedAssetItem>> ItemsAll;
	TArray<TSharedPtr<FPjcCorruptedAssetItem>> ItemsFiltered;
	TSharedPtr<SListView<TSharedPtr<FPjcCorruptedAssetItem>>> ListView;

	EColumnSortMode::Type ColumnSortModeFilePath = EColumnSortMode::None;
	EColumnSortMode::Type ColumnSortModeFileName = EColumnSortMode::None;
	EColumnSortMode::Type ColumnSortModeFileExt = EColumnSortMode::None;
	EColumnSortMode::Type ColumnSortModeFileSize = EColumnSortMode::None;
};
