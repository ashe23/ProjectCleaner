// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ProjectCleanerLibrary.generated.h"

USTRUCT(BlueprintType)
struct FProjectCleanerConfigs
{
	GENERATED_BODY()

	UPROPERTY(DisplayName = "Scan Developer Contents", EditAnywhere, Category = "CleanerConfigs")
	bool bScanDeveloperContents = false;
	
	UPROPERTY(DisplayName = "Excluded Paths", EditAnywhere, Category = "CleanerConfigs", meta = (ContentDir))
	TArray<FDirectoryPath> ExcludedPaths;

	UPROPERTY(DisplayName = "Excluded Classes", EditAnywhere, Category = "CleanerConfigs")
	TArray<UClass*> ExcludedClasses;

	UPROPERTY(DisplayName = "Excluded Assets", EditAnywhere, Category = "CleanerConfigs")
	TArray<UObject*> ExcludedAssets;
};

UENUM(BlueprintType)
enum class EProjectCleanerPathReturnType : uint8
{
	EPT_Absolute UMETA(DisplayName = "Absolute"), // C:/dev/MyProject/Content/NewMaterial.uasset
	EPT_Relative UMETA(DisplayName = "Relative"), // Content/NewMaterial.uasset
	EPT_Game     UMETA(DisplayName = "Game"),     // /Game/NewMaterial.NewMaterial
};

/**
 * 
 */
UCLASS(meta = (ScriptName = "ProjectCleanerAPI"))
class PROJECTCLEANER_API UProjectCleanerLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "ProjectCleanerAPI", meta = (ToolTip = "Return all unused assets (FAssetData structure)"))
	static TArray<FAssetData> GetUnusedAssets(const FProjectCleanerConfigs& CleanerConfigs);

	UFUNCTION(BlueprintCallable, Category = "ProjectCleanerAPI", meta = (ToolTip = "Return all unused assets paths based on given path type"))
	static TArray<FString> GetUnusedAssetsPaths(const FProjectCleanerConfigs& CleanerConfigs, EProjectCleanerPathReturnType PathType);

	UFUNCTION(BlueprintCallable, Category = "ProjectCleanerAPI|Util")
	static FString GetAssetPathByPathType(const FAssetData& AssetData, EProjectCleanerPathReturnType PathType);

	UFUNCTION(BlueprintCallable, Category = "ProjectCleanerAPI", meta = (ToolTip = "Return all empty folders in project"))
	static TArray<FString> GetEmptyFolders(const FProjectCleanerConfigs& CleanerConfigs);

	UFUNCTION(BlueprintCallable, Category = "ProjectCleanerAPI", meta = (ToolTip = "Return all non engine files in project"))
	static TArray<FString> GetNonEngineFiles(const FProjectCleanerConfigs& CleanerConfigs);

	UFUNCTION(BlueprintCallable, Category = "ProjectCleanerAPI", meta = (ToolTip = "Return all corrupted files in project"))
	static TArray<FString> GetCorruptedFiles(const FProjectCleanerConfigs& CleanerConfigs);

	UFUNCTION(BlueprintCallable, Category = "ProjectCleanerAPI", meta = (ToolTip = "Return all indirectly used assets in project"))
	static TArray<FString> GetIndirectlyUsedAssets(const FProjectCleanerConfigs& CleanerConfigs);

	UFUNCTION(BlueprintCallable, Category = "ProjectCleanerAPI|Util", meta = (ToolTip = "Save result of API calls in directory Saved/ProjectCleaner"))
	static bool ExportToFile(const TArray<FString>& List, const FString& FileName);
	
private:

	// Fills ProjectCleanerDataManager settings based on CleanerConfigs
	static void FillCleanerConfigs(class FProjectCleanerDataManager& DataManager, const FProjectCleanerConfigs& CleanerConfigs);
};
