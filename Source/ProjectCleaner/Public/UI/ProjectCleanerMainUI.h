// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/ProjectCleanerManager.h"
#include "Widgets/SCompoundWidget.h"

class SProjectCleanerMainUI : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SProjectCleanerMainUI) {}
		SLATE_ARGUMENT(ProjectCleanerManager*, CleanerManager)
    SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~SProjectCleanerMainUI() override;

private:
	/* Initializers */
	void InitTabs();
	void Update() const;
	void UpdateUIData() const;
	
	/* Callbacks */
	TSharedRef<SDockTab> OnUnusedAssetTabSpawn(const FSpawnTabArgs& SpawnTabArgs);
	TSharedRef<SDockTab> OnNonEngineFilesTabSpawn(const FSpawnTabArgs& SpawnTabArgs);
	TSharedRef<SDockTab> OnCorruptedFilesTabSpawn(const FSpawnTabArgs& SpawnTabArgs);
	TSharedRef<SDockTab> OnIndirectAssetsTabSpawn(const FSpawnTabArgs& SpawnTabArgs);

	/* Delegates */
	void OnUserDeletedAssets() const;
	void OnUserExcludedAssets(const TArray<FAssetData>& Assets) const;
	void OnUserIncludedAssets(const TArray<FAssetData>& Assets, const bool CleanFilters) const; 
	
	/* Btn Callbacks*/
	FReply OnRefreshBtnClick() const;
	FReply OnDeleteUnusedAssetsBtnClick() const;
	FReply OnDeleteEmptyFolderClick() const;
	EAppReturnType::Type ShowConfirmationWindow(const FText& Title, const FText& ContentText) const;
	static bool IsConfirmationWindowCanceled(EAppReturnType::Type Status);
	
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
	class ProjectCleanerManager* CleanerManager = nullptr;
};
