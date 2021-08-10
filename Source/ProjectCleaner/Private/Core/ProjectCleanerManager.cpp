// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#include "Core/ProjectCleanerManager.h"
#include "Core/ProjectCleanerDataManager.h"
#include "Core/ProjectCleanerUtility.h"
#include "StructsContainer.h"
#include "UI/ProjectCleanerNotificationManager.h"
// Engine Headers
#include "AssetRegistryModule.h"
#include "Misc/ScopedSlowTask.h"
#include "Settings/ContentBrowserSettings.h"
#include "IDirectoryWatcher.h"
#include "ObjectTools.h"
#include "Misc/FileHelper.h"
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
	ensure(ExcludeOptions);
	ensure(AssetRegistry);
}

ProjectCleanerManager::~ProjectCleanerManager()
{
	AssetRegistry = nullptr;
}

void ProjectCleanerManager::Update()
{
	FScopedSlowTask SlowTask(
		3.0f,
		FText::FromString(FStandardCleanerText::Scanning)
	);
	SlowTask.MakeDialog();
	
	ProjectCleanerUtility::FixupRedirectors();
	ProjectCleanerUtility::SaveAllAssets(true);

	SlowTask.EnterProgressFrame();
	
	LoadData();
	
	SlowTask.EnterProgressFrame();
	
	FindUnusedAssets();

	SlowTask.EnterProgressFrame();

	// Broadcast to all bounded objects that data is updated
	if (OnCleanerManagerUpdated.IsBound())
	{
		OnCleanerManagerUpdated.Execute();
	}
}

void ProjectCleanerManager::ExcludeSelectedAssets(const TArray<FAssetData>& NewUserExcludedAssets)
{
	if (NewUserExcludedAssets.Num() == 0) return;

	for (const auto& UserExcludedAsset : NewUserExcludedAssets)
	{
		UserExcludedAssets.AddUnique(UserExcludedAsset);
	}

	Update();
}

void ProjectCleanerManager::ExcludeSelectedAssetsByType(const TArray<FAssetData>& ExcludedByTypeAssets)
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

	Update();
}

void ProjectCleanerManager::ExcludePath(const FString& InPath)
{
	if (InPath.IsEmpty()) return;

	FDirectoryPath DirectoryPath;
	DirectoryPath.Path = InPath;

	const bool bAlreadyExcluded = ExcludeOptions->Paths.ContainsByPredicate([&] (const FDirectoryPath& DirPath)
	{
		return DirPath.Path.Equals(InPath); 
	});
	
	if (!bAlreadyExcluded)
	{
		ExcludeOptions->Paths.Add(DirectoryPath);
	}

	Update();
}

void ProjectCleanerManager::IncludePath(const FString& InPath)
{
	if (InPath.IsEmpty()) return;

	ExcludeOptions->Paths.RemoveAll([&] (const FDirectoryPath& DirPath)
	{
		return DirPath.Path.Equals(InPath);
	});

	Update();
}

void ProjectCleanerManager::IncludeSelectedAssets(const TArray<FAssetData>& Assets)
{
	UserExcludedAssets.RemoveAllSwap([&] (const FAssetData& Asset)
	{
		return Assets.Contains(Asset);
	}, false);
	UserExcludedAssets.Shrink();

	bool bHasConflictWithFilters = false;
	for (const auto& Asset : Assets)
	{
		if (IsExcludedByClass(Asset) || IsExcludedByPath(Asset))
		{
			bHasConflictWithFilters = true;
		}
	}

	if (bHasConflictWithFilters)
	{
		ProjectCleanerNotificationManager::AddTransient(
			FText::FromString(FStandardCleanerText::CantIncludeSomeAssets),
			SNotificationItem::CS_Fail,
			10.0f
		);
	}
	
	Update();
}

