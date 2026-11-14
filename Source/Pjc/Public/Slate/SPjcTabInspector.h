// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PjcInspector.h"
#include "PjcLocations.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/STreeView.h"

class FTabManager;
class SBox;
class SPjcClassBar;
class SSearchBox;
class SVerticalBox;
class SWrapBox;

// Asset of the finder
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
};

// Row of the Used by or Uses tree
struct FPjcInspectorItem
{
	enum class EKind : uint8
	{
		Asset,
		// assets of one class, status or folder of a big level
		Group,
		// part of a read asset (Details panel category) and the references it holds
		Category,
		// the rest of a group not shown yet
		More,
	};

	EKind Kind = EKind::Asset;
	// Unique in the tree, keeps it open over a rebuild
	FString Key;
	int32 Depth = 0;
	TWeakPtr<FPjcInspectorItem> Parent;
	TArray<TSharedPtr<FPjcInspectorItem>> Children;
	bool bBuilt = false;

	// Asset: its node and the reference to it. Holder: the asset that holds the reference (the row in Used by, the
	// asset above in Uses), Target the asset it points at
	int32 Node = INDEX_NONE;
	FPjcModelLink Link;
	int32 Holder = INDEX_NONE;
	int32 Target = INDEX_NONE;
	// Under a category: the property that holds it
	FString Property;
	// On the route the answer shows; RouteRest: the assets of the route above it
	bool bRoute = false;
	TArray<int32> RouteRest;
	bool bCircular = false;

	// Group, Category, More: the asset whose level it is part of, its label, the assets it holds and how many show
	int32 Owner = INDEX_NONE;
	FString Label;
	int32 IconNode = INDEX_NONE;
	bool bFolder = false;
	EPjcAssetStatus GroupStatus = EPjcAssetStatus::Used;
	bool bStatusGroup = false;
	TArray<FPjcModelLink> Members;
	int32 Shown = 0;
};

/**
 * Inspector tool: one asset and the questions about it. Finder on the left. On the right the answer on top (why it is
 * used or unused, what keeps it, what goes with it), then one tree of what uses it or what it uses, with where each
 * reference is set once the assets that hold them are read.
 */
class SPjcTabInspector final : public SCompoundWidget
{
  public:

	SLATE_BEGIN_ARGS(SPjcTabInspector) {}
	// Tabs it links to (none in tests)
	SLATE_ARGUMENT(TSharedPtr<FTabManager>, TabManager)
	SLATE_END_ARGS()

	using FItemPtr = TSharedPtr<FPjcInspectorItem>;

	void Construct(const FArguments& InArgs);
	virtual ~SPjcTabInspector() override;

	virtual bool SupportsKeyboardFocus() const override {
		return true;
	}
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;

	// Shows the asset, after the scan when none is done yet
	void Inspect(const FName Package);

	FName GetFocus() const {
		return Focus;
	}

	const FPjcInspectorModel& GetModel() const {
		return Model;
	}

	int32 GetNumFinderRows() const {
		return FinderRows.Num();
	}

	// Rows of the tree, top level
	const TArray<FItemPtr>& GetTreeItems() const {
		return TreeItems;
	}

	// Uses instead of Used by
	void ShowUses(const bool bUses);

