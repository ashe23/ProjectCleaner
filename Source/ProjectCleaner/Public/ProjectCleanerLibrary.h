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
	static bool IsUnderForbiddenFolders(const FString& InFolderPathAbs, const TSet<FString>& ForbiddenFolders);
	static bool IsEngineFileExtension(const FString& Extension);
	static bool IsCorruptedEngineFile(const FString& InFilePathAbs);
	static bool HasIndirectlyUsedAssets(const FString& FileContent);

	static FString GetAssetClassName(const FAssetData& AssetData);
	static FString PathConvertToAbs(const FString& InRelPath);
	static FString PathConvertToRel(const FString& InAbsPath);
	
	static int64 GetAssetsTotalSize(const TArray<FAssetData>& Assets);

	static void FixupRedirectors();
	static void SaveAllAssets(const bool bPromptUser = true);
	static void UpdateAssetRegistry(const bool bSyncScan = false);
	static void FocusOnDirectory(const FString& InRelPath);
	static void GetLinkedAssets(const TArray<FAssetData>& Assets, TArray<FAssetData>& LinkedAssets);
	
	// === REFACTOR ==
	static void GetSubFolders(const FString& InDirPath, const bool bRecursive, TSet<FString>& SubFolders);
	static int32 GetSubFoldersNum(const FString& InDirPath, const bool bRecursive);
	static void GetEmptyFolders(const FString& InDirPath, TSet<FString>& EmptyFolders);
	static int32 GetEmptyFoldersNum(const FString& InDirPath);
	static bool IsEmptyFolder(const FString& InDirPath);
	static void GetNonEngineFiles(TSet<FString>& FilesNonEngine);
	static void GetForbiddenFolders(TSet<FString>& ForbiddenFolders);

	static void GetAssetsByPath(const FString& InDirPathAbs, const bool bRecursive, TArray<FAssetData>& Assets);
	static int32 GetAssetsByPathNum(const FString& InDirPathAbs, const bool bRecursive, TArray<FAssetData>& Assets);
	static void GetAssetsUsed(TArray<FAssetData>& AssetsUsed);
	static void GetAssetsUnused(TArray<FAssetData>& AssetsUnused);
	static void GetAssetsPrimary(TArray<FAssetData>& AssetsPrimary, const bool bIncludeDerivedClasses = false);
	static void GetAssetsIndirect(TArray<FAssetData>& AssetsIndirect);
	static void GetAssetsIndirectAdvanced(TArray<FProjectCleanerIndirectAsset>& AssetsIndirect); 
	static void GetAssetsCorrupted(TSet<FString>& FilesCorrupted);
	static void GetAssetsWithExternalRefs(TArray<FAssetData>& Assets);
	
	static void GetPrimaryAssetClasses(TArray<FName>& PrimaryAssetClasses, const bool bIncludeDerivedClasses = false);
private:
	static FString ConvertPathInternal(const FString& From, const FString& To, const FString& Path);
};
