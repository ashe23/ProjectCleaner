// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectCleanerDelegates.h"
#include "Widgets/SCompoundWidget.h"

class UProjectCleanerScanSettings;
class UProjectCleanerTreeViewSettings;
struct FProjectCleanerTreeViewItem;
struct FProjectCleanerScanner;

// Responsible for showing project folder structure as tree view with additional information about every folder and its content
class SProjectCleanerTreeView final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SProjectCleanerTreeView)
		{
		}

		SLATE_ARGUMENT(TSharedPtr<FProjectCleanerScanner>, Scanner)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	FProjectCleanerDelegatePathSelected& OnPathSelected();
	FProjectCleanerDelegatePathExcluded& OnPathExcluded();
	FProjectCleanerDelegatePathIncluded& OnPathIncluded();
	FProjectCleanerDelegatePathCleaned& OnPathCleaned();

private:
	void TreeItemsUpdate();
	TSharedPtr<FProjectCleanerTreeViewItem> TreeItemCreate(const FString& InDirPathAbs) const;
	TSharedRef<ITableRow> OnTreeViewGenerateRow(TSharedPtr<FProjectCleanerTreeViewItem> Item, const TSharedRef<STableViewBase>& OwnerTable) const;
	TSharedPtr<SWidget> GetTreeItemContextMenu() const;
	TSharedRef<SHeaderRow> GetTreeViewHeaderRow() const;
	TSharedRef<SWidget> GetTreeViewOptionsBtnContent();
	FSlateColor GetTreeViewOptionsBtnForegroundColor() const;

	void OnTreeViewItemMouseDblClick(TSharedPtr<FProjectCleanerTreeViewItem> Item);
	void OnTreeViewGetChildren(TSharedPtr<FProjectCleanerTreeViewItem> Item, TArray<TSharedPtr<FProjectCleanerTreeViewItem>>& OutChildren) const;
	void OnTreeViewSelectionChange(TSharedPtr<FProjectCleanerTreeViewItem> Item, ESelectInfo::Type SelectType);
	void OnTreeViewExpansionChange(TSharedPtr<FProjectCleanerTreeViewItem> Item, bool bExpanded);
	void OnTreeViewSearchBoxTextChanged(const FText& InSearchText);
	void OnTreeViewSearchBoxTextCommitted(const FText& InSearchText, ETextCommit::Type InCommitType);
	void ToggleExpansionRecursive(TSharedPtr<FProjectCleanerTreeViewItem> Item, const bool bExpanded);

	TSharedPtr<FUICommandList> Cmds;
	TSet<TSharedPtr<FProjectCleanerTreeViewItem>> ItemsExpanded;
	TArray<TSharedPtr<FProjectCleanerTreeViewItem>> ItemsSelected;
	TSharedPtr<SComboButton> ViewOptionsComboButton;
	TSharedPtr<FProjectCleanerScanner> Scanner;
	UProjectCleanerScanSettings* ScanSettings = nullptr;
	UProjectCleanerTreeViewSettings* TreeViewSettings = nullptr;
	TArray<TSharedPtr<FProjectCleanerTreeViewItem>> TreeItems;
	TSharedPtr<STreeView<TSharedPtr<FProjectCleanerTreeViewItem>>> TreeView;

	FProjectCleanerDelegatePathSelected DelegatePathSelected;
	FProjectCleanerDelegatePathExcluded DelegatePathExcluded;
	FProjectCleanerDelegatePathIncluded DelegatePathIncluded;
	FProjectCleanerDelegatePathCleaned DelegatePathCleaned;
};
