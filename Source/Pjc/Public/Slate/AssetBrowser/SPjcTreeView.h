// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

struct FPjcTreeViewItem;

class SPjcTreeView final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SPjcTreeView) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	void TreeViewListUpdate();

private:
	TSharedRef<SHeaderRow> GetTreeViewHeaderRow() const;
	TSharedRef<ITableRow> OnTreeViewGenerateRow(TSharedPtr<FPjcTreeViewItem> Item, const TSharedRef<STableViewBase>& OwnerTable) const;
	TSharedPtr<FPjcTreeViewItem> CreateTreeItem(const FString& InPath) const;
	TSharedRef<SWidget> GetTreeViewOptionsBtnContent() const;
	void OnTreeViewGetChildren(TSharedPtr<FPjcTreeViewItem> Item, TArray<TSharedPtr<FPjcTreeViewItem>>& OutChildren);
	
	FSlateColor GetTreeViewOptionsBtnForegroundColor() const;
	FText GetSummaryText() const;
	// void TreeViewDataUpdate();
	// void OnTreeViewSelectionChange(TSharedPtr<FPjcTreeViewItem> Item, ESelectInfo::Type SelectInfo) const;
	// void OnTreeViewExpansionChange(TSharedPtr<FPjcTreeViewItem> Item, bool bExpansion) const;
	// void OnTreeViewSearchTextChanged(const FText& InText);
	// void OnTreeViewSearchTextCommitted(const FText& InText, ETextCommit::Type CommitType);

	FString SearchText;
	TSharedPtr<SComboButton> TreeViewOptionBtn;
	TArray<TSharedPtr<FPjcTreeViewItem>> TreeViewItems;
	TSharedPtr<STreeView<TSharedPtr<FPjcTreeViewItem>>> TreeView;
};
