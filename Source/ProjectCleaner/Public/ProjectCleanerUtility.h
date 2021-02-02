// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Engine/StreamableManager.h"
#include "StructsContainer.h"
#include "CoreMinimal.h"

struct FAssetData;


/**
 * This class responsible for different file and directory operations in unreal engine context
 */
class PROJECTCLEANER_API ProjectCleanerUtility
{
public:
	// Check if given path contains files in it, non recursive
	static bool HasFiles(const FString& SearchPath);
	
	// Check if given path contains directories in it, non recursive
	static bool HasDirectories(const FString& SearchPath);
	
	// Finds all empty folders in given path recursive version
	static bool GetAllEmptyDirectories(const FString& SearchPath, TArray<FString>& Directories, const bool bIsRootDirectory);
	
	// Finds all child directions in given path
	static void GetChildrenDirectories(const FString& SearchPath, TArray<FString>& Output);

	// Removes from given Directories "Developers" and "Collections" folders
	static void RemoveDevsAndCollectionsDirectories(TArray<FString>& Directories);

	// Deletes unused assets
	static int32 DeleteUnusedAssets(TArray<FAssetData>& AssetsToDelete);

	// Deletes Empty Folders
	static void DeleteEmptyFolders(TArray<FString>& EmptyFolders);
	
	// Finding all assets in "Game" Root directory of project
	static void FindAllGameAssets(TArray<FAssetData>& GameAssetsContainer);
	
	// Removes all level assets(Maps) from GameAssetsContainer 
	static void RemoveLevelAssets(TArray<FAssetData>& GameAssetsContainer);
	
	// todo:ashe23 docs
	static void GetAllDependencies(const struct FARFilter& InAssetRegistryFilter, const class IAssetRegistry& AssetRegistry, TSet<FName>& OutDependencySet);
	
	// Returns total number of unused assets
	static int32 GetUnusedAssetsNum(TArray<FAssetData>& UnusedAssets);

	// Returns total number of empty folders
	static int32 GetEmptyFoldersNum(TArray<FString>& EmptyFolders);

	// Returns total size of unused assets (in bytes)
	static int64 GetUnusedAssetsTotalSize(TArray<FAssetData>& UnusedAssets);

	static void FixupRedirectors();

	// REFACTOR START
	static void FindAndCreateAssetTree(TArray<FAssetData>& UnusedAssets, TArray<FAssetChunk>& AssetChunks);
	static bool DepResolve(const FName& Asset, TArray<FName>& Resolved);
	static bool GetDependencyTree(const IAssetRegistry& AssetRegistry, const FName& Asset, TArray<FName>& Checked);
	static bool HasReferencer(const FName& Asset);
	// static bool DepResolveIterative(TArray<FName> Resolved);
	static bool IsLevelAsset(const FName& Asset);
	static void FindAllAssetsWithNoDependencies(TArray<FName>& Assets, const TArray<FAssetData>& AllAssets);
	void FindAllRefs(const FName& Root);
	static void DeleteAssetChunks(TArray<FAssetChunk>& AssetChunks);
	static void DeleteAssetsv2(TArray<FAssetData>& Assets);
	static void GetRootAssets(TArray<FAssetData>& RootAssets, TArray<FAssetData>& AllAssets);
	// REFACTOR END

	
	TArray<FAssetData> LevelDependencyAssets;
	FStreamableManager StreamableManager;
};
