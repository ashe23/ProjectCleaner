// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "StructsContainer.h"
#include "IContentBrowserSingleton.h"

DECLARE_DELEGATE(FOnUserDeletedAssets);
DECLARE_DELEGATE_OneParam(FOnUserExcludedAssets, const TArray<FAssetData>&);
DECLARE_DELEGATE_OneParam(FOnUserExcludedAssetsOfType, const TArray<FAssetData>&);


class UCleanerConfigs;

class SProjectCleanerUnusedAssetsBrowserUI : public SCompoundWidget
{
public:
	
	SLATE_BEGIN_ARGS(SProjectCleanerUnusedAssetsBrowserUI) {}
		SLATE_ARGUMENT(TArray<FAssetData>*, UnusedAssets)
		SLATE_ARGUMENT(UCleanerConfigs*, CleanerConfigs)
		SLATE_ARGUMENT(TSet<FName>*, PrimaryAssetClasses)
	SLATE_END_ARGS()
	
	void Construct(const FArguments& InArgs);
	void SetUIData(const TArray<FAssetData>& NewUnusedAssets, UCleanerConfigs* NewConfigs, const TSet<FName>& NewPrimaryAssetClasses);

	/* Delegates */
	FOnUserDeletedAssets OnUserDeletedAssets;
	FOnUserExcludedAssets OnUserExcludedAssets;
	FOnUserExcludedAssetsOfType OnUserExcludedAssetsOfType;
private:
	/* Data */
	void SetUnusedAssets(const TArray<FAssetData>& NewUnusedAssets);
	void SetCleanerConfigs(UCleanerConfigs* Configs);
	void SetPrimaryAssetClasses(const TSet<FName>& NewPrimaryAssetClasses);
	UCleanerConfigs* CleanerConfigs = nullptr;
	TArray<FAssetData> UnusedAssets;
	TSet<FName> PrimaryAssetClasses;

	/* UI */
	TSharedPtr<FUICommandList> Commands;
	void RegisterCommands();
	void UpdateUI();
	void DeleteAsset() const;
	void ExcludeAsset() const;
	void ExcludeAssetsOfType() const;
	
	/* AssetPicker */
	struct FAssetPickerConfig AssetPickerConfig;
	struct FARFilter Filter;
	FGetCurrentSelectionDelegate CurrentSelectionDelegate;
	FRefreshAssetViewDelegate RefreshAssetViewDelegate;
	void GenerateFilter();
	TSharedPtr<SWidget> OnGetAssetContextMenu(const TArray<FAssetData>& SelectedAssets) const;
	static void OnAssetDblClicked(const FAssetData& AssetData);
	void FindInContentBrowser() const;
	bool IsAnythingSelected() const;

	/* PathPicker */
	struct FPathPickerConfig PathPickerConfig;
	FName SelectedPath = NAME_None;
	void OnPathSelected(const FString& Path);

	/* ContentBrowserModule */
	class FContentBrowserModule* ContentBrowserModule = nullptr;
};