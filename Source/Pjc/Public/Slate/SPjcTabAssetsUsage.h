// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Framework/Commands/UICommandList.h"
#include "Widgets/Views/SHeaderRow.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/STableRow.h"

struct FPjcUsageItem;
class FPjcAssetGraph;
class SPjcAssetGraph;
class SVerticalBox;

/**
 * Experimental: tells for every scanned asset why it is used or unused. Graph of project assets (or asset list); selected
 * asset opens on the right with plain words why, the chain from an asset used for its own reason down to it, and assets it
 * is used by and uses.
 */
class SPjcTabAssetsUsage final : public SCompoundWidget
{
  public:

	SLATE_BEGIN_ARGS(SPjcTabAssetsUsage) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** Shows usage of given package, scanned asset or not. Scans first if needed */
	void Inspect(const FName& PackageName, const bool bRecordHistory = true);

  private:

	enum class EFilter : uint8
	{
		All,
		Used,
		Unused,
		NotDeleted,
	};

	void Scan();
	void ListUpdateView();
	void ListSort();
	void RebuildChain();
	void ClearInspected();
	TSharedPtr<FPjcUsageItem> FindOrMakeItem(const FName& PackageName);
	FString GetShortReason(const FPjcUsageItem& Item);
	FString GetFolderLabelName(const FPjcUsageItem& Item);
	FText BuildWhyText(const FPjcUsageItem& Item);

	TSharedRef<SWidget> CreateToolbar() const;
	TSharedRef<SWidget> CreateViewToggle();
	TSharedRef<SWidget> CreateFilterMenu();
	TSharedRef<SWidget> CreateInspector();
	TSharedRef<SHeaderRow> GetHeaderRow(const bool bAssetList);
	TSharedRef<ITableRow> OnGenerateRow(TSharedPtr<FPjcUsageItem> Item, const TSharedRef<STableViewBase>& OwnerTable) const;
	void OnListSelectionChanged(TSharedPtr<FPjcUsageItem> Item, ESelectInfo::Type SelectInfo);
	void OnLinkDoubleClicked(TSharedPtr<FPjcUsageItem> Item);
	void OnGraphSelectionChanged(const FName& PackageName);
	void OnGraphAssetDoubleClicked(const FName& PackageName);
	void OnListSort(EColumnSortPriority::Type SortPriority, const FName& ColumnName, EColumnSortMode::Type InSortMode);
	EColumnSortMode::Type GetColumnSortMode(const FName ColumnName) const;
	void OnSearchTextChanged(const FText& InText);
	void OnBack();
	void OnForward();
	void OnShowInContentBrowser() const;
	void OnOpenAsset() const;
	void OnOpenReferenceViewer() const;
	bool CurrentHasAsset() const;

	FText GetSummaryText() const;
	FText GetFilterText() const;
	FText GetTitleText() const;
	FText GetSubtitleText() const;
	FText GetStatusText() const;
	FSlateColor GetStatusColor() const;
	FText GetUsedByHeaderText() const;
	FText GetUsesHeaderText() const;
	int32 GetInspectorIndex() const;

	TSharedPtr<FUICommandList> Cmds;
	TArray<TSharedPtr<FPjcUsageItem>> ItemsAll;
	TArray<TSharedPtr<FPjcUsageItem>> ItemsFiltered;
	TArray<TSharedPtr<FPjcUsageItem>> ItemsUsedBy;
	TArray<TSharedPtr<FPjcUsageItem>> ItemsUses;
	TMap<FName, TSharedPtr<FPjcUsageItem>> ItemsByPackage;
	TSharedPtr<SListView<TSharedPtr<FPjcUsageItem>>> ListView;
	TSharedPtr<SListView<TSharedPtr<FPjcUsageItem>>> ListUsedBy;
	TSharedPtr<SListView<TSharedPtr<FPjcUsageItem>>> ListUses;
	TSharedPtr<SPjcAssetGraph> GraphView;
	TSharedPtr<FPjcAssetGraph> Graph;
	TSharedPtr<SVerticalBox> ChainBox;
	TSharedPtr<FPjcUsageItem> Current;
	FString CurrentHolder;
	FText CurrentWhy;
	TArray<FName> HistoryBack;
	TArray<FName> HistoryForward;
	FText SearchText;
	EFilter Filter = EFilter::All;
	FName SortColumn = TEXT("Status");
	EColumnSortMode::Type SortMode = EColumnSortMode::Ascending;
	bool bScanned = false;
	bool bSyncingSelection = false;
	bool bSelectingInGraph = false;
	bool bShowList = false;
	int32 NumUsed = 0;
	int32 NumUnused = 0;
	int32 NumNotDeleted = 0;
	int64 SizeUsed = 0;
	int64 SizeUnused = 0;
};
