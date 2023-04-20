// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class SPjcContentBrowser;
class SPjcTreeView;
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

	FReply OnBtnScanAssetsClick() const;

	TSharedPtr<SPjcTreeView> TreeView;
	TSharedPtr<SPjcContentBrowser> ContentBrowser;
	TArray<TSharedPtr<FPjcAssetBrowserStatItem>> StatItems;
	TSharedPtr<SListView<TSharedPtr<FPjcAssetBrowserStatItem>>> StatListView;
};
