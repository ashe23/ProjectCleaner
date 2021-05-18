#pragma once

// Engine Headers
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Editor/ContentBrowser/Public/ContentBrowserDelegates.h"

DECLARE_DELEGATE(FOnUserDeletedAssets);
DECLARE_DELEGATE_OneParam(FOnUserExcludedAssets, const TArray<FAssetData>&);

class SProjectCleanerUnusedAssetsBrowserUI : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SProjectCleanerUnusedAssetsBrowserUI) {}
		SLATE_ARGUMENT(TArray<FAssetData>, UnusedAssets)
	SLATE_END_ARGS()
	
	void Construct(const FArguments& InArgs);
	void SetUnusedAssets(const TArray<FAssetData>& NewUnusedAssets);
	
	/** Delegates */
	FOnUserDeletedAssets OnUserDeletedAssets;
	FOnUserExcludedAssets OnUserExcludedAssets;
private:
	TSharedPtr<SWidget> OnGetAssetContextMenu(const TArray<FAssetData>& SelectedAssets);
	void OnAssetDblClicked(const FAssetData& AssetData) const;
	void FindInContentBrowser() const;
	bool IsAnythingSelected() const;
	void DeleteAsset() const;
	void ExcludeAsset() const;
	void RefreshUIContent();
	/** Delegate to interact with asset view */
	FGetCurrentSelectionDelegate GetCurrentSelectionDelegate;
	TSharedPtr<FUICommandList> Commands;
	
	TArray<FAssetData> UnusedAssets;
	TSharedRef<SWidget> WidgetRef = SNullWidget::NullWidget;

};
