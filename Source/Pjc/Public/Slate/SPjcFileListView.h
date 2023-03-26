// // Copyright Ashot Barkhudaryan. All Rights Reserved.
//
// #pragma once
//
// #include "CoreMinimal.h"
// #include "PjcDelegates.h"
// #include "Widgets/SCompoundWidget.h"
//
// struct FPjcFileListViewItem
// {
// 	int64 FileSize;
// 	FString FileName;
// 	FString FilePath;
// 	FString FileExt;
// };
//
// class SPjcFileListViewItem final : public SMultiColumnTableRow<TSharedPtr<FPjcFileListViewItem>>
// {
// public:
// 	SLATE_BEGIN_ARGS(SPjcFileListViewItem) {}
// 		SLATE_ARGUMENT(TSharedPtr<FPjcFileListViewItem>, Item)
// 	SLATE_END_ARGS()
//
// 	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InTable);
// 	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& InColumnName) override;
//
// private:
// 	TSharedPtr<FPjcFileListViewItem> Item;
// };
//
// class SPjcFileListView final : public SCompoundWidget
// {
// public:
// 	SLATE_BEGIN_ARGS(SPjcFileListView) {}
// 		SLATE_ARGUMENT(bool, OpenFileOnDblClick)
// 	SLATE_END_ARGS()
//
// 	void Construct(const FArguments& InArgs);
// 	void UpdateData(const TArray<FString>& InFiles);
// 	
// 	FPjcDelegateRequestedFilesDelete& OnFilesDeleteRequest();
// private:
// 	void CmdsRegister();
// 	void OnListItemDblClick(TSharedPtr<FPjcFileListViewItem> Item) const;
// 	void OnListSort(EColumnSortPriority::Type SortPriority, const FName& ColumnName, EColumnSortMode::Type SortMode);
// 	FText GetListSummaryText() const;
//
// 	TSharedRef<SHeaderRow> GetHeaderRow();
// 	TSharedPtr<SWidget> OnListContextMenu() const;
// 	TSharedRef<ITableRow> OnGenerateRow(TSharedPtr<FPjcFileListViewItem> Item, const TSharedRef<STableViewBase>& OwnerTable) const;
//
// 	EColumnSortMode::Type ColumnFileNameSortMode = EColumnSortMode::None;
// 	EColumnSortMode::Type ColumnFileExtSortMode = EColumnSortMode::None;
// 	EColumnSortMode::Type ColumnFileSizeSortMode = EColumnSortMode::None;
// 	EColumnSortMode::Type ColumnFilePathSortMode = EColumnSortMode::None;
//
// 	bool bOpenFileOnDblClick = false;
//
// 	int64 TotalSize = 0;
// 	TSharedPtr<FUICommandList> Cmds;
// 	TArray<TSharedPtr<FPjcFileListViewItem>> ListItems;
// 	TSharedPtr<SListView<TSharedPtr<FPjcFileListViewItem>>> ListView;
// 	FPjcDelegateRequestedFilesDelete DelegateRequestedFilesDelete;
// };
