// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#include "Core/ProjectCleanerManager.h"
#include "Core/ProjectCleanerDataManager.h"
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
	ExcludeOptions = GetMutableDefault<UExcludeOptions>();
	
	AssetRegistry = &FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

	ensure(CleanerConfigs);
	ensure(AssetRegistry);
	ensure(ExcludeOptions);
	// ensure(DirectoryWatcher);
}

void ProjectCleanerManager::Update()
{
	Clean();

	ProjectCleanerDataManagerV2::GetAllAssetsByPath(TEXT("/Game"),AllAssets);
	ProjectCleanerDataManagerV2::GetInvalidFilesByPath(FPaths::ProjectContentDir(), AllAssets, CorruptedAssets, NonEngineFiles);
	ProjectCleanerDataManagerV2::GetIndirectAssetsByPath(FPaths::ProjectDir(), IndirectAssets, AllAssets);
	ProjectCleanerDataManagerV2::GetEmptyFolders(FPaths::ProjectContentDir(), EmptyFolders, CleanerConfigs->bScanDeveloperContents);
	ProjectCleanerDataManagerV2::GetPrimaryAssetClasses(PrimaryAssetClasses);

	// now getting all unused assets in project
	TSet<FName> AllAssetsUsedByPrimaryAssets;
	AllAssetsUsedByPrimaryAssets.Reserve(AllAssets.Num());

	{
		FARFilter Filter;
		Filter.ClassNames.Append(PrimaryAssetClasses.Array());
		Filter.PackagePaths.Add(TEXT("/Game"));
		Filter.bRecursivePaths = true;
		Filter.bRecursiveClasses = true;

		TArray<FName> PackageNamesToProcess;
		{
			TArray<FAssetData> FoundAssets;
			AssetRegistry->Get().GetAssets(Filter, FoundAssets);
			for (const FAssetData& AssetData : FoundAssets)
			{
				PackageNamesToProcess.Add(AssetData.PackageName);
				AllAssetsUsedByPrimaryAssets.Add(AssetData.PackageName);
			}
		}

		TArray<FAssetIdentifier> AssetDependencies;
		while (PackageNamesToProcess.Num() > 0)
		{
			const FName PackageName = PackageNamesToProcess.Pop(false);
			AssetDependencies.Reset();
			AssetRegistry->Get().GetDependencies(FAssetIdentifier(PackageName), AssetDependencies);
			for (const FAssetIdentifier& Dependency : AssetDependencies)
			{
				bool bIsAlreadyInSet = false;
				if (!Dependency.PackageName.ToString().StartsWith(TEXT("/Game"))) continue;
		
				AllAssetsUsedByPrimaryAssets.Add(Dependency.PackageName, &bIsAlreadyInSet);
				if (!bIsAlreadyInSet && Dependency.IsValid())
				{
					PackageNamesToProcess.Add(Dependency.PackageName);
				}
			}
		}
	}

	// filling all assets that must be filtered
	TSet<FName> FilteredAssets;
	FilteredAssets.Reserve(AllAssets.Num());

	for (const auto& IndirectAsset : IndirectAssets)
	{
		FilteredAssets.Add(IndirectAsset.Key);
	}

	for (const auto& UserExcludedAsset : UserExcludedAssets)
	{
		FilteredAssets.Add(UserExcludedAsset.PackageName);
	}

	for (const auto& Asset : AllAssets)
	{
		if (ProjectCleanerDataManagerV2::HasExternalReferencersInPath(Asset.PackageName, TEXT("/Game")))
		{
			AssetsWithExternalRefs.AddUnique(Asset);
			FilteredAssets.Add(Asset.PackageName);
		}
		
		if (
			ProjectCleanerDataManagerV2::ExcludedByPath(Asset.PackagePath, ExcludeOptions) ||
			ProjectCleanerDataManagerV2::ExcludedByClass(Asset, ExcludeOptions)
		)
		{
			FilteredAssets.Add(Asset.PackageName);
		}
	}

	// for all filtered assets we must find all linked assets
	AssetsRelationalMap.Reserve(FilteredAssets.Num());
	for (const auto& FilteredAsset : FilteredAssets)
	{
		FLinkedAsset LinkedAsset;
		ProjectCleanerDataManagerV2::GetLinkedAssets(FilteredAsset, LinkedAsset.PackageNames);
		AssetsRelationalMap.Add(FilteredAsset, LinkedAsset);
	}

	// all assets that are not used by primary and not in filtered assets and their dependencies , goes as unused asset
	for (const auto& Asset : AllAssets)
	{
		if (AllAssetsUsedByPrimaryAssets.Contains(Asset.PackageName)) continue;
	
		bool IsUsedAsset = false;
		for (const auto& LinkedAsset : AssetsRelationalMap)
		{
			if (LinkedAsset.Value.PackageNames.Contains(Asset.PackageName))
			{
				IsUsedAsset = true;
				break;
			}
		}

		if (!IsUsedAsset)
		{
			UnusedAssets.AddUnique(Asset);
		}
	}

	TotalProjectSize = ProjectCleanerUtility::GetTotalSize(AllAssets);
	TotalUnusedAssetsSize = ProjectCleanerUtility::GetTotalSize(UnusedAssets);
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

const TSet<FName>& ProjectCleanerManager::GetPrimaryAssetClasses() const
{
	return PrimaryAssetClasses;
}

void ProjectCleanerManager::Clean()
{
	AssetsWithExternalRefs.Empty();
	// AllAssets.Empty();
	// CorruptedAssets.Empty();
	// NonEngineFiles.Empty();
	// IndirectAssets.Empty();
	// EmptyFolders.Em
}

#undef LOCTEXT_NAMESPACE