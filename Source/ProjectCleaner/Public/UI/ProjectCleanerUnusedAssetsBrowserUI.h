// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "IContentBrowserSingleton.h"

DECLARE_DELEGATE(FOnUserDeletedAssets);
DECLARE_DELEGATE_OneParam(FOnUserExcludedAssets, const TArray<FAssetData>&);
DECLARE_DELEGATE_OneParam(FOnUserExcludedAssetsOfType, const TArray<FAssetData>&);


class UCleanerConfigs;
class ProjectCleanerManager;

class SProjectCleanerUnusedAssetsBrowserUI : public SCompoundWidget
{
public:
	
	SLATE_BEGIN_ARGS(SProjectCleanerUnusedAssetsBrowserUI) {}
		SLATE_ARGUMENT(ProjectCleanerManager*, CleanerManager)		
	SLATE_END_ARGS()
	
	void Construct(const FArguments& InArgs);
	void SetCleanerManager(ProjectCleanerManager* CleanerManagerPtr);
	void UpdateUI();

	/* Delegates */
	FOnUserDeletedAssets OnUserDeletedAssets;
	FOnUserExcludedAssets OnUserExcludedAssets;
	FOnUserExcludedAssetsOfType OnUserExcludedAssetsOfType;
private:
	/* UI */
	TSharedPtr<FUICommandList> Commands;
	void RegisterCommands();
	void DeleteAsset() const;
	void ExcludeAsset();
	void ExcludeAssetsOfType();
	
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
	class ProjectCleanerManager* CleanerManager = nullptr;
};