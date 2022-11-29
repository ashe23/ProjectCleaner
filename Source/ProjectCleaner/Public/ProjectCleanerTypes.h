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