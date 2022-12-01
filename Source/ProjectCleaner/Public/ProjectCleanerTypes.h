// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

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
	FString DirPathAbs;
	FString DirPathRel;
	FString DirName;
	int64 SizeTotal = 0;
	int64 SizeUnused = 0;
	int32 AssetsTotal = 0;
	int32 AssetsUnused = 0;
	int32 FoldersTotal = 0;
	int32 FoldersEmpty = 0;
	bool bDeveloperFolder = false;
	bool bExpanded = false;
	bool bExcluded = false;
	bool bEmpty = false;
	TArray<TSharedPtr<FProjectCleanerTreeViewItem>> SubFolders;

	bool operator==(const FProjectCleanerTreeViewItem& Other) const
	{
		return DirPathAbs.Equals(Other.DirPathAbs);
	}

	bool operator!=(const FProjectCleanerTreeViewItem& Other) const
	{
		return !DirPathAbs.Equals(Other.DirPathAbs);
	}
};
