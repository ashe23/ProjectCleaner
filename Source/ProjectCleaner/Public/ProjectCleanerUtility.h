// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

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
	static void GetChildrenDirectories(const FString& SearchPath, TArray<FString>& Output);
	static void RemoveDevsAndCollectionsDirectories(TArray<FString>& Directories);
	// Finding all assets in "Game" Root directory of project
	static void FindAllGameAssets(TArray<FAssetData>& GameAssetsContainer);
};
