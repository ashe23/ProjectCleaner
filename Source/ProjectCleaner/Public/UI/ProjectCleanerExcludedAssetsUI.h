#pragma once

// Engine Headers
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Editor/ContentBrowser/Public/ContentBrowserDelegates.h"

DECLARE_DELEGATE_OneParam(FOnAssetIncluded, const TArray<FAssetData>& /* Excluded Assets */);

class SProjectCleanerExcludedAssetsUI : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SProjectCleanerExcludedAssetsUI) {}
		SLATE_ARGUMENT(TSet<FAssetData>, ExcludedAssets)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	void RefreshUIContent();
	void SetExcludedAssets(const TSet<FAssetData>& NewExcludedAssets);

	/** Delegates */
	FOnAssetIncluded OnAssetIncluded;
private:
	TSharedPtr<SWidget> OnGetAssetContextMenu(const TArray<FAssetData>& SelectedAssets);
	FGetCurrentSelectionDelegate GetCurrentSelectionDelegate;
	bool IsAnythingSelected() const;
	void AddForDeletion();
	
	TSet<FAssetData> ExcludedAssets;
	TSharedRef<SWidget> WidgetRef = SNullWidget::NullWidget;
	TSharedPtr<FUICommandList> Commands;
};
