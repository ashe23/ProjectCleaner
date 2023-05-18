// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ContentBrowserDelegates.h"
#include "Widgets/SCompoundWidget.h"

struct FPjcFileInfo;

class SPjcTabAssetsIndirect final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SPjcTabAssetsIndirect) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

protected:
	TSharedRef<SWidget> CreateToolbar() const;
	TSharedRef<SHeaderRow> GetHeaderRow();
	TSharedRef<ITableRow> OnGenerateRow(TSharedPtr<FPjcFileInfo> Item, const TSharedRef<STableViewBase>& OwnerTable) const;

private:
	TArray<TSharedPtr<FPjcFileInfo>> Items;
	TSharedPtr<SListView<TSharedPtr<FPjcFileInfo>>> ListView;

	EColumnSortMode::Type ColumnSortModeFilePath = EColumnSortMode::None;
	EColumnSortMode::Type ColumnSortModeFileNum = EColumnSortMode::None;

	FARFilter Filter;
	TSharedPtr<FUICommandList> Cmds;
	TSet<FAssetData> AssetsIndirect;
	FSetARFilterDelegate DelegateFilter;
	FRefreshAssetViewDelegate DelegateRefreshView;
	FGetCurrentSelectionDelegate DelegateSelection;
};
