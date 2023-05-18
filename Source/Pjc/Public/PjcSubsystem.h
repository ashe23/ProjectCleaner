// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "AssetToolsModule.h"
#include "ContentBrowserModule.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "PjcTypes.h"
#include "PjcSubsystem.generated.h"

UCLASS(Config=EditorPerProjectUserSettings, DisplayName="ProjectCleanerSubsystem")
class UPjcSubsystem final : public UEditorSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	static void ScanProjectAssets(TMap<EPjcAssetCategory, TSet<FAssetData>>& AssetsCategoryMapping, FString& ErrMsg);

	static void CleanProject();

	UFUNCTION(BlueprintCallable, Category="ProjectCleanerSubsystem")
	static void GetAssetsByCategory(const EPjcAssetCategory AssetCategory, TSet<FAssetData>& Assets);

	UFUNCTION(BlueprintCallable, Category="ProjectCleanerSubsystem")
	static void GetAssetIndirectInfo(const FAssetData& Asset, TArray<FPjcFileInfo>& Infos);

	UFUNCTION(BlueprintCallable, Category="ProjectCleanerSubsystem")
	static void GetFilesExternal(TSet<FString>& FilesExternal);

	UFUNCTION(BlueprintCallable, Category="ProjectCleanerSubsystem")
	static void GetFilesCorrupted(TSet<FString>& FilesCorrupted);

	UFUNCTION(BlueprintCallable, Category="ProjectCleanerSubsystem")
	static void GetFoldersEmpty(TSet<FString>& FoldersEmpty);

	static void GetClassNamesPrimary(TSet<FName>& ClassNames);
	static void GetClassNamesEditor(TSet<FName>& ClassNames);
	static void GetClassNamesExcluded(TSet<FName>& ClassNames);
	static void GetSourceAndConfigFiles(TSet<FString>& Files);
	static void GetAssetsDependencies(TSet<FAssetData>& Assets);
	static void ShowNotification(const FString& Msg, const SNotificationItem::ECompletionState State, const float Duration);
	static void ShowNotificationWithOutputLog(const FString& Msg, const SNotificationItem::ECompletionState State, const float Duration);
	static void ShaderCompilationEnable();
	static void ShaderCompilationDisable();
	static void OpenPathInFileExplorer(const FString& InPath);
	static void OpenAssetEditor(const FAssetData& InAsset);
	static void OpenSizeMapViewer(const TArray<FAssetData>& InAssets);
	static void OpenReferenceViewer(const TArray<FAssetData>& InAssets);
	static void OpenAssetAuditViewer(const TArray<FAssetData>& InAssets);
	static void TryOpenFile(const FString& InPath);
	static void FixupRedirectorsInProject();
	static void GetFilesInPath(const FString& InSearchPath, const bool bSearchRecursive, TSet<FString>& OutFiles);
	static void GetFilesInPathByExt(const FString& InSearchPath, const bool bSearchRecursive, const bool bExtSearchInvert, const TSet<FString>& InExtensions, TSet<FString>& OutFiles);
	static void GetFoldersInPath(const FString& InSearchPath, const bool bSearchRecursive, TSet<FString>& OutFolders);
	static void AssetCategoryMappingInit(TMap<EPjcAssetCategory, TSet<FAssetData>>& AssetsCategoryMapping);
	static bool AssetIsBlueprint(const FAssetData& InAsset);
	static bool AssetIsExtReferenced(const FAssetData& InAsset);
	static bool AssetIsCircular(const FAssetData& InAsset);
	static bool EditorIsInPlayMode();
	static bool ProjectContainsRedirectors();
	static bool PathIsEmpty(const FString& InPath);
	static bool PathIsExcluded(const FString& InPath);
	static bool PathIsEngineGenerated(const FString& InPath);
	static int64 GetAssetSize(const FAssetData& InAsset);
	static int64 GetAssetsTotalSize(const TSet<FAssetData>& InAssets);
	static FName GetAssetExactClassName(const FAssetData& InAsset);
	static UClass* GetAssetClassByName(const FName& InClassName);
	static FString PathNormalize(const FString& InPath);
	static FString PathConvertToAbsolute(const FString& InPath);
	static FString PathConvertToRelative(const FString& InPath);
	static FString PathConvertToObjectPath(const FString& InPath);
	static FAssetRegistryModule& GetModuleAssetRegistry();
	static FAssetToolsModule& GetModuleAssetTools();
	static FContentBrowserModule& GetModuleContentBrowser();
	static FPropertyEditorModule& GetModulePropertyEditor();

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Config, Category="ProjectCleanerSubsystem")
	bool bShowRealtimeThumbnails = false;

private:
	static void FindAssetsIndirect(TMap<FAssetData, TArray<FPjcFileInfo>>& AssetsIndirectInfo);
	static void FindAssetsExcludedByPaths(TMap<EPjcAssetCategory, TSet<FAssetData>>& AssetsCategoryMapping);
};
