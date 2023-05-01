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
	TSharedRef<SWidget> CreateToolbar();
	TSharedRef<SWidget> GetSettingsWidget();

	ECheckBoxState GetCheckboxStateScanFoldersDev() const;
	ECheckBoxState GetCheckboxStateCleanAssetsUnused() const;
	ECheckBoxState GetCheckboxStateCleanFoldersEmpty() const;
	
	void OnScanFoldersDevStateChanged(ECheckBoxState BoxState);
	void OnCleanAssetsUnusedStateChanged(ECheckBoxState BoxState);
	void OnCleanFoldersEmptyStateChanged(ECheckBoxState BoxState);

	FText SearchText;
	TSharedPtr<FUICommandList> Cmds;

	// stats
	void StatItemsInit();
	TSharedRef<SHeaderRow> GetStatHeaderRow() const;
	TSharedRef<ITableRow> OnStatGenerateRow(TSharedPtr<FPjcStatItem> Item, const TSharedRef<STableViewBase>& OwnerTable) const;

	TArray<TSharedPtr<FPjcStatItem>> StatItems;
	TSharedPtr<SListView<TSharedPtr<FPjcStatItem>>> StatView;

	// tree
	void TreeItemsUpdate();
	void TreeItemsFilter();
	void TreeItemExpandParentsRecursive(const TSharedPtr<FPjcTreeItem>& Item) const;
	void TreeItemMakeVisibleParentsRecursive(const TSharedPtr<FPjcTreeItem>& Item) const;
	void SetTreeItemVisibility(const TSharedPtr<FPjcTreeItem>& Item) const;
	void SetTreeItemExpansion(const TSharedPtr<FPjcTreeItem>& Item);
	void OnTreeGetChildren(TSharedPtr<FPjcTreeItem> Item, TArray<TSharedPtr<FPjcTreeItem>>& OutChildren);
	void OnTreeExpansionChanged(TSharedPtr<FPjcTreeItem> Item, const bool bIsExpanded) const;
	void OnTreeSearchTextChanged(const FText& InText);
	void OnTreeSearchTextCommitted(const FText& InText, ETextCommit::Type Type);
	bool TreeItemContainsSearchText(const TSharedPtr<FPjcTreeItem>& Item) const;
	TSharedRef<SHeaderRow> GetTreeHeaderRow() const;
	TSharedRef<ITableRow> OnTreeGenerateRow(TSharedPtr<FPjcTreeItem> Item, const TSharedRef<STableViewBase>& OwnerTable) const;
	TSharedRef<SWidget> GetTreeOptionsBtnContent();
	TSharedPtr<FPjcTreeItem> CreateTreeItem(const FString& InFolderPath) const;
	FSlateColor GetTreeOptionsBtnForegroundColor() const;
	FText GetTreeSummaryText() const;

	UPjcSubsystem* SubsystemPtr = nullptr;
	TSharedPtr<FPjcTreeItem> TreeRootItem;
	TSharedPtr<SComboButton> TreeOptionBtn;
	TArray<TSharedPtr<FPjcTreeItem>> TreeItems;
	TSet<TSharedPtr<FPjcTreeItem>> TreeItemsExpanded;
	TSharedPtr<STreeView<TSharedPtr<FPjcTreeItem>>> TreeView;
};
