// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ContentBrowserDelegates.h"
#include "Widgets/SCompoundWidget.h"

struct FAssetPickerConfig;
struct FPjcScanResult;

struct FPjcTreeViewItem
{
	int64 SizeUnused;
	int32 NumTotal;
	int32 NumUsed;
	int32 NumUnused;
	float PercentUnused;
	float PercentUnusedNormalized;
	bool bIsRoot;
	bool bIsEmpty;
	bool bIsExcluded;
	bool bIsDevFolder;
	bool bIsExpanded;
	FString PathAbs;
	FString PathRel;
	FString FolderName;

	TSharedPtr<FPjcTreeViewItem> ParentItem;
	TArray<TSharedPtr<FPjcTreeViewItem>> SubItems;

	bool IsValid() const
	{
		return !PathAbs.IsEmpty();
	}

	bool operator==(const FPjcTreeViewItem& Other) const
	{
		return PathAbs.Equals(Other.PathAbs, ESearchCase::CaseSensitive);
	}

	bool operator!=(const FPjcTreeViewItem& Other) const
	{
		return !PathAbs.Equals(Other.PathAbs, ESearchCase::CaseSensitive);
	}
};

class SPjcTreeViewItem final : public SMultiColumnTableRow<TSharedPtr<FPjcTreeViewItem>>
{
public:
	SLATE_BEGIN_ARGS(SPjcTreeViewItem) { }

		SLATE_ARGUMENT(TSharedPtr<FPjcTreeViewItem>, TreeItem)
		SLATE_ARGUMENT(FString, SearchText)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& OwnerTable);
	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& InColumnName) override;

private:
	const FSlateBrush* GetFolderIcon() const;
	FSlateColor GetFolderColor() const;

	FString SearchText;
	TSharedPtr<FPjcTreeViewItem> TreeItem;
};

class SPjcTabScanInfo final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SPjcTabScanInfo) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	void UpdateData(const FPjcScanResult& InScanData);

private:
	TSharedRef<SHeaderRow> GetTreeViewHeaderRow() const;
	TSharedRef<ITableRow> OnTreeViewGenerateRow(TSharedPtr<FPjcTreeViewItem> Item, const TSharedRef<STableViewBase>& OwnerTable) const;
	void OnTreeViewGetChildren(TSharedPtr<FPjcTreeViewItem> Item, TArray<TSharedPtr<FPjcTreeViewItem>>& OutChildren);
	
	TSharedRef<SWidget> GetTreeViewOptionsBtnContent();
	FSlateColor GetTreeViewOptionsBtnForegroundColor() const;
	
	FString TreeViewSearchText;
	TSharedPtr<SComboButton> TreeViewOptionBtn;
	TSharedPtr<FPjcTreeViewItem> RootItem;
	TArray<TSharedPtr<FPjcTreeViewItem>> TreeViewItems;
	TSharedPtr<STreeView<TSharedPtr<FPjcTreeViewItem>>> TreeView;
};
