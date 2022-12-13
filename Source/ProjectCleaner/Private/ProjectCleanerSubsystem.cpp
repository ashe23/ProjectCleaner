// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "ProjectCleanerSubsystem.h"
#include "ProjectCleaner.h"
// Engine Headers
#include "AssetToolsModule.h"
#include "AssetRegistry/AssetRegistryModule.h"

UProjectCleanerSubsystem::UProjectCleanerSubsystem()
	:
	PlatformFile(&FPlatformFileManager::Get().GetPlatformFile()),
	ModuleAssetRegistry(&FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName)),
	ModuleAssetTools(&FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")))
{
}

void UProjectCleanerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UProjectCleanerSubsystem::Deinitialize()
{
	Super::Deinitialize();

	// todo:ashe23 clean cached data
}

#if WITH_EDITOR
void UProjectCleanerSubsystem::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	SaveConfig();
}
#endif

int64 UProjectCleanerSubsystem::GetAssetsTotalSize(const TArray<FAssetData>& Assets) const
{
	if (!ModuleAssetRegistry) return 0;

	int64 Size = 0;

	for (const auto& Asset : Assets)
	{
		const auto AssetPackageData = ModuleAssetRegistry->Get().GetAssetPackageData(Asset.PackageName);
		if (!AssetPackageData) continue;
		Size += AssetPackageData->DiskSize;
	}

	return Size;
}

int64 UProjectCleanerSubsystem::GetFilesTotalSize(const TSet<FString>& Files) const
{
	if (Files.Num() == 0) return 0;

	int64 TotalSize = 0;
	for (const auto& File : Files)
	{
		if (File.IsEmpty() || !FPaths::FileExists(File)) continue;

		TotalSize += IFileManager::Get().FileSize(*File);
	}

	return TotalSize;
}

const TArray<FAssetData>& UProjectCleanerSubsystem::GetAssetsAll() const
{
	return AssetsAll;
}

const TArray<FAssetData>& UProjectCleanerSubsystem::GetAssetsIndirect() const
{
	return AssetsIndirect;
}

const TArray<FAssetData>& UProjectCleanerSubsystem::GetAssetsExcluded() const
{
	return AssetsExcluded;
}

const TArray<FAssetData>& UProjectCleanerSubsystem::GetAssetsUnused() const
{
	return AssetsUnused;
}

const TSet<FString>& UProjectCleanerSubsystem::GetFoldersTotal() const
{
	return FoldersTotal;
}

const TSet<FString>& UProjectCleanerSubsystem::GetFoldersEmpty() const
{
	return FoldersEmpty;
}

const TSet<FString>& UProjectCleanerSubsystem::GetFilesCorrupted() const
{
	return FilesCorrupted;
}

const TSet<FString>& UProjectCleanerSubsystem::GetFilesNonEngine() const
{
	return FilesNonEngine;
}

bool UProjectCleanerSubsystem::IsAssetRegistryWorking() const
{
	if (!ModuleAssetRegistry) return false;

	return ModuleAssetRegistry->Get().IsLoadingAssets();
}

bool UProjectCleanerSubsystem::IsEditorInPlayMode() const
{
	if (!GEditor) return false;

	return GEditor->PlayWorld || GIsPlayInEditorWorld;
}

bool UProjectCleanerSubsystem::IsScanningProject() const
{
	return bScanningProject;
}

bool UProjectCleanerSubsystem::IsCleaningProject() const
{
	return bCleaningProject;
}
