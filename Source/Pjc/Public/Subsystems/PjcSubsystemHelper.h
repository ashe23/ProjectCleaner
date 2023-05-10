// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "PjcSubsystemHelper.generated.h"

struct FPjcFileSearchFilter;
struct FPjcAssetSearchFilter;
struct FPjcFileInfo;

UCLASS()
class UPjcHelperSubsystem : public UEngineSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	static void GetClassNamesPrimary(TSet<FName>& OutClassNames);
	static void GetClassNamesEditor(TSet<FName>& OutClassNames);
	static void GetAssetsDependencies(TSet<FAssetData>& Assets);
	static void ShowNotification(const FString& Msg, const SNotificationItem::ECompletionState State, const float Duration);
	static void ShowNotificationWithOutputLog(const FString& Msg, const SNotificationItem::ECompletionState State, const float Duration);
	static void ShaderCompilationEnable();
	static void ShaderCompilationDisable();
	static void OpenPathInFileExplorer(const FString& InPath);
	static void OpenAssetEditor(const FAssetData& InAssetData);
	static void OpenSizeMapViewer(const TArray<FAssetData>& InAssetDatas);
	static void OpenReferenceViewer(const TArray<FAssetData>& InAssetDatas);
	static void OpenAssetAuditViewer(const TArray<FAssetData>& InAssetDatas);
	static void TryOpenFile(const FString& InPath);
	static FString PathNormalize(const FString& InPath);
	static FString PathConvertToAbsolute(const FString& InPath);
	static FString PathConvertToRelative(const FString& InPath);
	static FString PathConvertToObjectPath(const FString& InPath);

	static bool EditorIsInPlayMode();
	static bool ProjectContainsRedirectors();
	static bool AssetIsBlueprint(const FAssetData& InAssetData);
	static bool AssetIsExtReferenced(const FAssetData& InAssetData);
	static bool AssetIsCircular(const FAssetData& InAssetData);
	static bool PathIsEmpty(const FString& InPath);
	static bool PathIsExcluded(const FString& InPath);
	static bool PathIsEngineGenerated(const FString& InPath);

	static FName GetAssetExactClassName(const FAssetData& InAssetData);

	static int64 GetAssetSize(const FAssetData& InAssetData);
	static int64 GetAssetsTotalSize(const TSet<FAssetData>& InAssets);

	static FAssetRegistryModule& GetModuleAssetRegistry();
	static FAssetToolsModule& GetModuleAssetTools();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="PjcHelper")
	static void GetAssetsByFilter(const FPjcAssetSearchFilter& InSearchFilter, TArray<FAssetData>& OutAssets);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="PjcHelper")
	static void GetFilesInPath(const FString& InSearchPath, const bool bSearchRecursive, TSet<FString>& OutFiles);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="PjcHelper")
	static void GetFilesInPathByExt(const FString& InSearchPath, const bool bSearchRecursive, const bool bExtSearchInvert, const TSet<FString>& InExtensions, TSet<FString>& OutFiles);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="PjcHelper")
	static void GetFoldersInPath(const FString& InSearchPath, const bool bSearchRecursive, TSet<FString>& OutFolders);
};
