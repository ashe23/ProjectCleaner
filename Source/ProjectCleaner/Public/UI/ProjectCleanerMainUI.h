// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/ProjectCleanerManager.h"
#include "Widgets/SCompoundWidget.h"

class SProjectCleanerMainUI : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SProjectCleanerMainUI) {}
		SLATE_ARGUMENT(struct FProjectCleanerData*, CleanerData)
    SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~SProjectCleanerMainUI() override;

private:
	/* Initializers */
	void InitTabs();
	void Update();
	
	/* Callbacks */
	TSharedRef<SDockTab> OnUnusedAssetTabSpawn(const FSpawnTabArgs& SpawnTabArgs);
	TSharedRef<SDockTab> OnNonEngineFilesTabSpawn(const FSpawnTabArgs& SpawnTabArgs);
	TSharedRef<SDockTab> OnCorruptedFilesTabSpawn(const FSpawnTabArgs& SpawnTabArgs);
	TSharedRef<SDockTab> OnIndirectAssetsTabSpawn(const FSpawnTabArgs& SpawnTabArgs);
	
	void OnUserDeletedAssets();
	void OnUserExcludedAssets(const TArray<FAssetData>& Assets);
	void OnUserIncludedAssets(const TArray<FAssetData>& Assets, const bool CleanFilters);
	
	/* Btn Callbacks*/
	FReply OnRefreshBtnClick();
	// FReply OnDeleteUnusedAssetsBtnClick();
	// FReply OnDeleteEmptyFolderClick();
	
	/* UI Data */
	TWeakPtr<class SProjectCleanerBrowserStatisticsUI> StatisticsUI;
	TWeakPtr<class SProjectCleanerUnusedAssetsBrowserUI> UnusedAssetsBrowserUI;
	TWeakPtr<class SProjectCleanerNonEngineFilesUI> NonEngineFilesUI;
	TWeakPtr<class SProjectCleanerConfigsUI> CleanerConfigsUI;
	TWeakPtr<class SProjectCleanerCorruptedFilesUI> CorruptedFilesUI;
	TWeakPtr<class SProjectCleanerIndirectAssetsUI> IndirectAssetsUI;
	TWeakPtr<class SProjectCleanerExcludeOptionsUI> ExcludeOptionUI;
	TWeakPtr<class SProjectCleanerExcludedAssetsUI> ExcludedAssetsUI;
	TSharedPtr<class FTabManager> TabManager;
	TSharedPtr<FTabManager::FLayout> TabLayout;

	/* Data */
	class ProjectCleanerManager CleanerManager;
};
