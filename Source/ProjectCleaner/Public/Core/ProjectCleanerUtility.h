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
	static bool FindAllEmptyFolders(const FString& FolderPath, TArray<FString>& EmptyFolders);
	static bool IsUnderDeveloperFolder(const FString& PackagePath);	
	static void GetEmptyFolders(TArray<FString>& EmptyFolders, const bool bScanDeveloperContents);
	static void GetInvalidFiles(TSet<FString>& CorruptedFiles, TSet<FString>& NonEngineFiles);// remove
	static void GetCorruptedAssets(FProjectCleanerData& CleanerData);// remove
	static void GetNonEngineFiles(TSet<FString>& NonEngineFiles, const TSet<FString>& ProjectFiles);
	static void GetAllAssets(FProjectCleanerData& CleanerData);// remove
	static void GetUnusedAssets(FProjectCleanerData& CleanerData);// remove
	static int64 GetTotalSize(const TArray<FAssetData>& Assets);
	static void GetPrimaryAssetClasses(TSet<FName>& PrimaryAssetClasses);// remove
	static void GetSourceAndConfigFiles(TArray<FString>& AllFiles);
	static void GetProjectFilesFromDisk(TSet<FString>& ProjectFiles);
	// update
	// static void FindAssetsUsedIndirectly(const TArray<FAssetData>& UnusedAssets, TArray<FIndirectFileInfo>& IndirectFileInfos);
	static FString ConvertAbsolutePathToInternal(const FString& InPath);
	static FString ConvertInternalToAbsolutePath(const FString& InPath);
	static void FixupRedirectors();
	static void SaveAllAssets(const bool PromptUser);
	static void UpdateAssetRegistry(bool bSyncScan);
	// delete
	static bool DeleteEmptyFolders(TArray<FString>& EmptyFolders);
	static int32 DeleteAssets(TArray<FAssetData>& Assets);
	static bool IsEngineExtension(const FString& Extension);
private:
	static void GetUsedAssets(TSet<FName>& UsedAssets);
	static FString ConvertPathInternal(const FString& From, const FString To, const FString& Path);
	// static void RemoveMegascansPluginAssetsIfActive(TArray<FAssetData>& UnusedAssets);
};