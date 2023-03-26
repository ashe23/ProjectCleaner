// // Copyright Ashot Barkhudaryan. All Rights Reserved.
//
// #pragma once
//
// #include "CoreMinimal.h"
// #include "PjcTypes.h"
// #include "ContentBrowserDelegates.h"
// #include "Widgets/SCompoundWidget.h"
//
// class UPjcSettings;
// class UPjcSubsystem;
// struct FPjcScanData;
//
// struct FPjcTreeViewItem
// {
// 	int64 UnusedSize = 0;
// 	int32 AssetsTotal = 0;
// 	int32 AssetsUsed = 0;
// 	int32 AssetsUnused = 0;
// 	float PercentageUnused = 0.0f;
// 	float PercentageUnusedNormalized = 0.0f;
// 	bool bIsVisible = false;
// 	bool bIsExpanded = false;
// 	bool bIsExcluded = false;
// 	bool bIsEmpty = false;
// 	bool bIsRoot = false;
// 	bool bIsEngineGenerated = false;
// 	bool bIsDevFolder = false;
// 	FString FolderName;
// 	FString FolderPathAbs;
// 	FString FolderPathRel;
//
// 	TSharedPtr<FPjcTreeViewItem> ParentItem;
// 	TArray<TSharedPtr<FPjcTreeViewItem>> SubItems;
//
// 	bool IsValid() const
// 	{
// 		return !FolderPathAbs.IsEmpty();
// 	}
//
// 	bool operator==(const FPjcTreeViewItem& Other) const
// 	{
// 		return FolderPathAbs.Equals(Other.FolderPathAbs);
// 	}
//
// 	bool operator!=(const FPjcTreeViewItem& Other) const
// 	{
// 		return !FolderPathAbs.Equals(Other.FolderPathAbs);
// 	}
// };
//
// class SPjcTreeViewItem final : public SMultiColumnTableRow<TSharedPtr<FPjcTreeViewItem>>
// {
// public:
// 	SLATE_BEGIN_ARGS(SPjcTreeViewItem) { }
//
// 		SLATE_ARGUMENT(TSharedPtr<FPjcTreeViewItem>, TreeItem)
// 		SLATE_ARGUMENT(FString, SearchText)
// 	SLATE_END_ARGS()
//
// 	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& OwnerTable);
// 	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& InColumnName) override;
//
// private:
// 	const FSlateBrush* GetFolderIcon() const;
// 	FSlateColor GetFolderColor() const;
//
// 	FString SearchText;
// 	TSharedPtr<FPjcTreeViewItem> TreeItem;
// };
//
// class SPjcTabAssetsBrowser final : public SCompoundWidget
// {
// public:
// 	SLATE_BEGIN_ARGS(SPjcTabAssetsBrowser) {}
// 	SLATE_END_ARGS()
//
// 	void Construct(const FArguments& InArgs);
// 	void UpdateData(const FPjcScanData& InScanData);
//
// private:
// 	void CmdsRegister();
//
// 	TSharedRef<SHeaderRow> GetTreeViewHeaderRow() const;
// 	TSharedPtr<SWidget> OnTreeViewContextMenu() const;
// 	TSharedRef<ITableRow> OnTreeViewGenerateRow(TSharedPtr<FPjcTreeViewItem> Item, const TSharedRef<STableViewBase>& OwnerTable) const;
// 	FARFilter AssetBrowserCreateFilter() const;
// 	FSlateColor GetTreeViewOptionsBtnForegroundColor() const;
// 	TSharedRef<SWidget> GetTreeViewOptionsBtnContent();
// 	TSharedPtr<FPjcTreeViewItem> CreateItem(const FString& InPath) const;
// 	void SetItemVisibility(const TSharedPtr<FPjcTreeViewItem>& Item) const;
// 	void TreeViewItemsUpdate();
// 	void OnTreeViewGetChildren(TSharedPtr<FPjcTreeViewItem> Item, TArray<TSharedPtr<FPjcTreeViewItem>>& OutChildren);
// 	void OnTreeViewSelectionChange(TSharedPtr<FPjcTreeViewItem> Item, ESelectInfo::Type SelectInfo) const;
// 	void OnTreeViewExpansionChange(TSharedPtr<FPjcTreeViewItem> Item, bool bExpansion) const;
// 	void OnTreeViewSearchTextChanged(const FText& InText);
// 	void OnTreeViewSearchTextCommitted(const FText& InText, ETextCommit::Type CommitType);
// 	bool FilterAnyEnabled() const;
// 	bool FilterAllDisabled() const;
// 	bool FilterAllEnabled() const;
//
// 	bool bFilterPrimaryActive = false;
// 	bool bFilterExcludeActive = false;
// 	bool bFilterIndirectActive = false;
// 	bool bFilterExtReferencedActive = false;
// 	bool bFilterUsedActive = false;
//
// 	FPjcScanData ScanData;
// 	FString TreeViewSearchText;
// 	TSharedPtr<FUICommandList> Cmds;
// 	TSet<FString> TreeViewItemsExpanded;
// 	UPjcSettings* SettingsPtr = nullptr;
// 	UPjcSubsystem* SubsystemPtr = nullptr;
// 	TSharedPtr<SComboButton> TreeViewOptionBtn;
// 	TArray<TSharedPtr<FPjcTreeViewItem>> TreeViewItems;
// 	TSharedPtr<STreeView<TSharedPtr<FPjcTreeViewItem>>> TreeView;
// 	
// 	FSetARFilterDelegate AssetBrowserDelegateFilter;
// 	FRefreshAssetViewDelegate AssetBrowserDelegateRefreshView;
// 	FGetCurrentSelectionDelegate AssetBrowserDelegateSelection;
// };
