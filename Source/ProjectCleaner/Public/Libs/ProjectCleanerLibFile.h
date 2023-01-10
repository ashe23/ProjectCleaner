// // Copyright Ashot Barkhudaryan. All Rights Reserved.
//
// #pragma once
//
// #include "CoreMinimal.h"
// #include "Kismet/BlueprintFunctionLibrary.h"
// #include "ProjectCleanerLibFile.generated.h"
//
// UCLASS()
// class UProjectCleanerLibFile final : public UBlueprintFunctionLibrary
// {
// 	GENERATED_BODY()
//
// public:
// 	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner|Lib|File", meta=(Tooltip="Returns total size of given files"))
// 	static int64 GetFilesTotalSize(const TArray<FString>& Files);
//
// 	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner|Lib|File", meta=(Tooltip="Returns all non engine files inside Content folder"))
// 	static void GetFilesNonEngine(TArray<FString>& FilesNonEngine);
//
// 	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner|Lib|File", meta=(Tooltip="Returns all engine files that are corrupted inside Content folder"))
// 	static void GetFilesCorrupted(TArray<FString>& FilesCorrupted);
//
// 	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner|Lib|File", meta=(Tooltip="Returns subfolders under given path"))
// 	static void GetFolders(const FString& InPath, TArray<FString>& Folders, const bool bRecursive);
//
// 	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner|Lib|File", meta=(Tooltip="Returns all empty folders inside Content folder"))
// 	static void GetFoldersEmpty(TArray<FString>& FoldersEmpty);
//
// 	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner|Lib|File", meta=(ToolTip="Checks if given folder is empty"))
// 	static bool FolderIsEmpty(const FString& InPath);
//
// 	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner|Lib|File", meta=(ToolTip="Checks if given folder is excluded"))
// 	static bool FolderIsExcluded(const FString& InPath);
//
// 	// UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner|Lib|File", meta=(Tooltip="Check if given file is corrupted or not. Must have .uasset extension"))
// 	// static bool FileIsCorrupted(const FString& FilePathAbs);
//
// 	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner|Lib|File", meta=(Tooltip="Check if given file has engine extension"))
// 	static bool FileHasEngineExtension(const FString& FilePathAbs);
//
// 	static bool FolderIsEngineGenerated(const FString& InPath);
// };