	// Loads the assets that hold the references of the rows on screen and reads where they are set
	// bAsk: a big load is asked about first
	void ShowWhere(const bool bAsk = true);
	const TMap<FName, FPjcLocations>& GetLocations() const {
		return Locations;
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
	void Back();
	void Forward();
	// The model of this scan, read when the scan changed
	bool EnsureModel();
	void RebuildAll();

	// finder
	TSharedRef<SWidget> MakeFinder();
	void RebuildFinder();
	void ApplyFinderFilter();
	// Status filters as pills, each with how many assets the search leaves of it
	void RebuildFinderPills(const TMap<EStatusFilter, int32>& Counts);
	// The class bar's part of the filter: lists what its classes leave of the rows status and search kept
	void FilterFinderRows();
	TSharedRef<ITableRow> OnGenerateFinderRow(FRowPtr Row, const TSharedRef<STableViewBase>& Table);

	// the asset and the answer
	TSharedRef<SWidget> MakeMain();
	void RebuildHead();
	// routes, what goes with it, the counts: what the answer and the tree need of the model
	void ComputeAnswer();
	const FPjcRoute* GetPickedRoute() const;
	FString GetBasisText() const;
	FString GetEmptyText() const;
	TSharedRef<SWidget> MakeHead();
	TSharedRef<SWidget> MakeToolbar();
	TSharedRef<SWidget> MakeAnswer();
	TSharedRef<SWidget> MakeTabs();
	TSharedRef<SWidget> MakeChip(const int32 Node);
	TSharedRef<SWidget> MakeRouteMenu();
	TSharedRef<SWidget> MakeExcludeMenu();
	void DeleteFocus();

	// the tree
	void RebuildTree();
	TArray<FItemPtr> MakeLevel(const int32 Owner, const int32 Depth, const FItemPtr& Parent, const FString& Key, const TArray<int32>* RouteRest);
	void BuildChildren(const FItemPtr& Item);
	void AddGroupChildren(const FItemPtr& Item);
	FItemPtr MakeAssetItem(const int32 Owner, const FPjcModelLink& Link, const int32 Depth, const FItemPtr& Parent, const FString& Key);
	void SortLinks(TArray<FPjcModelLink>& Links) const;
	bool ShouldExpand(const FItemPtr& Item) const;
	void RestoreExpansion(const TArray<FItemPtr>& Items);
	void OnExpansionChanged(FItemPtr Item, bool bExpanded);
	TSharedRef<ITableRow> OnGenerateTreeRow(FItemPtr Item, const TSharedRef<STableViewBase>& Table);
	TSharedPtr<SWidget> OnTreeContextMenu();
	void OnTreeDoubleClick(FItemPtr Item);
	void ShowMore(const FItemPtr& Item);
	FItemPtr GetSelectedItem() const;
	TSharedRef<SWidget> MakeCell(const FItemPtr& Item, const FName Column, const TSharedRef<ITableRow>& TableRow);
	// Which assets keep the assets of a group, shown in the answer
	void ShowKeepers(const FItemPtr& Group);
	// The holders of the rows on screen that can be read and were not yet
	void UpdateNeedRead();
	void ReadHolders(const TArray<FName>& Holders, const bool bAsk);
	void UnloadRead();

	// what a row shows
	FString GetWhereText(const FPjcInspectorItem& Item, FString& OutToolTip, FName& OutColor) const;
	FString GetStatsText() const;
	bool IsTreeSearch() const {
		return !TreeSearch.IsEmpty();
	}

	TSharedPtr<FTabManager> TabManager;
	FDelegateHandle ScanFinishedHandle;

	FName Focus;
	int32 FocusNode = INDEX_NONE;
	// Inspect asked before a scan
	FName Pending;
	TArray<FName> HistoryBack;
	TArray<FName> HistoryForward;

	FPjcInspectorModel Model;
	// what was read from the asset registry for this scan
	FPjcInspectorCache Cache;
	FDateTime CacheTime {0};

	// what the answer and the tree show
	bool bUses = false;
	uint8 Kinds = PjcLinkKinds::All;
	int32 RouteRoot = INDEX_NONE;
	bool bDeleteList = false;
	int32 WhatIfUser = INDEX_NONE;
	TArray<int32> Keepers;
	FString KeepersLabel;
	FString TreeSearch;
	FPjcInspectorModel::FSearch SearchResult;
	TMap<int32, int32> Prefer;
	TArray<FPjcRoute> Routes;
	TArray<int32> KeyRoutes;
	TArray<int32> Retained;
	TSet<int32> RetainedSet;
	TArray<int32> Choke;
	FPjcInspectorModel::FReach ReachUp;
	FPjcInspectorModel::FReach ReachDown;
	TArray<FName> NeedRead;
	TSet<FString> ClassChips;
	// opened by the user, and rows open by default (route, categories) the user closed
	TSet<FString> Expanded;
	TSet<FString> Closed;
	// the asset each open row stands for: an asset opens once, a second row of it points at the first
	TMap<int32, TWeakPtr<FPjcInspectorItem>> OpenNodes;
	TMap<FString, int32> Pages;

	TArray<FRowPtr> FinderAll;
	TArray<FRowPtr> FinderRows;
	// Rows that pass status and search, before the class bar
	TArray<FRowPtr> FinderBase;
	TSharedPtr<SPjcClassBar> FinderClasses;
	TSharedPtr<SListView<FRowPtr>> FinderList;
	TSharedPtr<SSearchBox> FinderSearch;
	TSharedPtr<SVerticalBox> FinderPills;
	FString FinderText;
	EStatusFilter FinderStatus = EStatusFilter::All;

	TSharedPtr<SBox> HeadBox;
	TSharedPtr<SSearchBox> TreeSearchBox;
	TSharedPtr<SWrapBox> ChipsBox;
	TArray<FItemPtr> TreeItems;
	TSharedPtr<STreeView<FItemPtr>> TreeView;

	// Where references are set: the assets that hold them are loaded for that, only when asked. What was read stays
	// until the next scan, also after the loaded assets are let go
	TMap<FName, FPjcLocations> Locations;
	TArray<FName> LoadedHere;
};
