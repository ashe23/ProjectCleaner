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
	void FindEmptyFolderRecursive(const FString Path, bool bRootPath);
	void FindEmptyFolderRecursive2(const FString Path, bool bRootPath);
	void RemoveDevAndCollectionFolders(TArray<FString>& Directories);
	int64 FindUnusedAssetsFileSize();
	bool HasFiles(const FString& Dir) const;
	bool IsEmptyFolder(const FString& Dir) const;
	void GetChildFolders(const FString& Path, TArray<FString>& Output) const;
	// Excluding Build_data and Level assets
	void RemoveLevelAssets(TArray<FAssetData>& GameAssetsContainer) const;
	void GetAllDependencies(const struct FARFilter& InAssetRegistryFilter, const class IAssetRegistry& AssetRegistry, TSet<FName>& OutDependencySet);
// #if WITH_EDITOR
	int32 DeleteUnusedAssets(TArray<FAssetData>& AssetsToDelete);
	static void DeleteEmptyFolder(const TArray<FName>& DirectoriesToDelete);
// #endif

private:
	// Button events
	FReply OnDeleteEmptyFolderClick();
	FReply OnDeleteUnusedAssetsBtnClick();
private:
	TSharedRef<class SDockTab> OnSpawnPluginTab(const class FSpawnTabArgs& SpawnTabArgs);
	TSharedPtr<class FUICommandList> PluginCommands;
	TArray<FAssetData> UnusedAssets;
	TArray<FName> EmptyFolders;
};


