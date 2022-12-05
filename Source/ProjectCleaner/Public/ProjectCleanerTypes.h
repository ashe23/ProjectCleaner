// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

UENUM(BlueprintType)
enum class EProjectCleanerModalStatus : uint8
{
	None UMETA(DisplayName = "None"),
	Pending UMETA(DisplayName = "Pending"),
	Error UMETA(DisplayName = "Error"),
	OK UMETA(DisplayName = "OK"),
};

struct FProjectCleanerIndirectAsset
{
	bool operator==(const FProjectCleanerIndirectAsset& Other) const
	{
		return LineNum == Other.LineNum && FilePath.Equals(Other.FilePath);
	}

	bool operator!=(const FProjectCleanerIndirectAsset& Other) const
	{
		return LineNum != Other.LineNum || !FilePath.Equals(Other.FilePath);
	}

	FAssetData AssetData;
	int32 LineNum;
	FString FilePath;
};

struct FProjectCleanerFileViewItem
{
	FString FileName;
	FString FileExt;
	FString FilePath;
	int64 FileSize;

	bool operator==(const FProjectCleanerFileViewItem& Other) const
	{
		return FilePath.Equals(Other.FilePath);
	}

	bool operator!=(const FProjectCleanerFileViewItem& Other) const
	{
		return !FilePath.Equals(Other.FilePath);
	}
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

struct FProjectCleanerTabNonEngineListItem
{
	FString FileName;
	FString FileExtension;
	FString FilePathAbs;
	int64 FileSize;

	bool operator==(const FProjectCleanerTabNonEngineListItem& Other) const
	{
		return FilePathAbs.Equals(Other.FilePathAbs);
	}

	bool operator!=(const FProjectCleanerTabNonEngineListItem& Other) const
	{
		return !FilePathAbs.Equals(Other.FilePathAbs);
	}
};
