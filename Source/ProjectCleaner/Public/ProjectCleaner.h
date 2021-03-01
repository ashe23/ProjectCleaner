// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "StructsContainer.h"
#include "UI/SProjectCleanerBrowser.h"

// Engine Headers
#include "Input/Reply.h"
#include "Modules/ModuleInterface.h"
#include "CoreMinimal.h"

class FToolBarBuilder;
class FMenuBuilder;
class ITableRow;
class STableViewBase;
class SDockTab;
class ProjectCleanerNotificationManager;
struct FAssetData;
struct FSlateBrush;
struct FSlateColorBrush;

DECLARE_LOG_CATEGORY_EXTERN(LogProjectCleaner, Log, All);

class FProjectCleanerModule : public IModuleInterface
{
public:
	FProjectCleanerModule()
	{
		NotificationManager = nullptr;
		DirectoryFilterSettings = nullptr;
	}
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	
	/**
	 * @brief Opens ProjectCleanerBrowser Main Tab
	 */
	void PluginButtonClicked();
	
private:
	void InitModuleComponents();
	void AddToolbarExtension(FToolBarBuilder& Builder);
	void AddMenuExtension(FMenuBuilder& Builder);
	TSharedRef<SDockTab> OnSpawnPluginTab(const class FSpawnTabArgs& SpawnTabArgs);

	FReply RefreshBrowser();
	void UpdateStats();
	void InitCleaner();
	void Reset();
	/**
	 * Sets content browser focus to root directory and refreshes asset registry
	 * @brief Updates content browser
	 */
	void UpdateContentBrowser() const;

	// Button events Start //
	FReply OnDeleteEmptyFolderClick();
	FReply OnDeleteUnusedAssetsBtnClick();
	// Button events End //

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

	FCleaningStats CleaningStats;
	FStandardCleanerText StandardCleanerText;
	TSharedPtr<class FUICommandList> PluginCommands;
	TArray<FAssetData> UnusedAssets;
	TArray<FNode> AdjacencyList;
	TArray<FString> EmptyFolders;
	TArray<FString> NonProjectFiles;
	TArray<FSourceCodeFile> SourceFiles;
	ProjectCleanerNotificationManager* NotificationManager;
	TWeakPtr<SProjectCleanerBrowser> ProjectCleanerBrowserUI;
	UDirectoryFilterSettings* DirectoryFilterSettings;
};


