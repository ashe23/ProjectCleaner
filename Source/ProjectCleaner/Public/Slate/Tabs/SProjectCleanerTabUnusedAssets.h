// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class UProjectCleanerSubsystem;

struct FProjectCleanerTreeViewItem
{
	FString FolderPathAbs;
	FString FolderPathRel;
	FString FolderName;
	int32 FoldersTotal = 0;
	int32 FoldersEmpty = 0;
	int64 SizeTotal = 0;
	int64 SizeUnused = 0;
	int32 AssetsTotal = 0;
	int32 AssetsUnused = 0;
	bool bDevFolder = false;
	bool bExpanded = false;
	bool bExcluded = false;
	bool bEmpty = false;
	float PercentUnused = 0.0f; // 0 - 100 range
	float PercentUnusedNormalized = 0.0f; // 0 - 1 range

	~FProjectCleanerTreeViewItem()
	{
		SubItems.Empty(); // todo:ashe23 not sure about this
	}

	TArray<TSharedPtr<FProjectCleanerTreeViewItem>> SubItems;

	bool operator==(const FProjectCleanerTreeViewItem& Other) const
	{
		return FolderPathAbs.Equals(Other.FolderPathAbs);
	}

	bool operator!=(const FProjectCleanerTreeViewItem& Other) const
	{
		return !FolderPathAbs.Equals(Other.FolderPathAbs);
	}
};

class SProjectCleanerTreeViewItem final : public SMultiColumnTableRow<TSharedPtr<FProjectCleanerTreeViewItem>>
{
	SLATE_BEGIN_ARGS(SProjectCleanerTreeViewItem)
		{
		}

		SLATE_ARGUMENT(TSharedPtr<FProjectCleanerTreeViewItem>, TreeItem)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& OwnerTable);
	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& InColumnName) override;

private:
	const FSlateBrush* GetFolderIcon() const;
	FSlateColor GetFolderColor() const;
	FSlateColor GetProgressBarColor() const;

	TSharedPtr<FProjectCleanerTreeViewItem> TreeItem;
};

class SProjectCleanerTabUnusedAssets final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SProjectCleanerTabUnusedAssets)
		{
		}

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	void RegisterCmds();
	TSharedRef<ITableRow> OnTreeViewGenerateRow(TSharedPtr<FProjectCleanerTreeViewItem> Item, const TSharedRef<STableViewBase>& OwnerTable) const;
	void TreeViewItemsUpdate();
	void OnTreeViewSearchBoxTextChanged(const FText& InSearchText);
	void OnTreeViewSearchBoxTextCommitted(const FText& InSearchText, ETextCommit::Type InCommitType);
	void OnTreeViewItemMouseDblClick(TSharedPtr<FProjectCleanerTreeViewItem> Item);
	void OnTreeViewGetChildren(TSharedPtr<FProjectCleanerTreeViewItem> Item, TArray<TSharedPtr<FProjectCleanerTreeViewItem>>& OutChildren) const;
	void OnTreeViewSelectionChange(TSharedPtr<FProjectCleanerTreeViewItem> Item, ESelectInfo::Type SelectType);
	void OnTreeViewExpansionChange(TSharedPtr<FProjectCleanerTreeViewItem> Item, bool bExpanded);
	void TreeViewToggleExpansionRecursive(TSharedPtr<FProjectCleanerTreeViewItem> Item, const bool bExpanded);
	TSharedPtr<SWidget> GetTreeViewItemContextMenu() const;
	TSharedPtr<FProjectCleanerTreeViewItem> TreeViewItemCreate(const FString& InFolderPathAbs) const;
	FSlateColor GetTreeViewOptionsBtnForegroundColor() const;
	TSharedRef<SWidget> GetTreeViewOptionsBtnContent();
	TSharedRef<SHeaderRow> GetTreeViewHeaderRow() const;

	bool IsFolderEmpty(const FString& InFolderPath) const;
	bool IsFolderExcluded(const FString& InFolderPath) const;
	int32 GetFoldersTotalNum(const FString& InFolderPath) const;
	int32 GetFoldersEmptyNum(const FString& InFolderPath) const;
	int32 GetAssetsTotalNum(const FString& InFolderPath) const;
	int32 GetAssetsUnusedNum(const FString& InFolderPath) const;
	int64 GetSizeTotal(const FString& InFolderPath) const;
	int64 GetSizeUnused(const FString& InFolderPath) const;

private:
	TSharedPtr<FUICommandList> Cmds;
	TSharedPtr<SComboButton> TreeViewOptionsComboButton;
	TSet<TSharedPtr<FProjectCleanerTreeViewItem>> TreeViewItemsExpanded;
	TArray<TSharedPtr<FProjectCleanerTreeViewItem>> TreeViewItemsSelected;
	TArray<TSharedPtr<FProjectCleanerTreeViewItem>> TreeViewItems;
	TSharedPtr<STreeView<TSharedPtr<FProjectCleanerTreeViewItem>>> TreeView;
	UProjectCleanerSubsystem* SubsystemPtr = nullptr;
};
