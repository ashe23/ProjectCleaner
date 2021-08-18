// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "IContentBrowserSingleton.h"

class UCleanerConfigs;
class FProjectCleanerManager;

class SProjectCleanerUnusedAssetsBrowserUI : public SCompoundWidget
{
public:
	
	SLATE_BEGIN_ARGS(SProjectCleanerUnusedAssetsBrowserUI) {}
		SLATE_ARGUMENT(FProjectCleanerManager*, CleanerManager)
	SLATE_END_ARGS()
	
	void Construct(const FArguments& InArgs);
	void SetCleanerManager(FProjectCleanerManager* CleanerManagerPtr);
	void UpdateUI();

private:
	/* UI */
	TSharedPtr<FUICommandList> Commands;
	void RegisterCommands();
	void DeleteAsset() const;
	void ExcludeAsset() const;
	void ExcludeAssetsOfType() const;
	void ExcludePath() const;
	
	/* AssetPicker */
	struct FAssetPickerConfig AssetPickerConfig;
	struct FARFilter Filter;
	FGetCurrentSelectionDelegate CurrentSelectionDelegate;
	FRefreshAssetViewDelegate RefreshAssetViewDelegate;
	
	void GenerateFilter();
	TSharedPtr<SWidget> OnGetFolderContextMenu(
		const TArray<FString>& SelectedPaths,
		FContentBrowserMenuExtender_SelectedPaths InMenuExtender,
		FOnCreateNewFolder InOnCreateNewFolder
	) const;
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
	class FProjectCleanerManager* CleanerManager = nullptr;
};