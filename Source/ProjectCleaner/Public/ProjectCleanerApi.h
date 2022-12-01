// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectCleanerScanSettings.h"
#include "ProjectCleanerTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ProjectCleanerApi.generated.h"

class UProjectCleanerScanSettings;
struct FProjectCleanerIndirectAsset;

UCLASS(DisplayName="ProjectCleanerApi", meta=(ToolTip="Project Cleaner API"))
class UProjectCleanerApi final : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category="ProjectCleaner", meta=(ToolTip="Returns all unused assets in project"))
	static void GetAssetsUnused(const FString& InFolderPathRel, const UProjectCleanerScanSettings* ScanSettings, TArray<FAssetData>& UnusedAssets);
	
	// return all indirectly used assets inside Content folder
	static void GetAssetsIndirect(TArray<FAssetData>& IndirectAssets);
	// return all indirectly used assets inside Content folder, and their usage location info
	static void GetAssetsIndirect(TArray<FProjectCleanerIndirectAsset>& IndirectAssets);
private:
	// return list of folders that must not be scanned
	static void GetFoldersBlacklist(const UProjectCleanerScanSettings& ScanSettings, TSet<FString>& BlacklistFolders);
	// return blacklist assets 
	static void GetAssetsBlacklist(TArray<FAssetData>& BlacklistAssets);
	// return all primary assets in project
	static void GetAssetsPrimary(TArray<FAssetData>& PrimaryAssets);
	// return all assets that have referencers outside Content folder
	static void GetAssetsWithExternalRefs(TArray<FAssetData>& Assets);
	// return all assets excluded by user
	static void GetAssetsExcluded(const UProjectCleanerScanSettings& ScanSettings, TArray<FAssetData>& ExcludedAssets);
};



