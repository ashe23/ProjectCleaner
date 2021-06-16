// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct FSourceCodeFile;

class ProjectCleanerHelper
{
public:
	static void GetEmptyFolders(TArray<FString>& EmptyFolders, const bool bScanDeveloperContents);
	static void GetProjectFilesFromDisk(TSet<FName>& ProjectFiles);
	static void GetSourceCodeFilesFromDisk(TArray<FSourceCodeFile>& SourceCodeFiles);
private:
	static bool FindEmptyFolders(const FString& FolderPath, TArray<FString>& EmptyFolders);
};