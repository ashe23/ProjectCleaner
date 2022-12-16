// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectCleanerConstants.h"
#include "ProjectCleanerTypes.generated.h"

UENUM(BlueprintType)
enum class EProjectCleanerScanResult : uint8
{
	None,
	Success,
	// InvalidScanSettings,
	AssetRegistryWorking,
	EditorInPlayMode,
	ScanningInProgress,
	CleaningInProgress,
	FailedToSaveAssets
};

USTRUCT(BlueprintType)
struct FProjectCleanerScanSettings
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="ProjectCleaner|ScanSettings", meta=(ToolTip="Paths inside Content folder that must be excluded from scanning. Must be relative."))
	TArray<FString> ExcludedFolders;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="ProjectCleaner|ScanSettings", meta=(ToolTip="Assets classes that must be excluded from scanning."))
	TArray<UClass*> ExcludedClasses;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="ProjectCleaner|ScanSettings", meta=(ToolTip="Specific assets that must be excluded from scanning."))
	TArray<UObject*> ExcludedAssets;
};

USTRUCT(BlueprintType)
struct FProjectCleanerScanData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner|ScanData")
	EProjectCleanerScanResult ScanResult = EProjectCleanerScanResult::None;

	// UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner|ScanData")
	// TArray<FAssetData> AssetsAll;
	//
	// UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner|ScanData")
	// TArray<FAssetData> AssetsUsed;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner|ScanData")
	TArray<FAssetData> AssetsUnused;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner|ScanData")
	TArray<FString> FoldersAll;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner|ScanData")
	TArray<FString> FoldersEmpty;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner|ScanData")
	TArray<FString> FilesCorrupted;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner|ScanData")
	TArray<FString> FilesNonEngine;
};

USTRUCT(BlueprintType)
struct FProjectCleanerIndirectAssetInfo
{
	GENERATED_BODY()

	bool operator==(const FProjectCleanerIndirectAssetInfo& Other) const
	{
		return LineNum == Other.LineNum && FilePath.Equals(Other.FilePath);
	}

	bool operator!=(const FProjectCleanerIndirectAssetInfo& Other) const
	{
		return LineNum != Other.LineNum || !FilePath.Equals(Other.FilePath);
	}

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Indirect Asset")
	FAssetData AssetData;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Indirect Asset")
	int32 LineNum;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Indirect Asset")
	FString FilePath;
};

struct FProjectCleanerFolderInfo
{
	FString FolderName;
	FString FolderPathAbs;
	FString FolderPathRel;

	TArray<FString> FoldersTotal; // all folders inside this folder (recursive)
	TArray<FString> FoldersChild; // first level of child folders
	TArray<FString> FoldersEmpty;

	TArray<FString> FilesTotal; // all files inside this files
	TArray<FString> FilesChild; // first level of child files

	TArray<FAssetData> AssetsTotal;
	TArray<FAssetData> AssetsChild;
	TArray<FAssetData> AssetsUnusedTotal;
	TArray<FAssetData> AssetsUnusedChild;
	TArray<FAssetData> AssetsUsedTotal;
	TArray<FAssetData> AssetsUsedChild;
	TArray<FAssetData> AssetsExcludedTotal;
	TArray<FAssetData> AssetsExcludedChild;

	bool operator==(const FProjectCleanerFolderInfo& Other) const
	{
		return FolderPathAbs == Other.FolderPathAbs;
	}

	bool operator!=(const FProjectCleanerFolderInfo& Other) const
	{
		return FolderPathAbs != Other.FolderPathAbs;
	}
};


// USTRUCT(BlueprintType)
// struct FProjectCleanerScanResult
// {
// 	GENERATED_BODY()
//
// 	// UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner|ScanResult")
// 	// bool bSuccess = false;
//
// 	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner|ScanResult")
// 	EProjectCleanerScanFailReason FailReason = EProjectCleanerScanFailReason::None;
//
// 	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner|ScanResult")
// 	FString ErrorMsg;
//
// 	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner|ScanResult")
// 	TArray<FAssetData> AssetsTotal;
//
// 	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner|ScanResult")
// 	TArray<FAssetData> AssetsUsed;
//
// 	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner|ScanResult")
// 	TArray<FAssetData> AssetsUnused;
//
// 	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner|ScanResult")
// 	TArray<FString> FilesCorrupted;
//
// 	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner|ScanResult")
// 	TArray<FString> FilesNonEngine;
//
// 	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner|ScanResult")
// 	TArray<FString> FoldersTotal;
//
// 	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner|ScanResult")
// 	TArray<FString> FoldersEmpty;
// };
