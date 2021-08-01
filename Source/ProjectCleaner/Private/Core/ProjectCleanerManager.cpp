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
#include "Engine/MapBuildDataRegistry.h"

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
			SNotificationItem::CS_Fail,
			2.0f
		);
		return;
	}

	ProjectCleanerUtility::FixupRedirectors();
	ProjectCleanerUtility::SaveAllAssets(true);

	ProjectCleanerDataManagerV2::GetAllAssetsByPath(TEXT("/Game"),AllAssets);
	ProjectCleanerDataManagerV2::GetInvalidFilesByPath(FPaths::ProjectContentDir(), AllAssets, CorruptedAssets, NonEngineFiles);
	ProjectCleanerDataManagerV2::GetIndirectAssetsByPath(FPaths::ProjectDir(), IndirectAssets, AllAssets);
	ProjectCleanerDataManagerV2::GetEmptyFolders(FPaths::ProjectContentDir(), EmptyFolders, CleanerConfigs->bScanDeveloperContents);
	ProjectCleanerDataManagerV2::GetPrimaryAssetClasses(PrimaryAssetClasses);
	ProjectCleanerDataManagerV2::GetAllAssetsWithExternalReferencers(AssetsWithExternalRefs, AllAssets);

	FindUnusedAssets();
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

void ProjectCleanerManager::AddToExcludeClasses(const TArray<FAssetData>& ExcludedByTypeAssets)
{
	if (ExcludedByTypeAssets.Num() == 0) return;

	for (const auto& Asset : ExcludedByTypeAssets)
	{
		if (Asset.AssetClass.IsEqual(TEXT("Blueprint")))
		{
			const auto LoadedAsset = Asset.GetAsset();
			if (!LoadedAsset) continue;
			
			const auto BlueprintAsset = Cast<UBlueprint>(LoadedAsset);
			if (BlueprintAsset && BlueprintAsset->GeneratedClass)
			{
				ExcludeOptions->Classes.AddUnique(BlueprintAsset->GeneratedClass);
			}
		}
		else
		{
			ExcludeOptions->Classes.AddUnique(Asset.GetClass());
		}
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

const TMap<FAssetData, FIndirectAsset>& ProjectCleanerManager::GetIndirectAssets() const
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

const TMap<FName, FExcludedAsset>& ProjectCleanerManager::GetExcludedAssets() const
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

void ProjectCleanerManager::Clean()
{
	AssetsInfoMap.Empty();
	UnusedAssets.Empty();
	// AssetsWithExternalRefs.Empty();
	// ExcludedAssets.Empty();
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
	//  5. Is Under Developer folder ( if user selected not to scan those folders, by default it wont)
	//  6. Is Under MegascansPlugin Content (if user selected so, by default it is)
	//  7. Is excluded asset

	const double Start = FPlatformTime::Seconds();

	TArray<FAssetData> PrimaryAssets;
	{
		FARFilter Filter;
		Filter.bRecursiveClasses = true;
		Filter.bRecursivePaths = true;
		Filter.PackagePaths.Add(TEXT("/Game"));
		Filter.ClassNames.Append(PrimaryAssetClasses.Array());

		AssetRegistry->Get().GetAssets(Filter, PrimaryAssets);
	}

	TSet<FName> UsedAssets;
	for (const auto& Asset : PrimaryAssets)
	{
		UsedAssets.Add(Asset.PackageName);
	}

	for (const auto& Asset : IndirectAssets)
	{
		UsedAssets.Add(Asset.Key.PackageName);
	}

	for (const auto& Asset : AssetsWithExternalRefs)
	{
		UsedAssets.Add(Asset.PackageName);
	}

	if (!CleanerConfigs->bScanDeveloperContents)
	{
		TArray<FAssetData> AssetsInDeveloperFolder;
		AssetRegistry->Get().GetAssetsByPath(TEXT("/Game/Developers"), AssetsInDeveloperFolder, true);

		for (const auto& Asset : AssetsInDeveloperFolder)
		{
			UsedAssets.Add(Asset.PackageName);
		}
	}

	// excluded by user
	for (const auto& Asset : UserExcludedAssets)
	{
		UsedAssets.Add(Asset.PackageName);
	}

	// excluded by path
	{
		TArray<FAssetData> AllExcludedAssets;
	
		FARFilter Filter;		
		Filter.bRecursivePaths = true;
		for (const auto& ExcludedPath : ExcludeOptions->Paths)
		{
			Filter.PackagePaths.Add(FName{*ExcludedPath.Path});
		}

		AssetRegistry->Get().GetAssets(Filter, AllExcludedAssets);

		for (const auto& Asset : AllExcludedAssets)
		{
			UsedAssets.Add(Asset.PackageName);
		}
	}

	// excluded by class
	TSet<FName> ExcludedByClassAssets;
	for (const auto& Asset : AllAssets)
	{
		if (IsExcludedByClass(Asset))
		{
			ExcludedByClassAssets.Add(Asset.PackageName);
			UsedAssets.Add(Asset.PackageName);
		}
	}

	
	// 2. Used Assets dependencies
	TSet<FName> UsedAssetsDeps;
	TArray<FName> Stack;
	for (const auto& Asset : UsedAssets)
	{
		UsedAssetsDeps.Add(Asset);
		Stack.Add(Asset);

		TArray<FName> Deps;
		while (Stack.Num() > 0)
		{
			const auto CurrentPackageName = Stack.Pop(false);
			Deps.Reset();

			AssetRegistry->Get().GetDependencies(CurrentPackageName, Deps);

			Deps.RemoveAllSwap([] (const FName& Dep)
			{
				return !Dep.ToString().StartsWith(TEXT("/Game"));
			});

			for (const auto& Dep : Deps)
			{
				bool bIsAlreadyInSet = false;
				UsedAssetsDeps.Add(Dep, &bIsAlreadyInSet);
				if (!bIsAlreadyInSet)
				{
					Stack.Add(Dep);
				}
			}
		}
	}

	// unused assets
	const bool IsMegascansLoaded = FModuleManager::Get().IsModuleLoaded("MegascansPlugin");
	for (const auto& Asset : AllAssets)
	{
		if (UsedAssetsDeps.Contains(Asset.PackageName)) continue;
		if (IsMegascansLoaded && !CleanerConfigs->bScanMegascansContent && IsUnderMegascansFolder(Asset)) continue;
		UnusedAssets.Add(Asset);
	}
	
	
	const double End = FPlatformTime::Seconds() - Start;

	UE_LOG(LogTemp, Warning, TEXT("%f"), End);
}

void ProjectCleanerManager::FindExcludedAssets()
{
	// ExcludedAssets.Empty();
	//
	// // excluded by user
	// for (const auto& Asset : UserExcludedAssets)
	// {
	// 	FExcludedAsset ExcludedAsset;
	// 	ProjectCleanerDataManagerV2::GetAllRefsAndDeps(Asset.PackageName, ExcludedAsset.PackageNames);
	// 	ExcludedAssets.Add(Asset.PackageName, ExcludedAsset);
	// }
	//
	// // excluded by path
	// TArray<FAssetData> TempContainer;
	// for (const auto& ExcludedPath : ExcludeOptions->Paths)
	// {
	// 	AssetRegistry->Get().GetAssetsByPath(FName{*ExcludedPath.Path}, TempContainer, true);
	//
	// 	for (const auto& Asset : TempContainer)
	// 	{
	// 		FExcludedAsset ExcludedAsset;
	// 		ProjectCleanerDataManagerV2::GetAllRefsAndDeps(Asset.PackageName, ExcludedAsset.PackageNames);
	// 		ExcludedAssets.Add(Asset.PackageName, ExcludedAsset);
	// 	}
	//
	// 	TempContainer.Reset();
	// }
	//
	// TempContainer.Empty();
	//
	// // excluded by class
	// for (const auto& ExcludedClass : ExcludeOptions->Classes)
	// {
	// 	if (!ExcludedClass) continue;
	// 	AssetRegistry->Get().GetAssetsByClass(ExcludedClass->GetFName(), TempContainer);
	//
	// 	TempContainer.RemoveAllSwap([] (const FAssetData& Asset)
	// 	{
	// 		return !Asset.PackagePath.ToString().StartsWith("/Game");
	// 	}, false);
	//
	// 	for (const auto& Asset : TempContainer)
	// 	{
	// 		FExcludedAsset ExcludedAsset;
	// 		ProjectCleanerDataManagerV2::GetAllRefsAndDeps(Asset.PackageName, ExcludedAsset.PackageNames);
	// 		ExcludedAssets.Add(Asset.PackageName, ExcludedAsset);
	// 	}
	// 	
	// 	TempContainer.Reset();
	// }
}

bool ProjectCleanerManager::IsPrimaryAsset(const FAssetData& AssetData) const
{
	return PrimaryAssetClasses.Contains(ProjectCleanerDataManagerV2::GetClassName(AssetData));
}

bool ProjectCleanerManager::IsExcludedByClass(const FAssetData& AssetData) const
{
	const FName ClassName = ProjectCleanerDataManagerV2::GetClassName(AssetData);

	TSet<FName> ExcludedClassNames;
	for (const auto& ExcludedClass : ExcludeOptions->Classes)
	{
		if (!ExcludedClass) continue;
		ExcludedClassNames.Add(ExcludedClass->GetFName());
	}
	
	return ExcludedClassNames.Contains(ClassName);
}

bool ProjectCleanerManager::IsUnderDevelopersFolder(const FAssetData& AssetData) const
{
	return AssetData.PackagePath.ToString().StartsWith(TEXT("/Game/Developers"));
}

bool ProjectCleanerManager::IsUnderMegascansFolder(const FAssetData& AssetData) const
{
	return AssetData.PackagePath.ToString().StartsWith(TEXT("/Game/MSPresets"));
}

#undef LOCTEXT_NAMESPACE
