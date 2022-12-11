// // Copyright Ashot Barkhudaryan. All Rights Reserved.
//
// #pragma once
//
// #include "CoreMinimal.h"
// #include "ProjectCleanerTypes.h"
// #include "Kismet/BlueprintFunctionLibrary.h"
// #include "Widgets/Notifications/SNotificationList.h"
// #include "ProjectCleanerLibrary.generated.h"
//
// class UProjectCleanerScanSettings;
// struct FProjectCleanerIndirectAsset;
//
// UCLASS(DisplayName="ProjectCleanerLibrary", meta=(ToolTip="Project Cleaner collection of helper functions"))
// class UProjectCleanerLibrary final : public UBlueprintFunctionLibrary
// {
// 	GENERATED_BODY()
// public:
// 	// AssetRegistry
//
// 	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner|AssetRegistry", meta=(ToolTip="Checks if AssetRegistry currently working"))
// 	static bool AssetRegistryWorking();
//
// 	UFUNCTION(BlueprintCallable, Category="ProjectCleaner|AssetRegistry", meta=(ToolTip="Updates AssetRegistry by scanning for assets on disk"))
// 	static void AssetRegistryUpdate(const bool bSyncScan = false);
//
// 	UFUNCTION(BlueprintCallable, Category="ProjectCleaner|AssetRegistry", meta=(ToolTip="Fixes all redirectors in given relative path ('/Game/...')"))
// 	static void AssetRegistryFixupRedirectors(const FString& InPathRel);
//
// 	// Assets
//
// 	UFUNCTION(BlueprintCallable, Category="ProjectCleaner|Assets", meta=(ToolTip="Saves all unsaved assets in project"))
// 	static void AssetsSaveAll(const bool bPromptUser = true);
//
// 	UFUNCTION(BlueprintCallable, Category="ProjectCleaner|Assets", meta=(ToolTip="Returns all assets that have external referencers outside /Game folder"))
// 	static void AssetsGetWithExternalRefs(TArray<FAssetData>& Assets);
//
// 	UFUNCTION(BlueprintCallable, Category="ProjectCleaner|Assets", meta=(ToolTip="Returns all assets that used indirectly"))
// 	static void AssetsGetIndirect(TArray<FAssetData>& AssetsIndirect);
//
// 	UFUNCTION(BlueprintCallable, Category="ProjectCleaner|Assets", meta=(ToolTip="Returns all assets that used indirectly with information about where they used"))
// 	static void AssetsGetIndirectAdvanced(TArray<FProjectCleanerIndirectAsset>& AssetsIndirect);
//
// 	UFUNCTION(BlueprintCallable, Category="ProjectCleaner|Assets", meta=(ToolTip="Returns all primary assets in project"))
// 	static void AssetsGetPrimary(TArray<FAssetData>& AssetsPrimary, const bool bIncludeDerivedClasses = false);
//
// 	UFUNCTION(BlueprintCallable, Category="ProjectCleaner|Assets", meta=(ToolTip="Returns all linked assets (refs and deps) to given assets"))
// 	static void AssetsGetLinked(const TArray<FAssetData>& Assets, TArray<FAssetData>& LinkedAssets);
//
// 	UFUNCTION(BlueprintCallable, Category="ProjectCleaner|Assets", meta=(ToolTip="Returns total disk size of given assets"))
// 	static int64 AssetsGetTotalSize(const TArray<FAssetData>& Assets);
//
// 	UFUNCTION(BlueprintCallable, Category="ProjectCleaner|Assets", meta=(ToolTip="Returns given asset class name"))
// 	static FString AssetGetClassName(const FAssetData& AssetData);
//
// 	// Paths
//
// 	// todo:ashe23 for ue5 add __ExternalObject__ and __ExternalActors__ folder paths
// 	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner|Paths", meta=(ToolTip="Returns absolute or relative path to project Content folder (/Game)"))
// 	static FString PathGetContentFolder(const bool bAbsolutePath);
//
// 	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner|Paths", meta=(ToolTip="Returns absolute or relative path to project Developers folder (/Game/Developers)"))
// 	static FString PathGetDevelopersFolder(const bool bAbsolutePath);
//
// 	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner|Paths", meta=(ToolTip="Returns absolute or relative path to current user Developers folder (/Game/Developers/{user_name})"))
// 	static FString PathGetDeveloperFolder(const bool bAbsolutePath);
//
// 	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner|Paths", meta=(ToolTip="Returns absolute or relative path to project Collections folder (/Game/Collections)"))
// 	static FString PathGetCollectionsFolder(const bool bAbsolutePath);
//
// 	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner|Paths",
// 		meta=(ToolTip="Returns absolute or relative path to project Developer Collections folder (/Game/Developers/{user_name}/Collections)"))
// 	static FString PathGetDeveloperCollectionFolder(const bool bAbsolutePath);
//
// 	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner|Paths", meta=(ToolTip="Returns absolute or relative path to MSPresets path (/Game/MSPresets)"))
// 	static FString PathGetMsPresetsFolder(const bool bAbsolutePath);
//
// 	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner|Paths",
// 		meta=(ToolTip="Convert given relative path to absolute. If given path is not under /Game folder, then empty string will be returned"))
// 	static FString PathConvertToAbs(const FString& InRelPath);
//
// 	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner|Paths",
// 		meta=(ToolTip="Convert given absolute path to relative. If given path is not under Content folder, then empty string will be returned"))
// 	static FString PathConvertToRel(const FString& InAbsPath);
//
// 	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner|Paths", meta=(ToolTip="Check if given path in under folder or not"))
// 	static bool PathIsUnderFolder(const FString& InSearchFolderPath, const FString& InRootFolderPath);
//
// 	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner|Paths", meta=(ToolTip="Check if given path in under any of given folders or not"))
// 	static bool PathIsUnderFolders(const FString& InSearchFolderPath, const TSet<FString>& Folders);
//
// 	// Utility
//
// 	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner|File", meta=(ToolTip="Checks if given extension has engine file extension or not"))
// 	static bool FileHasEngineExtension(const FString& Extension);
//
// 	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner|File", meta=(ToolTip="Checks if given engine file is corrupted or not"))
// 	static bool FileIsCorrupted(const FString& InFilePathAbs);
//
// 	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner|File", meta=(ToolTip="Checks if given file content constains indirectly used assets"))
// 	static bool FileContainsIndirectAssets(const FString& FileContent);
//
// 	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner|File", meta=(ToolTip="Returns total disk size of given files"))
// 	static int64 FilesGetTotalSize(const TArray<FString>& Files);
//
//
// 	// Confirmation Windows
//
// 	static EAppReturnType::Type ConfirmationWindowShow(const FText& Title, const FText& ContentText, const EAppMsgType::Type MsgType = EAppMsgType::YesNo);
// 	static bool ConfirmationWindowCancelled(const EAppReturnType::Type ReturnType);
//
// 	// Notifications
//
// 	UFUNCTION(BlueprintCallable, Category="ProjectCleaner|Notification", meta=(ToolTip="Shows simple notification modal window in editor"))
// 	static void NotificationShow(const FString& Msg, const EProjectCleanerModalStatus ModalStatus = EProjectCleanerModalStatus::None, const float Duration = 2.0f);
//
// 	UFUNCTION(BlueprintCallable, Category="ProjectCleaner|Notification", meta=(ToolTip="Shows simple notification modal window with OutputLog link in editor"))
// 	static void NotificationShowWithOutputLog(const FString& Msg, const EProjectCleanerModalStatus ModalStatus = EProjectCleanerModalStatus::None, const float Duration = 2.0f);
//
// 	// Content Browser
//
// 	static void FocusOnDirectory(const FString& InPathRel);
// 	static void FocusOnAssets(const TArray<FAssetData>& Assets);
//
// private:
// 	static SNotificationItem::ECompletionState GetCompletionStateFromModalStatus(const EProjectCleanerModalStatus ModalStatus);
// };