void ProjectCleanerManager::DeleteSelectedAssets(const TArray<FAssetData>& Assets)
{
	if (Assets.Num() == 0) return;

	const int32 DeletedAssetsNum = ObjectTools::DeleteAssets(Assets);
	if (DeletedAssetsNum == 0) return;

	if (DeletedAssetsNum != Assets.Num())
	{
		ProjectCleanerNotificationManager::AddTransient(
			FText::FromString(FStandardCleanerText::FailedToDeleteSomeAssets),
			SNotificationItem::CS_Fail,
			3.0f
		);
	}

	Update();
}

void ProjectCleanerManager::DeleteAllUnusedAssets()
{
	Update();
	
	int32 DeletedAssetsNum = 0;
	const int32 Total = UnusedAssets.Num();
	
	TWeakPtr<SNotificationItem> DeletionProgressNotification;
	ProjectCleanerNotificationManager::Add(
		GetDeletionProgressText(DeletedAssetsNum, Total),
		SNotificationItem::CS_Pending,
		DeletionProgressNotification
	);
	
	TArray<FAssetData> DeletionChunk;
	FScopedSlowTask DeleteSlowTask(
		UnusedAssets.Num(),
		FText::FromString(TEXT("Deleting..."))
	);
	DeleteSlowTask.MakeDialog();
	while (UnusedAssets.Num() > 0)
	{
		GetDeletionChunk(DeletionChunk);
		DeleteSlowTask.EnterProgressFrame(DeletionChunk.Num());

		DeletedAssetsNum += ProjectCleanerUtility::DeleteAssets(DeletionChunk, CleanerConfigs->bForceDeleteAssets);
		ProjectCleanerNotificationManager::Update(
			DeletionProgressNotification,
			GetDeletionProgressText(DeletedAssetsNum, Total)
		);
	
		UnusedAssets.RemoveAllSwap([&] (const FAssetData& Asset)
		{
			return DeletionChunk.Contains(Asset); 
		}, false);
		
		DeletionChunk.Reset();
	}

	if (Total != DeletedAssetsNum)
	{
		ProjectCleanerNotificationManager::Hide(
			DeletionProgressNotification,
			SNotificationItem::CS_Fail,
			FText::FromString(FStandardCleanerText::FailedToDeleteSomeAssets)
		);
	}
	else
	{
		ProjectCleanerNotificationManager::Hide(
			DeletionProgressNotification,
			SNotificationItem::CS_Success,
			FText::FromString(FStandardCleanerText::AssetsSuccessfullyDeleted)
		);
	}

	// Cleaning empty packages
	const TSet<FName> EmptyPackages = AssetRegistry->Get().GetCachedEmptyPackages();
	TArray<UPackage*> AssetPackages;
	for (const auto& EmptyPackage : EmptyPackages)
	{
		UPackage* Package = FindPackage(nullptr, *EmptyPackage.ToString());
		if (Package && Package->IsValidLowLevel())
		{
			AssetPackages.Add(Package);
		}
	}
	
	if (AssetPackages.Num() > 0)
	{
		ObjectTools::CleanupAfterSuccessfulDelete(AssetPackages);
	}
	
	ProjectCleanerUtility::UpdateAssetRegistry(true);
	
	if (CleanerConfigs->bAutomaticallyDeleteEmptyFolders)
	{
		DeleteAllEmptyFolders();
	}

	Update();
	ProjectCleanerUtility::FocusOnGameFolder();
}

void ProjectCleanerManager::DeleteAllEmptyFolders()
{
	ProjectCleanerDataManager::GetEmptyFolders(FPaths::ProjectContentDir(), EmptyFolders, CleanerConfigs->bScanDeveloperContents);
	if (EmptyFolders.Num() == 0) return;
	
	if (!ProjectCleanerUtility::DeleteEmptyFolders(EmptyFolders))
	{
		ProjectCleanerNotificationManager::AddTransient(
			FText::FromString(FStandardCleanerText::FailedToDeleteSomeFolders),
			SNotificationItem::CS_Fail,
			5.0f
		);
	}

	Update();
	ProjectCleanerUtility::FocusOnGameFolder();
}

