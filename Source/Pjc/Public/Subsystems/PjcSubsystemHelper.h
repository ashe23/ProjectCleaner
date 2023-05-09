// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "PjcSubsystemHelper.generated.h"

struct FPjcFileSearchFilter;
struct FPjcAssetSearchFilter;
struct FPjcFileInfo;

UCLASS(DisplayName="ProjectCleanerHelperSubsystem", meta=(Tooltip="Class containing various helper functions", ShortToolTip="My Short Tooltip"))
class UPjcHelperSubsystem : public UEngineSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	static void GetClassNamesPrimary(TSet<FName>& OutClassNames);
	static void GetClassNamesEditor(TSet<FName>& OutClassNames);
	static void GetAssetsDependencies(TSet<FAssetData>& Assets);
	static FString PathNormalize(const FString& InPath);
	static FString PathConvertToAbsolute(const FString& InPath);
	static FString PathConvertToRelative(const FString& InPath);
	static FString PathConvertToObjectPath(const FString& InPath);

	static bool EditorIsInPlayMode();
	static bool ProjectContainsRedirectors();
	static bool AssetIsBlueprint(const FAssetData& InAssetData);
	static bool AssetIsExtReferenced(const FAssetData& InAssetData);
	static bool AssetIsCircular(const FAssetData& InAssetData);

	static FName GetAssetExactClassName(const FAssetData& InAssetData);

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
