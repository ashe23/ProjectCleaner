#pragma once

// Engine Headers
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Editor/ContentBrowser/Public/ContentBrowserDelegates.h"
// #include "ProjectCleanerUnusedAssetsBrowserUI.generated.h"

class SProjectCleanerUnusedAssetsBrowserUI : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SProjectCleanerUnusedAssetsBrowserUI) {}
		SLATE_ARGUMENT(TArray<FAssetData*>, UnusedAssets)
	SLATE_END_ARGS()
	
	void Construct(const FArguments& InArgs);
	void SetUnusedAssets(TArray<FAssetData*>& NewUnusedAssets);
private:
	// UI specific stuff start
	TSharedPtr<SWidget> OnGetAssetContextMenu(const TArray<FAssetData>& SelectedAssets);
	void OnAssetDblClicked(const FAssetData& AssetData) const;
	void FindInContentBrowser() const;
	bool IsAnythingSelected() const;
	void DeleteAsset() const;
	void RefreshUIContent();
	TSharedRef<SWidget> WidgetRef = SNullWidget::NullWidget;
	/** Delegate to interact with asset view */
	FGetCurrentSelectionDelegate GetCurrentSelectionDelegate;
	TSharedPtr<FUICommandList> Commands;
	// UI specific stuff end
	
	TArray<FAssetData*> UnusedAssets;
};
