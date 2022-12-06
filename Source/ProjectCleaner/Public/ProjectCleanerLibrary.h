// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectCleanerTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "ProjectCleanerLibrary.generated.h"

class UProjectCleanerScanSettings;
struct FProjectCleanerIndirectAsset;

UCLASS(DisplayName="ProjectCleanerLibrary", meta=(ToolTip="Project Cleaner collection of helper functions"))
class UProjectCleanerLibrary final : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	// AssetRegistry
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner|AssetRegistry", meta=(ToolTip="Checks if AssetRegistry currently working"))
	static bool AssetRegistryWorking();

	UFUNCTION(BlueprintCallable, Category="ProjectCleaner|AssetRegistry", meta=(ToolTip="Updates AssetRegistry by scanning for assets on disk"))
	static void AssetRegistryUpdate(const bool bSyncScan = false);

	UFUNCTION(BlueprintCallable, Category="ProjectCleaner|AssetRegistry", meta=(ToolTip="Fixes all redirectors in given relative path ('/Game/...')"))
	static void AssetRegistryFixupRedirectors(const FString& InPathRel);

	// Assets
	UFUNCTION(BlueprintCallable, Category="ProjectCleaner|Assets", meta=(ToolTip="Saves all unsaved assets in project"))
	static void AssetsSaveAll(const bool bPromptUser = true);

	// Paths
	// todo:ashe23 for ue5 add __ExternalObject__ and __ExternalActors__ folder paths
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner|Paths", meta=(ToolTip="Returns absolute or relative path to project Content folder (/Game)"))
	static FString PathGetContentFolder(const bool bAbsolutePath);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner|Paths", meta=(ToolTip="Returns absolute or relative path to project Developers folder (/Game/Developers)"))
	static FString PathGetDevelopersFolder(const bool bAbsolutePath);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner|Paths", meta=(ToolTip="Returns absolute or relative path to current user Developers folder (/Game/Developers/{user_name})"))
	static FString PathGetDeveloperFolder(const bool bAbsolutePath);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner|Paths", meta=(ToolTip="Returns absolute or relative path to project Collections folder (/Game/Collections)"))
	static FString PathGetCollectionsFolder(const bool bAbsolutePath);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner|Paths",
		meta=(ToolTip="Returns absolute or relative path to project Developer Collections folder (/Game/Developers/{user_name}/Collections)"))
	static FString PathGetDeveloperCollectionFolder(const bool bAbsolutePath);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner|Paths", meta=(ToolTip="Returns absolute or relative path to MSPresets path (/Game/MSPresets)"))
	static FString PathGetMsPresetsFolder(const bool bAbsolutePath);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner|Paths",
		meta=(ToolTip="Convert given relative path to absolute. If given path is not under /Game folder, then empty string will be returned"))
	static FString PathConvertToAbs(const FString& InRelPath);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner|Paths",
		meta=(ToolTip="Convert given absolute path to relative. If given path is not under Content folder, then empty string will be returned"))
	static FString PathConvertToRel(const FString& InAbsPath);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner|Paths", meta=(ToolTip="Check if given path in under folder or not"))
	static bool PathIsUnderFolder(const FString& InSearchFolderPath, const FString& InRootFolderPath);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner|Paths", meta=(ToolTip="Check if given path in under any of given folders or not"))
	static bool PathIsUnderFolders(const FString& InSearchFolderPath, const TSet<FString>& Folders);

	// Misc
	// Utility
	// Notification
	static bool IsEngineFileExtension(const FString& Extension);
	static bool IsCorruptedEngineFile(const FString& InFilePathAbs);
	static bool HasIndirectlyUsedAssets(const FString& FileContent);
	static bool ConfirmationWindowCancelled(const EAppReturnType::Type ReturnType);

	static FString GetAssetClassName(const FAssetData& AssetData);

	static int64 GetAssetsTotalSize(const TArray<FAssetData>& Assets);

	static void GetAssetsWithExternalRefs(TArray<FAssetData>& Assets);
	static void GetAssetsIndirect(TArray<FAssetData>& AssetsIndirect);
	static void GetAssetsIndirectAdvanced(TArray<FProjectCleanerIndirectAsset>& AssetsIndirect);
	static void GetAssetsPrimary(TArray<FAssetData>& AssetsPrimary, const bool bIncludeDerivedClasses = false);
	static void GetPrimaryAssetClasses(TArray<FName>& PrimaryAssetClasses, const bool bIncludeDerivedClasses = false);
	static void FocusOnDirectory(const FString& InRelPath);
	static void FocusOnAssets(const TArray<FAssetData>& Assets);
	static void GetLinkedAssets(const TArray<FAssetData>& Assets, TArray<FAssetData>& LinkedAssets);

	static EAppReturnType::Type ShowConfirmationWindow(const FText& Title, const FText& ContentText, const EAppMsgType::Type MsgType = EAppMsgType::YesNo);

	static void ShowModal(const FString& Msg, const EProjectCleanerModalStatus ModalStatus = EProjectCleanerModalStatus::None, const float Duration = 2.0f);
	static void ShowModalOutputLog(const FString& Msg, const EProjectCleanerModalStatus ModalStatus = EProjectCleanerModalStatus::None, const float Duration = 2.0f);
private:
	static SNotificationItem::ECompletionState GetCompletionStateFromModalStatus(const EProjectCleanerModalStatus ModalStatus);
};
