// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

struct FPjcAssetBrowserStatItem;

class SPjcAssetBrowser : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SPjcAssetBrowser) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	TSharedRef<ITableRow> OnStatGenerateRow(TSharedPtr<FPjcAssetBrowserStatItem> Item, const TSharedRef<STableViewBase>& OwnerTable) const;
	TSharedRef<SHeaderRow> GetStatHeaderRow() const;

	TArray<TSharedPtr<FPjcAssetBrowserStatItem>> StatItems;
	TSharedPtr<SListView<TSharedPtr<FPjcAssetBrowserStatItem>>> StatListView;
};
