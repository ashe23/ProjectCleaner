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


	UFUNCTION(BlueprintCallable, Category="ProjectCleaner", meta=(ToolTip="Returns all assets in directory. Specify ExcludeFilter for exclusion"))
	static void GetAssetsInPath(const FString& InRelPath, const bool bRecursive, TArray<FAssetData>& Assets);

	UFUNCTION(BlueprintCallable, Category="ProjectCleaner", meta=(ToolTip="Returns total size of given assets array"))
	static int64 GetAssetsTotalSize(const TArray<FAssetData>& Assets);

	static FString PathConvertToAbs(const FString& InRelPath);
	static FString PathConvertToRel(const FString& InAbsPath);
private:
	static FString ConvertPathInternal(const FString& From, const FString& To, const FString& Path);
};
