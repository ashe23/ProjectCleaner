// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

struct FPjcFileExternalItem;

class SPjcTabFilesExternal final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SPjcTabFilesExternal) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

protected:
	void ListUpdate();
	
	TSharedRef<SWidget> CreateToolbar() const;
	TSharedRef<SWidget> GetBtnOptionsContent();
	TSharedRef<SHeaderRow> GetListHeaderRow() const;
	TSharedRef<ITableRow> OnListGenerateRow(TSharedPtr<FPjcFileExternalItem> Item, const TSharedRef<STableViewBase>& OwnerTable) const;
	FSlateColor GetOptionsBtnForegroundColor() const;
private:
	TSharedPtr<FUICommandList> Cmds;
	TSharedPtr<SComboButton> OptionBtn;
	TArray<TSharedPtr<FPjcFileExternalItem>> ListItems;
	TSharedPtr<SListView<TSharedPtr<FPjcFileExternalItem>>> ListView;

	EColumnSortMode::Type ColumnSortModeFilePath = EColumnSortMode::None;
	EColumnSortMode::Type ColumnSortModeFileName = EColumnSortMode::None;
	EColumnSortMode::Type ColumnSortModeFileExt = EColumnSortMode::None;
	EColumnSortMode::Type ColumnSortModeFileSize = EColumnSortMode::None;
};
