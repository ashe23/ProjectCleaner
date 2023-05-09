// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PjcTypes.h"
#include "Widgets/SCompoundWidget.h"

class SPjcTreeView final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SPjcTreeView) {}
		SLATE_ARGUMENT(FMargin, HeaderPadding)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

protected:
	TSharedRef<SWidget> GetTreeBtnActionsContent();
	TSharedRef<SWidget> GetTreeBtnOptionsContent();
	TSharedRef<SHeaderRow> GetTreeHeaderRow() const;
	TSharedRef<ITableRow> OnTreeGenerateRow(TSharedPtr<FPjcTreeItem> Item, const TSharedRef<STableViewBase>& OwnerTable) const;
	void OnTreeGetChildren(TSharedPtr<FPjcTreeItem> Item, TArray<TSharedPtr<FPjcTreeItem>>& OutChildren);
	void OnTreeSearchTextChanged(const FText& InText);
	void OnTreeSearchTextCommitted(const FText& InText, ETextCommit::Type Type);
	FText GetTreeSummaryText() const;
	FSlateColor GetTreeOptionsBtnForegroundColor() const;

private:
	FText SearchText;
	FMargin HeaderPadding;
	TSharedPtr<SComboButton> TreeOptionBtn;
	TArray<TSharedPtr<FPjcTreeItem>> TreeItems;
	TSharedPtr<STreeView<TSharedPtr<FPjcTreeItem>>> TreeView;
};
