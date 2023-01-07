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

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner|Lib|Asset", meta=(ToolTip="Returns all assets in project"))
	static void GetAssetsAll(TArray<FAssetData>& Assets);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner|Lib|Asset", meta=(ToolTip="Returns all primary assets in project"))
	static void GetAssetsPrimary(TArray<FAssetData>& AssetsPrimary);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner|Lib|Asset", meta=(ToolTip="Returns all indirect assets in project"))
	static void GetAssetsIndirect(TArray<FAssetData>& AssetsIndirect);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner|Lib|Asset", meta=(ToolTip="Returns all indirect assets in project with their info where they used"))
	static void GetAssetsIndirectInfo(TArray<FProjectCleanerIndirectAssetInfo>& AssetsIndirectInfos);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner|Lib|Asset", meta=(ToolTip="Returns all used assets in project"))
	static void GetAssetsUsed(TArray<FAssetData>& AssetsUsed);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner|Lib|Asset", meta=(ToolTip="Returns total disk size of given assets"))
	static int64 GetAssetsTotalSize(const TArray<FAssetData>& Assets);

	// static bool AssetIsPrimary(const FAssetData AssetData, const TArray<FAssetData>& PrimaryAssets);
	// static bool AssetIsIndirect(const FAssetData AssetData, const TArray<FAssetData>& IndirectAssets);
	// static bool AssetIsExcluded(const FAssetData AssetData, const FProjectCleanerScanSettings& ScanSettings);

	static void FixupRedirectors();

private:
	// Returns all assets in project that are editor specific. Assets like EditorUtility widgets etc
	static void GetAssetsEditor(TArray<FAssetData>& AssetsEditor);

	// Returns all base assets of megascans plugin (if plugin enabled)
	static void GetAssetsMegascans(TArray<FAssetData>& AssetsMegascans);

	// Returns all assets in project that have referencers outside /Game folder
	static void GetAssetsWithExternalRefs(TArray<FAssetData>& Assets, const TArray<FAssetData>& AssetsAll);
};
