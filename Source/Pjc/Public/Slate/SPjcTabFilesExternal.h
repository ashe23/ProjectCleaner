// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

struct FPjcStatItem;
struct FPjcFileExternalItem;

class SPjcTabFilesExternal final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SPjcTabFilesExternal) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

protected:
	void StatsInit();
	void StatsUpdate();
	void ListUpdate();
	
	TSharedRef<SWidget> CreateToolbar() const;
	TSharedRef<SHeaderRow> GetStatHeaderRow() const;
	TSharedRef<SHeaderRow> GetListHeaderRow() const;
	TSharedRef<ITableRow> OnStatGenerateRow(TSharedPtr<FPjcStatItem> Item, const TSharedRef<STableViewBase>& OwnerTable) const;
	TSharedRef<ITableRow> OnListGenerateRow(TSharedPtr<FPjcFileExternalItem> Item, const TSharedRef<STableViewBase>& OwnerTable) const;
private:
	TSharedPtr<FUICommandList> Cmds;
	TArray<TSharedPtr<FPjcStatItem>> StatItems;
	TArray<TSharedPtr<FPjcFileExternalItem>> ListItems;
	TSharedPtr<SListView<TSharedPtr<FPjcStatItem>>> StatView;
	TSharedPtr<SListView<TSharedPtr<FPjcFileExternalItem>>> ListView;

	EColumnSortMode::Type ColumnSortModeFilePath = EColumnSortMode::None;
	EColumnSortMode::Type ColumnSortModeFileName = EColumnSortMode::None;
	EColumnSortMode::Type ColumnSortModeFileExt = EColumnSortMode::None;
	EColumnSortMode::Type ColumnSortModeFileSize = EColumnSortMode::None;
};
