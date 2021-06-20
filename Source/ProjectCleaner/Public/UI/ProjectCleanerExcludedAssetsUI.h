// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#pragma once

// Engine Headers
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Editor/ContentBrowser/Public/ContentBrowserDelegates.h"

DECLARE_DELEGATE_OneParam(FOnUserIncludedAsset, const TArray<FAssetData>&);

class UCleanerConfigs;

class SProjectCleanerExcludedAssetsUI : public SCompoundWidget
{
public:
	
	SLATE_BEGIN_ARGS(SProjectCleanerExcludedAssetsUI) {}
		SLATE_ARGUMENT(TArray<FAssetData>*, ExcludedAssets)
		SLATE_ARGUMENT(TArray<FAssetData>*, LinkedAssets)
		SLATE_ARGUMENT(UCleanerConfigs*, CleanerConfigs)
	SLATE_END_ARGS()
	
	void Construct(const FArguments& InArgs);
	void SetUIData(const TArray<FAssetData>& NewExcludedAssets, const TArray<FAssetData>& NewLinkedAssets, UCleanerConfigs* NewConfigs);
	
	FOnUserIncludedAsset OnUserIncludedAssets;
private:
	void SetExcludedAssets(const TArray<FAssetData>& Assets);
	void SetLinkedAssets(const TArray<FAssetData>& Assets);
	void SetCleanerConfigs(UCleanerConfigs* Configs);
	void UpdateUI();
	TSharedPtr<SWidget> OnGetAssetContextMenu(const TArray<FAssetData>& SelectedAssets);
	void OnAssetDblClicked(const FAssetData& AssetData) const;
	void FindInContentBrowser() const;
	bool IsAnythingSelected() const;
	void IncludeAssets() const;

	UCleanerConfigs* CleanerConfigs;
	FGetCurrentSelectionDelegate GetCurrentSelectionDelegate;
	TSharedPtr<FUICommandList> Commands;
	TArray<FAssetData> ExcludedAssets;
	TArray<FAssetData> LinkedAssets;
	TSharedRef<SWidget> WidgetRef = SNullWidget::NullWidget;
};