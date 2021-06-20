// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "StructsContainer.h"
// Engine Headers
#include "CoreMinimal.h"

class UCleanerConfigs;
class USourceCodeAsset;
class UExcludeOptions;
class AssetRelationalMap;
struct FAssetData;
class FAssetRegistryModule;
class ProjectCleanerNotificationManager;

/**
 * This class responsible for different utility operations in unreal engine context
 */
class PROJECTCLEANER_API ProjectCleanerUtility
{
public:
	static void GetAllAssets(const FAssetRegistryModule* AssetRegistry, TArray<FAssetData>& Assets);
	static void GetInvalidProjectFiles(const FAssetRegistryModule* AssetRegistry, const TSet<FString>& ProjectFilesFromDisk, TSet<FString>& CorruptedFiles, TSet<FString>& NonUAssetFiles);
	static void GetAllPrimaryAssetClasses(const UAssetManager& AssetManager, TSet<FName>& PrimaryAssetClasses);
	static void RemoveMegascansPluginAssetsIfActive(TArray<FAssetData>& UnusedAssets);
	static void RemoveAssetsUsedIndirectly(TArray<FAssetData>& UnusedAssets, AssetRelationalMap& RelationalMap, TArray<TWeakObjectPtr<USourceCodeAsset>>& SourceCodeAssets);
	static void RemoveAssetsWithExternalReferences(TArray<FAssetData>& UnusedAssets, AssetRelationalMap& RelationalMap);
	static void RemoveUsedAssets(TArray<FAssetData>& Assets, const TSet<FName>& PrimaryAssetClasses);
	static void RemoveContentFromDeveloperFolder(TArray<FAssetData>& UnusedAssets, AssetRelationalMap& RelationalMap, UCleanerConfigs* CleanerConfigs, const TSharedPtr<ProjectCleanerNotificationManager> NotificationManager);
	static void RemoveAssetsExcludedByUser(
		TArray<FAssetData>& UnusedAssets,
		AssetRelationalMap& RelationalMap,
		const UExcludeOptions& ExcludeOptions,
		TArray<FAssetData>& ExcludedAssets,
		const TArray<FAssetData>& UserExcludedAssets,
		TArray<FAssetData>& LinkedAssets
	);
	static int32 DeleteAssets(TArray<FAssetData>& Assets);
	static void FixupRedirectors();
	static void SaveAllAssets();
	static int64 GetTotalSize(const TArray<FAssetData>& AssetContainer);
private:
	static bool IsEngineExtension(const FString& Extension);
	static const FSourceCodeFile* GetFileWhereAssetUsed(const FAssetData& Asset, const TArray<FSourceCodeFile>& SourceCodeFiles);
	static bool IsExcludedByPath(const FAssetData& AssetData, const UExcludeOptions& ExcludeOptions);
	static bool IsExcludedByClass(const FAssetData& AssetData, const UExcludeOptions& ExcludeOptions);
};