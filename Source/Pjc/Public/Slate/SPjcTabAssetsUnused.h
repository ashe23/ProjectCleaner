// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PjcTypes.h"
#include "Widgets/SCompoundWidget.h"

class UPjcSubsystem;

class SPjcTabAssetsUnused final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SPjcTabAssetsUnused) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	TSharedRef<SWidget> CreateToolbar() const;
	TSharedRef<SWidget> GetTreeOptionsBtnContent();
	TSharedPtr<SWidget> GetTreeContextMenu() const;
	TSharedPtr<SWidget> GetContentBrowserContextMenu(const TArray<FAssetData>& Assets) const;
	TSharedRef<SHeaderRow> GetStatHeaderRow() const;
	TSharedRef<SHeaderRow> GetTreeHeaderRow() const;
	TSharedRef<ITableRow> OnStatGenerateRow(TSharedPtr<FPjcStatItem> Item, const TSharedRef<STableViewBase>& OwnerTable) const;
	TSharedRef<ITableRow> OnTreeGenerateRow(TSharedPtr<FPjcTreeItem> Item, const TSharedRef<STableViewBase>& OwnerTable) const;
	TSharedPtr<FPjcTreeItem> CreateTreeItem(const FString& InFolderPath) const;
	FSlateColor GetTreeOptionsBtnForegroundColor() const;
	FText GetTreeSummaryText() const;
	void ScanProjectAssets();
	void StatItemsUpdate();
	void TreeItemsUpdate();
	void TreeItemsFilter();
	void TreeItemsCollapseAll();
	void TreeItemsExpandAll();
	void TreeItemExpandParentsRecursive(const TSharedPtr<FPjcTreeItem>& Item) const;
	void TreeItemMakeVisibleParentsRecursive(const TSharedPtr<FPjcTreeItem>& Item) const;
	void SetTreeItemVisibility(const TSharedPtr<FPjcTreeItem>& Item) const;
	void SetTreeItemExpansion(const TSharedPtr<FPjcTreeItem>& Item);
	void OnTreeGetChildren(TSharedPtr<FPjcTreeItem> Item, TArray<TSharedPtr<FPjcTreeItem>>& OutChildren);
	void OnTreeExpansionChanged(TSharedPtr<FPjcTreeItem> Item, const bool bIsExpanded) const;
	void OnTreeSearchTextChanged(const FText& InText);
	void OnTreeSearchTextCommitted(const FText& InText, ETextCommit::Type Type);
	bool TreeItemContainsSearchText(const TSharedPtr<FPjcTreeItem>& Item) const;

	FText SearchText;
	TSharedPtr<FUICommandList> Cmds;
	UPjcSubsystem* SubsystemPtr = nullptr;
	TSharedPtr<FPjcTreeItem> TreeRootItem;
	TSharedPtr<SComboButton> TreeOptionBtn;
	TArray<TSharedPtr<FPjcStatItem>> StatItems;
	TArray<TSharedPtr<FPjcTreeItem>> TreeItems;
	TSet<TSharedPtr<FPjcTreeItem>> TreeItemsExpanded;
	TSharedPtr<SListView<TSharedPtr<FPjcStatItem>>> StatView;
	TSharedPtr<STreeView<TSharedPtr<FPjcTreeItem>>> TreeView;

	TArray<FAssetData> AssetsAll;
	TArray<FAssetData> AssetsUnused;
	TArray<FAssetData> AssetsUsed;
	TArray<FAssetData> AssetsPrimary;
	TArray<FAssetData> AssetsEditor;
	TArray<FAssetData> AssetsIndirect;
	TArray<FAssetData> AssetsExcluded;
	TArray<FAssetData> AssetsExtReferenced;
	TMap<FString, int32> NumAssetsTotalByPath;
	TMap<FString, int32> NumAssetsUsedByPath;
	TMap<FString, int32> NumAssetsUnusedByPath;
	TMap<FString, float> SizeAssetsUnusedByPath;
};
