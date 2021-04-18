// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "StructsContainer.h"
// Engine Headers
#include "CoreMinimal.h"


struct FAssetData;
class USourceCodeAsset;
class UExcludeDirectoriesFilterSettings;


/**
 * This class responsible for different utility operations in unreal engine context
 */
class PROJECTCLEANER_API ProjectCleanerUtility
{
public:
	/**
	 * @brief Check if given path contains files in it, non recursive
	 * @param SearchPath 
	 * @return bool
	 */
	static bool HasFiles(const FString& SearchPath);

	/**
	 * @brief Finds all empty folders in given path recursive version
	 * @param SearchPath 
	 * @param Directories 
	 * @param bIsRootDirectory 
	 * @return bool
	 */
	static bool GetAllEmptyDirectories(const FString& SearchPath,
	                                   TSet<FName>& Directories,
	                                   const bool bIsRootDirectory);

	/**
	 * @brief Finds all child directions in given path
	 * @param SearchPath 
	 * @param Output 
	 */
	static void GetChildrenDirectories(const FString& SearchPath, TArray<FString>& Output);

	/**
	 * @brief Removes from given Directories "Developers" and "Collections" folders
	 * @param Directories 
	 */
	static void RemoveDevsAndCollectionsDirectories(TArray<FString>& Directories);

	/**
	 * @brief Deletes Empty Folders
	 * @param EmptyFolders 
	 */
	static void DeleteEmptyFolders(TSet<FName>& EmptyFolders);

	/**
	 * @brief Returns total number of empty folders and finds all non project files
	 * @param EmptyFolders 
	 * @return Number of empty folders
	 */
	static int32 GetEmptyFolders(TSet<FName>& EmptyFolders);

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
	 * @brief Finds all ".h" and ".cpp" source files in Project "Source" and "Plugins" directories
	 * @param SourceFiles 
	 */
	static void FindAllSourceFiles(TArray<FSourceCodeFile>& SourceFiles);

	/**
	 * @brief Loads source code files content to array
	 * @param AllSourceFiles 
	 * @param SourceCodeFilesContent 
	 */
	static void LoadSourceCodeFilesContent(TArray<FString>& AllSourceFiles, TArray<FString>& SourceCodeFilesContent);

	/**
	 * @brief Check if given asset used in source codes (hardcoded path)
	 * @param AllFiles 
	 * @param Asset 
	 * @return bool
	 */
	static bool UsedInSourceFiles(const TArray<FString>& AllFiles, const FAssetData& Asset);

	/**
	 * @brief Get all assets from "Game" Folder
	 * @param Assets 
	 */
	static void GetAllAssets(TArray<FAssetData>& Assets);
	/**
	 * @brief Saves all unsaved assets
	 */
	static void SaveAllAssets();

	/**
	 * @brief Create relational list for given Assets (Adjacency List)
	 * @param Assets - Given assets
	 * @param List - Adjacency List
	 * @param OnlyProjectFiles - Include only Asset from "Game" folder
	 */
	static void CreateAdjacencyList(TArray<FAssetData>& Assets, TArray<FNode>& List, const bool OnlyProjectFiles);

	/**
	 * @brief Returns Related Assets for given Asset
	 * @param Node 
	 * @param RelatedAssets 
	 * @param List 
	 */
	static void FindAllRelatedAssets(const FNode& Node, TSet<FName>& RelatedAssets, const TArray<FNode>& List);

	/**
	 * @brief Returns assets that has no references or circular assets
	 * @param RootAssets 
	 * @param Assets 
	 * @param List 
	 */
	static void GetRootAssets(TArray<FAssetData>& RootAssets, TArray<FAssetData>& Assets, TArray<FNode>& List);
	
	/**
	* Detects if given referencer is in dependencies of CurrentAsset.
	* @brief Detects Cycle
	* @param Referencer 
	* @param Deps 
	* @param CurrentAsset 
	* @return bool
	*/
	static bool IsCycle(const FName& Referencer, const TArray<FName>& Deps, const FAssetData& CurrentAsset);

	/**
	 * @brief Returns AssetData for given AssetName
	 * @param AssetName 
	 * @param AssetContainer 
	 * @return 
	 */
	static FAssetData* GetAssetData(const FName& AssetName, TArray<FAssetData>& AssetContainer);

	/**
	* @brief Returns total size of given assets (in bytes)
	* @param AssetContainer Container for all game assets
	* @return 
	*/
	static int64 GetTotalSize(const TArray<FAssetData>& AssetContainer);


	/// =================== REFACTORING START HERE ============================
	static bool IsEmptyDirectory(const FString& Path);

	static bool IsEngineExtension(const FString& Extension);

	static FString ConvertRelativeToAbsolutePath(const FName& PackageName);
	
	static bool UsedInSourceFiles(
		const FAssetData& Asset,
		TArray<FSourceCodeFile>& SourceFiles,
		TArray<TWeakObjectPtr<USourceCodeAsset>>& SourceCodeFiles
	);

	/**
	 * @brief Compares number of assets founded with DirectoryVisitor, with number of assets in AssetRegistry
	 *	If that number is different, we may have corrupted files.
	 *	This could happen if
	 *		1) asset failed to save
	 *		2) asset simply copied from other project with different version
	 *		3) asset migrated from other project with different version
	 *		4) other reasons that i dont know :)
	 * @param Assets - Asset List founded with AssetRegistry
	 * @param UassetFiles - Uasset files founded with DirectoryVisitor
	 * @param CorruptedFiles - Container for corrupted files
	 */
	static void CheckForCorruptedFiles(TArray<FAssetData>& Assets, TSet<FName>& UassetFiles, TSet<FName>& CorruptedFiles);

	/**
	 * @brief Removing all used assets from given asset list
	 * Used assets are those who are dependency for any level
	 * @param Assets - Asset list
	 */
	static void RemoveUsedAssets(TArray<FAssetData>& Assets);

	static void RemoveAssetsWithExternalDependencies(TArray<FAssetData>& Assets, TArray<FNode>& List);

	static void RemoveAssetsUsedInSourceCode(
			TArray<FAssetData>& Assets,
			TArray<FNode>& List,
			TArray<FSourceCodeFile>& SourceCodeFiles,
			TArray<TWeakObjectPtr<USourceCodeAsset>>& SourceCodeAssets
	);

	static void RemoveAssetsExcludedByUser(
		TArray<FAssetData>& Assets,
		TArray<FNode>& List,
		UExcludeDirectoriesFilterSettings* DirectoryFilterSettings
	);
};
