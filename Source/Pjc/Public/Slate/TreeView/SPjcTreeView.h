// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PjcDelegates.h"
#include "Widgets/SCompoundWidget.h"

class UPjcSubsystem;
struct FPjcTreeViewItem;

class SPjcTreeView final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SPjcTreeView) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~SPjcTreeView() override;

	FPjcDelegatePathSelectionChanged& OnPathSelectionChanged();
private:
	void OnScanAssets();
	void TreeViewListUpdate();
	TSharedRef<SHeaderRow> GetTreeViewHeaderRow() const;
	TSharedRef<ITableRow> OnTreeViewGenerateRow(TSharedPtr<FPjcTreeViewItem> Item, const TSharedRef<STableViewBase>& OwnerTable) const;
	TSharedPtr<FPjcTreeViewItem> CreateTreeItem(const FString& InPath) const;
	TSharedRef<SWidget> GetTreeViewOptionsBtnContent() const;
	void OnTreeViewGetChildren(TSharedPtr<FPjcTreeViewItem> Item, TArray<TSharedPtr<FPjcTreeViewItem>>& OutChildren);
	void OnTreeViewSelectionChange(TSharedPtr<FPjcTreeViewItem> Item, ESelectInfo::Type SelectInfo) const;
	
	FSlateColor GetTreeViewOptionsBtnForegroundColor() const;
	FText GetSummaryText() const;
	// void TreeViewDataUpdate();
	// void OnTreeViewSelectionChange(TSharedPtr<FPjcTreeViewItem> Item, ESelectInfo::Type SelectInfo) const;
	// void OnTreeViewExpansionChange(TSharedPtr<FPjcTreeViewItem> Item, bool bExpansion) const;
	// void OnTreeViewSearchTextChanged(const FText& InText);
	// void OnTreeViewSearchTextCommitted(const FText& InText, ETextCommit::Type CommitType);

	FString SearchText;
	UPjcSubsystem* SubsystemPtr = nullptr;
	TSharedPtr<SComboButton> TreeViewOptionBtn;
	TArray<TSharedPtr<FPjcTreeViewItem>> TreeViewItems;
	TSharedPtr<STreeView<TSharedPtr<FPjcTreeViewItem>>> TreeView;
	TMap<FString, int64> SizesByPaths;
	TMap<FString, float> PercentageByPaths;
	TMap<FString, int32> NumAssetsTotalByPaths;
	TMap<FString, int32> NumAssetsUsedByPaths;
	TMap<FString, int32> NumAssetsUnusedByPaths;
	FPjcDelegatePathSelectionChanged DelegatePathSelectionChanged;
};
