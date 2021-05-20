// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#pragma once

// Engine Headers
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Editor/ContentBrowser/Public/ContentBrowserDelegates.h"

DECLARE_DELEGATE_OneParam(FOnUserIncludedAsset, const TArray<FAssetData>&);

class SProjectCleanerExcludedAssetsUI : public SCompoundWidget
{
public:
	
	SLATE_BEGIN_ARGS(SProjectCleanerExcludedAssetsUI) {}
		SLATE_ARGUMENT(TArray<FAssetData>, ExcludedAssets)
	SLATE_END_ARGS()
	
	void Construct(const FArguments& InArgs);
	void SetExcludedAssets(const TArray<FAssetData>& Assets);
	
	FOnUserIncludedAsset OnUserIncludedAssets;
private:
	void RefreshUIContent();
	TSharedPtr<SWidget> OnGetAssetContextMenu(const TArray<FAssetData>& SelectedAssets);
	void OnAssetDblClicked(const FAssetData& AssetData) const;
	void FindInContentBrowser() const;
	bool IsAnythingSelected() const;
	void IncludeAssets() const;
	FGetCurrentSelectionDelegate GetCurrentSelectionDelegate;
	TSharedPtr<FUICommandList> Commands;
	TArray<FAssetData> ExcludedAssets;
	TSharedRef<SWidget> WidgetRef = SNullWidget::NullWidget;
};