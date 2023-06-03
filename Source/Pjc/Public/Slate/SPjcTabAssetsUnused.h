// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ContentBrowserDelegates.h"
#include "Widgets/SCompoundWidget.h"

struct FPjcTreeItem;
struct FPjcStatItem;
class UPjcSubsystem;
class FPjcFilterAssetsExtReferenced;
class FPjcFilterAssetsEditor;
class FPjcFilterAssetsCircular;
class FPjcFilterAssetsIndirect;
class FPjcFilterAssetsPrimary;
class FPjcFilterAssetsUsed;
class FPjcFilterAssetsExcluded;

class SPjcTabAssetsUnused final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SPjcTabAssetsUnused) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	TSharedRef<SWidget> CreateToolbarMain() const;
	TSharedRef<SWidget> CreateToolbarTreeView() const;
	TSharedRef<SWidget> CreateToolbarContentBrowser() const;
	void OnProjectScan();
	void OnProjectClean();
	void OnResetExcludeSettings();
	void OnDeleteEmptyFolders();
	void OnPathExclude();
	void OnPathInclude();
	void OnClearSelection() const;
	void ScanProject();
	void UpdateStats();
	void UpdateTreeView();
	void UpdateContentBrowser();
	void OnTreeGetChildren(TSharedPtr<FPjcTreeItem> Item, TArray<TSharedPtr<FPjcTreeItem>>& OutChildren);
	void OnTreeSearchTextChanged(const FText& InText);
	void OnTreeSearchTextCommitted(const FText& InText, ETextCommit::Type Type);
	void OnTreeSort(EColumnSortPriority::Type SortPriority, const FName& ColumnName, EColumnSortMode::Type InSortMode);
	void OnTreeSelectionChanged(TSharedPtr<FPjcTreeItem> Selection, ESelectInfo::Type SelectInfo);
	void OnTreeExpansionChanged(TSharedPtr<FPjcTreeItem> Item, bool bIsExpanded);
	void SortTreeItems(const bool UpdateSortingOrder);
	void ChangeItemExpansionRecursive(const TSharedPtr<FPjcTreeItem>& Item, const bool bExpansion, const bool bRebuildList) const;
	bool TreeItemIsVisible(const TSharedPtr<FPjcTreeItem>& Item) const;
	bool TreeItemIsExpanded(const TSharedPtr<FPjcTreeItem>& Item, const TSet<TSharedPtr<FPjcTreeItem>>& CachedItems) const;
	bool TreeItemContainsSearchText(const TSharedPtr<FPjcTreeItem>& Item) const;
	bool TreeHasSelection() const;
	bool CanCleanProject() const;
	bool CanDeleteEmptyFolders() const;
	bool AnyFilterActive() const;
	FText GetTreeSummaryText() const;
	FText GetTreeSelectionText() const;
	FSlateColor GetTreeOptionsBtnForegroundColor() const;
	TSharedRef<SWidget> GetTreeBtnOptionsContent();
	TSharedPtr<SWidget> GetTreeContextMenu() const;
	TSharedRef<SHeaderRow> GetStatsHeaderRow() const;
	TSharedRef<SHeaderRow> GetTreeHeaderRow();
	TSharedRef<ITableRow> OnStatsGenerateRow(TSharedPtr<FPjcStatItem> Item, const TSharedRef<STableViewBase>& OwnerTable) const;
	TSharedRef<ITableRow> OnTreeGenerateRow(TSharedPtr<FPjcTreeItem> Item, const TSharedRef<STableViewBase>& OwnerTable) const;

	void ResetFilters();
	void ResetCachedData();
	void UpdateMapInfo(TMap<FString, int32>& MapNum, TMap<FString, int64>& MapSize, const FString& AssetPath, int64 AssetSize);

	UPjcSubsystem* SubsystemPtr = nullptr;
	TSharedPtr<FUICommandList> Cmds;
	FText TreeSearchText;
	const FMargin HeaderMargin{5.0f};
	TSet<FName> SelectedPaths;
	FARFilter Filter;
	FSetARFilterDelegate DelegateFilter;
	FRefreshAssetViewDelegate DelegateRefreshView;
	FGetCurrentSelectionDelegate DelegateSelection;
	TSharedPtr<SComboButton> TreeOptionBtn;
	TSharedPtr<FPjcTreeItem> RootItem;
	TArray<TSharedPtr<FPjcStatItem>> StatsListItems;
	TArray<TSharedPtr<FPjcTreeItem>> TreeListItems;
	TSharedPtr<SListView<TSharedPtr<FPjcStatItem>>> StatsListView;
	TSharedPtr<STreeView<TSharedPtr<FPjcTreeItem>>> TreeListView;

	FName LastSortedColumn;
	EColumnSortMode::Type ColumnPathSortMode = EColumnSortMode::None;
	EColumnSortMode::Type ColumnAssetsTotalSortMode = EColumnSortMode::None;
	EColumnSortMode::Type ColumnAssetsUsedSortMode = EColumnSortMode::None;
	EColumnSortMode::Type ColumnAssetsUnusedSortMode = EColumnSortMode::None;
	EColumnSortMode::Type ColumnUnusedPercentSortMode = EColumnSortMode::None;
	EColumnSortMode::Type ColumnUnusedSizeSortMode = EColumnSortMode::None;

	bool bFilterAssetsUsedActive = false;
	bool bFilterAssetsPrimaryActive = false;
	bool bFilterAssetsEditorActive = false;
	bool bFilterAssetsIndirectActive = false;
	bool bFilterAssetsExcludedActive = false;
	bool bFilterAssetsExtReferencedActive = false;
	bool bFilterAssetsCircularActive = false;
	bool bFilterAssetsUnusedActive = true;

	TSharedPtr<FPjcFilterAssetsUsed> FilterUsed;
	TSharedPtr<FPjcFilterAssetsPrimary> FilterPrimary;
	TSharedPtr<FPjcFilterAssetsIndirect> FilterIndirect;
	TSharedPtr<FPjcFilterAssetsCircular> FilterCircular;
	TSharedPtr<FPjcFilterAssetsEditor> FilterEditor;
	TSharedPtr<FPjcFilterAssetsExcluded> FilterExcluded;
	TSharedPtr<FPjcFilterAssetsExtReferenced> FilterExtReferenced;

	TArray<FAssetData> AssetsAll;
	TArray<FAssetData> AssetsUsed;
	TArray<FAssetData> AssetsUnused;
	TArray<FAssetData> AssetsPrimary;
	TArray<FAssetData> AssetsIndirect;
	TArray<FAssetData> AssetsCircular;
	TArray<FAssetData> AssetsEditor;
	TArray<FAssetData> AssetsExcluded;
	TArray<FAssetData> AssetsExtReferenced;

	TMap<FString, int32> MapNumAssetsAllByPath;
	TMap<FString, int32> MapNumAssetsUsedByPath;
	TMap<FString, int32> MapNumAssetsUnusedByPath;
	TMap<FString, int64> MapSizeAssetsAllByPath;
	TMap<FString, int64> MapSizeAssetsUsedByPath;
	TMap<FString, int64> MapSizeAssetsUnusedByPath;

	int32 NumAssetsAll = 0;
	int32 NumAssetsUsed = 0;
	int32 NumAssetsUnused = 0;
	int32 NumAssetsPrimary = 0;
	int32 NumAssetsIndirect = 0;
	int32 NumAssetsEditor = 0;
	int32 NumAssetsExcluded = 0;
	int32 NumAssetsExtReferenced = 0;
	int32 NumFoldersTotal = 0;
	int32 NumFoldersEmpty = 0;

	int64 SizeAssetsAll = 0;
	int64 SizeAssetsUsed = 0;
	int64 SizeAssetsUnused = 0;
	int64 SizeAssetsPrimary = 0;
	int64 SizeAssetsIndirect = 0;
	int64 SizeAssetsEditor = 0;
	int64 SizeAssetsExcluded = 0;
	int64 SizeAssetsExtReferenced = 0;
};
