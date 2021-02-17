// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "StructsContainer.h"
#include "CoreMinimal.h"

struct FAssetData;


/**
 * This class responsible for different utility operations in unreal engine context
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
	
	// Returns total number of empty folders
	static int32 GetEmptyFoldersNum(TArray<FString>& EmptyFolders,TArray<FString>& NonProjectFiles);

	// Fixup Redirectors , same as in content browser menu
	static void FixupRedirectors();

	// Deletes given array of assets
	static int32 DeleteAssets(TArray<FAssetData>& Assets);

	static void FindNonProjectFiles(const FString& SearchPath, TArray<FString>& NonProjectFilesList);

	// Finds all ".h" and ".cpp" source files in Project "Source" and "Plugins" directories
	static void FindAllSourceFiles(TArray<FString>& AllFiles);
	
	// Check if given asset used in source codes (hardcoded path)
	static bool UsedInSourceFiles(const TArray<FString>& AllFiles, const FName& Asset);

	static void SaveAllAssets();
};
