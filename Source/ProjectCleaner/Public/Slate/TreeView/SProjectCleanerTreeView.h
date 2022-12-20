// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectCleanerDelegates.h"
#include "Widgets/SCompoundWidget.h"

class UProjectCleanerSubsystem;
struct FProjectCleanerTreeViewItem;

class SProjectCleanerTreeView final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SProjectCleanerTreeView)
		{
		}

		SLATE_EVENT(FProjectCleanerDelegateTreeViewPathSelected, OnPathSelected)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~SProjectCleanerTreeView() override;
private:
	void CommandsRegister();
	void OnProjectScanned();
	void ItemsUpdate();
	TSharedPtr<FProjectCleanerTreeViewItem> ItemCreate(const FString& InFolderPathAbs) const;

	TSharedRef<ITableRow> OnGenerateRow(TSharedPtr<FProjectCleanerTreeViewItem> Item, const TSharedRef<STableViewBase>& OwnerTable) const;
	TSharedRef<SHeaderRow> GetHeaderRow() const;
	TSharedPtr<SWidget> GetItemContextMenu() const;
	FSlateColor GetOptionsBtnForegroundColor() const;
	TSharedRef<SWidget> GetOptionsBtnContent();
	void OnSearchBoxTextChanged(const FText& InSearchText);
	void OnSearchBoxTextCommitted(const FText& InSearchText, ETextCommit::Type InCommitType);
	void OnItemMouseDblClick(TSharedPtr<FProjectCleanerTreeViewItem> Item);
	void OnGetChildren(TSharedPtr<FProjectCleanerTreeViewItem> Item, TArray<TSharedPtr<FProjectCleanerTreeViewItem>>& OutChildren) const;
	void OnSelectionChange(TSharedPtr<FProjectCleanerTreeViewItem> Item, ESelectInfo::Type SelectType);
	void OnExpansionChange(TSharedPtr<FProjectCleanerTreeViewItem> Item, bool bExpanded) const;
	void ToggleExpansionRecursive(TSharedPtr<FProjectCleanerTreeViewItem> Item, const bool bExpanded);

	int32 GetFoldersTotalNum(const FProjectCleanerTreeViewItem& Item) const;
	int32 GetFoldersEmptyNum(const FProjectCleanerTreeViewItem& Item) const;
	int32 GetAssetsTotalNum(const FProjectCleanerTreeViewItem& Item) const;
	int32 GetAssetsUnusedNum(const FProjectCleanerTreeViewItem& Item) const;
	int64 GetSizeTotal(const FProjectCleanerTreeViewItem& Item) const;
	int64 GetSizeUnused(const FProjectCleanerTreeViewItem& Item) const;

	TSet<FString> SelectedPaths;
	TSharedPtr<FUICommandList> Cmds;
	TSet<TSharedPtr<FProjectCleanerTreeViewItem>> ItemsExpanded;
	TArray<TSharedPtr<FProjectCleanerTreeViewItem>> ItemsSelected;
	TArray<TSharedPtr<FProjectCleanerTreeViewItem>> Items;
	TSharedPtr<STreeView<TSharedPtr<FProjectCleanerTreeViewItem>>> TreeView;
	TSharedPtr<SComboButton> OptionsComboButton;
	UProjectCleanerSubsystem* SubsystemPtr = nullptr;

	FProjectCleanerDelegateTreeViewPathSelected OnPathSelected;
};
