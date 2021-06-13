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
	static void GetAllAssets(const FAssetRegistryModule* AssetRegistry, TArray<FAssetData>& Assets);
	static void GetInvalidProjectFiles(const FAssetRegistryModule* AssetRegistry, const TSet<FName>& ProjectFilesFromDisk, TSet<FName>& CorruptedFiles, TSet<FName>& NonUAssetFiles);
	static void GetAllPrimaryAssetClasses(UAssetManager& AssetManager, TSet<FName>& PrimaryAssetClasses);
	static void RemoveMegascansPluginAssetsIfActive(TArray<FAssetData>& UnusedAssets);
	static void RemoveAssetsUsedIndirectly(TArray<FAssetData>& UnusedAssets, AssetRelationalMap& RelationalMap, TArray<FSourceCodeFile> SourceCodeFiles, TArray<TWeakObjectPtr<USourceCodeAsset>>& SourceCodeAssets);
	static void RemoveAssetsWithExternalReferences(TArray<FAssetData>& UnusedAssets, AssetRelationalMap& RelationalMap);
	static void RemoveUsedAssets(TArray<FAssetData>& Assets, const TSet<FName>& PrimaryAssetClasses, const UExcludeDirectoriesFilterSettings* ExcludeDirectoryFilterSettings);
	static void RemoveContentFromDeveloperAndCollectionsFolders(TArray<FAssetData>& UnusedAssets, TSet<FName>& EmptyFolders);
	static void RemoveAssetsExcludedByUser(
		const FAssetRegistryModule* AssetRegistry,
		TArray<FAssetData>& UnusedAssets,
		TSet<FAssetData>& ExcludedAssets,
		TArray<FAssetData>& UserExcludedAssets,
		AssetRelationalMap& RelationalMap,
		const UExcludeDirectoriesFilterSettings* DirectoryFilterSettings);
	static FName ConvertRelativeToAbsPath(const FName& InPath);
	static FString ConvertRelativeToAbsPath(const FString& InPath);
	static FName ConvertAbsToRelativePath(const FName& InPath);
	static FString ConvertAbsToRelativePath(const FString& InPath);
	static int32 DeleteAssets(TArray<FAssetData>& Assets);
	static void DeleteEmptyFolders(TSet<FName>& EmptyFolders);
	static void FixupRedirectors();
	static void SaveAllAssets();
	static int64 GetTotalSize(const TArray<FAssetData>& AssetContainer);
	static FString ConvertRelativeToAbsolutePath(const FName& PackageName);
	static FName ConvertAbsolutePathToRelative(const FName& InPath);
private:
	static FName ConvertPath(FName Path, const FName& From, const FName& To);
	static FString ConvertPath(FString Path, const FString& From, const FString& To);
	static bool IsEngineExtension(const FString& Extension);
	static const FSourceCodeFile* GetFileWhereAssetUsed(const FAssetData& Asset, const TArray<FSourceCodeFile>& SourceCodeFiles);
};