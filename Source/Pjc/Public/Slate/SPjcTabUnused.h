// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PjcUnused.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/STileView.h"
#include "Widgets/Views/STreeView.h"

class FMenuBuilder;
class SPjcSizeMap;
class FTabManager;
class SBox;
class SHorizontalBox;
class SVerticalBox;
class SPjcClassBar;
struct FSlateBrush;

// Asset in the list of the Assets tool
struct FPjcUnusedAssetRow
{
	// Index into FPjcScanResult::Assets
	int32 Index = INDEX_NONE;
	FName Package;
	FString Name;
	FName ClassName;
	// Type name as the Content Browser shows it
	FString ClassLabel;
	FLinearColor Color = FLinearColor::White;
	const FSlateBrush* Icon = nullptr;
	// Big class icon for the grid (the Content Browser's class thumbnail), the icon when there is none
	const FSlateBrush* Thumb = nullptr;
	int64 Size = 0;
	// Folder relative to the selected folder
	FString Folder;
	// In the folder of a label whose cook rule does not package it
	bool bLabel = false;
	// Unused or review assets that reference it and are deleted with it
	int32 Group = 0;
	// What held it when the last delete could not remove it, empty when it was not kept
	FString Holder;
	// Why it is in the list, with its explanation; the filter pills filter by key, in the key's color
	FString Reason;
	FString ReasonTip;
	FString ReasonKey;
	FLinearColor ReasonColor = FLinearColor::Gray;
	// Source & config rows: one place the asset is named, the file relative to the project and its line (0: none)
	FString RefFile;
	int32 RefLine = 0;
	// Reason shown as a warning (excluded, unused without its rule)
	bool bReasonWarn = false;
};

/**
 * Assets tool, folder by folder, in two lists: Unused (cleanup deletes them) and Used (why each is used, the assets
 * exclusion rules keep among them). Assets to review are in Issues. Left the folder tree with the totals of the list,
 * right the assets of the selected folder.
 */
class SPjcTabUnused final : public SCompoundWidget
{
  public:

	SLATE_BEGIN_ARGS(SPjcTabUnused) {}
	// Tabs it links to (none in tests)
	SLATE_ARGUMENT(TSharedPtr<FTabManager>, TabManager)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~SPjcTabUnused() override;

	// Shows a folder, e.g. a folder the Overview links to
	void SelectFolder(const FString& Path);

	const FString& GetSelectedFolder() const {
		return SelectedFolder;
	}

	// Shows the assets the last delete could not remove
	void ShowNotDeleted();

	// Shows one of the lists, only the assets of one reason when given (as the reason column names it)
	void ShowSet(const EPjcAssetSet InSet, const FString& Reason = FString {});

	EPjcAssetSet GetSet() const {
		return Set;
	}

  private:

	using FFolderPtr = TSharedPtr<FPjcUnusedFolder>;

	void OnScanFinished();
	void RebuildTree();
	// the lists as tabs over both panes, with the folder and what its list holds
	TSharedRef<SWidget> MakeTabsRow();
	TSharedRef<SWidget> MakeSetSwitch();
	void MakeRowReason(const FPjcScanResult& Result, FPjcUnusedAssetRow& Row);

	// folder tree
	TSharedRef<SWidget> MakeFolderPane();
	// Icon of the Tree and Size map buttons, in the button's text color
	static TSharedRef<SWidget> MakeViewIcon(const FName Brush);
	void RefreshTree();
	bool IsFolderShown(const FPjcUnusedFolder& Folder) const;
	void SortFolders(TArray<FFolderPtr>& Folders) const;
	void SetExpansionRecursive(const FFolderPtr& Folder, const bool bExpanded);
	TSharedRef<ITableRow> OnGenerateFolderRow(FFolderPtr Folder, const TSharedRef<STableViewBase>& Table);
	void OnGetFolderChildren(FFolderPtr Folder, TArray<FFolderPtr>& OutChildren);
	void OnFolderSelectionChanged(FFolderPtr Folder, ESelectInfo::Type SelectInfo);
	void OnFolderExpansionChanged(FFolderPtr Folder, bool bExpanded);
	void OnFolderSort(EColumnSortPriority::Type Priority, const FName& Column, EColumnSortMode::Type Mode);
	EColumnSortMode::Type GetFolderSortMode(const FName Column) const;
	void OnFolderSearchChanged(const FText& Text);
	TSharedPtr<SWidget> OnFolderContextMenu();
	TSharedRef<SWidget> MakeViewMenu();

	// size map of the selected folder
	TSharedRef<SWidget> MakeMapBar();
	TSharedRef<SWidget> MakeMapLegend() const;
	void SetMapView(const bool bMap);
	void RefreshMap();
	void RebuildBreadcrumb();
	void OnMapAssetClicked(int32 Index);