void ProjectCleanerManager::IncludeAllAssets()
{
	ExcludeOptions->Classes.Empty();
	ExcludeOptions->Paths.Empty();
	ExcludedAssets.Empty();
	UserExcludedAssets.Empty();

	Update();
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

const TSet<FName>& ProjectCleanerManager::GetExcludedAssets() const
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

float ProjectCleanerManager::GetUnusedAssetsPercent() const
{
	if (AllAssets.Num() == 0) return 0.0f;

	return UnusedAssets.Num() * 100.0f / AllAssets.Num();
}

const FAssetRegistryModule* ProjectCleanerManager::GetAssetRegistry() const
{
	return AssetRegistry;
}

void ProjectCleanerManager::LoadData()
{
	ProjectCleanerDataManager::GetAllAssetsByPath(TEXT("/Game"),AllAssets);
	ProjectCleanerDataManager::GetInvalidFilesByPath(FPaths::ProjectContentDir(), AllAssets, CorruptedAssets, NonEngineFiles);
	ProjectCleanerDataManager::GetIndirectAssetsByPath(FPaths::ProjectDir(), IndirectAssets, AllAssets);
	ProjectCleanerDataManager::GetEmptyFolders(FPaths::ProjectContentDir(), EmptyFolders, CleanerConfigs->bScanDeveloperContents);
	ProjectCleanerDataManager::GetPrimaryAssetClasses(PrimaryAssetClasses);
	ProjectCleanerDataManager::GetAllAssetsWithExternalReferencers(AssetsWithExternalRefs, AllAssets);
}

void ProjectCleanerManager::FindUnusedAssets()
{
	UnusedAssets.Empty();
	ExcludedAssets.Empty();
	PrimaryAssets.Empty();
	
	FScopedSlowTask SlowTask(
		3.0f,
		FText::FromString(FStandardCleanerText::SearchingForUnusedAssets)
	);
	SlowTask.MakeDialog();
	
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

	// Used assets
	TSet<FName> UsedAssets;
	FindUsedAssets(UsedAssets);

	// excluded by user
	for (const auto& Asset : UserExcludedAssets)
	{
		UsedAssets.Add(Asset.PackageName);

		if (!PrimaryAssets.Contains(Asset))
		{
			ExcludedAssets.Add(Asset.PackageName);
		}
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
			if (!PrimaryAssets.Contains(Asset))
			{
				ExcludedAssets.Add(Asset.PackageName);
			}
		}
	}

	// excluded by class
	for (const auto& Asset : AllAssets)
	{
		if (IsExcludedByClass(Asset))
		{
			UsedAssets.Add(Asset.PackageName);
			if (!PrimaryAssets.Contains(Asset))
			{
				ExcludedAssets.Add(Asset.PackageName);
			}
		}
	}

	SlowTask.EnterProgressFrame();
	
	// Used Assets dependencies
	TSet<FName> UsedAssetsDeps;
	UsedAssets.Reserve(AllAssets.Num());
	UsedAssetsDeps.Reserve(AllAssets.Num());
	FindUsedAssetsDependencies(UsedAssets, UsedAssetsDeps);
	
	SlowTask.EnterProgressFrame();

	// Unused assets
	UnusedAssets.Reserve(AllAssets.Num());
	
	const bool IsMegascansLoaded = FModuleManager::Get().IsModuleLoaded("MegascansPlugin");
	for (const auto& Asset : AllAssets)
	{
		if (UsedAssetsDeps.Contains(Asset.PackageName)) continue;
		if (PrimaryAssets.Contains(Asset)) continue;
		
		if (
			IsMegascansLoaded &&
			!CleanerConfigs->bScanMegascansContent &&
			ProjectCleanerDataManager::IsUnderMegascansFolder(Asset)
		) continue;
		
		UnusedAssets.Add(Asset);
	}
	UnusedAssets.Shrink();

	SlowTask.EnterProgressFrame();
}

