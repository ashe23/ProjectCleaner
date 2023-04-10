// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct FPjcLibPath
{
	static FString ProjectDir();
	static FString ContentDir();
	static FString SourceDir();
	static FString ConfigDir();
	static FString PluginsDir();
	static FString SavedDir();
	static FString DevelopersDir();
	static FString CollectionsDir();
	static FString CurrentUserDevelopersDir();
	static FString CurrentUserCollectionsDir();
	static FString Normalize(const FString& InPath);
	static FString ToAbsolute(const FString& InPath);
	static FString ToAssetPath(const FString& InPath);
	static FString GetFilePath(const FString& InPath);
	static FString GetPathName(const FString& InPath);
	static FString GetFileExtension(const FString& InPath, const bool bIncludeDot);
	static FName ToObjectPath(const FString& InPath);
	static bool IsValid(const FString& InPath);
	static bool IsFile(const FString& InPath);
	static bool IsDir(const FString& InPath);
	static bool IsPathEmpty(const FString& InPath);
	static bool IsPathEngineGenerated(const FString& InPath);
	// static bool IsPathExcluded(const FString& InPath);
	static int64 GetFileSize(const FString& InPath);
	static int64 GetFilesSize(const TArray<FString>& InPaths);
	static void GetFoldersInPath(const FString& SearchPath, const bool bSearchRecursive, TSet<FString>& OutFolders);
	static void GetFilesInPath(const FString& SearchPath, const bool bSearchRecursive, TSet<FString>& OutFiles);
	static void GetFilesInPathByExt(const FString& SearchPath, const bool bSearchRecursive, const bool bSearchInvert, const TSet<FString>& Extensions, TSet<FString>& OutFiles);
};
