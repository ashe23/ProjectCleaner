// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Core/ProjectCleanerPathInfo.h"
#include "Core/ProjectCleanerPath.h"

FProjectCleanerPathInfo::FProjectCleanerPathInfo(const FProjectCleanerPath& InPath) : FolderPath(InPath)
{
	if (!FolderPath.IsValid() || FolderPath.IsFile()) return;

	FillData();
}

FProjectCleanerPathInfo::~FProjectCleanerPathInfo()
{
	FoldersAll.Empty();
	FoldersSub.Empty();
	FilesAll.Empty();
	FilesSub.Empty();
}

const TArray<FProjectCleanerPath>& FProjectCleanerPathInfo::GetFoldersAll() const
{
	return FoldersAll;
}

const TArray<FProjectCleanerPath>& FProjectCleanerPathInfo::GetFoldersSub() const
{
	return FoldersSub;
}

const TArray<FProjectCleanerPath>& FProjectCleanerPathInfo::GetFilesAll() const
{
	return FilesAll;
}

const TArray<FProjectCleanerPath>& FProjectCleanerPathInfo::GetFilesSub() const
{
	return FilesSub;
}

void FProjectCleanerPathInfo::FillData()
{
	TArray<FString> AllFolders;
	IFileManager::Get().FindFilesRecursive(AllFolders, *FolderPath.GetPathAbs(), TEXT("*.*"), false, true);

	FoldersAll.Reserve(AllFolders.Num());
	for (const auto& Folder : AllFolders)
	{
		const FProjectCleanerPath Path{Folder};
		if (!Path.IsValid()) continue;

		FoldersAll.Add(Path);
	}

	TArray<FString> ChildFolders;
	IFileManager::Get().FindFiles(ChildFolders, *(FolderPath.GetPathAbs() / TEXT("*")), false, true);

	FoldersSub.Reserve(ChildFolders.Num());
	for (const auto& Folder : ChildFolders)
	{
		const FProjectCleanerPath Path{FolderPath.GetPathAbs() / Folder};
		if (!Path.IsValid()) continue;

		FoldersSub.Add(Path);
	}

	TArray<FString> AllFiles;
	IFileManager::Get().FindFilesRecursive(AllFiles, *FolderPath.GetPathAbs(), TEXT("*.*"), true, false);

	FilesAll.Reserve(AllFiles.Num());
	for (const auto& File : AllFiles)
	{
		const FProjectCleanerPath Path{File};
		if (!Path.IsValid()) continue;

		FilesAll.Add(Path);
	}

	TArray<FString> ChildFiles;
	IFileManager::Get().FindFiles(ChildFiles, *(FolderPath.GetPathAbs() / TEXT("*")), true, false);

	FilesSub.Reserve(ChildFiles.Num());
	for (const auto& File : ChildFiles)
	{
		const FProjectCleanerPath Path{FolderPath.GetPathAbs() / File};
		if (!Path.IsValid()) continue;

		FilesSub.Add(Path);
	}
}
