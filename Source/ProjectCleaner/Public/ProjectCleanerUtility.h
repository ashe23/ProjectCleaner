// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "StructsContainer.h"
// Engine Headers
#include "CoreMinimal.h"

struct FAssetData;
class AssetRelationalMap;
class USourceCodeAsset;
class UExcludeDirectoriesFilterSettings;
class FAssetRegistryModule;

/**
 * This class responsible for different utility operations in unreal engine context
 */
class PROJECTCLEANER_API ProjectCleanerUtility
{
public:
	// REFACTORING START
	static void GetAllAssets(const FAssetRegistryModule* AssetRegistry, TArray<FAssetData>& Assets);
	static void GetAllProjectFiles(TArray<FName>& AllProjectFiles);
	static void GetInvalidProjectFiles(const FAssetRegistryModule* AssetRegistry, const TArray<FName>& AllProjectFiles, TSet<FName>& CorruptedFiles, TSet<FName>& NonUAssetFiles);
	static void GetAllPrimaryAssetClasses(UAssetManager& AssetManager, TSet<FName>& PrimaryAssetClasses);
	static int32 GetEmptyFolders(TSet<FName>& EmptyFolders);
	static void RemovePrimaryAssets(TArray<FAssetData>& UnusedAssets, TSet<FName>& PrimaryAssetClasses);
	static void RemoveMegascansPluginAssetsIfActive(TArray<FAssetData>& UnusedAssets);
	static void RemoveAssetsUsedIndirectly(TArray<FAssetData>& UnusedAssets, AssetRelationalMap& RelationalMap, TArray<TWeakObjectPtr<USourceCodeAsset>>& SourceCodeAssets);
	static void RemoveAssetsWithExternalReferences(TArray<FAssetData>& UnusedAssets, AssetRelationalMap& RelationalMap);
	static void RemoveAssetsExcludedByUser(const FAssetRegistryModule* AssetRegistry, TArray<FAssetData>& UnusedAssets, TSet<FAssetData>& ExcludedAssets, AssetRelationalMap& RelationalMap, const UExcludeDirectoriesFilterSettings* DirectoryFilterSettings);
	static FName ConvertRelativeToAbsPath(const FName& InPath);
	static FName ConvertAbsToRelativePath(const FName& InPath);
private:
	static bool GetAllEmptyDirectories(const FString& SearchPath, TSet<FName>& Directories, const bool bIsRootDirectory);
	static void GetChildrenDirectories(const FString& SearchPath, TArray<FString>& Output);
	static void RemoveDevsAndCollectionsDirectories(TArray<FString>& Directories);
	static bool HasFiles(const FString& SearchPath);
	static FName ConvertPath(FName Path, const FName& From, const FName& To);	
	static bool IsEngineExtension(const FString& Extension);
	static const FSourceCodeFile* GetFileWhereAssetUsed(const FAssetData& Asset, const TArray<FSourceCodeFile>& SourceCodeFiles);
	// REFACTORING END
public:
	/**
	 * @brief Deletes Empty Folders
	 * @param EmptyFolders
	 */
	static void DeleteEmptyFolders(TSet<FName>& EmptyFolders);

	/**
	 * @brief Fixup Redirectors , same as in content browser menu
	 */
	static void FixupRedirectors();

	/**
	 * @brief Deletes given array of assets
	 * @param Assets
	 * @return Number of deleted assets
	 */
	static int32 DeleteAssets(TArray<FAssetData>& Assets);

	/**
	 * @brief Saves all unsaved assets
	 */
	static void SaveAllAssets();

	/**
	 * @brief Returns assets that has no references or circular assets
	 * @param RootAssets
	 * @param Assets
	 * @param List
	 */
	//static void GetRootAssets(TArray<FAssetData>& RootAssets, TArray<FAssetData>& Assets, TArray<FNode>& List);

	/**
	* @brief Returns total size of given assets (in bytes)
	* @param AssetContainer Container for all game assets
	* @return
	*/
	static int64 GetTotalSize(const TArray<FAssetData>& AssetContainer);

	/**
	 * @brief Converts given relative path to absolute
	 * @param PackageName
	 * @return FString
	 */
	static FString ConvertRelativeToAbsolutePath(const FName& PackageName);

	/**
	 * @brief Converts given absolute path to relative
	 * @param InPath
	 * @return FName
	 */
	static FName ConvertAbsolutePathToRelative(const FName& InPath);

	/**
	 * @brief Removing all used assets from given asset list
	 * Used assets are those who are dependency for any level
	 * @param Assets - Asset list
	 */
	static void RemoveUsedAssets(TArray<FAssetData>& Assets, const TSet<FName>& PrimaryAssetClasses);
};