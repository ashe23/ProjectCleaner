// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Input/Reply.h"

class FToolBarBuilder;
class FMenuBuilder;
struct FAssetData;

class FProjectCleanerModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	
	/** This function will be bound to Command. */
	void PluginButtonClicked();
	
private:

	void AddToolbarExtension(FToolBarBuilder& Builder);
	void AddMenuExtension(FMenuBuilder& Builder);

	// Finding all assets in "Game" Root directory of project
	void FindAllGameAssets(TArray<FAssetData>& GameAssetsContainer) const;
	int32 FindUnusedAssets();
	int32 FindEmptyFolders();
	int64 FindUnusedAssetsFileSize();
	void UpdateStats();
	// Excluding Build_data and Level assets
	void RemoveLevelAssets(TArray<FAssetData>& GameAssetsContainer) const;
	void GetAllDependencies(const struct FARFilter& InAssetRegistryFilter, const class IAssetRegistry& AssetRegistry, TSet<FName>& OutDependencySet);
// #if WITH_EDITOR
	int32 DeleteUnusedAssets(TArray<FAssetData>& AssetsToDelete);
	void DeleteEmptyFolders();
	static void DeleteEmptyFolder(const TArray<FName>& DirectoriesToDelete);
// #endif

private:
	// Button events
	FReply OnDeleteEmptyFolderClick();
	FReply OnDeleteUnusedAssetsBtnClick();
	// Stats
	int32 UnusedAssetsCount = 0;
	int64 UnusedAssetsFilesSize = 0;
	int32 EmptyFoldersCount = 0;
private:
	TSharedRef<class SDockTab> OnSpawnPluginTab(const class FSpawnTabArgs& SpawnTabArgs);
	TSharedPtr<class FUICommandList> PluginCommands;
	TArray<FAssetData> UnusedAssets;
	TArray<FString> EmptyFolders;
};


