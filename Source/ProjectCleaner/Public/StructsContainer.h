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
	FString FilePath;

	UPROPERTY(DisplayName = "Line where asset used", VisibleAnywhere, Category="AssetUsedIndirectly")
	int32 LineNum;

	UPROPERTY(DisplayName = "AssetData", VisibleAnywhere, Category="AssetUsedIndirectly")
	FAssetData AssetData;
};

UCLASS(Transient)
class UCorruptedFile : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY(DisplayName = "Name", VisibleAnywhere, Category = "CorruptedFile")
	FString Name;
	UPROPERTY(DisplayName = "AbsolutePath", VisibleAnywhere, Category = "CorruptedFile")
	FString AbsolutePath;
};

struct FIndirectFileInfo
{
	FAssetData AssetData;
	FString FileName;
	FString FilePath;
	int32 LineNum;
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
	TArray<FIndirectFileInfo> IndirectFileInfos;
	TSet<FName> PrimaryAssetClasses;
	TArray<FAssetData> UserExcludedAssets;
	TArray<FAssetData> ExcludedAssets;
	TArray<FAssetData> LinkedAssets;
	int64 TotalSize;

	// Helpers for asset deletion stats
	uint32 TotalAssetsNum;
	uint32 DeletedAssetsNum;

	FProjectCleanerData()
	{
		TotalSize = 0;
		TotalAssetsNum = 0;
		DeletedAssetsNum = 0;
	}

	void Reset()
	{
		UnusedAssets.Reset();
		EmptyFolders.Reset();
		NonEngineFiles.Reset();
		CorruptedFiles.Reset();
		IndirectFileInfos.Reset();
		PrimaryAssetClasses.Reset();
		ExcludedAssets.Reset();
		LinkedAssets.Reset();
		TotalSize = 0;
		TotalAssetsNum = 0;
		DeletedAssetsNum = 0;
	}

	void Empty()
	{
		UnusedAssets.Empty();
		EmptyFolders.Empty();
		NonEngineFiles.Empty();
		CorruptedFiles.Empty();
		IndirectFileInfos.Empty();
		PrimaryAssetClasses.Empty();
		ExcludedAssets.Empty();
		LinkedAssets.Empty();
		TotalSize = 0;
		TotalAssetsNum = 0;
		DeletedAssetsNum = 0;
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
	constexpr static TCHAR* SomeAssetsHaveRefsInDevFolder = TEXT("Some assets have referencers in Developer Contents Folder.");
	constexpr static TCHAR* CantIncludeSomeAssets = TEXT("Cant include some filtered assets.Clear 'ExcludeOptions' filters and try again.");
	constexpr static TCHAR* FailedToDeleteSomeFolders = TEXT("Failed to delete some folders. See Output Log for more information.");
	
};