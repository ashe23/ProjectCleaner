// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "AssetData.h"
#include "StructsContainer.generated.h"

UCLASS(Transient)
class UCleanerConfigs : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY(DisplayName = "Scan Developer Contents Folders", EditAnywhere, Category = "CleanerConfigs")
	bool bScanDeveloperContents = false;

	UPROPERTY(DisplayName = "Remove Empty Folders After Assets Deleted", EditAnywhere, Category = "CleanerConfigs")
	bool bAutomaticallyDeleteEmptyFolders = true;

	UPROPERTY(DisplayName = "Deletion Chunk Limit", EditAnywhere, Category = "CleanerConfigs", meta = (ClampMin = "20", ClampMax = "1000", UIMin = "20", UIMax = "1000", ToolTip = "To prevent engine from freezing when deleting a lot of assets, we delete them by chunks.Here you can specify chunk limit.Pick lower values if your PC got low RAM capacity. Default is 20"))
	int32 DeleteChunkLimit = 20;
};

UCLASS(Transient)
class UExcludeOptions : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY(DisplayName = "Paths", EditAnywhere, Category = "ExcludeOptions", meta = (ContentDir))
	TArray<FDirectoryPath> Paths;

	UPROPERTY(DisplayName = "Classes", EditAnywhere, Category = "ExcludeOptions")
	TArray<UClass*> Classes;
};

UCLASS(Transient)
class UIndirectAsset : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY(DisplayName = "AssetName", VisibleAnywhere, Category="AssetUsedIndirectly")
	FString AssetName;

	UPROPERTY(DisplayName = "AssetPath", VisibleAnywhere, Category="AssetUsedIndirectly")
	FString AssetPath;

	UPROPERTY(DisplayName = "FilePath where asset used", VisibleAnywhere, Category="AssetUsedIndirectly")
	FString SourceCodePath;

	UPROPERTY(DisplayName = "AssetData", VisibleAnywhere, Category="AssetUsedIndirectly")
	FAssetData AssetData;
};

UCLASS(Transient)
class UNonEngineFile : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY(DisplayName = "FileName", VisibleAnywhere, Category = "NonEngineFile")
	FString FileName;

	UPROPERTY(DisplayName = "FilePath", VisibleAnywhere, Category = "NonEngineFile")
	FString FilePath;
};

struct FProjectCleanerData
{
	TArray<FAssetData> UnusedAssets;
	TArray<FString> EmptyFolders;
	TSet<FString> NonEngineFiles;
	TSet<FString> CorruptedFiles;
	
	void Reset()
	{
		UnusedAssets.Reset();
		EmptyFolders.Reset();
		NonEngineFiles.Reset();
		CorruptedFiles.Reset();
	}

	void Empty()
	{
		UnusedAssets.Empty();
		EmptyFolders.Empty();
		NonEngineFiles.Empty();
		CorruptedFiles.Empty();
	}
};

// struct FCleaningStats
// {
// 	int32 UnusedAssetsNum;
// 	int64 UnusedAssetsTotalSize;
// 	int32 NonUassetFilesNum;
// 	int32 SourceCodeAssetsNum;
// 	int32 CorruptedFilesNum;
// 	int32 EmptyFolders;
// 	
// 	int32 DeletedAssetCount;
// 	int32 TotalAssetNum;
//
// 	FCleaningStats()
// 	{
// 		Reset();
// 	}
//
// 	int32 GetPercentage() const
// 	{
// 		if (TotalAssetNum == 0) return 0;
// 		return (DeletedAssetCount * 100.0f) / TotalAssetNum;
// 	}
//
// 	void Reset()
// 	{
// 		UnusedAssetsNum = 0;
// 		EmptyFolders = 0;
// 		UnusedAssetsTotalSize = 0;
// 		NonUassetFilesNum = 0;
// 		SourceCodeAssetsNum = 0;
// 		CorruptedFilesNum = 0;
// 		DeletedAssetCount = 0;
// 		TotalAssetNum = 0;
// 	}
// };

struct FSourceCodeFile
{
	FName Name;
	FString AbsoluteFilePath;
	FString Content;

	bool operator==(const FSourceCodeFile& Other) const
	{
		return Name.IsEqual(Other.Name);
	}
};

struct FStandardCleanerText
{
	constexpr static TCHAR* AssetsDeleteWindowTitle = TEXT("Confirm deletion");
	constexpr static TCHAR* AssetsDeleteWindowContent = TEXT("Are you sure you want to permanently delete unused assets?");
	constexpr static TCHAR* EmptyFolderWindowTitle = TEXT("Confirm deletion of empty folders");
	constexpr static TCHAR* EmptyFolderWindowContent = TEXT("Are you sure you want to delete all empty folders in project?");
	constexpr static TCHAR* StartingCleanup = TEXT("Starting Cleanup. This could take some time, please wait");
	constexpr static TCHAR* NoAssetsToDelete = TEXT("There are no assets to delete!");
	constexpr static TCHAR* NoEmptyFolderToDelete = TEXT("There are no empty folders to delete!");
	constexpr static TCHAR* NonUAssetFilesFound = TEXT("Project contains non engine files. Check Output Log for more info.");
	constexpr static TCHAR* SearchingEmptyFolders = TEXT("Searching empty folders...");
	constexpr static TCHAR* AssetsWithReferencersInDeveloperFolder = TEXT("Some of assets has references in Developers folder. To view them click 'Scan Developers Folder' checkbox.");
	constexpr static TCHAR* AssetRegistryStillWorking = TEXT("Asset Registry still working! Please wait...");
};