// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectCleanerTypes.generated.h"

USTRUCT(BlueprintType)
struct FProjectCleanerIndirectAsset
{
	GENERATED_BODY()

	bool operator==(const FProjectCleanerIndirectAsset& Other) const
	{
		return LineNum == Other.LineNum && FilePath.Equals(Other.FilePath);
	}

	bool operator!=(const FProjectCleanerIndirectAsset& Other) const
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

UENUM(BlueprintType)
enum class EProjectCleanerScanFailReason : uint8
{
	None,
	AssetRegistryWorking,
	EditorInPlayMode,
};

USTRUCT(BlueprintType)
struct FProjectCleanerScanSettings
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="ProjectCleaner|ScanSettings")
	FString ScanDirectory;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="ProjectCleaner|ScanSettings")
	TArray<FString> ExcludeFolders;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="ProjectCleaner|ScanSettings")
	TArray<UClass*> ExcludeClasses;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="ProjectCleaner|ScanSettings")
	TArray<UObject*> ExcludeAssets;
};

USTRUCT(BlueprintType)
struct FProjectCleanerScanResult
{
	GENERATED_BODY()

	bool bSuccess;
	
	FString ErrorMsg;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner|ScanResult")
	TArray<FAssetData> AssetsTotal;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner|ScanResult")
	TArray<FAssetData> AssetsUsed;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner|ScanResult")
	TArray<FAssetData> AssetsUnused;
};
