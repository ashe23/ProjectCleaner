// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PjcIssues.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"

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
	TSharedRef<SWidget> MakeFooter();
	FText GetTitle() const;
	FText GetDescription() const;
	FText GetSelectionText() const;
	TArray<FRowPtr> GetSelected() const;

	// redirectors
	void FixRedirectors(const TArray<int32>& Indices);

	// files and folders
	void RebuildChips();
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
	// Kept files or assets to review of one reason, empty for all
	FString ChipFilter;
	// Assets to review, and how many of each reason
	int32 NumReview = 0;
	TMap<FString, int32> ReviewReasons;
	TSharedPtr<SHorizontalBox> Chips;
	TSharedPtr<SBox> ListBox;
	TSharedPtr<SListView<FRowPtr>> ListView;
};
