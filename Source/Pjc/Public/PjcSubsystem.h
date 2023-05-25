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


	/**
	 * @brief Returns all assets in project (particularly Content folder)
	 * @param Assets TArray<FAssetData>
	 */
	UFUNCTION(BlueprintCallable, Category="ProjectCleanerSubsystem|Lib_Asset")
	static void GetAssetsAll(TArray<FAssetData>& Assets);

	/**
	 * @brief Returns all used assets in project
	 * @param Assets TArray<FAssetData>
	 */
	UFUNCTION(BlueprintCallable, Category="ProjectCleanerSubsystem|Lib_Asset")
	static void GetAssetsUsed(TArray<FAssetData>& Assets);

	/**
	 * @brief Returns all unused assets in project
	 * @param Assets TArray<FAssetData>
	 */
	UFUNCTION(BlueprintCallable, Category="ProjectCleanerSubsystem|Lib_Asset")
	static void GetAssetsUnused(TArray<FAssetData>& Assets);

	/**
	 * @brief Returns all primary and derived from primary assets in project. See AssetManager Settings for more info.
	 * @param Assets TArray<FAssetData>
	 */
	UFUNCTION(BlueprintCallable, Category="ProjectCleanerSubsystem|Lib_Asset")
	static void GetAssetsPrimary(TArray<FAssetData>& Assets);

	/**
	 * @brief Returns assets that used in source code or config files indirectly.
	 * @param Assets TArray<FAssetData> - Assets
	 * @param AssetsIndirectInfos TArray<FPjcAssetIndirectInfo> - Assets and their usage information
	 * @param bShowSlowTask bool - Show slow task progress bar or not
	 */
	UFUNCTION(BlueprintCallable, Category="ProjectCleanerSubsystem|Lib_Asset")
	static void GetAssetsIndirect(TArray<FAssetData>& Assets, TArray<FPjcAssetIndirectInfo>& AssetsIndirectInfos, const bool bShowSlowTask = true);

	/**
	 * @brief Returns assets that have circular dependencies
	 * @param Assets TArray<FAssetData>
	 */
	UFUNCTION(BlueprintCallable, Category="ProjectCleanerSubsystem|Lib_Asset")
	static void GetAssetsCircular(TArray<FAssetData>& Assets);

	/**
	 * @brief Returns assets that are editor specific. Like Tutorial asset, EditorUtilityBlueprint or EditorUtilityWidgets.
	 * @param Assets TArray<FAssetData>
	 */
	UFUNCTION(BlueprintCallable, Category="ProjectCleanerSubsystem|Lib_Asset")
	static void GetAssetsEditor(TArray<FAssetData>& Assets);

	/**
	 * @brief Returns all excluded assets in project
	 * @param Assets TArray<FAssetData>
	 */
	UFUNCTION(BlueprintCallable, Category="ProjectCleanerSubsystem|Lib_Asset")
	static void GetAssetsExcluded(TArray<FAssetData>& Assets);

	/**
	 * @brief Returns all assets that have external referencers outside Content folder
	 * @param Assets TArray<FAssetData>
	 */
	UFUNCTION(BlueprintCallable, Category="ProjectCleanerSubsystem|Lib_Asset")
	static void GetAssetsExtReferenced(TArray<FAssetData>& Assets);

	/**
	 * @brief Returns all primary assets class names
	 * @param ClassNames TSet<FName>
	 */
	UFUNCTION(BlueprintCallable, Category="ProjectCleanerSubsystem|Lib_Asset")
	static void GetClassNamesPrimary(TSet<FName>& ClassNames);

	/**
	 * @brief Returns all editor assets class names
	 * @param ClassNames TSet<FName>
	 */
	UFUNCTION(BlueprintCallable, Category="ProjectCleanerSubsystem|Lib_Asset")
	static void GetClassNamesEditor(TSet<FName>& ClassNames);

	/**
	 * @brief Returns all excluded assets class names
	 * @param ClassNames TSet<FName>
	 */
	UFUNCTION(BlueprintCallable, Category="ProjectCleanerSubsystem|Lib_Asset")
	static void GetClassNamesExcluded(TSet<FName>& ClassNames);

	/**
	 * @brief Returns list of files inside given path with specified settings
	 * @param InSearchPath FString
	 * @param bSearchRecursive bool
	 * @param OutFiles
	 */
	UFUNCTION(BlueprintCallable, Category="ProjectCleanerSubsystem|Lib_Path")
	static void GetFiles(const FString& InSearchPath, const bool bSearchRecursive, TArray<FString>& OutFiles);

	/**
	 * @brief Returns list of files inside given path with specified file extension settings
	 * @param InSearchPath FString
	 * @param bSearchRecursive bool
	 * @param bExtSearchInvert bool
	 * @param InExtensions TSet<FString>
	 * @param OutFiles
	 */
	UFUNCTION(BlueprintCallable, Category="ProjectCleanerSubsystem|Lib_Path")
	static void GetFilesByExt(const FString& InSearchPath, const bool bSearchRecursive, const bool bExtSearchInvert, const TSet<FString>& InExtensions, TArray<FString>& OutFiles);

	/**
	 * @brief Returns all external files in project. Files that dont .uasset or .umap extensions
	 * @param Files
	 */
	UFUNCTION(BlueprintCallable, Category="ProjectCleanerSubsystem|Lib_Path")
	static void GetFilesExternalAll(TArray<FString>& Files);

	/**
	 * @brief Returns all external files minus excluded files
	 * @param Files
	 */
	UFUNCTION(BlueprintCallable, Category="ProjectCleanerSubsystem|Lib_Path")
	static void GetFilesExternalFiltered(TArray<FString>& Files);

	/**
	 * @brief Returns all external files that has been excluded
	 * @param Files
	 */
	UFUNCTION(BlueprintCallable, Category="ProjectCleanerSubsystem|Lib_Path")
	static void GetFilesExternalExcluded(TArray<FString>& Files);

	/**
	 * @brief Returns all corrupted asset files in project
	 * @param Files
	 */
	UFUNCTION(BlueprintCallable, Category="ProjectCleanerSubsystem|Lib_Path")
	static void GetFilesCorrupted(TArray<FString>& Files);

	/**
	 * @brief Returns all subfolders in given path
	 * @param InSearchPath FString
	 * @param bSearchRecursive bool
	 * @param OutFolders
	 */
	UFUNCTION(BlueprintCallable, Category="ProjectCleanerSubsystem|Lib_Path")
	static void GetFolders(const FString& InSearchPath, const bool bSearchRecursive, TArray<FString>& OutFolders);

	/**
	 * @brief Returns all empty folders in project
	 * @param Folders TSet<FString>
	 */
	UFUNCTION(BlueprintCallable, Category="ProjectCleanerSubsystem|Lib_Path")
	static void GetFoldersEmpty(TArray<FString>& Folders);


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
	static FAssetToolsModule& GetModuleAssetTools();
	static FAssetRegistryModule& GetModuleAssetRegistry();
	static FContentBrowserModule& GetModuleContentBrowser();
	static FPropertyEditorModule& GetModulePropertyEditor();

	static void ScanProjectAssets(TMap<EPjcAssetCategory, TSet<FAssetData>>& AssetsCategoryMapping, FString& ErrMsg);
	static void CleanProject();

	UPROPERTY(Config)
	bool bShowFilesExternal = true;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
	static void FindAssetsIndirect(TMap<FAssetData, TArray<FPjcFileInfo>>& AssetsIndirectInfo);
	static void FindAssetsExcludedByPaths(TMap<EPjcAssetCategory, TSet<FAssetData>>& AssetsCategoryMapping);
};
