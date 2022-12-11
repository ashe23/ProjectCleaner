// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectCleanerTypes.generated.h"

UENUM(BlueprintType)
enum class EProjectCleanerPathType: uint8
{
	Absolute,
	Relative
};

UENUM(BlueprintType)
enum class EProjectCleanerModalStatus : uint8
{
	None UMETA(DisplayName = "None"),
	Pending UMETA(DisplayName = "Pending"),
	Error UMETA(DisplayName = "Error"),
	OK UMETA(DisplayName = "OK"),
};

UENUM(BlueprintType)
enum class EProjectCleanerEditorState : uint8
{
	Idle,
	PlayMode,
	AssetRegistryWorking
};

UENUM(BlueprintType)
enum class EProjectCleanerScanState : uint8
{
	Idle,
	Scanning,
	Cleaning
};

UENUM(BlueprintType)
enum class EProjectCleanerScanDataState : uint8
{
	None,
	ObsoleteByAssetRegistry,
	ObsoleteBySettings,
	Actual
};

UENUM(BlueprintType)
enum class EProjectCleanerScanMethod : uint8
{
	Editor,
	Cli
};

struct FProjectCleanerScanSettings
{
	EProjectCleanerScanMethod ScanMethod = EProjectCleanerScanMethod::Editor;
	TSet<FString> ExcludedFolders;
	TSet<UClass*> ExcludedClasses;
	TSet<FAssetData> ExcludedAssets;
};

struct FProjectCleanerScanResult
{
	TArray<FAssetData> AssetsUnused;
	TArray<FAssetData> AssetsIndirect;
};


//

//
// USTRUCT(BlueprintType)
// struct FProjectCleanerIndirectAsset
// {
// 	GENERATED_BODY()
//
// 	bool operator==(const FProjectCleanerIndirectAsset& Other) const
// 	{
// 		return LineNum == Other.LineNum && FilePath.Equals(Other.FilePath);
// 	}
//
// 	bool operator!=(const FProjectCleanerIndirectAsset& Other) const
// 	{
// 		return LineNum != Other.LineNum || !FilePath.Equals(Other.FilePath);
// 	}
//
// 	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Indirect Asset")
// 	FAssetData AssetData;
//
// 	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Indirect Asset")
// 	int32 LineNum;
//
// 	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Indirect Asset")
// 	FString FilePath;
// };
//
// struct FProjectCleanerFileViewItem
// {
// 	FString FileName;
// 	FString FileExt;
// 	FString FilePath;
// 	int64 FileSize;
//
// 	bool operator==(const FProjectCleanerFileViewItem& Other) const
// 	{
// 		return FilePath.Equals(Other.FilePath);
// 	}
//
// 	bool operator!=(const FProjectCleanerFileViewItem& Other) const
// 	{
// 		return !FilePath.Equals(Other.FilePath);
// 	}
// };
//
// struct FProjectCleanerTreeViewItem
// {
// 	FString FolderPathAbs;
// 	FString FolderPathRel;
// 	FString FolderName;
// 	int32 FoldersTotal = 0;
// 	int32 FoldersEmpty = 0;
// 	int64 SizeTotal = 0;
// 	int64 SizeUnused = 0;
// 	int32 AssetsTotal = 0;
// 	int32 AssetsUnused = 0;
// 	bool bDevFolder = false;
// 	bool bExpanded = false;
// 	bool bExcluded = false;
// 	bool bEmpty = false;
// 	float PercentUnused = 0.0f; // 0 - 100 range
// 	float PercentUnusedNormalized = 0.0f; // 0 - 1 range
//
// 	TArray<TSharedPtr<FProjectCleanerTreeViewItem>> SubItems;
//
// 	bool operator==(const FProjectCleanerTreeViewItem& Other) const
// 	{
// 		return FolderPathAbs.Equals(Other.FolderPathAbs);
// 	}
//
// 	bool operator!=(const FProjectCleanerTreeViewItem& Other) const
// 	{
// 		return !FolderPathAbs.Equals(Other.FolderPathAbs);
// 	}
// };
//
// struct FProjectCleanerTabNonEngineListItem
// {
// 	FString FileName;
// 	FString FileExtension;
// 	FString FilePathAbs;
// 	int64 FileSize;
//
// 	bool operator==(const FProjectCleanerTabNonEngineListItem& Other) const
// 	{
// 		return FilePathAbs.Equals(Other.FilePathAbs);
// 	}
//
// 	bool operator!=(const FProjectCleanerTabNonEngineListItem& Other) const
// 	{
// 		return !FilePathAbs.Equals(Other.FilePathAbs);
// 	}
// };
//
// struct FProjectCleanerTabCorruptedListItem
// {
// 	FString FileName;
// 	FString FileExtension;
// 	FString FilePathAbs;
// 	int64 FileSize;
//
// 	bool operator==(const FProjectCleanerTabCorruptedListItem& Other) const
// 	{
// 		return FilePathAbs.Equals(Other.FilePathAbs);
// 	}
//
// 	bool operator!=(const FProjectCleanerTabCorruptedListItem& Other) const
// 	{
// 		return !FilePathAbs.Equals(Other.FilePathAbs);
// 	}
// };
