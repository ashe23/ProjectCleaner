// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectCleanerTypes.generated.h"

UENUM(BlueprintType)
enum class EProjectCleanerScanResult : uint8
{
	None,
	Success,
	AssetRegistryWorking,
	EditorInPlayMode,
	ScanningInProgress,
	CleaningInProgress,
	FailedToSaveAssets
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
	int32 LineNum = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Indirect Asset")
	FString FilePath;
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
	TArray<TSoftObjectPtr<UObject>> ExcludedAssets;
};

USTRUCT(BlueprintType)
struct FProjectCleanerScanData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner|ScanData")
	EProjectCleanerScanResult ScanResult = EProjectCleanerScanResult::None;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner|ScanData")
	FString ScanResultMsg;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner|ScanData")
	TArray<FAssetData> AssetsAll;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner|ScanData")
	TArray<FAssetData> AssetsUsed;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner|ScanData")
	TArray<FAssetData> AssetsPrimary;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner|ScanData")
	TArray<FAssetData> AssetsIndirect;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner|ScanData")
	TArray<FProjectCleanerIndirectAssetInfo> AssetsIndirectInfo;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner|ScanData")
	TArray<FAssetData> AssetsExcluded;

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

struct FProjectCleanerTreeViewItem
{
	FString FolderPathAbs;
	FString FolderPathRel;
	FString FolderName;
	int32 FoldersTotal = 0;
	int32 FoldersEmpty = 0;
	int64 SizeTotal = 0;
	int64 SizeUnused = 0;
	int32 AssetsTotal = 0;
	int32 AssetsUnused = 0;
	bool bDevFolder = false;
	bool bExpanded = false;
	bool bExcluded = false;
	bool bEmpty = false;
	float PercentUnused = 0.0f; // 0 - 100 range
	float PercentUnusedNormalized = 0.0f; // 0 - 1 range

	TArray<TSharedPtr<FProjectCleanerTreeViewItem>> SubItems;

	bool operator==(const FProjectCleanerTreeViewItem& Other) const
	{
		return FolderPathAbs.Equals(Other.FolderPathAbs);
	}

	bool operator!=(const FProjectCleanerTreeViewItem& Other) const
	{
		return !FolderPathAbs.Equals(Other.FolderPathAbs);
	}
};
