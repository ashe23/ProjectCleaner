// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "StructsContainer.h"

// Engine Headers
#include "CoreMinimal.h"

struct FAssetData;


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
	 * @param NonProjectFiles 
	 * @param bIsRootDirectory 
	 * @return bool
	 */
	static bool GetAllEmptyDirectories(const FString& SearchPath,
	                                   TArray<FString>& Directories,
	                                   TArray<FString>& NonProjectFiles,
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
	static void DeleteEmptyFolders(TArray<FString>& EmptyFolders);

	/**
	 * @brief Returns total number of empty folders and finds all non project files
	 * @param EmptyFolders 
	 * @param NonProjectFiles 
	 * @return Number of empty folders
	 */
	static int32 GetEmptyFoldersAndNonProjectFiles(TArray<FString>& EmptyFolders, TArray<FString>& NonProjectFiles);

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
	 * @brief Finds all non .uproject files
	 * @param SearchPath 
	 * @param NonProjectFilesList 
	 */
	static void FindNonProjectFiles(const FString& SearchPath, TArray<FString>& NonProjectFilesList);

	/**
	 * @brief Finds all ".h" and ".cpp" source files in Project "Source" and "Plugins" directories
	 * @param AllFiles 
	 */
	static void FindAllSourceFiles(TArray<FString>& AllFiles);

	static void LoadSourceCodeFilesContent(TArray<FString>& AllSourceFiles, TArray<FString>& SourceCodeFilesContent);

	/**
	 * @brief Check if given asset used in source codes (hardcoded path)
	 * @param AllFiles 
	 * @param Asset 
	 * @return bool
	 */
	static bool UsedInSourceFiles(const TArray<FString>& AllFiles, const FAssetData& Asset);

	/**
	 * @brief Saves all unsaved assets
	 */
	static void SaveAllAssets();
};
