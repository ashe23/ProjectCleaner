// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"
#include "Input/Reply.h"
#include "SlateColorBrush.h"
#include "StructsContainer.h"
#include "UI/SProjectCleanerBrowser.h"
#include "CoreMinimal.h"

class FToolBarBuilder;
class FMenuBuilder;
struct FAssetData;
struct FSlateBrush;
struct FSlateColorBrush;
class ITableRow;
class STableViewBase;
class ProjectCleanerNotificationManager;

class FProjectCleanerModule : public IModuleInterface
{
public:
	FProjectCleanerModule() :
			TipOneBrushColor(FSlateColorBrush{ FLinearColor{1.0f, 0.973208f, 0.0f, 0.227f } }),
			TipTwoBrushColor(FSlateColorBrush{ FLinearColor{1.0f, 0.039437f, 0.0f, 0.227f } })
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

	void AddToolbarExtension(FToolBarBuilder& Builder);
	void AddMenuExtension(FMenuBuilder& Builder);

	FReply RefreshBrowser();
	void UpdateStats();
	void InitCleaner();
	
	/**
	 * If user entered any exclude directory, then we should exclude given directories from scanning
	 * @brief Checks if any exclude directory exists. 
	 * @return bool 
	 */
	bool ShouldApplyDirectoryFilters() const;
	
	/**
	 * Excludes assets and empty folders that not passing user filter
	 * @brief 
	 */
	void ApplyDirectoryFilters();
private:
	// Button events
	FReply OnDeleteEmptyFolderClick();
	FReply OnDeleteUnusedAssetsBtnClick();

	// Stats
	FCleaningStats CleaningStats;
private:
	TSharedRef<SDockTab> OnSpawnPluginTab(const class FSpawnTabArgs& SpawnTabArgs);
	TSharedPtr<class FUICommandList> PluginCommands;
	TArray<FAssetData> UnusedAssets;
	TArray<FString> EmptyFolders;
	TArray<FString> NonProjectFiles;
	TArray<FString> ProjectAllSourceFiles;
	ProjectCleanerNotificationManager* NotificationManager;
	TSharedPtr<SWindow> TestWindow;
	TWeakPtr<SProjectCleanerBrowser> ProjectCleanerBrowserUI;
	UDirectoryFilterSettings* DirectoryFilterSettings;
	// Slate styles
	FSlateColorBrush TipOneBrushColor;
	FSlateColorBrush TipTwoBrushColor;
};


