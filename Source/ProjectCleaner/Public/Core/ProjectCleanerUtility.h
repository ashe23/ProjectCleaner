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
	// Refactor Start
public:
	// query
	static void GetEmptyFolders(TArray<FString>& EmptyFolders, const bool bScanDeveloperContents);
	static void GetInvalidFiles(TSet<FString>& CorruptedFiles, TSet<FString>& NonEngineFiles);
	static void GetUnusedAssets(TArray<FAssetData>& UnusedAssets);
	static int64 GetTotalSize(TArray<FAssetData>& UnusedAssets);
	static void GetPrimaryAssetClasses(TSet<FName>& PrimaryAssetClasses);
	static bool IsExcludedByPath(const FAssetData& AssetData, const UExcludeOptions& ExcludeOptions);
	static bool IsExcludedByClass(const FAssetData& AssetData, const UExcludeOptions& ExcludeOptions);
	// update
	static void FindAssetsUsedIndirectly(const TArray<FAssetData>& UnusedAssets, TArray<FIndirectFileInfo>& IndirectFileInfos);
	static FString ConvertAbsolutePathToInternal(const FString& InPath);
	static FString ConvertInternalToAbsolutePath(const FString& InPath);
	// delete
	static bool DeleteEmptyFolders(TArray<FString>& EmptyFolders);
private:
	static void GetUsedAssets(TSet<FName>& UsedAssets);
	static void GetProjectFilesFromDisk(TSet<FString>& ProjectFiles);
	static bool IsEngineExtension(const FString& Extension);
	static bool FindAllEmptyFolders(const FString& FolderPath, TArray<FString>& EmptyFolders);
	static FString ConvertPathInternal(const FString& From, const FString To, const FString& Path);
	static void RemoveMegascansPluginAssetsIfActive(TArray<FAssetData>& UnusedAssets);
	static void FindSourceAndConfigFiles(TArray<FString>& AllFiles);	

	// Refactor END
public:
	// static void GetAllAssets(TArray<FAssetData>& Assets);
	//static void GetAllUnusedAssets(TArray<FAssetData>& UnusedAssets);
	//static void GetInvalidProjectFiles(const FAssetRegistryModule* AssetRegistry, const TSet<FString>& ProjectFilesFromDisk, TSet<FString>& CorruptedFiles, TSet<FString>& NonUAssetFiles);
	//static void GetAllPrimaryAssetClasses(const UAssetManager& AssetManager, TSet<FName>& PrimaryAssetClasses);
	//static void RemoveAssetsUsedIndirectly(TArray<FAssetData>& UnusedAssets, AssetRelationalMap& RelationalMap, TArray<TWeakObjectPtr<UIndirectAsset>>& SourceCodeAssets);
	//static void RemoveAssetsWithExternalReferences(TArray<FAssetData>& UnusedAssets, AssetRelationalMap& RelationalMap);
	// static void RemoveUsedAssets(TArray<FAssetData>& Assets, const TSet<FName>& PrimaryAssetClasses);
	// static void RemoveContentFromDeveloperFolder(TArray<FAssetData>& UnusedAssets, AssetRelationalMap& RelationalMap, UCleanerConfigs* CleanerConfigs, const TSharedPtr<ProjectCleanerNotificationManager> NotificationManager);
	//static void RemoveAssetsExcludedByUser(
	//	TArray<FAssetData>& UnusedAssets,
	//	AssetRelationalMap& RelationalMap,
	//	const UExcludeOptions& ExcludeOptions,
	//	TArray<FAssetData>& ExcludedAssets,
	//	const TArray<FAssetData>& UserExcludedAssets,
	//	TArray<FAssetData>& LinkedAssets
	//);
	//static int32 DeleteAssets(TArray<FAssetData>& Assets);
	//static void FixupRedirectors();
	//static void SaveAllAssets();
	//static int64 GetTotalSize(const TArray<FAssetData>& AssetContainer);
private:

	//static const FSourceCodeFile* GetFileWhereAssetUsed(const FAssetData& Asset, const TArray<FSourceCodeFile>& SourceCodeFiles);
};