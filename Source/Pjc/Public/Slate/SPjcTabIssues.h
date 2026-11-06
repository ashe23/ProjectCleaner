// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PjcIssues.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"

class FTabManager;
class SBox;
class SVerticalBox;

enum class EPjcIssueCategory : uint8
{
	Redirectors,
	External,
	Corrupted,
	Empty,
};

/** Row of an Issues list: the first column is a name with a tag and a muted path, then plain cells */
struct FPjcIssueRow
{
	/** Package name or absolute path */
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
	/** Index into the redirectors */
	int32 Redirector = INDEX_NONE;
	/** Why a kept file is kept */
	FString Reason;
};

/**
 * Issues tool: what the scan found besides unused assets. Redirectors, external files, corrupted files and empty
 * folders, each with its list and the actions that fix it.
 */
class SPjcTabIssues final : public SCompoundWidget
{
  public:

	SLATE_BEGIN_ARGS(SPjcTabIssues) {}
	/** Tabs it links to (none in tests) */
	SLATE_ARGUMENT(TSharedPtr<FTabManager>, TabManager)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~SPjcTabIssues() override;

	void ShowCategory(const EPjcIssueCategory InCategory);

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
		/** Fixed width, 0 fills with Fill */
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

	FDelegateHandle ScanFinishedHandle;
	TWeakPtr<FTabManager> TabManager;

	EPjcIssueCategory Category = EPjcIssueCategory::Redirectors;
	TArray<FPjcRedirectorInfo> Redirectors;
	/** Redirectors the last fix kept, and why */
	TMap<FName, FString> FixKept;

	TArray<FRowPtr> AllRows;
	TArray<FRowPtr> Rows;
	FString Search;
	TSharedPtr<SBox> ListBox;
	TSharedPtr<SListView<FRowPtr>> ListView;
};
