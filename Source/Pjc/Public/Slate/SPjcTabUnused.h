// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PjcUnused.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/STreeView.h"

class FTabManager;
class SBox;

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

	// unused assets of the selected folder
	TSharedRef<SWidget> MakeAssetPane();
	FText GetWhereText() const;

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
};
