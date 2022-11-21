// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ProjectCleanerAssetLibrary.generated.h"

UCLASS(DisplayName="ProjectCleanerAssetLibrary", meta=(ToolTip="Project Cleaner collection of function for working with assets"))
class UProjectCleanerAssetLibrary final : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category="ProjectCleaner|Lib|Asset", meta=(ToolTip="Check is AssetRegistry module currently loading assets or not"))
	static bool AssetRegistryIsLoadingAssets();

	UFUNCTION(BlueprintCallable, Category="ProjectCleaner|Lib|Asset", meta=(ToolTip="Updates AssetRegistry internal database"))
	static void AssetRegistryUpdate(const bool bSyncScan = false);

	UFUNCTION(BlueprintCallable, Category="ProjectCleaner|Lib|Asset", meta=(ToolTip="Saves all unsaved assets"))
	static void AssetsSaveAll(const bool bShowDialogWindow = true);

	UFUNCTION(BlueprintCallable, Category="ProjectCleaner|Lib|Asset", meta=(ToolTip="Returns all assets in project"))
	static void AssetsGetAll(TArray<FAssetData>& Assets);

	UFUNCTION(BlueprintCallable, Category="ProjectCleaner|Lib|Asset", meta=(ToolTip="Returns total size of specified assets"))
	static int64 AssetsGetTotalSize(const TArray<FAssetData>& Assets);

	UFUNCTION(BlueprintCallable, Category="ProjectCleaner|Lib|Asset", meta=(ToolTip="Fixes all redirectors in given path"))
	static void FixupRedirectors(const FString& Path = TEXT("/Game"));

	UFUNCTION(BlueprintCallable, Category="ProjectCleaner|Lib|Asset", meta=(ToolTip="Focuses on specified folder in content browser"))
	static void ContentBrowserFocusOnFolder(const FString& FolderPath = TEXT("/Game"));

	UFUNCTION(BlueprintCallable, Category="ProjectCleaner|Lib|Asset", meta=(ToolTip="Checks if given extension is engine extension or not"))
	static bool IsEngineExtension(const FString& Extension);

	UFUNCTION(BlueprintCallable, Category="ProjectCleaner|Lib|Asset", meta=(ToolTip="Checks if given asset path is under megascans plugin folder or not"))
	static bool IsUnderMegascansFolder(const FString& AssetPackagePath);
};
