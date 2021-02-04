// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"
#include "Input/Reply.h"
#include "SlateColorBrush.h"
#include "StructsContainer.h"
#include "CoreMinimal.h"

class FToolBarBuilder;
class FMenuBuilder;
struct FAssetData;
struct FSlateBrush;
struct FSlateColorBrush;
class ProjectCleanerNotificationManager;

class FProjectCleanerModule : public IModuleInterface
{
public:
	FProjectCleanerModule() :
			TipOneBrushColor(FSlateColorBrush{ FLinearColor{1.0f, 0.973208f, 0.0f, 0.227f } }),
			TipTwoBrushColor(FSlateColorBrush{ FLinearColor{1.0f, 0.039437f, 0.0f, 0.227f } })
	{
		NotificationManager = nullptr;
	}
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	
	/** This function will be bound to Command. */
	void PluginButtonClicked();
	
private:

	void AddToolbarExtension(FToolBarBuilder& Builder);
	void AddMenuExtension(FMenuBuilder& Builder);

	void UpdateStats();
	void InitCleaner();
private:
	// Button events
	FReply OnDeleteEmptyFolderClick();
	FReply OnDeleteUnusedAssetsBtnClick();
	FReply CloseModalWindow() const;
	// Stats
	FCleaningStats CleaningStats;
private:
	void OnSpawnPluginTab(const class FSpawnTabArgs& SpawnTabArgs);
	TSharedPtr<class FUICommandList> PluginCommands;
	TArray<FAssetData> UnusedAssets;
	TArray<FString> EmptyFolders;
	TArray<FString> NonProjectFiles;
	TArray<FString> ProjectAllSourceFiles;
	ProjectCleanerNotificationManager* NotificationManager;
	TSharedPtr<SWindow> ParentWindow;
	
	// Slate styles
	FSlateColorBrush TipOneBrushColor;
	FSlateColorBrush TipTwoBrushColor;
};


