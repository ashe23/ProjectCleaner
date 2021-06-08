// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct FSourceCodeFile;

class ProjectCleanerHelper
{
public:
	static void GetEmptyFolders(TSet<FName>& EmptyFolders);
	static void GetProjectFilesFromDisk(TSet<FName>& ProjectFiles);
	static void GetSourceCodeFilesFromDisk(TArray<FSourceCodeFile>& SourceCodeFiles);
private:
	static bool FindEmptyFolders(const FString& FolderPath, TSet<FName>& EmptyFolders, const bool IsProjectRootFolder);
};