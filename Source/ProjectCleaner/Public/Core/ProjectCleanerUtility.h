// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "StructsContainer.h"
// Engine Headers
#include "CoreMinimal.h"

class UIndirectAsset;
class FAssetRegistryModule;
struct FAssetData;

/**
 * This class responsible for different utility operations in unreal engine context
 */
class PROJECTCLEANER_API ProjectCleanerUtility
{
public:
	static int64 GetTotalSize(const TArray<FAssetData>& Assets);
	static FName GetClassName(const FAssetData& AssetData);
	static FText GetDeletionProgressText(const int32 DeletedAssetNum, const int32 Total, const bool bShowPercent);
	static FString ConvertAbsolutePathToInternal(const FString& InPath);
	static FString ConvertInternalToAbsolutePath(const FString& InPath);
	static void SaveAllAssets(const bool PromptUser);
	static void UpdateAssetRegistry(bool bSyncScan);
	static void FocusOnGameFolder();
	static bool FindEmptyFoldersInPath(const FString& FolderPath, TSet<FName>& EmptyFolders);
	static int32 DeleteAssets(TArray<FAssetData>& Assets, const bool ForceDelete);
	static bool IsEngineExtension(const FString& Extension);
	static bool IsUnderMegascansFolder(const FAssetData& AssetData);
	static bool HasIndirectlyUsedAssets(const FString& FileContent);
private:
	static FString ConvertPathInternal(const FString& From, const FString To, const FString& Path);
};