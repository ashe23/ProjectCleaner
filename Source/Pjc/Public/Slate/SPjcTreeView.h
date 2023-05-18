// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PjcTypes.h"
#include "Widgets/SCompoundWidget.h"

class SPjcTreeView final : public SCompoundWidget
{
public:
	DECLARE_DELEGATE_OneParam(FPjcDelegateSelectionChanged, const TSet<FString>&)

	SLATE_BEGIN_ARGS(SPjcTreeView) {}
		SLATE_EVENT(FPjcDelegateSelectionChanged, OnSelectionChanged)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	const TSet<FString>& GetSelectedPaths() const;
	void TreeItemsUpdateData(TMap<EPjcAssetCategory, TSet<FAssetData>>& AssetsCategoryMapping);

protected:
	TSharedRef<SWidget> CreateToolbar() const;
	TSharedRef<SWidget> GetTreeBtnOptionsContent();
	TSharedPtr<SWidget> GetTreeContextMenu() const;
	TSharedRef<SHeaderRow> GetTreeHeaderRow();
	TSharedRef<ITableRow> OnTreeGenerateRow(TSharedPtr<FPjcTreeItem> Item, const TSharedRef<STableViewBase>& OwnerTable) const;
	void OnTreeGetChildren(TSharedPtr<FPjcTreeItem> Item, TArray<TSharedPtr<FPjcTreeItem>>& OutChildren);
	void OnTreeSearchTextChanged(const FText& InText);
	void OnTreeSearchTextCommitted(const FText& InText, ETextCommit::Type Type);
	void OnTreeSort(EColumnSortPriority::Type SortPriority, const FName& ColumnName, EColumnSortMode::Type InSortMode);
	void OnTreeSelectionChanged(TSharedPtr<FPjcTreeItem> Selection, ESelectInfo::Type SelectInfo);
	void TreeItemsInit();
	void TreeItemsUpdateView();
	bool ItemIsExpanded(const TSharedPtr<FPjcTreeItem>& Item, const TSet<TSharedPtr<FPjcTreeItem>>& ExpandedItems);
	FText GetTreeSummaryText() const;
	FText GetTreeSelectionText() const;
	FSlateColor GetTreeOptionsBtnForegroundColor() const;

private:
	void UpdateMapInfo(TMap<FString, int32>& MapNum, TMap<FString, int64>& MapSize, const FString& AssetPath, int64 AssetSize);

	int32 NumFoldersTotal = 0;
	FText SearchText;
	TSet<FString> SelectedPaths;
	TSharedPtr<FUICommandList> Cmds;
	TSharedPtr<FPjcTreeItem> RootItem;
	TSharedPtr<SComboButton> TreeOptionBtn;
	TArray<TSharedPtr<FPjcTreeItem>> TreeItems;
	TSharedPtr<STreeView<TSharedPtr<FPjcTreeItem>>> TreeView;

	TMap<FString, int32> MapNumAssetsAllByPath;
	TMap<FString, int32> MapNumAssetsUsedByPath;
	TMap<FString, int32> MapNumAssetsUnusedByPath;
	TMap<FString, int64> MapSizeAssetsAllByPath;
	TMap<FString, int64> MapSizeAssetsUsedByPath;
	TMap<FString, int64> MapSizeAssetsUnusedByPath;

	EColumnSortMode::Type ColumnPathSortMode = EColumnSortMode::None;
	EColumnSortMode::Type ColumnAssetsTotalSortMode = EColumnSortMode::None;
	EColumnSortMode::Type ColumnAssetsUsedSortMode = EColumnSortMode::None;
	EColumnSortMode::Type ColumnAssetsUnusedSortMode = EColumnSortMode::None;
	EColumnSortMode::Type ColumnUnusedPercentSortMode = EColumnSortMode::None;
	EColumnSortMode::Type ColumnUnusedSizeSortMode = EColumnSortMode::None;

	FPjcDelegateSelectionChanged DelegateSelectionChanged;
};
