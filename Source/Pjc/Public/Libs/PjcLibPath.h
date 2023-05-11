// // Copyright Ashot Barkhudaryan. All Rights Reserved.
//
// #pragma once
//
// #include "CoreMinimal.h"
//
// struct FPjcLibPath
// {
// 	static FString Normalize(const FString& InPath);
// 	static FString ToAbsolute(const FString& InPath);
// 	static FString ToContentPath(const FString& InPath);
// 	static FString ToObjectPath(const FString& InPath);
// 	static bool IsPathEmpty(const FString& InPath);
// 	static bool IsPathExcluded(const FString& InPath);
// 	static void GetFilesInPath(const FString& InSearchPath, const bool bSearchRecursive, TSet<FString>& OutFiles);
// 	static void GetFilesInPathByExt(const FString& InSearchPath, const bool bSearchRecursive, const bool bExtSearchInvert, const TSet<FString>& InExtensions, TSet<FString>& OutFiles);
// 	static void GetFoldersInPath(const FString& InSearchPath, const bool bSearchRecursive, TSet<FString>& OutFolders);
// 	static int32 DeleteFiles(const TSet<FString>& InFiles);
// 	static int32 DeleteFolders(const TSet<FString>& InFolders);
// };
