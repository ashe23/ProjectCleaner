// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "StructsContainer.h"
#include "UI/ProjectCleanerBrowserStatisticsUI.h"
#include "UI/ProjectCleanerDirectoryExclusionUI.h"
#include "UI/ProjectCleanerUnusedAssetsBrowserUI.h"
#include "UI/ProjectCleanerNonUassetFilesUI.h"
#include "UI/ProjectCleanerAssetsUsedInSourceCodeUI.h"
#include "UI/ProjectCleanerCorruptedFilesUI.h"
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

struct FSlateColorBrush;
struct FAssetData;
struct FSlateBrush;
struct FNonUassetFile;

DECLARE_LOG_CATEGORY_EXTERN(LogProjectCleaner, Log, All);

class FProjectCleanerModule : public IModuleInterface
{
public:
	FProjectCleanerModule():
		ExcludeDirectoryFilterSettings(nullptr),
		StreamableManager(nullptr)
	{
	}

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	virtual bool IsGameModule() const override;


	void ExcludeAssetsFromDeletionList(const TArray<FAssetData>& Assets) const;
private:
	void InitModuleComponents();
	void AddToolbarExtension(FToolBarBuilder& Builder);
	void AddMenuExtension(FMenuBuilder& Builder);
	/**
	 * @brief Opens ProjectCleanerBrowser Main Tab
	 */
	void PluginButtonClicked();
	TSharedRef<SDockTab> OnSpawnPluginTab(const class FSpawnTabArgs& SpawnTabArgs);

	/** Button events */
	FReply RefreshBrowser();
	FReply OnDeleteUnusedAssetsBtnClick();
	FReply OnDeleteEmptyFolderClick();
	
	/**
	 * @brief Updates Cleaning stats
	 */
	void UpdateStats();
	/**
	 * @brief Saves all unsaved assets, fixes up redirectors and updates cleaner data
	 */
	void UpdateCleaner();
	/**
	 * @brief Resets all data containers
	 */
	void Reset();
	/**
	 * @brief Scans project for unused assets, empty folders, corrupted files and non .uasset files
	 */
	void UpdateCleanerData();
	/**
	 * Sets content browser focus to root directory and refreshes asset registry
	 * @brief Updates content browser
	 */
	void UpdateContentBrowser() const;

	void ScanProjectFiles();

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

	/** UI */
	TSharedPtr<FUICommandList> PluginCommands;
	TSharedPtr<ProjectCleanerNotificationManager> NotificationManager;
	TWeakPtr<SProjectCleanerDirectoryExclusionUI> ProjectCleanerDirectoryExclusionUI;
	UExcludeDirectoriesFilterSettings* ExcludeDirectoryFilterSettings;

	TWeakPtr<SProjectCleanerBrowserStatisticsUI> ProjectCleanerBrowserStatisticsUI;
	TWeakPtr<SProjectCleanerUnusedAssetsBrowserUI> ProjectCleanerUnusedAssetsBrowserUI;
	TWeakPtr<SProjectCleanerNonUassetFilesUI> ProjectCleanerNonUassetFilesUI;
	TWeakPtr<SProjectCleanerAssetsUsedInSourceCodeUI> ProjectCleanerAssetsUsedInSourceCodeUI;
	TWeakPtr<SProjectCleanerCorruptedFilesUI> ProjectCleanerCorruptedFilesUI;

	/** Data Containers */ 
	TArray<FAssetData> UnusedAssets;
	TArray<FAssetData> ExcludedAssets;
	TArray<FString> EmptyFolders;
	TArray<FNode> AdjacencyList;
	TArray<FAssetData> CorruptedFiles;
	TArray<TWeakObjectPtr<UNonUassetFile>> NonUassetFiles;
	TArray<FSourceCodeFile> SourceFiles;
	TArray<TWeakObjectPtr<USourceCodeAsset>> SourceCodeAssets;
	FCleaningStats CleaningStats;
	FStandardCleanerText StandardCleanerText;

	/** Streamable Manager */
	struct FStreamableManager* StreamableManager;
	void OpenCorruptedFilesWindow();
	/**
	 * @brief Loads given Assets synchronously and checking for corrupted files and adding them to list.
	 * @param Assets
	 * @param CorruptedAssets
	 */
	void FindCorruptedAssets(const TArray<FAssetData>& Assets, TArray<FAssetData>& CorruptedAssets);
};
