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

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	/**
	 * @brief Returns all assets in project (particularly Content folder)
	 * @param Assets TArray<FAssetData>
	 */
	UFUNCTION(BlueprintCallable, Category="ProjectCleanerSubsystem|Lib_Asset")
	static void GetAssetsAll(TArray<FAssetData>& Assets);

	/**
	 * @brief Returns all used assets in project
	 * @param Assets TArray<FAssetData>
	 * @param bShowSlowTask bool
	 */
	UFUNCTION(BlueprintCallable, Category="ProjectCleanerSubsystem|Lib_Asset")
	static void GetAssetsUsed(TArray<FAssetData>& Assets, const bool bShowSlowTask = true);

	/**
	 * @brief Returns all unused assets in project
	 * @param Assets TArray<FAssetData>
	 * @param bShowSlowTask bool
	 */
	UFUNCTION(BlueprintCallable, Category="ProjectCleanerSubsystem|Lib_Asset")
	static void GetAssetsUnused(TArray<FAssetData>& Assets, const bool bShowSlowTask = true);

	/**
	 * @brief Returns all primary and derived from primary assets in project. See AssetManager Settings for more info.
	 * @param Assets TArray<FAssetData>
	 * @param bShowSlowTask bool
	 */
	UFUNCTION(BlueprintCallable, Category="ProjectCleanerSubsystem|Lib_Asset")
	static void GetAssetsPrimary(TArray<FAssetData>& Assets, const bool bShowSlowTask = true);

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
	 * @param bShowSlowTask bool
	 */
	UFUNCTION(BlueprintCallable, Category="ProjectCleanerSubsystem|Lib_Asset")
	static void GetAssetsCircular(TArray<FAssetData>& Assets, const bool bShowSlowTask = true);

	/**
	 * @brief Returns assets that are editor specific. Like Tutorial asset, EditorUtilityBlueprint or EditorUtilityWidgets.
	 * @param Assets TArray<FAssetData>
	 * @param bShowSlowTask bool
	 */
	UFUNCTION(BlueprintCallable, Category="ProjectCleanerSubsystem|Lib_Asset")
	static void GetAssetsEditor(TArray<FAssetData>& Assets, const bool bShowSlowTask = true);

	/**
	 * @brief Returns all excluded assets in project
	 * @param Assets TArray<FAssetData>
	 * @param bShowSlowTask bool
	 */
	UFUNCTION(BlueprintCallable, Category="ProjectCleanerSubsystem|Lib_Asset")
	static void GetAssetsExcluded(TArray<FAssetData>& Assets, const bool bShowSlowTask = true);

	/**
	 * @brief Returns all assets that have external referencers outside Content folder
	 * @param Assets TArray<FAssetData>
	 *  @param bShowSlowTask bool
	 */
	UFUNCTION(BlueprintCallable, Category="ProjectCleanerSubsystem|Lib_Asset")
	static void GetAssetsExtReferenced(TArray<FAssetData>& Assets, const bool bShowSlowTask = true);

	/**
	 * @brief Returns all primary assets class names
	 * @param ClassNames TSet<FName>
	 */
	UFUNCTION(BlueprintCallable, Category="ProjectCleanerSubsystem|Lib_Asset")
	static void GetClassNamesPrimary(TSet<FTopLevelAssetPath>& ClassNames);

	/**
	 * @brief Returns all editor assets class names
	 * @param ClassNames TSet<FName>
	 */
	UFUNCTION(BlueprintCallable, Category="ProjectCleanerSubsystem|Lib_Asset")
	static void GetClassNamesEditor(TSet<FTopLevelAssetPath>& ClassNames);

	/**
	 * @brief Returns all excluded assets class names
	 * @param ClassNames TSet<FName>
	 */
	UFUNCTION(BlueprintCallable, Category="ProjectCleanerSubsystem|Lib_Asset")
	static void GetClassNamesExcluded(TSet<FTopLevelAssetPath>& ClassNames);

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
	 * @param bShowSlowTask bool
	 */
	UFUNCTION(BlueprintCallable, Category="ProjectCleanerSubsystem|Lib_Path")
	static void GetFilesExternalFiltered(TArray<FString>& Files, const bool bShowSlowTask = true);

	/**
	 * @brief Returns all external files that has been excluded
	 * @param Files
	 * @param bShowSlowTask bool
	 */
	UFUNCTION(BlueprintCallable, Category="ProjectCleanerSubsystem|Lib_Path")
	static void GetFilesExternalExcluded(TArray<FString>& Files, const bool bShowSlowTask = true);

	/**
	 * @brief Returns all corrupted asset files in project
	 * @param Files
	 * @param bShowSlowTask bool
	 */
	UFUNCTION(BlueprintCallable, Category="ProjectCleanerSubsystem|Lib_Path")
	static void GetFilesCorrupted(TArray<FString>& Files, const bool bShowSlowTask = true);

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

	/**
	 * @brief Delete all unused assets in project. This does not delete excluded assets.
	 * @param bShowSlowTask bool
	 * @param bShowEditorNotification bool
	 */
	UFUNCTION(BlueprintCallable, Category="ProjectCleanerSubsystem|Lib_Asset")
	static void DeleteAssetsUnused(const bool bShowSlowTask = true, const bool bShowEditorNotification = false);

	/**
	 * @brief Delete all empty folders in project
	 * @param bShowSlowTask bool
	 * @param bShowEditorNotification bool
	 */
	UFUNCTION(BlueprintCallable, Category="ProjectCleanerSubsystem|Lib_Path")
	static void DeleteFoldersEmpty(const bool bShowSlowTask = true, const bool bShowEditorNotification = false);

	/**
	 * @brief Delete all external files in project. This does not delete excluded files.
	 * @param bShowSlowTask bool
	 * @param bShowEditorNotification bool
	 */
	UFUNCTION(BlueprintCallable, Category="ProjectCleanerSubsystem|Lib_Path")
	static void DeleteFilesExternal(const bool bShowSlowTask = true, const bool bShowEditorNotification = false);

	/**
	 * @brief Delete all corrupted asset files in project
	 * @param bShowSlowTask bool
	 * @param bShowEditorNotification bool
	 */
	UFUNCTION(BlueprintCallable, Category="ProjectCleanerSubsystem|Lib_Path")
	static void DeleteFilesCorrupted(const bool bShowSlowTask = true, const bool bShowEditorNotification = false);

	/**
	 * @brief Returns all redirectors in project
	 * @param Redirectors TArray<FAssetData>
	 */
	UFUNCTION(BlueprintCallable, Category="ProjectCleanerSubsystem|Lib_Asset")
	static void GetProjectRedirectors(TArray<FAssetData>& Redirectors);

	/**
	 * @brief Checks if project contains any redirector asset
	 * @return bool
	 */
	UFUNCTION(BlueprintCallable, Category="ProjectCleanerSubsystem|Lib_Asset")
	static bool ProjectHasRedirectors();

	/**
	 * @brief Fixup given redirector assets
	 * @param Redirectors TArray<FAssetData>
	 * @param bShowSlowTask bool
	 */
	UFUNCTION(BlueprintCallable, Category="ProjectCleanerSubsystem|Lib_Asset")
	static void FixProjectRedirectors(const TArray<FAssetData>& Redirectors, const bool bShowSlowTask = true);

	/**
	 * @brief Checks if editor is in play mode or simulation or not
	 * @return bool
	 */
	UFUNCTION(BlueprintCallable, Category="ProjectCleanerSubsystem|Lib_Editor")
	static bool EditorIsInPlayMode();

	/**
	 * @brief Checks if given asset is blueprint or not
	 * @param InAsset FAssetData
	 * @return bool
	 */
	UFUNCTION(BlueprintCallable, Category="ProjectCleanerSubsystem|Lib_Asset")
	static bool AssetIsBlueprint(const FAssetData& InAsset);

	/**
	 * @brief Checks if given asset has external referencers outside Content folder or not
	 * @param InAsset FAssetData
	 * @return bool
	 */
	UFUNCTION(BlueprintCallable, Category="ProjectCleanerSubsystem|Lib_Asset")
	static bool AssetIsExtReferenced(const FAssetData& InAsset);

	/**
	 * @brief Checks if given asset has circular dependencies or not
	 * @param InAsset FAssetData
	 * @return bool
	 */
	UFUNCTION(BlueprintCallable, Category="ProjectCleanerSubsystem|Lib_Asset")
	static bool AssetIsCircular(const FAssetData& InAsset);

	/**
	 * @brief Normalize given path
	 * @param InPath FString
	 * @return FString
	 */
	UFUNCTION(BlueprintCallable, Category="ProjectCleanerSubsystem|Lib_Path")
	static FString PathNormalize(const FString& InPath);

	/**
	 * @brief Convert given path to absolute
	 * @param InPath FString
	 * @return FString
	 */
	UFUNCTION(BlueprintCallable, Category="ProjectCleanerSubsystem|Lib_Path")
	static FString PathConvertToAbsolute(const FString& InPath);

	/**
	 * @brief Convert given path relative to Content folder
	 * @param InPath FString
	 * @return FString
	 */
	UFUNCTION(BlueprintCallable, Category="ProjectCleanerSubsystem|Lib_Path")
	static FString PathConvertToRelative(const FString& InPath);

	/**
	 * @brief Convert given path to object path 
	 * @param InPath FString
	 * @return FString
	 */
	UFUNCTION(BlueprintCallable, Category="ProjectCleanerSubsystem|Lib_Path")
	static FString PathConvertToObjectPath(const FString& InPath);

	/**
	 * @brief Returns given asset size on disk in bytes
	 * @param InAsset FAssetData
	 * @return int64
	 */
	UFUNCTION(BlueprintCallable, Category="ProjectCleanerSubsystem|Lib_Asset")
	static int64 GetAssetSize(const FAssetData& InAsset);

	/**
	 * @brief Returns total size of given assets in bytes
	 * @param InAssets TArray<FAssetData>
	 * @return int64
	 */
	UFUNCTION(BlueprintCallable, Category="ProjectCleanerSubsystem|Lib_Asset")
	static int64 GetAssetsTotalSize(const TArray<FAssetData>& InAssets);

	/**
	 * @brief Returns size of given file in bytes
	 * @param InFile FString
	 * @return int64
	 */
	UFUNCTION(BlueprintCallable, Category="ProjectCleanerSubsystem|Lib_Path")
	static int64 GetFileSize(const FString& InFile);

	/**
	 * @brief Returns total size of given files in bytes
	 * @param Files TArray<FString>
	 * @return int64
	 */
	UFUNCTION(BlueprintCallable, Category="ProjectCleanerSubsystem|Lib_Path")
	static int64 GetFilesTotalSize(const TArray<FString>& Files);

	/**
	 * @brief Returns asset exact class name, if its blueprint it will return generated class name
	 * @param InAsset FAssetData
	 * @return FName 
	 */
	UFUNCTION(BlueprintCallable, Category="ProjectCleanerSubsystem|Lib_Asset")
	static FTopLevelAssetPath GetAssetExactClassName(const FAssetData& InAsset);

	static bool FolderIsEmpty(const FString& InPath);
	static bool FolderIsExcluded(const FString& InPath);
	static bool FolderIsEngineGenerated(const FString& InPath);
	static bool FolderIsExternal(const FString& InPath);
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
	static FString GetPathExternalActors();
	static FString GetPathExternalObjects();

	static FAssetToolsModule& GetModuleAssetTools();
	static FAssetRegistryModule& GetModuleAssetRegistry();
	static FContentBrowserModule& GetModuleContentBrowser();
	static FPropertyEditorModule& GetModulePropertyEditor();

	UPROPERTY(Config)
	bool bShowFilesExternal = true;

	UPROPERTY(Config)
	bool bShowFoldersEmpty = true;

	UPROPERTY(Config)
	bool bShowFoldersExcluded = true;

	UPROPERTY(Config)
	bool bShowFoldersUsed = true;

	UPROPERTY(Config)
	bool bShowFoldersEngine = true;

	bool bFirstScan = true;

private:
	static void BucketFill(TArray<FAssetData>& AssetsUnused, TArray<FAssetData>& Bucket, const int32 BucketSize);
	static bool BucketPrepare(const TArray<FAssetData>& Bucket, TArray<UObject*>& LoadedAssets);
	static int32 BucketDelete(const TArray<UObject*>& LoadedAssets);
};
