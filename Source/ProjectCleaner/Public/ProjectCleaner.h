// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Input/Reply.h"
#include "Brushes/SlateColorBrush.h"
#include "StructsContainer.h"
#include "UI/SProjectCleanerBrowser.h"
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
	FProjectCleanerModule() :
			TipOneBrushColor(FSlateColorBrush{ FLinearColor{0.5625f, 0.9296875f, 0.5625f, 1.0f } }),
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

	// Stats
	FCleaningStats CleaningStats;
	FStandardCleanerText StandardCleanerText;
private:
	TSharedRef<SDockTab> OnSpawnPluginTab(const class FSpawnTabArgs& SpawnTabArgs);
	TSharedPtr<class FUICommandList> PluginCommands;
	TArray<FAssetData> UnusedAssets;
	TArray<FString> EmptyFolders;
	TArray<FString> NonProjectFiles;
	ProjectCleanerNotificationManager* NotificationManager;
	TSharedPtr<SWindow> TestWindow;
	TWeakPtr<SProjectCleanerBrowser> ProjectCleanerBrowserUI;
	UDirectoryFilterSettings* DirectoryFilterSettings;
	// Slate styles
	FSlateColorBrush TipOneBrushColor;
	FSlateColorBrush TipTwoBrushColor;
	
	// test
	TArray<FNode> AdjacencyList;

private:
	void InitModuleComponents();
};


