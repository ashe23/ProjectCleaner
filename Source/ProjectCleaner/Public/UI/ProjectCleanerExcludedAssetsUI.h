// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#pragma once

// Engine Headers
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "StructsContainer.h"
#include "Editor/ContentBrowser/Public/ContentBrowserDelegates.h"

DECLARE_DELEGATE_OneParam(FOnUserIncludedAsset, const TArray<FAssetData>&);

class SProjectCleanerExcludedAssetsUI : public SCompoundWidget
{
public:
	
	SLATE_BEGIN_ARGS(SProjectCleanerExcludedAssetsUI) {}
		SLATE_ARGUMENT(TSet<FAssetData>, ExcludedAssets)
		SLATE_ARGUMENT(TArray<FAssetData>, LinkedAssets)
		SLATE_ARGUMENT(FCleanerConfigs, CleanerConfigs)
	SLATE_END_ARGS()
	
	void Construct(const FArguments& InArgs);
	void SetExcludedAssets(const TSet<FAssetData>& Assets);
	void SetLinkedAssets(const TArray<FAssetData>& Assets);
	void SetCleanerConfigs(const FCleanerConfigs& Configs);
	
	FOnUserIncludedAsset OnUserIncludedAssets;
private:
	void RefreshUIContent();
	TSharedPtr<SWidget> OnGetAssetContextMenu(const TArray<FAssetData>& SelectedAssets);
	void OnAssetDblClicked(const FAssetData& AssetData) const;
	void FindInContentBrowser() const;
	bool IsAnythingSelected() const;
	void IncludeAssets() const;

	FCleanerConfigs CleanerConfigs;
	FGetCurrentSelectionDelegate GetCurrentSelectionDelegate;
	TSharedPtr<FUICommandList> Commands;
	TSet<FAssetData> ExcludedAssets;
	TSet<FAssetData> LinkedAssets;
	TSharedRef<SWidget> WidgetRef = SNullWidget::NullWidget;
};