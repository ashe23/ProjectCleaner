// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class UPjcSubsystem;
struct FPjcFileBrowserItem;

class SPjcFileBrowser final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SPjcFileBrowser) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~SPjcFileBrowser() override;
private:
	TSharedPtr<FPjcFileBrowserItem> CreateListItem(const FString& InFilePath) const;
	TSharedRef<SHeaderRow> GetHeaderRow();
	TSharedRef<ITableRow> OnGenerateRow(TSharedPtr<FPjcFileBrowserItem> Item, const TSharedRef<STableViewBase>& OwnerTable) const;

	FReply OnBtnScanFilesClicked() const;
	FReply OnBtnDeleteFilesClicked();
	FReply OnBtnClearSelectionClicked() const;

	void OnScanFiles();
	void OnSearchTextChanged(const FText&);
	void OnSearchTextCommitted(const FText&, ETextCommit::Type);
	void OnListItemDblClick(TSharedPtr<FPjcFileBrowserItem> Item) const;
	void OnListSort(EColumnSortPriority::Type SortPriority, const FName& ColumnName, EColumnSortMode::Type SortMode);
	void ListClearSelection() const;
	void ListItemsUpdate();
	void ListViewUpdate();

	bool IsAnyItemSelected() const;
	bool DeleteFile(const FString& InFilePath);

	int32 GetWidgetIndex() const;

	FText GetSummary() const;

	FString SearchText;
	int64 TotalSize = 0;
	UPjcSubsystem* SubsystemPtr = nullptr;
	TArray<TSharedPtr<FPjcFileBrowserItem>> ListItems;
	TArray<TSharedPtr<FPjcFileBrowserItem>> ListItemsCached;
	TSharedPtr<SListView<TSharedPtr<FPjcFileBrowserItem>>> ListView;
	EColumnSortMode::Type ColumnFileNameSortMode = EColumnSortMode::None;
	EColumnSortMode::Type ColumnFileTypeSortMode = EColumnSortMode::None;
	EColumnSortMode::Type ColumnFileExtSortMode = EColumnSortMode::None;
	EColumnSortMode::Type ColumnFileSizeSortMode = EColumnSortMode::None;
	EColumnSortMode::Type ColumnFilePathSortMode = EColumnSortMode::None;
};
