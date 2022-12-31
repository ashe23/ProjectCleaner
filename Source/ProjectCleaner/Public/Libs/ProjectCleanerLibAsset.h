// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ProjectCleanerLibAsset.generated.h"

struct FProjectCleanerIndirectAssetInfo;
struct FProjectCleanerScanSettings;

UCLASS()
class UProjectCleanerLibAsset final : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="ProjectCleaner|Lib|Asset")
	static bool AssetRegistryWorking();

	UFUNCTION(BlueprintCallable, Category="ProjectCleaner|Lib|Asset", meta=(ToolTip="Returns class name of given asset"))
	static FString GetAssetClassName(const FAssetData& AssetData);

	UFUNCTION(BlueprintCallable, Category="ProjectCleaner|Lib|Asset", meta=(ToolTip="Returns class of given asset"))
	static UClass* GetAssetClass(const FAssetData& AssetData);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner|Lib|Asset", meta=(ToolTip="Returns all dependecies for given assets"))
	static void GetAssetsDependencies(const TArray<FAssetData>& Assets, TArray<FAssetData>& Dependencies);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner|Lib|Asset", meta=(ToolTip="Returns all referencers for given assets"))
	static void GetAssetsReferencers(const TArray<FAssetData>& Assets, TArray<FAssetData>& Referencers);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner|Lib|Asset", meta=(ToolTip="Returns total disk size of given assets"))
	static int64 GetAssetsTotalSize(const TArray<FAssetData>& Assets);

	UFUNCTION(BlueprintCallable, Category="ProjectCleaner", meta=(Tooltip="Returns total size of given files"))
	static int64 GetFilesTotalSize(const TArray<FString>& Files);

	static void FixupRedirectors();
};
