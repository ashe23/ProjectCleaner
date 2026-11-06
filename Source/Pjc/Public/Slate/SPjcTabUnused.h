// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PjcUnused.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/STreeView.h"

class FMenuBuilder;
class SPjcSizeMap;
class FTabManager;
class SBox;
class SHorizontalBox;
struct FSlateBrush;

/** Unused asset in the list of the Assets Unused tool */
struct FPjcUnusedAssetRow
{
	/** Index into FPjcScanResult::Assets */
	int32 Index = INDEX_NONE;
	FName Package;
	FString Name;
	FName ClassName;
	/** Type name as the Content Browser shows it */
	FString ClassLabel;
	FLinearColor Color = FLinearColor::White;
	const FSlateBrush* Icon = nullptr;
	int64 Size = 0;
	/** Folder relative to the selected folder */
	FString Folder;
	/** Only a folder label keeps it, while Folder labels is on */
	bool bLabel = false;
	/** Unused assets that reference it and are deleted with it */
	int32 Group = 0;
};

/**
 * Assets Unused tool: cleanup folder by folder. Left the folder tree with unused totals, right the unused assets of
 * the selected folder. Used assets are not listed (the Content Browser has them).
 */
class SPjcTabUnused final : public SCompoundWidget
{
  public:

	SLATE_BEGIN_ARGS(SPjcTabUnused) {}
	/** Tabs it links to (none in tests) */
	SLATE_ARGUMENT(TSharedPtr<FTabManager>, TabManager)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~SPjcTabUnused() override;

	/** Shows a folder, e.g. a folder the Overview links to */
	void SelectFolder(const FString& Path);

	const FString& GetSelectedFolder() const {
		return SelectedFolder;
	}

  private:

	using FFolderPtr = TSharedPtr<FPjcUnusedFolder>;

	void OnScanFinished();

	// folder tree
	TSharedRef<SWidget> MakeFolderPane();
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
	struct FTypeInfo
	{
		FString Label;
		FLinearColor Color;
		const FSlateBrush* Icon = nullptr;
	};

	TSharedRef<SWidget> MakeAssetPane();
	void RefreshAssets();
	void ApplyAssetFilter();
	void RebuildChips();
	const FTypeInfo& GetTypeInfo(const FAssetData& Asset);
	TSharedRef<ITableRow> OnGenerateAssetRow(FAssetRowPtr Row, const TSharedRef<STableViewBase>& Table);
	void OnAssetSort(EColumnSortPriority::Type Priority, const FName& Column, EColumnSortMode::Type Mode);
	EColumnSortMode::Type GetAssetSortMode(const FName Column) const;
	void OnAssetSearchChanged(const FText& Text);
	void OnChipClicked(const FName ClassName);
	FText GetWhereText() const;
	FText GetWhereInfoText() const;
	TSharedPtr<SWidget> OnAssetContextMenu();
	FReply OnAssetKeyDown(const FGeometry& Geometry, const FKeyEvent& KeyEvent);

	// actions
	TArray<FName> GetSelectedPackages() const;
	/** Asks, then deletes the packages with the unused assets that reference them, and scans again */
	void DeleteWithGroup(const TArray<FName>& Packages);
	void GetFolderUnused(const FString& Path, TArray<FName>& OutPackages) const;
	/** Adds exclusions to the project settings (and the settings draft), saves DefaultEditor.ini, scans again */
	void Exclude(const TArray<FString>& Folders, const TArray<FString>& Classes, const TArray<FString>& Assets, const FString& Message);
	void ExcludeSelectedAssets();
	void ExcludeSelectedClasses();
	void ExcludeFolder(const FString& Path);
	void InspectSelected() const;
	void BrowseSelected() const;
	void OnAssetSelectionChanged(FAssetRowPtr Row, ESelectInfo::Type SelectInfo);
	TSharedRef<SWidget> MakeFooter();
	TSharedRef<SWidget> MakeExcludeMenu();
	void AddExcludeEntries(FMenuBuilder& Menu);
	FText GetSelectionText() const;
	FText GetSelectionGroupText() const;

	void SaveViewState() const;

	FDelegateHandle ScanFinishedHandle;
	TWeakPtr<FTabManager> TabManager;

	FPjcUnusedTree Tree;
	TArray<FFolderPtr> TreeRoots;
	TSharedPtr<STreeView<FFolderPtr>> TreeView;
	FString SelectedFolder = TEXT("/Game");
	TSet<FString> ExpandedFolders;
	bool bExpansionSynced = false;

	// folder view options
	bool bOnlyUnused = true;
	bool bShowEmpty = false;
	FName FolderSortColumn = TEXT("Size");
	EColumnSortMode::Type FolderSortMode = EColumnSortMode::Descending;
	FString FolderSearch;
	/** Folders matching the search, with their parents */
	TSet<FString> SearchMatches;

	// asset list
	TArray<FAssetRowPtr> FolderRows;
	TArray<FAssetRowPtr> ListRows;
	TSharedPtr<SListView<FAssetRowPtr>> ListView;
	TSharedPtr<SHorizontalBox> ChipsBox;
	TMap<FName, FTypeInfo> TypeInfos;
	/** Assets only folder labels keep (unused while Folder labels is off) */
	TSet<FName> LabelOnly;
	/** Delete group size per package, made when listed */
	TMap<FName, int32> GroupSizes;
	/** Unused assets that go with the selection */
	int32 NumSelectionGroup = 0;

	// size map
	bool bMapView = false;
	bool bMapAll = false;
	TSharedPtr<SPjcSizeMap> SizeMap;
	TSharedPtr<SHorizontalBox> Breadcrumb;
	TSet<FName> ClassFilter;
	bool bSubfolders = true;
	FString AssetSearch;
	FName AssetSortColumn = TEXT("Size");
	EColumnSortMode::Type AssetSortMode = EColumnSortMode::Descending;
};
