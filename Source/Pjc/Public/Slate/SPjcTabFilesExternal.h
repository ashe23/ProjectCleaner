// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class UPjcSubsystem;
struct FPjcFileExternalItem;

class SPjcTabFilesExternal final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SPjcTabFilesExternal) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

protected:
	void ListUpdateData();
	void ListUpdateView();
	void OnListSort(EColumnSortPriority::Type SortPriority, const FName& ColumnName, EColumnSortMode::Type InSortMode);
	void OnSearchTextChanged(const FText&);
	void OnSearchTextCommitted(const FText&, ETextCommit::Type);

	TSharedRef<SWidget> CreateToolbar() const;
	TSharedRef<SWidget> GetBtnOptionsContent();
	TSharedRef<SHeaderRow> GetListHeaderRow();
	TSharedRef<ITableRow> OnListGenerateRow(TSharedPtr<FPjcFileExternalItem> Item, const TSharedRef<STableViewBase>& OwnerTable) const;
	TSharedPtr<SWidget> OnContextMenuOpening() const;
	FSlateColor GetOptionsBtnForegroundColor() const;
	FText GetTxtSummary() const;
	FText GetTxtSelection() const;

private:
	FText SearchText;
	int32 NumFilesTotal = 0;
	int32 NumFilesExcluded = 0;
	TSharedPtr<FUICommandList> Cmds;
	TSharedPtr<SComboButton> OptionBtn;
	UPjcSubsystem* SubsystemPtr = nullptr;
	TArray<TSharedPtr<FPjcFileExternalItem>> ItemsFiltered;
	TArray<TSharedPtr<FPjcFileExternalItem>> ItemsAll;
	TSharedPtr<SListView<TSharedPtr<FPjcFileExternalItem>>> ListView;

	EColumnSortMode::Type ColumnSortModeFilePath = EColumnSortMode::None;
	EColumnSortMode::Type ColumnSortModeFileName = EColumnSortMode::None;
	EColumnSortMode::Type ColumnSortModeFileExt = EColumnSortMode::None;
	EColumnSortMode::Type ColumnSortModeFileSize = EColumnSortMode::None;
};
