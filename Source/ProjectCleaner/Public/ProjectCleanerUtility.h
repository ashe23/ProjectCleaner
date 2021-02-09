// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

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

	// Finds all empty folders in given path recursive version
	static bool GetAllEmptyDirectories(const FString& SearchPath, TArray<FString>& Directories, TArray<FString>& NonProjectFiles, const bool bIsRootDirectory);
	
	// Finds all child directions in given path
	static void GetChildrenDirectories(const FString& SearchPath, TArray<FString>& Output);

	// Removes from given Directories "Developers" and "Collections" folders
	static void RemoveDevsAndCollectionsDirectories(TArray<FString>& Directories);

	// Deletes Empty Folders
	static void DeleteEmptyFolders(TArray<FString>& EmptyFolders);
	
	
	/**
	 * Finds all game assets in "Root" project directory
	 * @brief Finds all assets in project
	 * @param GameAssetsContainer Container for all game assets
	 */
	static void FindAllGameAssets(TArray<FAssetData>& GameAssetsContainer);
	
	// Removes all level assets(Maps) from GameAssetsContainer 
	static void RemoveLevelAssets(TArray<FAssetData>& GameAssetsContainer);
	
	// Returns all level dependencies by given filter
	static void GetAllDependencies(const struct FARFilter& InAssetRegistryFilter, const class IAssetRegistry& AssetRegistry, TSet<FName>& OutDependencySet);
	
	/**
	 * Finds all unused assets in project
	 * This function looking asset only in root project content directory
	 * Assets in Developer and Collections will not be scanned
	 * Also this function will scan any source files ".h" and ".cpp" files that contains hardlink to asset
	 * 
	 * @brief Returns total amount of founded unused assets
	 * @param UnusedAssets - Container for all unused assets
	 * @param AllSourceFiles - Container for all source files that contains hardlinks to asset
	 * @return int32 Number of unused assets
	 */
	static int32 GetUnusedAssets(TArray<FAssetData>& UnusedAssets, TArray<FString>& AllSourceFiles);

	// Returns total number of empty folders
	static int32 GetEmptyFoldersNum(TArray<FString>& EmptyFolders,TArray<FString>& NonProjectFiles);

	// Returns total size of unused assets (in bytes)
	static int64 GetUnusedAssetsTotalSize(TArray<FAssetData>& UnusedAssets);

	// Fixup Redirectors , same as in content browser menu
	static void FixupRedirectors();

	// Deletes given array of assets
	static int32 DeleteAssets(TArray<FAssetData>& Assets);

	// Returns all assets that has not any referencer on it
	static void GetRootAssets(TArray<FAssetData>& RootAssets, TArray<FAssetData>& AllAssets,const FCleaningStats& CleaningStats);

	static void FindNonProjectFiles(const FString& SearchPath, TArray<FString>& NonProjectFilesList);

	// Finds all ".h" and ".cpp" source files in Project "Source" and "Plugins" directories
	static void FindAllSourceFiles(TArray<FString>& AllFiles);
	
	// Check if given asset used in source codes (hardcoded path)
	static bool UsedInSourceFiles(TArray<FString>& AllFiles, const FName& Asset);

	static void SaveAllAssets();
};
