// // Copyright Ashot Barkhudaryan. All Rights Reserved.
//
// #pragma once
//
// #include "CoreMinimal.h"
// #include "PjcTypes.h"
// #include "Widgets/SCompoundWidget.h"
//
// DECLARE_DELEGATE_OneParam(FPjcDelegateTreeViewSelectionChanged, const TSet<FString>& SelectedPaths)
//
// class UPjcScannerSubsystem;
//
// class SPjcTreeView final : public SCompoundWidget
// {
// public:
// 	SLATE_BEGIN_ARGS(SPjcTreeView) {}
// 		SLATE_ARGUMENT(FMargin, HeaderPadding)
// 	SLATE_END_ARGS()
//
// 	void Construct(const FArguments& InArgs);
// 	FPjcDelegateTreeViewSelectionChanged& OnTreeViewSelectionChanged();
// 	virtual ~SPjcTreeView() override;
//
// protected:
// 	TSharedRef<SWidget> GetTreeBtnActionsContent();
// 	TSharedRef<SWidget> GetTreeBtnOptionsContent();
// 	TSharedPtr<SWidget> GetTreeContextMenu() const;
// 	TSharedRef<SHeaderRow> GetTreeHeaderRow();
// 	TSharedRef<ITableRow> OnTreeGenerateRow(TSharedPtr<FPjcTreeItem> Item, const TSharedRef<STableViewBase>& OwnerTable) const;
// 	void OnTreeGetChildren(TSharedPtr<FPjcTreeItem> Item, TArray<TSharedPtr<FPjcTreeItem>>& OutChildren);
// 	void OnTreeSearchTextChanged(const FText& InText);
// 	void OnTreeSearchTextCommitted(const FText& InText, ETextCommit::Type Type);
// 	void OnTreeSort(EColumnSortPriority::Type SortPriority, const FName& ColumnName, EColumnSortMode::Type InSortMode);
// 	void OnTreeSelectionChanged(TSharedPtr<FPjcTreeItem> Selection, ESelectInfo::Type SelectInfo);
// 	void CategorizeAssetsPerPath();
// 	void TreeItemsUpdateData();
// 	void TreeItemsUpdateView();
// 	void OnProjectAssetsScanSuccess();
// 	bool ItemIsExpanded(const TSharedPtr<FPjcTreeItem>& Item, const TSet<TSharedPtr<FPjcTreeItem>>& ExpandedItems);
// 	FText GetTreeSummaryText() const;
// 	FSlateColor GetTreeOptionsBtnForegroundColor() const;
//
// private:
// 	void UpdateMapInfo(TMap<FString, int32>& MapNum, TMap<FString, int64>& MapSize, const FString& AssetPath, int64 AssetSize);
//
// 	FText SearchText;
// 	FMargin HeaderPadding;
// 	TSharedPtr<FUICommandList> Cmds;
// 	TSharedPtr<FPjcTreeItem> RootItem;
// 	TSharedPtr<SComboButton> TreeOptionBtn;
// 	TArray<TSharedPtr<FPjcTreeItem>> TreeItems;
// 	TSharedPtr<STreeView<TSharedPtr<FPjcTreeItem>>> TreeView;
// 	UPjcScannerSubsystem* ScannerSubsystemPtr = nullptr;
//
// 	TMap<FString, int32> MapNumAssetsAllByPath;
// 	TMap<FString, int32> MapNumAssetsUsedByPath;
// 	TMap<FString, int32> MapNumAssetsUnusedByPath;
// 	TMap<FString, int64> MapSizeAssetsAllByPath;
// 	TMap<FString, int64> MapSizeAssetsUsedByPath;
// 	TMap<FString, int64> MapSizeAssetsUnusedByPath;
//
// 	EColumnSortMode::Type ColumnPathSortMode = EColumnSortMode::None;
// 	EColumnSortMode::Type ColumnAssetsTotalSortMode = EColumnSortMode::None;
// 	EColumnSortMode::Type ColumnAssetsUsedSortMode = EColumnSortMode::None;
// 	EColumnSortMode::Type ColumnAssetsUnusedSortMode = EColumnSortMode::None;
// 	EColumnSortMode::Type ColumnUnusedPercentSortMode = EColumnSortMode::None;
// 	EColumnSortMode::Type ColumnUnusedSizeSortMode = EColumnSortMode::None;
//
// 	FPjcDelegateTreeViewSelectionChanged DelegateTreeViewSelectionChanged;
// };
