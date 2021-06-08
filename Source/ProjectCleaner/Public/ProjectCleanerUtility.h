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
	//static void GetAllProjectFiles(TArray<FName>& AllProjectFiles);
	static void GetInvalidProjectFiles(const FAssetRegistryModule* AssetRegistry, const TSet<FName>& ProjectFilesFromDisk, TSet<FName>& CorruptedFiles, TSet<FName>& NonUAssetFiles);
	static void GetAllPrimaryAssetClasses(UAssetManager& AssetManager, TSet<FName>& PrimaryAssetClasses);
	//static int32 GetEmptyFolders(TSet<FName>& EmptyFolders);
	//static void RemovePrimaryAssets(TArray<FAssetData>& UnusedAssets, TSet<FName>& PrimaryAssetClasses);
	static void RemoveMegascansPluginAssetsIfActive(TArray<FAssetData>& UnusedAssets);
	static void RemoveAssetsUsedIndirectly(TArray<FAssetData>& UnusedAssets, AssetRelationalMap& RelationalMap, TArray<TWeakObjectPtr<USourceCodeAsset>>& SourceCodeAssets);
	static void RemoveAssetsWithExternalReferences(TArray<FAssetData>& UnusedAssets, AssetRelationalMap& RelationalMap);
	static void RemoveUsedAssets(TArray<FAssetData>& Assets, const TSet<FName>& PrimaryAssetClasses);
	static void RemoveAssetsExcludedByUser(
		const FAssetRegistryModule* AssetRegistry,
		TArray<FAssetData>& UnusedAssets,
		TSet<FAssetData>& ExcludedAssets,
		TArray<FAssetData>& UserExcludedAssets,
		AssetRelationalMap& RelationalMap,
		const UExcludeDirectoriesFilterSettings* DirectoryFilterSettings);
	static FName ConvertRelativeToAbsPath(const FName& InPath);
	static FName ConvertAbsToRelativePath(const FName& InPath);
	static int32 DeleteAssets(TArray<FAssetData>& Assets);
	static void DeleteEmptyFolders(TSet<FName>& EmptyFolders);
	static void FixupRedirectors();
	static void SaveAllAssets();
	static int64 GetTotalSize(const TArray<FAssetData>& AssetContainer);
	static FString ConvertRelativeToAbsolutePath(const FName& PackageName);
	static FName ConvertAbsolutePathToRelative(const FName& InPath);
	//static bool HasFiles(const FString& SearchPath);
	//static bool HasFiles(const FName& SearchPath);
	//static void GetSubFolders(const FString& Folder, TArray<FString>& Output);
	//static void GetSubFolders(const FName& Folder, TArray<FString>& Output);
	//static bool IsEmptyFolder(const FName& Folder);
private:
	//static bool GetAllEmptyDirectories(const FString& SearchPath, TSet<FName>& Directories, const bool bIsRootDirectory);
	//static void GetChildrenDirectories(const FString& SearchPath, TArray<FString>& Output);
	//static void RemoveDevsAndCollectionsDirectories(TArray<FString>& Directories);
	static FName ConvertPath(FName Path, const FName& From, const FName& To);
	static bool IsEngineExtension(const FString& Extension);
	static const FSourceCodeFile* GetFileWhereAssetUsed(const FAssetData& Asset, const TArray<FSourceCodeFile>& SourceCodeFiles);
};