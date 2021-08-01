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
	TSharedPtr<SWidget> GetExcludedAssetsView();
	TSharedPtr<SWidget> OnGetAssetContextMenu(const TArray<FAssetData>& SelectedAssets) const;
	static void OnAssetDblClicked(const FAssetData& AssetData);
	void FindInContentBrowser() const;
	bool IsAnythingSelected() const;
	void IncludeAssets() const;
	FReply IncludeAllAssets() const;

	/* Data */
	FGetCurrentSelectionDelegate GetCurrentSelectionDelegate;
	TSharedPtr<FUICommandList> Commands;

	/* PathPickerConfig */
	FName SelectedPath;
	struct FPathPickerConfig PathPickerConfig;
	void OnPathSelected(const FString& Path);

	/* ProjectCleanerManager */
	ProjectCleanerManager* CleanerManager = nullptr;
	
	/* ContentBrowserModule */
	class FContentBrowserModule* ContentBrowserModule = nullptr; // todo:ashe23 change this
};