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
	if (AssetRegistry->Get().IsLoadingAssets())
	{
		ProjectCleanerNotificationManager::AddTransient(
			FText::FromString(FStandardCleanerText::AssetRegistryStillWorking),
			SNotificationItem::CS_Pending,
			3.0f
		);
		return;
	}

	ProjectCleanerUtility::FixupRedirectors();
	ProjectCleanerUtility::SaveAllAssets(true);
	
	Clean();

	ProjectCleanerDataManagerV2::GetAllAssetsByPath(TEXT("/Game"),AllAssets);
	ProjectCleanerDataManagerV2::GetInvalidFilesByPath(FPaths::ProjectContentDir(), AllAssets, CorruptedAssets, NonEngineFiles);
	ProjectCleanerDataManagerV2::GetIndirectAssetsByPath(FPaths::ProjectDir(), IndirectAssets, AllAssets);
	ProjectCleanerDataManagerV2::GetEmptyFolders(FPaths::ProjectContentDir(), EmptyFolders, CleanerConfigs->bScanDeveloperContents);
	ProjectCleanerDataManagerV2::GetPrimaryAssetClasses(PrimaryAssetClasses);
	
	FindUnusedAssets();
	
	TotalProjectSize = ProjectCleanerUtility::GetTotalSize(AllAssets);
	TotalUnusedAssetsSize = ProjectCleanerUtility::GetTotalSize(UnusedAssets);
}

void ProjectCleanerManager::AddToUserExcludedAssets(const TArray<FAssetData>& NewUserExcludedAssets)
{
	if (NewUserExcludedAssets.Num() == 0) return;

	for (const auto& UserExcludedAsset : NewUserExcludedAssets)
	{
		UserExcludedAssets.AddUnique(UserExcludedAsset);
	}

	FindUnusedAssets();
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

const TArray<FAssetData>& ProjectCleanerManager::GetUnusedAssets() const
{
	return UnusedAssets;
}

const TMap<FName, FLinkedAssets>& ProjectCleanerManager::GetExcludedAssets() const
{
	return ExcludedAssets;
}

UCleanerConfigs* ProjectCleanerManager::GetCleanerConfigs() const
{
	return CleanerConfigs;
}

UExcludeOptions* ProjectCleanerManager::GetExcludeOptions() const
{
	return ExcludeOptions;
}

int64 ProjectCleanerManager::GetTotalProjectSize() const
{
	return TotalProjectSize;
}

int64 ProjectCleanerManager::GetTotalUnusedAssetsSize() const
{
	return TotalUnusedAssetsSize;
}

void ProjectCleanerManager::Clean()
{
	AssetsWithExternalRefs.Empty();
	UnusedAssets.Empty();
	ExcludedAssets.Empty();
}

void ProjectCleanerManager::FindUnusedAssets()
{
	Clean();
	
	// 1) Getting all used assets
	//  We assume asset to be used, if one of those criteria apply to it
	//  Asset
	//  1. Is Primary asset
	//  2. Is Dependency of primary asset
	//  3. Indirectly used (in source code or in config files)
	//  4. Has External Referencers outside "/Game" folder
	//  5. Is Under MegascansPlugin Content (if user selected so, by default it is)
	//  6. Is excluded asset
	
	TSet<FName> UsedAssets;
	UsedAssets.Reserve(AllAssets.Num());

	{
		// Primary assets
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
				UsedAssets.Add(AssetData.PackageName);
			}
		}

		// Primary assets dependencies
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
		
				UsedAssets.Add(Dependency.PackageName, &bIsAlreadyInSet);
				if (!bIsAlreadyInSet && Dependency.IsValid())
				{
					PackageNamesToProcess.Add(Dependency.PackageName);
				}
			}
		}

		// indirect assets and their linked assets
		for (const auto& IndirectAsset : IndirectAssets)
		{
			UsedAssets.Add(IndirectAsset.Key);
			TSet<FName> LinkedAssetsOfIndirectAsset;
			ProjectCleanerDataManagerV2::GetLinkedAssets(IndirectAsset.Key, LinkedAssetsOfIndirectAsset);
			UsedAssets.Append(LinkedAssetsOfIndirectAsset);
		}

		const bool IsMegascansLoaded = FModuleManager::Get().IsModuleLoaded("MegascansPlugin");		
		for (const auto& Asset : AllAssets)
		{
			// Assets with external referencers
			if (ProjectCleanerDataManagerV2::HasExternalReferencersInPath(Asset.PackageName, TEXT("/Game")))
			{
				AssetsWithExternalRefs.AddUnique(Asset);

				TSet<FName> LinkedAssetsOfExternalAsset;
				ProjectCleanerDataManagerV2::GetLinkedAssets(Asset.PackageName, LinkedAssetsOfExternalAsset);
				UsedAssets.Append(LinkedAssetsOfExternalAsset);
			}
			
			// MegascansContent
			if (CleanerConfigs->bExcludeMegascansPluginIfActive && IsMegascansLoaded)
			{
				if (FPaths::IsUnderDirectory(Asset.PackagePath.ToString(), TEXT("/Game/MSPresets")))
				{
					UsedAssets.Add(Asset.PackageName);
				}
			}

			// Excluded assets
			if (
				ProjectCleanerDataManagerV2::ExcludedByPath(Asset.PackagePath, ExcludeOptions) ||
				ProjectCleanerDataManagerV2::ExcludedByClass(Asset, ExcludeOptions)
			)
			{
				FLinkedAssets LinkedAssets;
				ProjectCleanerDataManagerV2::GetLinkedAssets(Asset.PackageName, LinkedAssets.PackageNames);
				ExcludedAssets.Add(Asset.PackageName, LinkedAssets);
				
				UsedAssets.Add(Asset.PackageName);
				UsedAssets.Append(LinkedAssets.PackageNames);
			}
		}
	}

	// also including User excluded assets
	for (const auto& UserExcludedAsset : UserExcludedAssets)
	{
		FLinkedAssets LinkedAssets;
		ProjectCleanerDataManagerV2::GetLinkedAssets(UserExcludedAsset.PackageName, LinkedAssets.PackageNames);
		ExcludedAssets.Add(UserExcludedAsset.PackageName, LinkedAssets);
		UsedAssets.Add(UserExcludedAsset.PackageName);
		UsedAssets.Append(LinkedAssets.PackageNames);
	}

	// Filtering used assets from all assets we got unused assets
	for (const auto& Asset : AllAssets)
	{
		if (UsedAssets.Contains(Asset.PackageName)) continue;
		UnusedAssets.AddUnique(Asset);
	}
}

#undef LOCTEXT_NAMESPACE
