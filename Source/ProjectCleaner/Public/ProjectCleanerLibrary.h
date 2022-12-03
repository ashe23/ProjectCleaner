// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectCleanerTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ProjectCleanerLibrary.generated.h"

class UProjectCleanerScanSettings;
struct FProjectCleanerIndirectAsset;

UCLASS(DisplayName="ProjectCleanerLibrary", meta=(ToolTip="Project Cleaner collection of helper functions"))
class UProjectCleanerLibrary final : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	static bool IsAssetRegistryWorking();
	static bool IsUnderFolder(const FString& InFolderPathAbs, const FString& RootFolder);
	static bool IsUnderAnyFolder(const FString& InFolderPathAbs, const TSet<FString>& Folders);
	static bool IsEngineFileExtension(const FString& Extension);
	static bool IsCorruptedEngineFile(const FString& InFilePathAbs);
	static bool HasIndirectlyUsedAssets(const FString& FileContent);

	static FString GetAssetClassName(const FAssetData& AssetData);
	static FString PathConvertToAbs(const FString& InRelPath);
	static FString PathConvertToRel(const FString& InAbsPath);

	static int64 GetAssetsTotalSize(const TArray<FAssetData>& Assets);

	static void GetAssetsWithExternalRefs(TArray<FAssetData>& Assets);
	static void GetAssetsIndirect(TArray<FAssetData>& AssetsIndirect);
	static void GetAssetsIndirectAdvanced(TArray<FProjectCleanerIndirectAsset>& AssetsIndirect);
	static void GetAssetsPrimary(TArray<FAssetData>& AssetsPrimary, const bool bIncludeDerivedClasses = false);
	static void GetPrimaryAssetClasses(TArray<FName>& PrimaryAssetClasses, const bool bIncludeDerivedClasses = false);
	static void FixupRedirectors();
	static void SaveAllAssets(const bool bPromptUser = true);
	static void UpdateAssetRegistry(const bool bSyncScan = false);
	static void FocusOnDirectory(const FString& InRelPath);
	static void FocusOnAssets(const TArray<FAssetData>& Assets);
	static void GetLinkedAssets(const TArray<FAssetData>& Assets, TArray<FAssetData>& LinkedAssets);
};
