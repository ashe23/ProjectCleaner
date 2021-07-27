// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#pragma once

// Engine Headers
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "StructsContainer.h"
#include "Editor/ContentBrowser/Public/ContentBrowserDelegates.h"
#include "IContentBrowserSingleton.h"

DECLARE_DELEGATE_TwoParams(FOnUserIncludedAsset, const TArray<FAssetData>&, const bool);

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
	
	FOnUserIncludedAsset OnUserIncludedAssets;
private:
	
	void RegisterCommands();
	void UpdateUI();
	TSharedPtr<SWidget> GetExcludedAssetsView();
	TSharedPtr<SWidget> GetLinkedAssetsView();
	TSharedPtr<SWidget> OnGetAssetContextMenu(const TArray<FAssetData>& SelectedAssets) const;
	static void OnAssetDblClicked(const FAssetData& AssetData);
	void OnAssetSelected(const FAssetData& AssetData);
	void FindInContentBrowser() const;
	bool IsAnythingSelected() const;
	void IncludeAssets() const;
	FReply IncludeAllAssets() const;

	/* Data */
	// UCleanerConfigs* CleanerConfigs = nullptr;
	FGetCurrentSelectionDelegate GetCurrentSelectionDelegate;
	TSharedPtr<FUICommandList> Commands;

	/* PathPickerConfig */
	FName SelectedPath;
	void OnPathSelected(const FString& Path);
	struct FPathPickerConfig PathPickerConfig;
	FName SelectedAssetName;

	/* ProjectCleanerManager */
	ProjectCleanerManager* CleanerManager = nullptr;
	
	/* ContentBrowserModule */
	class FContentBrowserModule* ContentBrowserModule = nullptr; // todo:ashe23 change this
};