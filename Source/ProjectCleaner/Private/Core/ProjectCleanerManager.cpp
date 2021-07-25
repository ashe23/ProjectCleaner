// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#include "Core/ProjectCleanerManager.h"
#include "Core/ProjectCleanerUtility.h"
#include "ProjectCleaner.h"
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
#include "Internationalization/Regex.h"

#define LOCTEXT_NAMESPACE "FProjectCleanerModule"

ProjectCleanerManager::ProjectCleanerManager()
{
	AssetRegistry = &FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
}

void ProjectCleanerManager::Update()
{
	Clean();

	ProjectCleanerDataManagerV2::GetAllAssetsByPath(TEXT("/Game"),AllAssets);
	ProjectCleanerDataManagerV2::GetInvalidFilesByPath(FPaths::ProjectContentDir(), AllAssets, CorruptedAssets, NonEngineFiles);
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

void ProjectCleanerManager::Clean()
{
	AllAssets.Empty();
	CorruptedAssets.Empty();
	NonEngineFiles.Empty();
}

#undef LOCTEXT_NAMESPACE