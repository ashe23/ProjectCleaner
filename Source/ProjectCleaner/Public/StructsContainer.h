// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "AssetData.h"
#include "StructsContainer.generated.h"

UCLASS(Transient)
class UCleanerConfigs : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY(DisplayName = "Scan Developer Content", EditAnywhere, Category = "CleanerConfigs", meta = (ToolTip = "Scan assets in 'Developers' folder. By Default false"))
	bool bScanDeveloperContents = false;

	UPROPERTY(DisplayName = "Scan Megascans Plugin Content", EditAnywhere, Category = "CleanerConfigs", meta = (ToolTip = "Scan assets in Megascans base content(MSPreset),if Megascans plugin is active. By Default false"))
	bool bScanMegascansContent = false;// todo:ashe23 not sure about this
	
	UPROPERTY(DisplayName = "Delete Empty Folders After Assets Deleted", EditAnywhere, Category = "CleanerConfigs")
	bool bAutomaticallyDeleteEmptyFolders = true;

	UPROPERTY(DisplayName = "Force Delete Assets", EditAnywhere, Category = "CleanerConfigs", meta = (ToolTip = "If assets failed to delete normally, we could try to force delete them. By Default true"))
	bool bForceDeleteAssets = true; // todo:ashe23 not sure about this
};

UCLASS(Transient)
class UExcludeOptions : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY(DisplayName = "Paths", EditAnywhere, Category = "ExcludeOptions", meta = (ContentDir))
	TArray<FDirectoryPath> Paths;

	UPROPERTY(DisplayName = "Classes", EditAnywhere, Category = "ExcludeOptions")
	TArray<UClass*> Classes;
};

UCLASS(Transient)
class UIndirectAsset : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY(DisplayName = "AssetName", VisibleAnywhere, Category="AssetUsedIndirectly")
	FString AssetName;

	UPROPERTY(DisplayName = "AssetPath", VisibleAnywhere, Category="AssetUsedIndirectly")
	FString AssetPath;

	UPROPERTY(DisplayName = "FilePath where asset used", VisibleAnywhere, Category="AssetUsedIndirectly")
	FString FilePath;

	UPROPERTY(DisplayName = "Line where asset used", VisibleAnywhere, Category="AssetUsedIndirectly")
	int32 LineNum;

	UPROPERTY(DisplayName = "AssetData", VisibleAnywhere, Category="AssetUsedIndirectly")
	FAssetData AssetData;
};

UCLASS(Transient)
class UCorruptedFile : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY(DisplayName = "Name", VisibleAnywhere, Category = "CorruptedFile")
	FString Name;
	UPROPERTY(DisplayName = "AbsolutePath", VisibleAnywhere, Category = "CorruptedFile")
	FString AbsolutePath;
};

UCLASS(Transient)
class UNonEngineFile : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY(DisplayName = "FileName", VisibleAnywhere, Category = "NonEngineFile")
	FString FileName;

	UPROPERTY(DisplayName = "FilePath", VisibleAnywhere, Category = "NonEngineFile")
	FString FilePath;
};

struct FIndirectAsset
{
	FString File;
	int32 Line;
	FName RelativePath;

	FIndirectAsset(): File(FString{}), Line(0), RelativePath(NAME_None) {}
};

enum class EUnusedAssetType
{
	Default,
	RootAsset,
	CircularAsset,
};

struct FUnusedAssetInfo
{
	EUnusedAssetType UnusedAssetType;
	
	TSet<FName> Refs;
	TSet<FName> Deps;
	TArray<FAssetData> CommonAssets;

	FUnusedAssetInfo() : UnusedAssetType(EUnusedAssetType::Default) {}
};

struct FStandardCleanerText
{
	constexpr static TCHAR* AssetsDeleteWindowTitle = TEXT("Confirm deletion");
	constexpr static TCHAR* AssetsDeleteWindowContent = TEXT("Are you sure you want to permanently delete unused assets?");
	constexpr static TCHAR* EmptyFolderWindowTitle = TEXT("Confirm deletion of empty folders");
	constexpr static TCHAR* EmptyFolderWindowContent = TEXT("Are you sure you want to delete all empty folders in project?");
	constexpr static TCHAR* StartingCleanup = TEXT("Starting Cleanup. This could take some time, please wait");
	constexpr static TCHAR* DeletingUnusedAssets = TEXT("Deleting unused assets...");
	constexpr static TCHAR* NoAssetsToDelete = TEXT("There are no assets to delete!");
	constexpr static TCHAR* NoEmptyFolderToDelete = TEXT("There are no empty folders to delete!");
	constexpr static TCHAR* AssetRegistryStillWorking = TEXT("Asset registry still working. Please wait while scan completes");
	constexpr static TCHAR* CantIncludeSomeAssets = TEXT("Cant include selected assets, because they excluded by 'Exclude Options' filter.");
	constexpr static TCHAR* FailedToDeleteSomeFolders = TEXT("Failed to delete some folders. Open 'Output Log' for more information.");
	constexpr static TCHAR* FailedToDeleteSomeAssets = TEXT("Failed to delete some assets. Try to check 'Force Delete assets' option");
	constexpr static TCHAR* SearchingForUnusedAssets = TEXT("Searching for unused assets...");
	constexpr static TCHAR* Scanning = TEXT("Scanning...");
	constexpr static TCHAR* AssetsSuccessfullyDeleted = TEXT("Assets deleted successfully");
	constexpr static TCHAR* LoadingAssets = TEXT("Loading assets...");
	constexpr static TCHAR* FixingUpRedirectors = TEXT("Fixing up redirectors...");
	constexpr static TCHAR* FixingUpRedirectorsConverting = TEXT("Converting assets objects to redirectors...");
};