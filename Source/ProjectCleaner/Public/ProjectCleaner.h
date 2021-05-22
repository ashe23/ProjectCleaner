// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "StructsContainer.h"
#include "UI/ProjectCleanerBrowserStatisticsUI.h"
#include "UI/ProjectCleanerDirectoryExclusionUI.h"
#include "UI/ProjectCleanerUnusedAssetsBrowserUI.h"
#include "UI/ProjectCleanerNonUassetFilesUI.h"
#include "UI/ProjectCleanerSourceCodeAssetsUI.h"
#include "UI/ProjectCleanerCorruptedFilesUI.h"
#include "UI/ProjectCleanerExcludedAssetsUI.h"
// Engine Headers
#include "Input/Reply.h"
#include "Modules/ModuleInterface.h"
#include "ContentBrowserDelegates.h"
#include "CoreMinimal.h"

class ProjectCleanerNotificationManager;
class FToolBarBuilder;
class STableViewBase;
class FMenuBuilder;
class ITableRow;
class SDockTab;
class FUICommandList;
class FSpawnTabArgs;

struct FSlateColorBrush;
struct FAssetData;
struct FSlateBrush;
struct FNonUassetFile;

DECLARE_LOG_CATEGORY_EXTERN(LogProjectCleaner, Log, All);

class FProjectCleanerModule : public IModuleInterface
{
public:
	FProjectCleanerModule():
		ExcludeDirectoryFilterSettings(nullptr)
	{
	}
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	virtual bool IsGameModule() const override;
private:
    /** Module **/	
	void RegisterMenus();
	void PluginButtonClicked();
	TSharedRef<SDockTab> OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs);

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
	
	//
	// /**
	//  * @brief Opens window with corrupted files in it
	//  */
	// void OpenCorruptedFilesWindow();
	
	/** UI */
	TSharedPtr<FUICommandList> PluginCommands;
	TSharedPtr<ProjectCleanerNotificationManager> NotificationManager;
	TWeakPtr<SProjectCleanerUnusedAssetsBrowserUI> UnusedAssetsBrowserUI;
	TWeakPtr<SProjectCleanerNonUassetFilesUI> NonUassetFilesUI;
	TWeakPtr<SProjectCleanerBrowserStatisticsUI> StatisticsUI;
	TWeakPtr<SProjectCleanerSourceCodeAssetsUI> SourceCodeAssetsUI;
	TWeakPtr<SProjectCleanerDirectoryExclusionUI> DirectoryExclusionUI;
	TWeakPtr<SProjectCleanerCorruptedFilesUI> CorruptedFilesUI;
	TWeakPtr<SProjectCleanerExcludedAssetsUI> ExcludedAssetsUI;
	
	/** Data Containers */ 
	TArray<FAssetData> UnusedAssets;
	TSet<FAssetData> ExcludedAssets;
	TSet<FName> EmptyFolders;
	TSet<FName> NonUassetFiles;
	TSet<FName> CorruptedFiles;
	FCleaningStats CleaningStats;
	TArray<TWeakObjectPtr<USourceCodeAsset>> SourceCodeAssets;
	UExcludeDirectoriesFilterSettings* ExcludeDirectoryFilterSettings;
	
	/** Helper Containers **/
	TArray<FNode> AdjacencyList;
	TArray<FSourceCodeFile> SourceFiles;
	FStandardCleanerText StandardCleanerText;
};