// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ProjectCleanerLibrary.generated.h"

UCLASS(DisplayName="ProjectCleanerLibrary", meta=(ToolTip="Project Cleaner collection of helper functions"))
class UProjectCleanerLibrary final : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category="ProjectCleaner", meta=(ToolTip="Returns all subdirectories for given directory. Specify Exclude Directories that you want to exclude"))
	static void GetSubDirectories(const FString& RootDir, const bool bRecursive, TSet<FString>& SubDirectories, const TSet<FString>& ExcludeDirectories);

	UFUNCTION(BlueprintCallable, Category="ProjectCleaner", meta=(ToolTip="Returns all empty folders in given path"))
	static void GetEmptyDirectories(const FString& InAbsPath, TSet<FString>& EmptyDirectories, const TSet<FString>& ExcludeDirectories);

	UFUNCTION(BlueprintCallable, Category="ProjectCleaner", meta=(ToolTip="Returns all primary asset classes in project. This list is same as in AssetManager->Primary Asset Types To Scan"))
	static void GetPrimaryAssetClasses(TArray<UClass*>& PrimaryAssetClasses);

	UFUNCTION(BlueprintCallable, Category="ProjectCleaner", meta=(ToolTip="Returns all primary assets in project"))
	static void GetPrimaryAssets(TArray<FAssetData>& PrimaryAssets);

	UFUNCTION(BlueprintCallable, Category="ProjectCleaner", meta=(ToolTip="Returns all assets in directory. Specify ExcludeFilter for exclusion"))
	static void GetAssetsInPath(const FString& InRelPath, const bool bRecursive, TArray<FAssetData>& Assets);

	UFUNCTION(BlueprintCallable, Category="ProjectCleaner", meta=(ToolTip="Returns total size of given assets array"))
	static int64 GetAssetsTotalSize(const TArray<FAssetData>& Assets);

	UFUNCTION(BlueprintCallable, Category="ProjectCleaner",
		meta=(ToolTip="Converts given relative to absolute. Example /Game/StarterContent => C:/{Your_Project_Path_To_Content_Folder}/StarterContent"))
	static FString PathConvertToAbs(const FString& InRelPath);

	UFUNCTION(BlueprintCallable, Category="ProjectCleaner",
		meta=(ToolTip="Converts given absolute to relative. Example C:/{Your_Project_Path_To_Content_Folder}/StarterContent => /Game/StarterContent"))
	static FString PathConvertToRel(const FString& InAbsPath);

	// Fixing up all redirectors in project
	static void FixupRedirectors();
	static void SaveAllAssets(const bool bPromptUser = true);
	static void UpdateAssetRegistry(const bool bSyncScan = false);
	static void FocusOnDirectory(const FString& InRelPath);
private:
	static FString ConvertPathInternal(const FString& From, const FString& To, const FString& Path);
};
