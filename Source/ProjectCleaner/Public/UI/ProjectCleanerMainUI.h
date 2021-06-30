// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/ProjectCleanerManager.h"
#include "Widgets/SCompoundWidget.h"

class FTabManager;
class FUICommandList;
class ProjectCleanerManager;
class SProjectCleanerBrowserStatisticsUI;
class SProjectCleanerUnusedAssetsBrowserUI;
struct FProjectCleanerData;

class SProjectCleanerMainUI : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SProjectCleanerMainUI) {}
		SLATE_ARGUMENT(FProjectCleanerData*, CleanerData)
    SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~SProjectCleanerMainUI() override;

private:
	/* Initializers */
	void InitTabs();
	
	/* Callbacks */
	TSharedRef<SDockTab> OnUnusedAssetTabSpawn(const FSpawnTabArgs& SpawnTabArgs);
	TSharedRef<SDockTab> OnNonEngineFilesTabSpawn(const FSpawnTabArgs& SpawnTabArgs);
	TSharedRef<SDockTab> OnCorruptedFilesTabSpawn(const FSpawnTabArgs& SpawnTabArgs);
	TSharedRef<SDockTab> OnIndirectAssetsTabSpawn(const FSpawnTabArgs& SpawnTabArgs);
	
	/* UI Data */
	TWeakPtr<SProjectCleanerBrowserStatisticsUI> StatisticsUI;
	TWeakPtr<SProjectCleanerUnusedAssetsBrowserUI> UnusedAssetsBrowserUI;
	TWeakPtr<SProjectCleanerUnusedAssetsBrowserUI> NonEngineFilesUI;
	TSharedPtr<FTabManager> TabManager;
	TSharedPtr<FTabManager::FLayout> TabLayout;

	/* Data */
	ProjectCleanerManager CleanerManager;
};
