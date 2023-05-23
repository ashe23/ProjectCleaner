// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ContentBrowserDelegates.h"
#include "Widgets/SCompoundWidget.h"

struct FPjcFileInfo;

class SPjcTabAssetsIndirect final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SPjcTabAssetsIndirect) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

protected:
	TSharedRef<SWidget> CreateToolbar() const;
	TSharedRef<SHeaderRow> GetHeaderRow();
	TSharedRef<ITableRow> OnGenerateRow(TSharedPtr<FPjcFileInfo> Item, const TSharedRef<STableViewBase>& OwnerTable) const;
	void ListUpdateData();
	void ListUpdateView();
	void OnListSort(EColumnSortPriority::Type SortPriority, const FName& ColumnName, EColumnSortMode::Type InSortMode);
	void OnSearchTextChanged(const FText& InText);
	void OnSearchTextCommitted(const FText& InText, ETextCommit::Type);

private:
	TArray<TSharedPtr<FPjcFileInfo>> ItemsAll;
	TArray<TSharedPtr<FPjcFileInfo>> ItemsFiltered;
	TSharedPtr<SListView<TSharedPtr<FPjcFileInfo>>> ListView;

	EColumnSortMode::Type ColumnSortModeFilePath = EColumnSortMode::None;
	EColumnSortMode::Type ColumnSortModeFileNum = EColumnSortMode::None;

	FText SearchText;
	FARFilter Filter;
	TSharedPtr<FUICommandList> Cmds;
	TMap<FAssetData, TArray<FPjcFileInfo>> AssetsIndirectInfos;
	FSetARFilterDelegate DelegateFilter;
	FRefreshAssetViewDelegate DelegateRefreshView;
	FGetCurrentSelectionDelegate DelegateSelection;
};
