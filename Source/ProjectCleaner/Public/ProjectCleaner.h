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
class FAssetRegistryModule;
class SProjectCleanerUnusedAssetsBrowserUI;
class SProjectCleanerNonUassetFilesUI;
class SProjectCleanerBrowserStatisticsUI;
class SProjectCleanerSourceCodeAssetsUI;
class SProjectCleanerExcludeOptionsUI;
class SProjectCleanerCorruptedFilesUI;
class SProjectCleanerExcludedAssetsUI;
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
	void OnUserIncludedAssets(const TArray<FAssetData>& Assets);

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
	TSharedPtr<FTabManager> TabManager;
	TSharedPtr<FTabManager::FLayout> TabLayout;
	
	/** Data Containers */ 
	TArray<FAssetData> UnusedAssets;
	TArray<FString> EmptyFolders;
	TSet<FName> NonUAssetFiles;
	TSet<FName> CorruptedFiles;
	FCleaningStats CleaningStats;
	FCleanerConfigs CleanerConfigs;
	TArray<TWeakObjectPtr<USourceCodeAsset>> SourceCodeAssets;


	TSet<FAssetData> ExcludedAssets;
	TArray<FAssetData> LinkedAssets;
	UExcludeOptions* ExcludeOptions;
	
	/** Helper Containers **/
	TSet<FName> ProjectFilesFromDisk;
	TArray<FSourceCodeFile> SourceCodeFiles;
	AssetRelationalMap RelationalMap;
	FStandardCleanerText StandardCleanerText;
	TSet<FName> PrimaryAssetClasses;
	TArray<FAssetData> UserExcludedAssets;

	/** Other Engine Modules **/
	FAssetRegistryModule* AssetRegistry;
	void OnFilesLoaded();
	bool bCanOpenTab = false;
};