void ProjectCleanerManager::FindUsedAssets(TSet<FName>& UsedAssets)
{
	FARFilter Filter;
	Filter.bRecursiveClasses = true;
	Filter.bRecursivePaths = true;
	Filter.PackagePaths.Add(TEXT("/Game"));
	Filter.ClassNames.Append(PrimaryAssetClasses.Array());
	Filter.ClassNames.Add(UMapBuildDataRegistry::StaticClass()->GetFName());

	AssetRegistry->Get().GetAssets(Filter, PrimaryAssets);

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
}

void ProjectCleanerManager::FindUsedAssetsDependencies(const TSet<FName>& UsedAssets, TSet<FName>& UsedAssetsDeps) const
{
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
			}, false);

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
}

bool ProjectCleanerManager::IsExcludedByClass(const FAssetData& AssetData) const
{
	const FName ClassName = ProjectCleanerDataManager::GetClassName(AssetData);

	TSet<FName> ExcludedClassNames;
	for (const auto& ExcludedClass : ExcludeOptions->Classes)
	{
		if (!ExcludedClass) continue;
		ExcludedClassNames.Add(ExcludedClass->GetFName());
	}
	
	return ExcludedClassNames.Contains(ClassName);
}

bool ProjectCleanerManager::IsExcludedByPath(const FAssetData& AssetData) const
{
	return ExcludeOptions->Paths.ContainsByPredicate([&](const FDirectoryPath& DirPath)
	{
		return AssetData.PackagePath.ToString().StartsWith(DirPath.Path);
	});
}

void ProjectCleanerManager::GetDeletionChunk(TArray<FAssetData>& Chunk)
{
	const int32 DeleteChunkLimit = 100;
	// 1. search for root assets
	for (const auto& Asset : UnusedAssets)
	{
		if (Chunk.Num() >= DeleteChunkLimit) break;
		
		if (ProjectCleanerDataManager::IsRootAsset(Asset))
		{
			Chunk.AddUnique(Asset);
		}
	}

	if (Chunk.Num() > 0)
	{
		return;
	}

	// 2. If root assets not detected, then searching for circular assets
	for (const auto& Asset : UnusedAssets)
	{
		TArray<FName> Refs;
		TArray<FName> Deps;
		TArray<FName> CommonAssets;
		if (ProjectCleanerDataManager::IsCircularAsset(Asset, Refs, Deps, CommonAssets))
		{
			if (Refs.Num() == 1)
			{
				Chunk.AddUnique(Asset);
	
				for (const auto& CommonAsset : CommonAssets)
				{
					const auto& AssetObj = UnusedAssets.FindByPredicate([&](const FAssetData& Elem)
					{
						return Elem.PackageName.IsEqual(CommonAsset);
					});
	
					if (AssetObj)
					{
						Chunk.AddUnique(*AssetObj);
					}
				}
				break;
			}
		}
	}

	if (Chunk.Num() > 0)
	{
		return;
	}

	// 3. If we did not detected circular or root assets, just deleting remaining
	for (const auto& Asset : UnusedAssets)
	{
		if (Chunk.Num() >= DeleteChunkLimit) break;
		Chunk.AddUnique(Asset);
	}
}

FText ProjectCleanerManager::GetDeletionProgressText(const int32 DeletedAssetNum, const int32 Total) const
{
	const int32 Percent = Total > 0 ? (DeletedAssetNum * 100.0f) / Total : 0;
	return FText::FromString(
		FString::Printf(
		TEXT("Deleted %d of %d assets. %d %%"),
			DeletedAssetNum,
			Total,
			Percent
		)
	);
}

#undef LOCTEXT_NAMESPACE
