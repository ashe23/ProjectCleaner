// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"
#include "Input/Reply.h"
#include "CoreMinimal.h"
#include "SlateColorBrush.h"

class FToolBarBuilder;
class FMenuBuilder;
struct FAssetData;
struct FSlateBrush;
struct FSlateColorBrush;


class FProjectCleanerModule : public IModuleInterface
{
public:
	FProjectCleanerModule() :
			TipOneBrushColor(FSlateColorBrush{ FLinearColor{1.0f, 0.973208f, 0.0f, 0.227f } }),
			TipTwoBrushColor(FSlateColorBrush{ FLinearColor{1.0f, 0.039437f, 0.0f, 0.227f } })
	{
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
	void ShowOperationProgress();
private:
	// Button events
	FReply OnDeleteEmptyFolderClick();
	FReply OnDeleteUnusedAssetsBtnClick();
	// Stats
	int32 UnusedAssetsCount = 0;
	int32 EmptyFoldersCount = 0;
	int64 UnusedAssetsFilesSize = 0;
private:
	TSharedRef<class SDockTab> OnSpawnPluginTab(const class FSpawnTabArgs& SpawnTabArgs);
	TSharedPtr<class FUICommandList> PluginCommands;
	TArray<FAssetData> UnusedAssets;
	TArray<FString> EmptyFolders;

	// slate
	FSlateColorBrush TipOneBrushColor;
	FSlateColorBrush TipTwoBrushColor;
};


