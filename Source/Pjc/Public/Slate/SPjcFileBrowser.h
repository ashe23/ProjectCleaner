// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class UPjcSubsystem;

enum class EPjcFileType : uint8
{
	External,
	Corrupted
};

class FPjcFileBrowserItem
{
public:
	int64 FileSize = 0;
	FString FileName;
	FString FilePathAbs;
	FString FileExtension;
	EPjcFileType FileType;
};

class SPjcFileBrowserItem final : public SMultiColumnTableRow<TSharedPtr<FPjcFileBrowserItem>>
{
public:
	SLATE_BEGIN_ARGS(SPjcFileBrowserItem) {}
		SLATE_ARGUMENT(TSharedPtr<FPjcFileBrowserItem>, Item)
		SLATE_ARGUMENT(FString, SearchText)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InTable);
	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& InColumnName) override;

private:
	TSharedPtr<FPjcFileBrowserItem> Item;
	FString SearchText;
};


class SPjcFileBrowser final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SPjcFileBrowser) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	void ListDataUpdate();
	void ListUpdate();
	void OnSearchTextChanged(const FText& InText);
	void OnSearchTextCommitted(const FText& InText, ETextCommit::Type);
	void OnListSort(EColumnSortPriority::Type SortPriority, const FName& ColumnName, EColumnSortMode::Type SortMode);
	void OnListItemDblClick(TSharedPtr<FPjcFileBrowserItem> Item) const;
	FText GetListSummaryText() const;
	TSharedPtr<FPjcFileBrowserItem> CreateListItem(const FString& InFilePath) const;
	TSharedRef<SHeaderRow> GetHeaderRow();
	TSharedRef<ITableRow> OnGenerateRow(TSharedPtr<FPjcFileBrowserItem> Item, const TSharedRef<STableViewBase>& OwnerTable) const;
	FSlateColor GetViewOptionsForegroundColor() const;
	TSharedRef<SWidget> GetViewOptionsBtnContent();
	FReply OnBtnScanFilesClick();
	FReply OnBtnDeleteFilesClick();
	FReply OnBtnClearSelectionClick() const;
	bool IsAnyItemSelected() const;
	bool IsListViewEnabled() const;
	int32 GetListViewWidgetIndex() const;

	EColumnSortMode::Type ColumnFileNameSortMode = EColumnSortMode::None;
	EColumnSortMode::Type ColumnFileTypeSortMode = EColumnSortMode::None;
	EColumnSortMode::Type ColumnFileExtSortMode = EColumnSortMode::None;
	EColumnSortMode::Type ColumnFileSizeSortMode = EColumnSortMode::None;
	EColumnSortMode::Type ColumnFilePathSortMode = EColumnSortMode::None;
	FString SearchText;
	int64 TotalSize = 0;
	TArray<FString> Files; // todo:ashe23 this should be in subsystem class, so its caches scan data when user closes and then reopens it
	TSharedPtr<SComboButton> ViewOptionsBtn;
	TArray<TSharedPtr<FPjcFileBrowserItem>> ListItems;
	TSharedPtr<SListView<TSharedPtr<FPjcFileBrowserItem>>> ListView;
};
