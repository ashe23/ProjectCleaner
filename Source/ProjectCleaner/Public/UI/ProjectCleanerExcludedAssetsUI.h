// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#pragma once

// Engine Headers
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Editor/ContentBrowser/Public/ContentBrowserDelegates.h"
#include "IContentBrowserSingleton.h"

class UCleanerConfigs;
class ProjectCleanerManager;

class SProjectCleanerExcludedAssetsUI : public SCompoundWidget
{
public:
	
	SLATE_BEGIN_ARGS(SProjectCleanerExcludedAssetsUI) {}
		SLATE_ARGUMENT(ProjectCleanerManager*, CleanerManager)
	SLATE_END_ARGS()
	
	void Construct(const FArguments& InArgs);
	void SetCleanerManager(ProjectCleanerManager* CleanerManagerPtr);
	void UpdateUI();
private:
	
	void RegisterCommands();

	/* AssetPickerConfig */
	void GenerateFilter();
	TSharedPtr<SWidget> OnGetAssetContextMenu(const TArray<FAssetData>& SelectedAssets) const;
	TSharedPtr<SWidget> OnGetFolderContextMenu(
		const TArray<FString>& SelectedPaths,
		FContentBrowserMenuExtender_SelectedPaths InMenuExtender,
		FOnCreateNewFolder InOnCreateNewFolder
	) const;
	static void OnAssetDblClicked(const FAssetData& AssetData);
	void FindInContentBrowser() const;
	bool IsAnythingSelected() const;
	void IncludeAssets() const;
	void IncludePath() const;
	FReply IncludeAllAssets() const;
	struct FARFilter Filter;
	struct FAssetPickerConfig AssetPickerConfig;

	/* Data */
	FGetCurrentSelectionDelegate GetCurrentSelectionDelegate;
	TSharedPtr<FUICommandList> Commands;

	/* PathPickerConfig */
	FName SelectedPath;
	struct FPathPickerConfig PathPickerConfig;
	void OnPathSelected(const FString& Path);

	/* ProjectCleanerManager */
	ProjectCleanerManager* CleanerManager = nullptr;
	
	class FContentBrowserModule* ContentBrowserModule = nullptr;
};