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

struct FProjectCleanerFolderInfo
{
	FString FolderPathAbs;
	FString FolderPathRel;
	FString FolderName;

	bool bEmpty = false;
	bool bExcluded = false;
	bool bDevFolder = false;
	
	TSet<FString> SubFolders;
	TSet<FString> SubFoldersAll;
	TSet<FString> SubFoldersEmpty;

	TArray<FAssetData> AssetsTotal;
	TArray<FAssetData> AssetsUnused;

	bool operator==(const FProjectCleanerFolderInfo& Other) const
	{
		return FolderPathAbs.Equals(Other.FolderPathAbs);
	}

	bool operator!=(const FProjectCleanerFolderInfo& Other) const
	{
		return !FolderPathAbs.Equals(Other.FolderPathAbs);
	}
};