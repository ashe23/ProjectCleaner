// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

struct FPjcStatItem;

class SPjcStatAssets final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SPjcStatAssets) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	void UpdateView();

protected:
	TSharedRef<SHeaderRow> GetHeaderRow() const;
	TSharedRef<ITableRow> OnGenerateRow(TSharedPtr<FPjcStatItem> Item, const TSharedRef<STableViewBase>& OwnerTable) const;

private:
	TArray<TSharedPtr<FPjcStatItem>> Items;
	TSharedPtr<SListView<TSharedPtr<FPjcStatItem>>> ListView;
};
