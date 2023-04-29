// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PjcTypes.h"
#include "Widgets/SCompoundWidget.h"

class SPjcTabAssetsUnused final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SPjcTabAssetsUnused) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	void StatItemsInit();
	TSharedRef<SWidget> CreateToolbar() const;
	TSharedRef<ITableRow> OnStatGenerateRow(TSharedPtr<FPjcStatItem> Item, const TSharedRef<STableViewBase>& OwnerTable) const;
	TSharedRef<SHeaderRow> GetStatHeaderRow() const;

	TSharedPtr<FUICommandList> Cmds;
	TArray<TSharedPtr<FPjcStatItem>> StatItems;
	TSharedPtr<SListView<TSharedPtr<FPjcStatItem>>> StatView;
};
