#pragma once

// Engine Headers
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Editor/ContentBrowser/Public/ContentBrowserDelegates.h"

class SProjectCleanerExcludedAssetsUI : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SProjectCleanerExcludedAssetsUI) {}
		SLATE_ARGUMENT(TSet<FAssetData>, ExcludedAssets)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	void RefreshUIContent();
	void SetExcludedAssets(const TSet<FAssetData>& NewExcludedAssets);
private:
	TSharedPtr<SWidget> OnGetAssetContextMenu(const TArray<FAssetData>& SelectedAssets);
	FGetCurrentSelectionDelegate GetCurrentSelectionDelegate;
	bool IsAnythingSelected() const;
	void AddForDeletion();
	
	TArray<FAssetData> ExcludedAssets;
	TSharedRef<SWidget> WidgetRef = SNullWidget::NullWidget;
	TSharedPtr<FUICommandList> Commands;
};
