// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectCleanerPath.h"

struct FProjectCleanerPath;

struct FProjectCleanerPathInfo
{
	explicit FProjectCleanerPathInfo(const FProjectCleanerPath& InPath);
	~FProjectCleanerPathInfo();

	const TArray<FProjectCleanerPath>& GetFoldersAll() const;
	const TArray<FProjectCleanerPath>& GetFoldersSub() const;
	const TArray<FProjectCleanerPath>& GetFilesAll() const;
	const TArray<FProjectCleanerPath>& GetFilesSub() const;

	bool operator==(const FProjectCleanerPath& Other) const
	{
		return FolderPath == Other;
	}

private:
	void FillData();
	
	FProjectCleanerPath FolderPath;
	TArray<FProjectCleanerPath> FoldersAll;
	TArray<FProjectCleanerPath> FoldersSub;
	TArray<FProjectCleanerPath> FilesAll;
	TArray<FProjectCleanerPath> FilesSub;
};
