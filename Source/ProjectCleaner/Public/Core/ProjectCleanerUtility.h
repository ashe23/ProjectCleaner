// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "StructsContainer.h"
// Engine Headers
#include "CoreMinimal.h"

class UCleanerConfigs;
class UIndirectAsset;
class UExcludeOptions;
class AssetRelationalMap;
class FAssetRegistryModule;
class ProjectCleanerNotificationManager;
struct FAssetData;
struct FProjectCleanerData;

/**
 * This class responsible for different utility operations in unreal engine context
 */
class PROJECTCLEANER_API ProjectCleanerUtility
{
public:
	// query
	static void GetEmptyFolders(TArray<FString>& EmptyFolders, const bool bScanDeveloperContents);
	static void GetInvalidFiles(TSet<FString>& CorruptedFiles, TSet<FString>& NonEngineFiles);
	static void GetUnusedAssets(TArray<FAssetData>& UnusedAssets);
	static int64 GetTotalSize(TArray<FAssetData>& UnusedAssets);
	static void GetPrimaryAssetClasses(TSet<FName>& PrimaryAssetClasses);
	// update
	static void FindAssetsUsedIndirectly(const TArray<FAssetData>& UnusedAssets, TArray<FIndirectFileInfo>& IndirectFileInfos);
	static FString ConvertAbsolutePathToInternal(const FString& InPath);
	static FString ConvertInternalToAbsolutePath(const FString& InPath);
	static void FixupRedirectors();
	static void SaveAllAssets();
	// delete
	static bool DeleteEmptyFolders(TArray<FString>& EmptyFolders);
	static int32 DeleteAssets(TArray<FAssetData>& Assets);
private:
	static void GetUsedAssets(TSet<FName>& UsedAssets);
	static void GetProjectFilesFromDisk(TSet<FString>& ProjectFiles);
	static bool IsEngineExtension(const FString& Extension);
	static bool FindAllEmptyFolders(const FString& FolderPath, TArray<FString>& EmptyFolders);
	static FString ConvertPathInternal(const FString& From, const FString To, const FString& Path);
	static void RemoveMegascansPluginAssetsIfActive(TArray<FAssetData>& UnusedAssets);
	static void FindSourceAndConfigFiles(TArray<FString>& AllFiles);
};