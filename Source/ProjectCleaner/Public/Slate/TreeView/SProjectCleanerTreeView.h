// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectCleanerDelegates.h"
#include "Widgets/SCompoundWidget.h"

class UProjectCleanerScanSettings;
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
	void TreeItemsUpdate();
	FProjectCleanerDelegatePathChanged& OnPathChange();
private:
	TSharedPtr<FProjectCleanerTreeViewItem> TreeItemCreate(const FString& InDirPathAbs) const;

	TSharedRef<ITableRow> OnTreeViewGenerateRow(TSharedPtr<FProjectCleanerTreeViewItem> Item, const TSharedRef<STableViewBase>& OwnerTable) const;
	TSharedRef<SHeaderRow> GetTreeViewHeaderRow() const;
	void OnTreeViewItemMouseDblClick(TSharedPtr<FProjectCleanerTreeViewItem> Item);
	void OnTreeViewGetChildren(TSharedPtr<FProjectCleanerTreeViewItem> Item, TArray<TSharedPtr<FProjectCleanerTreeViewItem>>& OutChildren) const;
	void OnTreeViewSelectionChange(TSharedPtr<FProjectCleanerTreeViewItem> Item, ESelectInfo::Type SelectType);
	void OnTreeViewExpansionChange(TSharedPtr<FProjectCleanerTreeViewItem> Item, bool bExpanded);
	void OnTreeViewSearchBoxTextChanged(const FText& InSearchText);
	void OnTreeViewSearchBoxTextCommitted(const FText& InSearchText, ETextCommit::Type InCommitType);
	void ToggleExpansionRecursive(TSharedPtr<FProjectCleanerTreeViewItem> Item, const bool bExpanded);

	TSharedPtr<FProjectCleanerScanner> Scanner;
	TWeakObjectPtr<UProjectCleanerScanSettings> ScanSettings;
	TArray<TSharedPtr<FProjectCleanerTreeViewItem>> TreeItems;
	TSharedPtr<STreeView<TSharedPtr<FProjectCleanerTreeViewItem>>> TreeView;
	TSet<TSharedPtr<FProjectCleanerTreeViewItem>> TreeItemsExpanded;
	FProjectCleanerDelegatePathChanged DelegatePathChanged;
};
