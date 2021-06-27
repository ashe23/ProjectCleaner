// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/ProjectCleanerSourceCodeAssetsUI.h"
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

USTRUCT()
struct FProjectCleanerData
{
	GENERATED_BODY()
	
	UPROPERTY()
	TArray<FAssetData> UnusedAssets;
	UPROPERTY()
	TArray<FString> EmptyFolders;
	UPROPERTY()
	TSet<FString> NonEngineFiles;
	UPROPERTY()
	TSet<FString> CorruptedFiles;
	UPROPERTY()
	TArray<FString> ExcludedAssets;
	UPROPERTY()
	TArray<FString> UserExcludedAssets;
	UPROPERTY()
	TArray<FString> LinkedAssets;
	UPROPERTY()
	TArray<FString> PrimaryAssetClasses;
	UPROPERTY()
	TArray<TWeakObjectPtr<UIndirectAsset>> IndirectAssets;

	void Reset()
	{
		UnusedAssets.Reset();
		EmptyFolders.Reset();
		NonEngineFiles.Reset();
		CorruptedFiles.Reset();
		ExcludedAssets.Reset();
		LinkedAssets.Reset();
		PrimaryAssetClasses.Reset();
		IndirectAssets.Reset();
	}

	void Empty()
	{
		UnusedAssets.Empty();
		EmptyFolders.Empty();
		NonEngineFiles.Empty();
		CorruptedFiles.Empty();
		ExcludedAssets.Empty();
		LinkedAssets.Empty();
		PrimaryAssetClasses.Empty();
		IndirectAssets.Empty();
	}
};

struct FCleaningStats
{
	int32 UnusedAssetsNum;
	int64 UnusedAssetsTotalSize;
	int32 NonUassetFilesNum;
	int32 SourceCodeAssetsNum;
	int32 CorruptedFilesNum;
	int32 EmptyFolders;
	
	int32 DeletedAssetCount;
	int32 TotalAssetNum;

	FCleaningStats()
	{
		Reset();
	}

	int32 GetPercentage() const
	{
		if (TotalAssetNum == 0) return 0;
		return (DeletedAssetCount * 100.0f) / TotalAssetNum;
	}

	void Reset()
	{
		UnusedAssetsNum = 0;
		EmptyFolders = 0;
		UnusedAssetsTotalSize = 0;
		NonUassetFilesNum = 0;
		SourceCodeAssetsNum = 0;
		CorruptedFilesNum = 0;
		DeletedAssetCount = 0;
		TotalAssetNum = 0;
	}
};

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
	FText AssetsDeleteWindowTitle;
	FText AssetsDeleteWindowContent;
	FText EmptyFolderWindowTitle;
	FText EmptyFolderWindowContent;
	FText StartingCleanup;
	FText NoAssetsToDelete;
	FText NoEmptyFolderToDelete;
	FText NonUAssetFilesFound;
	FText SearchingEmptyFolders;
	FText AssetsWithReferencersInDeveloperFolder;

	FStandardCleanerText()
	{
		AssetsDeleteWindowTitle = FText::FromString("Confirm deletion");
		AssetsDeleteWindowContent = FText::FromString("Are you sure you want to permanently delete unused assets?");
		EmptyFolderWindowTitle = FText::FromString("Confirm deletion of empty folders");
		EmptyFolderWindowContent = FText::FromString("Are you sure you want to delete all empty folders in project?");
		StartingCleanup = FText::FromString("Starting Cleanup. This could take some time, please wait");
		NoAssetsToDelete = FText::FromString("There are no assets to delete!");
		NoEmptyFolderToDelete = FText::FromString("There are no empty folders to delete!");
		NonUAssetFilesFound = FText::FromString("Project contains non .uasset files. Check Output Log for more info.");
		SearchingEmptyFolders = FText::FromString("Searching empty folders...");
		AssetsWithReferencersInDeveloperFolder = FText::FromString("Some of assets has references in Developers folder. To view them click 'Scan Developers Folder' checkbox.");
	}
};