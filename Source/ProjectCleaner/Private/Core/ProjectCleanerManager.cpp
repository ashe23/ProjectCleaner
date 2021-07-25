// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#include "Core/ProjectCleanerManager.h"
#include "Core/ProjectCleanerUtility.h"
#include "ProjectCleaner.h"
#include "StructsContainer.h"
#include "UI/ProjectCleanerNotificationManager.h"
// Engine Headers
#include "AssetRegistryModule.h"
#include "Misc/ScopedSlowTask.h"
#include "Settings/ContentBrowserSettings.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "IDirectoryWatcher.h"
#include "DirectoryWatcherModule.h"
#include "Misc/FileHelper.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Engine/AssetManagerSettings.h"
#include "Engine/AssetManager.h"

#define LOCTEXT_NAMESPACE "FProjectCleanerModule"

ProjectCleanerManager::ProjectCleanerManager()
{
	CleanerConfigs = GetMutableDefault<UCleanerConfigs>();
	AssetRegistry = &FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

	ensure(CleanerConfigs);
	// ensure(ExcludeOptions);
	ensure(AssetRegistry);
	// ensure(DirectoryWatcher);
}

void ProjectCleanerManager::Update()
{
	// Clean();

	ProjectCleanerDataManagerV2::GetAllAssetsByPath(TEXT("/Game"),AllAssets);
	ProjectCleanerDataManagerV2::GetInvalidFilesByPath(FPaths::ProjectContentDir(), AllAssets, CorruptedAssets, NonEngineFiles);
	ProjectCleanerDataManagerV2::GetIndirectAssetsByPath(FPaths::ProjectDir(), IndirectAssets);
	ProjectCleanerDataManagerV2::GetEmptyFolders(FPaths::ProjectContentDir(), EmptyFolders, CleanerConfigs->bScanDeveloperContents);
}

const TArray<FAssetData>& ProjectCleanerManager::GetAllAssets() const
{
	return AllAssets;
}

const TSet<FName>& ProjectCleanerManager::GetCorruptedAssets() const
{
	return CorruptedAssets;
}

const TSet<FName>& ProjectCleanerManager::GetNonEngineFiles() const
{
	return NonEngineFiles;
}

const TMap<FName, FIndirectAsset>& ProjectCleanerManager::GetIndirectAssets() const
{
	return IndirectAssets;
}

const TSet<FName>& ProjectCleanerManager::GetEmptyFolders() const
{
	return EmptyFolders;
}

void ProjectCleanerManager::Clean()
{
	// AllAssets.Empty();
	// CorruptedAssets.Empty();
	// NonEngineFiles.Empty();
	// IndirectAssets.Empty();
	// EmptyFolders.Em
}

#undef LOCTEXT_NAMESPACE