// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PjcInspector.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"

class FTabManager;
class SBox;
class SPjcClassBar;
class SPjcInspectorGraph;
class SSearchBox;
class SWrapBox;

// Asset of the finder or of the Used by and Uses lists
struct FPjcInspectorRow
{
	FName Package;
	FString Name;
	FString Path;
	FName ClassName;
	FString ClassLabel;
	const FSlateBrush* Icon = nullptr;
	FLinearColor IconColor = FLinearColor::White;
	EPjcAssetStatus Status = EPjcAssetStatus::Used;
	bool bScanned = true;
	// Lists: how the selected asset and this one reference each other, and whether the graph shows it
	EPjcLinkKind Kind = EPjcLinkKind::Hard;
	bool bInGraph = false;
};

/**
 * Inspector tool: why one asset is used. Finder on the left, the graph from every root down to the asset in the
 * middle, the selected asset on the right with its roots, what uses it and what it uses. For an unused asset the
 * graph shows the unused assets cleanup deletes with it.
 */
class SPjcTabInspector final : public SCompoundWidget
{
  public:

	SLATE_BEGIN_ARGS(SPjcTabInspector) {}
	// Tabs it links to (none in tests)
	SLATE_ARGUMENT(TSharedPtr<FTabManager>, TabManager)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~SPjcTabInspector() override;

	// Shows the asset, after the scan when none is done yet
	void Inspect(const FName Package);

	FName GetFocus() const {
		return Focus;
	}

	const FPjcInspectorGraph& GetGraph() const {
		return Graph;
	}

	int32 GetNumFinderRows() const {
		return FinderRows.Num();
	}

  private:

	using FRowPtr = TSharedPtr<FPjcInspectorRow>;

	enum class EStatusFilter : uint8
	{
		All,
		Used,
		EditorOnly,
		Review,
		Unused,
	};

	void OnScanFinished();
	void SetFocus(const FName Package, const bool bRecord);
	void RebuildGraph();
	void Select(const int32 Node);
	void Back();
	void Forward();

	// finder
	TSharedRef<SWidget> MakeFinder();
	void RebuildFinder();
	void ApplyFinderFilter();
	// The class bar's part of the filter: lists what its classes leave of the rows status and search kept
	void FilterFinderRows();
	TSharedRef<ITableRow> OnGenerateFinderRow(FRowPtr Row, const TSharedRef<STableViewBase>& Table);

	// graph
	TSharedRef<SWidget> MakeGraphPane();
	void OnNodeClicked(const int32 Node);
	void OnNodeDoubleClicked(const int32 Node);
	FText GetGraphTitle() const;
	FText GetGraphSubtitle() const;

	// details
	TSharedRef<SWidget> MakeDetails();
	void RebuildDetails();
	TSharedRef<SWidget> MakeFacts();
	TSharedRef<SWidget> MakeActions();
	TSharedRef<SWidget> MakeGroupDetails(const FPjcInspectorNode& Node);
	TSharedRef<SWidget> MakeLinkList(const bool bUsedBy);
	TSharedRef<ITableRow> OnGenerateLinkRow(FRowPtr Row, const TSharedRef<STableViewBase>& Table);
	void OnLinkClicked(FRowPtr Row);
	TSharedRef<SWidget> MakeExcludeMenu();
	void DeleteSelected();
	void OpenSelected() const;
	void BrowseSelected() const;

	FRowPtr MakeRow(const FName Package) const;
	FName GetSelectedPackage() const;

	TSharedPtr<FTabManager> TabManager;
	FDelegateHandle ScanFinishedHandle;

	FName Focus;
	// Inspect asked before a scan
	FName Pending;
	FPjcInspectorGraph Graph;
	TSet<FName> OpenGroups;
	int32 Selected = INDEX_NONE;
	TArray<FName> HistoryBack;
	TArray<FName> HistoryForward;

	TArray<FRowPtr> FinderAll;
	TArray<FRowPtr> FinderRows;
	// Rows that pass status and search, before the class bar
	TArray<FRowPtr> FinderBase;
	TSharedPtr<SPjcClassBar> FinderClasses;
	TSharedPtr<SListView<FRowPtr>> FinderList;
	TSharedPtr<SSearchBox> FinderSearch;
	FString FinderText;
	EStatusFilter FinderStatus = EStatusFilter::All;

	TSharedPtr<SPjcInspectorGraph> GraphView;

	TSharedPtr<SBox> DetailsBox;
	TArray<FRowPtr> UsedByRows;
	TArray<FRowPtr> UsesRows;
	// Level actors saved in their own files the lists went through
	int32 UsedByThrough = 0;
	int32 UsesThrough = 0;
	TSharedPtr<SListView<FRowPtr>> UsedByList;
	TSharedPtr<SListView<FRowPtr>> UsesList;
};
