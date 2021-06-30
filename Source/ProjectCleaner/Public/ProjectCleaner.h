// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#pragma once


// Engine Headers
#include "Input/Reply.h"
#include "Modules/ModuleInterface.h"
#include "ContentBrowserDelegates.h"
#include "CoreMinimal.h"



class FAssetRegistryModule;
class SProjectCleanerMainUI;
// class ProjectCleanerRelationalMap; 
// class ProjectCleanerNotificationManager;
// class FToolBarBuilder;
// class STableViewBase;
// class FMenuBuilder;
// class ITableRow;
// class SDockTab;
// class FUICommandList;
// class FSpawnTabArgs;
// Other Engine Modules
// class FContentBrowserModule;
// class UAssetManager;

// class SProjectCleanerUnusedAssetsBrowserUI;
// class SProjectCleanerNonUassetFilesUI;
// class SProjectCleanerBrowserStatisticsUI;
// class SProjectCleanerSourceCodeAssetsUI;
// class SProjectCleanerExcludeOptionsUI;
// class SProjectCleanerCorruptedFilesUI;
// class SProjectCleanerExcludedAssetsUI;
// class SProjectCleanerConfigsUI;
// class UCleanerConfigs;
// class UIndirectAsset;
// class UExcludeOptions;
// class AssetRelationalMap;

// struct FSlateColorBrush;
// struct FAssetData;
// struct FSlateBrush;
// struct FNonUassetFile;
// struct FPrimaryAssetTypeInfo;

DECLARE_LOG_CATEGORY_EXTERN(LogProjectCleaner, Log, All);

class FProjectCleanerModule : public IModuleInterface
{
public:
	FProjectCleanerModule();
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	virtual bool IsGameModule() const override;
private:
    /** Module **/	
	void RegisterMenus();
	void PluginButtonClicked();
	TSharedRef<SDockTab> OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs);
	// TSharedRef<SDockTab> OnUnusedAssetTabSpawn(const FSpawnTabArgs& SpawnTabArgs);
	// TSharedRef<SDockTab> OnNonUAssetFilesTabSpawn(const FSpawnTabArgs& SpawnTabArgs);
	// TSharedRef<SDockTab> OnCorruptedFilesTabSpawn(const FSpawnTabArgs& SpawnTabArgs);
	// TSharedRef<SDockTab> OnSourceCodeAssetsTabSpawn(const FSpawnTabArgs& SpawnTabArgs);
	// void OnScanDeveloperContentCheckboxChanged(ECheckBoxState State);
	// void OnAutomaticallyRemoveEmptyFoldersCheckboxChanged(ECheckBoxState State);

	/** Cleaner **/	
	// void UpdateCleaner();
	// void UpdateCleanerData();
	// void UpdateStats();
	// void Reset();
	// void UpdateContentBrowser() const;
	// void CleanEmptyFolders();
	
	// EAppReturnType::Type ShowConfirmationWindow(const FText& Title, const FText& ContentText) const;
	// static bool IsConfirmationWindowCanceled(EAppReturnType::Type Status);
	
	/** Delegate functions **/
	// void OnUserDeletedAssets();
	// void OnUserExcludedAssets(const TArray<FAssetData>& Assets);
	// void OnUserIncludedAssets(const TArray<FAssetData>& Assets, const bool CleanFilters);
	
	/** Button events */
	// FReply OnRefreshBtnClick();
	// FReply OnDeleteUnusedAssetsBtnClick();
	// FReply OnDeleteEmptyFolderClick();
	
	
	/** UI */
	TSharedPtr<FUICommandList> PluginCommands;
	TWeakPtr<SProjectCleanerMainUI> CleanerMainUI;
	// TSharedPtr<ProjectCleanerNotificationManager> NotificationManager;
	// TWeakPtr<SProjectCleanerUnusedAssetsBrowserUI> UnusedAssetsBrowserUI;
	// TWeakPtr<SProjectCleanerNonUassetFilesUI> NonUassetFilesUI;
	// TWeakPtr<SProjectCleanerBrowserStatisticsUI> StatisticsUI;
	// // TWeakPtr<SProjectCleanerSourceCodeAssetsUI> SourceCodeAssetsUI;
	// TWeakPtr<SProjectCleanerExcludeOptionsUI> ExcludeOptionUI;
	// TWeakPtr<SProjectCleanerCorruptedFilesUI> CorruptedFilesUI;
	// TWeakPtr<SProjectCleanerExcludedAssetsUI> ExcludedAssetsUI;
	// TWeakPtr<SProjectCleanerConfigsUI> CleanerConfigsUI;
	// TSharedPtr<FTabManager> TabManager;
	// TSharedPtr<FTabManager::FLayout> TabLayout;
	
	/** Other Engine Modules **/
	FAssetRegistryModule* AssetRegistry;
};