	// unused assets of the selected folder
	using FAssetRowPtr = TSharedPtr<FPjcUnusedAssetRow>;
	TSharedRef<SWidget> MakeAssetPane();
	void RefreshAssets();
	void ApplyAssetFilter();
	void RebuildChips();
	TSharedRef<ITableRow> OnGenerateAssetRow(FAssetRowPtr Row, const TSharedRef<STableViewBase>& Table);
	void OnAssetSort(EColumnSortPriority::Type Priority, const FName& Column, EColumnSortMode::Type Mode);
	EColumnSortMode::Type GetAssetSortMode(const FName Column) const;
	void OnAssetSearchChanged(const FText& Text);
	void OnReasonChipClicked(const FString& Key);
	// Reason keys the folder's assets have (the filters offered)
	TSet<FString> GetReasonKeys() const;
	FText GetWhereText() const;
	FText GetEmptyText() const;

	// asset list columns: headers can be dragged onto another to move there, the order is saved per user
	void RebuildHeader();
	void MoveColumn(const FName Column, const FName Before);
	void SortByColumn(const FName Column);
	static TArray<FName> GetDefaultColumns();
	TSharedPtr<SWidget> OnAssetContextMenu();
	FReply OnAssetKeyDown(const FGeometry& Geometry, const FKeyEvent& KeyEvent);

	// actions
	TArray<FName> GetSelectedPackages() const;
	void GetFolderUnused(const FString& Path, TArray<FName>& OutPackages) const;
	void ExcludeSelectedAssets();
	void ExcludeSelectedClasses();
	void ExcludeFolder(const FString& Path);
	void RemoveSelectedExclusions();
	bool HasSelectedExclusions() const;
	void InspectSelected() const;
	void BrowseSelected() const;
	void OnAssetSelectionChanged(FAssetRowPtr Row, ESelectInfo::Type SelectInfo);
	// search, the actions on the selection, View
	TSharedRef<SWidget> MakeToolbar();
	TSharedRef<SWidget> MakeListViewMenu();
	TSharedRef<SWidget> MakeExcludeMenu();
	void AddExcludeEntries(FMenuBuilder& Menu);
	FText GetSelectionText() const;
	FText GetSelectionGroupText() const;

	void SaveViewState() const;

	// the list as rows or as a grid of tiles, sharing the rows, their order and the selection
	TSharedPtr<SListView<TSharedPtr<FPjcUnusedAssetRow>>> GetView() const;
	void SetGridView(const bool bGrid);
	TSharedRef<ITableRow> OnGenerateAssetTile(TSharedPtr<FPjcUnusedAssetRow> Row, const TSharedRef<STableViewBase>& Table);
	float GetTileWidth() const;
	virtual FReply OnKeyDown(const FGeometry& Geometry, const FKeyEvent& KeyEvent) override;

	FDelegateHandle ScanFinishedHandle;
	TWeakPtr<FTabManager> TabManager;

	// list shown, and how many assets each list has
	EPjcAssetSet Set = EPjcAssetSet::Unused;
	TMap<EPjcAssetSet, int32> SetCounts;

	FPjcUnusedTree Tree;
	TArray<FFolderPtr> TreeRoots;
	TSharedPtr<STreeView<FFolderPtr>> TreeView;
	FString SelectedFolder = TEXT("/Game");
	TSet<FString> ExpandedFolders;
	bool bExpansionSynced = false;

	// folder view options
	bool bOnlyUnused = true;
	bool bShowEmpty = false;
	// thin lines down the tree under each open folder (View), on by default
	bool bFolderGuides = true;
	FName FolderSortColumn = TEXT("Size");
	EColumnSortMode::Type FolderSortMode = EColumnSortMode::Descending;
	FString FolderSearch;
	// Folders matching the search, with their parents
	TSet<FString> SearchMatches;

	// asset list
	TArray<FAssetRowPtr> FolderRows;
	TArray<FAssetRowPtr> ListRows;
	TSharedPtr<SListView<FAssetRowPtr>> ListView;
	// Filters under the class bar, one above the other: what keeps the assets in the list (Used), Delete failed (Unused)
	TSharedPtr<SVerticalBox> PillsBox;
	bool bHasPills = false;
	TSharedPtr<SPjcClassBar> ClassBar;
	// Count and size of the listed assets, for the line above the list
	FString ListSummary;
	// Delete group size per package, made when listed
	TMap<FName, int32> GroupSizes;
	// Unused assets that go with the selection
	int32 NumSelectionGroup = 0;

	// size map
	bool bMapView = false;
	bool bMapAll = false;
	TSharedPtr<SPjcSizeMap> SizeMap;
	TSharedPtr<SHorizontalBox> Breadcrumb;
	// Reason keys of the picked filter pills, none for every reason; several can be picked
	TSet<FString> ReasonFilters;
	// Only Source & config picked: a row for every place a file names the asset, with the file and the line
	bool bSourceRows = false;
	// Only assets the last delete could not remove
	bool bHeldOnly = false;
	bool bSubfolders = true;
	FString AssetSearch;
	FName AssetSortColumn = TEXT("Size");
	EColumnSortMode::Type AssetSortMode = EColumnSortMode::Descending;
	TArray<FName> ColumnOrder;
	TSharedPtr<SHeaderRow> AssetHeader;
	TSharedPtr<STileView<TSharedPtr<FPjcUnusedAssetRow>>> TileView;
	// the grid's box: its width sets how many tiles fit in a line
	TSharedPtr<SBox> TileBox;
	bool bGridView = false;
	// Small, Medium, Large, Huge
	int32 TileSize = 1;
};
