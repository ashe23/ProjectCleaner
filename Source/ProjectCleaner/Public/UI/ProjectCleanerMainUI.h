// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/ProjectCleanerManager.h"
#include "Widgets/SCompoundWidget.h"

class SProjectCleanerMainUI : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SProjectCleanerMainUI) {}
		SLATE_ARGUMENT(FProjectCleanerManager*, CleanerManager)
    SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~SProjectCleanerMainUI() override;
	void SetCleanerManager(FProjectCleanerManager* CleanerManagerPtr);

private:
	/* Initializers */
	void InitTabs();
	void OnCleanerManagerUpdated() const;

	bool IsWidgetEnabled() const;
	int32 GetDefaultWidgetIndex() const;
	
	/* Tab Callbacks */
	TSharedRef<SDockTab> OnUnusedAssetTabSpawn(const FSpawnTabArgs& SpawnTabArgs);
	TSharedRef<SDockTab> OnExcludedAssetsTabSpawn(const FSpawnTabArgs& SpawnTabArgs);
	TSharedRef<SDockTab> OnNonEngineFilesTabSpawn(const FSpawnTabArgs& SpawnTabArgs);
	TSharedRef<SDockTab> OnCorruptedFilesTabSpawn(const FSpawnTabArgs& SpawnTabArgs);
	TSharedRef<SDockTab> OnIndirectAssetsTabSpawn(const FSpawnTabArgs& SpawnTabArgs);
	
	/* Btn Callbacks */
	FReply OnRefreshBtnClick() const;
	FReply OnDeleteUnusedAssetsBtnClick() const;
	FReply OnDeleteEmptyFolderClick() const;
	
	/* UI Data */
	TWeakPtr<class SProjectCleanerStatisticsUI> StatisticsUI;
	TWeakPtr<class SProjectCleanerUnusedAssetsBrowserUI> UnusedAssetsBrowserUI;
	TWeakPtr<class SProjectCleanerNonEngineFilesUI> NonEngineFilesUI;
	TWeakPtr<class SProjectCleanerConfigsUI> CleanerConfigsUI;
	TWeakPtr<class SProjectCleanerCorruptedFilesUI> CorruptedFilesUI;
	TWeakPtr<class SProjectCleanerIndirectAssetsUI> IndirectAssetsUI;
	TWeakPtr<class SProjectCleanerExcludedAssetsUI> ExcludedAssetsUI;
	TSharedPtr<class FTabManager> TabManager;
	TSharedPtr<FTabManager::FLayout> TabLayout;

	/* CleanerManager */
	FProjectCleanerManager* CleanerManager = nullptr;
};
