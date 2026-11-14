// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PjcIssues.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"

class FMenuBuilder;
class FTabManager;
class SBox;
class SHorizontalBox;
class SVerticalBox;

enum class EPjcIssueCategory : uint8
{
	Redirectors,
	External,
	Corrupted,
	Empty,
	// assets the scan is not sure about (user, 2026-11-14: moved here from Assets), listed first
	Review,
};

// Row of an Issues list: the first column is a name with a tag and a muted path, then plain cells
struct FPjcIssueRow
{
	// Package name or absolute path
	FString Id;
	FString Name;
	FString Sub;
	FString Tag;
	FName TagColor;
	FString TagToolTip;
	TArray<FString> Cells;
	TArray<FString> CellSubs;
	TArray<FName> CellColors;
	FString ToolTip;
	int64 Size = -1;
	// Index into the redirectors
	int32 Redirector = INDEX_NONE;
	// Assets to review: the asset, index into FPjcScanResult::Assets
	FName Package;
	int32 Asset = INDEX_NONE;
	// Why a kept file is kept
	FString Reason;
	// What the filters of the list match: reason to review, redirector state, why kept
	FString FilterKey;
};

/**
 * Issues tool: what the scan found besides unused assets. Assets to review, redirectors, external files, corrupted files
 * and empty folders, each with its list and the actions that fix it.
 */
class SPjcTabIssues final : public SCompoundWidget
{
  public:

	SLATE_BEGIN_ARGS(SPjcTabIssues) {}
	// Tabs it links to (none in tests)
	SLATE_ARGUMENT(TSharedPtr<FTabManager>, TabManager)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~SPjcTabIssues() override;

	// Shows a category; for external files the kept ones when asked, for assets to review only one reason when given
	void ShowCategory(const EPjcIssueCategory InCategory, const bool bShowKept = false, const FString& InReason = FString {});

	EPjcIssueCategory GetCategory() const {
		return Category;
	}

	int32 GetNumRows() const {
		return Rows.Num();
	}

  private:

	using FRowPtr = TSharedPtr<FPjcIssueRow>;

	struct FColumn
	{
		FName Id;
		FString Label;
		// Fixed width, 0 fills with Fill
		float Width = 0.0f;
		float Fill = 1.0f;
		bool bRight = false;
		bool bMono = false;
		FString ToolTip;
	};

	void OnScanFinished();
	void RebuildRows();
	void RebuildList();
	void ApplyFilter();
	int32 GetCount(const EPjcIssueCategory InCategory) const;
	TArray<FColumn> GetColumns() const;

	TSharedRef<SWidget> MakeCategories();
	TSharedRef<SWidget> MakeHeader();
	// search, the actions of the category with icons, the selection
	TSharedRef<SWidget> MakeToolbar();
	FText GetCountText() const;
	FText GetTitle() const;
	FText GetDescription() const;
	FText GetSelectionText() const;
	TArray<FRowPtr> GetSelected() const;

	// redirectors
	void FixRedirectors(const TArray<int32>& Indices);

	// filters of the category, under the categories: several can be picked
	void RebuildFilters();
	void OnFilterClicked(const FString& Key);
	// sort by a column, right-click menu, Delete key (as Assets)
	void OnSort(EColumnSortPriority::Type Priority, const FName& Column, EColumnSortMode::Type Mode);
	TSharedPtr<SWidget> OnContextMenu();
	FReply OnListKeyDown(const FGeometry& Geometry, const FKeyEvent& KeyEvent);
	void AddExcludeEntries(FMenuBuilder& Menu);

	// files and folders
	void DeleteRows(const TArray<FRowPtr>& InRows);
	TSharedRef<SWidget> MakeExcludeMenu();
	// Assets to review: kept for good (exclusions), deleted, or inspected
	void KeepAssets(const TArray<FRowPtr>& InRows);
	void DeleteAssets(const TArray<FRowPtr>& InRows);
	// Adds file exclusions to the project settings (DefaultEditor.ini) and scans again
	void ExcludeFiles(const TArray<FString>& Files, const TArray<FString>& Extensions, const TArray<FString>& Folders, const FString& What);

	FDelegateHandle ScanFinishedHandle;
	TWeakPtr<FTabManager> TabManager;

	EPjcIssueCategory Category = EPjcIssueCategory::Redirectors;
	TArray<FPjcRedirectorInfo> Redirectors;
	// Redirectors the last fix kept, and why
	TMap<FName, FString> FixKept;

	TArray<FRowPtr> AllRows;
	TArray<FRowPtr> Rows;
	FString Search;
	// External files: kept ones instead of those to review
	bool bKept = false;
	// Picked filters (reason, state, why kept), none for all
	TSet<FString> Filters;
	// Every filter the category offers: all picked is all
	TSet<FString> FilterKeys;
	TSharedPtr<SVerticalBox> FilterBox;
	bool bHasFilters = false;
	// Sorted by this column (index), none: as found
	int32 SortColumn = INDEX_NONE;
	EColumnSortMode::Type SortMode = EColumnSortMode::Ascending;
	// Assets to review, and how many of each reason
	int32 NumReview = 0;
	TMap<FString, int32> ReviewReasons;
	TSharedPtr<SBox> ListBox;
	TSharedPtr<SListView<FRowPtr>> ListView;
};
