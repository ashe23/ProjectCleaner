// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "StructsContainer.h"
#include "Graph/AssetRelationalMap.h"
// Engine Headers
#include "Input/Reply.h"
#include "Modules/ModuleInterface.h"
#include "ContentBrowserDelegates.h"
#include "CoreMinimal.h"

class AssetRelationalMap;
class ProjectCleanerNotificationManager;
class FToolBarBuilder;
class STableViewBase;
class FMenuBuilder;
class ITableRow;
class SDockTab;
class FUICommandList;
class FSpawnTabArgs;
// Other Engine Modules
class FAssetRegistryModule;
class FContentBrowserModule;
class UAssetManager;

class SProjectCleanerUnusedAssetsBrowserUI;
class SProjectCleanerNonUassetFilesUI;
class SProjectCleanerBrowserStatisticsUI;
class SProjectCleanerSourceCodeAssetsUI;
class SProjectCleanerExcludeOptionsUI;
class SProjectCleanerCorruptedFilesUI;
class SProjectCleanerExcludedAssetsUI;
class SProjectCleanerConfigsUI;
class UCleanerConfigs;
class USourceCodeAsset;
class UExcludeOptions;
class AssetRelationalMap;

struct FSlateColorBrush;
struct FAssetData;
struct FSlateBrush;
struct FNonUassetFile;
struct FPrimaryAssetTypeInfo;

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
	TSharedRef<SDockTab> OnUnusedAssetTabSpawn(const FSpawnTabArgs& SpawnTabArgs);
	TSharedRef<SDockTab> OnNonUAssetFilesTabSpawn(const FSpawnTabArgs& SpawnTabArgs);
	TSharedRef<SDockTab> OnCorruptedFilesTabSpawn(const FSpawnTabArgs& SpawnTabArgs);
	TSharedRef<SDockTab> OnSourceCodeAssetsTabSpawn(const FSpawnTabArgs& SpawnTabArgs);
	void OnScanDeveloperContentCheckboxChanged(ECheckBoxState State);
	void OnAutomaticallyRemoveEmptyFoldersCheckboxChanged(ECheckBoxState State);

	/** Cleaner **/
	
	/**
	 * @brief Saves all unsaved assets, fixes up redirectors and updates cleaner data
	 */
	void UpdateCleaner();
	
	/**
	 * @brief Scans project for unused assets, empty folders, corrupted files and non .uasset files
	 */
	void UpdateCleanerData();
	
	/**
	 * @brief Updates Cleaning stats
	 */
	void UpdateStats();
	
	/**
	 * @brief Resets all data containers
	 */
	void Reset();

	/**
	 * Sets content browser focus to root directory and refreshes asset registry
	 * @brief Updates content browser
	 */
	void UpdateContentBrowser() const;

	void CleanEmptyFolders();
	
	/**
	 * @brief Creates confirmation window with yes/no options
	 * @param Title - Window Title
	 * @param ContentText - Window content text
	 * @return EAppReturnType::Type Window confirmation status
	 */
	EAppReturnType::Type ShowConfirmationWindow(const FText& Title, const FText& ContentText) const;
	
	/**
	 * @brief Checks if confirmation window cancelled or not
	 * @param Status EAppReturnType::Type
	 * @return bool
	 */
	static bool IsConfirmationWindowCanceled(EAppReturnType::Type Status);
	
	/** Delegate functions **/
	void OnUserDeletedAssets();
	void OnUserExcludedAssets(const TArray<FAssetData>& Assets);
	void OnUserIncludedAssets(const TArray<FAssetData>& Assets, const bool CleanFilters);

	/** Button events */
	FReply OnRefreshBtnClick();
	FReply OnDeleteUnusedAssetsBtnClick();
	FReply OnDeleteEmptyFolderClick();
	
	
	/** UI */
	TSharedPtr<FUICommandList> PluginCommands;
	TSharedPtr<ProjectCleanerNotificationManager> NotificationManager;
	TWeakPtr<SProjectCleanerUnusedAssetsBrowserUI> UnusedAssetsBrowserUI;
	TWeakPtr<SProjectCleanerNonUassetFilesUI> NonUassetFilesUI;
	TWeakPtr<SProjectCleanerBrowserStatisticsUI> StatisticsUI;
	TWeakPtr<SProjectCleanerSourceCodeAssetsUI> SourceCodeAssetsUI;
	TWeakPtr<SProjectCleanerExcludeOptionsUI> ExcludeOptionUI;
	TWeakPtr<SProjectCleanerCorruptedFilesUI> CorruptedFilesUI;
	TWeakPtr<SProjectCleanerExcludedAssetsUI> ExcludedAssetsUI;
	TWeakPtr<SProjectCleanerConfigsUI> CleanerConfigsUI;
	TSharedPtr<FTabManager> TabManager;
	TSharedPtr<FTabManager::FLayout> TabLayout;

	/** UI data*/
	UCleanerConfigs* CleanerConfigs;
	UExcludeOptions* ExcludeOptions;
	
	/** Data Containers */ 
	TArray<FAssetData> UnusedAssets;
	TArray<FString> EmptyFolders;
	TSet<FString> NonUAssetFiles;
	TSet<FString> CorruptedFiles;
	FCleaningStats CleaningStats;
	TArray<FAssetData> ExcludedAssets;
	TArray<FAssetData> LinkedAssets;
	TArray<FAssetData> UserExcludedAssets;
	TSet<FName> PrimaryAssetClasses;
	TArray<TWeakObjectPtr<USourceCodeAsset>> SourceCodeAssets;
	
	/** Helper Containers **/
	TSet<FString> ProjectFilesFromDisk;
	AssetRelationalMap RelationalMap;
	FStandardCleanerText StandardCleanerText;

	/** Other Engine Modules **/
	FAssetRegistryModule* AssetRegistry;
	UAssetManager* AssetManager;
	FContentBrowserModule* ContentBrowser;
	void OnFilesLoaded();
	bool bCanOpenTab = false;
};