// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ProjectCleanerLibAsset.generated.h"

UCLASS()
class UProjectCleanerLibAsset final : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="ProjectCleaner|Lib|Asset")
	static bool AssetRegistryWorking();

	UFUNCTION(BlueprintCallable, Category="ProjectCleaner|Lib|Asset", meta=(ToolTip="Returns class name of given asset"))
	static FString GetAssetClassName(const FAssetData& AssetData);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner|Lib|Asset", meta=(ToolTip="Returns all assets in project, that visible in ContentBrowser"))
	static void GetAssetsAll(TArray<FAssetData>& Assets);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner|Lib|Asset", meta=(ToolTip="Returns all primary assets in project"))
	static void GetAssetsPrimary(TArray<FAssetData>& AssetsPrimary);
};
