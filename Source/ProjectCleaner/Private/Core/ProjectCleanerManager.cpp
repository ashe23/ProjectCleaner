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
#include "ObjectTools.h"
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
	DirectoryWatcher = &FModuleManager::LoadModuleChecked<FDirectoryWatcherModule>(TEXT("DirectoryWatcher"));

	ensure(CleanerConfigs);
	ensure(AssetRegistry);
	ensure(ExcludeOptions);
	ensure(DirectoryWatcher);
}

ProjectCleanerManager::~ProjectCleanerManager()
{
	UnRegisterAssetRegistryDelegates();
	UnRegisterDirectoryWatcherDelegates();

	AssetRegistry = nullptr;
	DirectoryWatcher = nullptr;
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

	if (!bIsActualData)
	{
		LoadInitialData();

		if (!bDelegatesRegistered)
		{
			// Registering asset registry delegates
			RegisterAssetRegistryDelegates();

			// Registering directory watcher delegates
			RegisterDirectoryWatcherDelegates();
			
			bDelegatesRegistered = true;
		}
	}

	FindUnusedAssets();

	// Broadcast to all bounded object that data is updated
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

void ProjectCleanerManager::ExcludedSelectedAssetsByType(const TArray<FAssetData>& ExcludedByTypeAssets)
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

void ProjectCleanerManager::IncludeSelectedAssets(const TArray<FAssetData>& Assets)
{
	UserExcludedAssets.RemoveAllSwap([&] (const FAssetData& Asset)
	{
		return Assets.Contains(Asset);
	}, false);
	UserExcludedAssets.Shrink();

	// if asset is excluded by path or class show notification about conflict
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
			5.0f
		);
	}
	// todo:ashe23 update only when something changed
	Update();
}

void ProjectCleanerManager::DeleteSelectedAssets(const TArray<FAssetData>& Assets)
{
	if (Assets.Num() == 0) return;

	const int32 DeletedAssetsNum = ObjectTools::DeleteAssets(Assets);
	if (DeletedAssetsNum == 0) return;

	bIsActualData = false;

	Update();
}

void ProjectCleanerManager::DeleteAllUnusedAssets()
{
	int32 DeletedAssetsNum = 0;
	const int32 Total = UnusedAssets.Num();
	
	TWeakPtr<SNotificationItem> DeletionProgressNotification;
	ProjectCleanerNotificationManager::Add(
		GetDeletionProgressText(DeletedAssetsNum, Total),
		SNotificationItem::CS_Pending,
		DeletionProgressNotification
	);
	
	TArray<FAssetData> DeletionChunk;
	while (UnusedAssets.Num() > 0)
	{
		GetDeletionChunk(DeletionChunk);
		
		DeletedAssetsNum += ProjectCleanerUtility::DeleteAssets(DeletionChunk);
		ProjectCleanerNotificationManager::Update(DeletionProgressNotification, GetDeletionProgressText(DeletedAssetsNum, Total));
	
		UnusedAssets.RemoveAllSwap([&] (const FAssetData& Asset)
		{
			return DeletionChunk.Contains(Asset); 
		}, false);
		
		DeletionChunk.Reset();
	}
	
	ProjectCleanerNotificationManager::Hide(DeletionProgressNotification, FText::FromString(TEXT("Assets deleted successfully")));

	if (CleanerConfigs->bAutomaticallyDeleteEmptyFolders)
	{
		// After assets deleted we should update empty folder data
		ProjectCleanerDataManager::GetEmptyFolders(FPaths::ProjectContentDir(), EmptyFolders, CleanerConfigs->bScanDeveloperContents);
		if (!ProjectCleanerUtility::DeleteEmptyFolders(EmptyFolders))
		{
			ProjectCleanerNotificationManager::AddTransient(
				FText::FromString(FStandardCleanerText::FailedToDeleteSomeFolders),
				SNotificationItem::CS_Fail,
				5.0f
			);
		}
	}

	Update();
}

void ProjectCleanerManager::DeleteAllEmptyFolders()
{
	if (!ProjectCleanerUtility::DeleteEmptyFolders(EmptyFolders))
	{
		ProjectCleanerNotificationManager::AddTransient(
			FText::FromString(FStandardCleanerText::FailedToDeleteSomeFolders),
			SNotificationItem::CS_Fail,
			5.0f
		);
	}

	Update();
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

void ProjectCleanerManager::LoadInitialData()
{
	FScopedSlowTask InitialDataTask{1.0f, FText::FromString(FStandardCleanerText::LoadingInitialData)};
	InitialDataTask.MakeDialog();
	
	ProjectCleanerDataManager::GetAllAssetsByPath(TEXT("/Game"),AllAssets);
	ProjectCleanerDataManager::GetInvalidFilesByPath(FPaths::ProjectContentDir(), AllAssets, CorruptedAssets, NonEngineFiles);
	ProjectCleanerDataManager::GetIndirectAssetsByPath(FPaths::ProjectDir(), IndirectAssets, AllAssets);
	ProjectCleanerDataManager::GetEmptyFolders(FPaths::ProjectContentDir(), EmptyFolders, CleanerConfigs->bScanDeveloperContents);
	ProjectCleanerDataManager::GetPrimaryAssetClasses(PrimaryAssetClasses);
	ProjectCleanerDataManager::GetAllAssetsWithExternalReferencers(AssetsWithExternalRefs, AllAssets);

	InitialDataTask.EnterProgressFrame();

	bIsActualData = true;
}

void ProjectCleanerManager::FindUnusedAssets()
{
	UnusedAssets.Empty();
	ExcludedAssets.Empty();
	
	FScopedSlowTask SlowTask(3.0f, FText::FromString(FStandardCleanerText::SearchingForUnusedAssets));
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

	const double Start = FPlatformTime::Seconds();

	// 1. Used assets
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
		ExcludedAssets.Add(Asset.PackageName);
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
			ExcludedAssets.Add(Asset.PackageName);
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
			ExcludedAssets.Add(Asset.PackageName);
		}
	}

	SlowTask.EnterProgressFrame();
	
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
	
	SlowTask.EnterProgressFrame();

	// 3. Unused assets
	const bool IsMegascansLoaded = FModuleManager::Get().IsModuleLoaded("MegascansPlugin");
	for (const auto& Asset : AllAssets)
	{
		if (UsedAssetsDeps.Contains(Asset.PackageName)) continue;
		if (
			IsMegascansLoaded &&
			!CleanerConfigs->bScanMegascansContent &&
			ProjectCleanerDataManager::IsUnderMegascansFolder(Asset)
		) continue;
		
		UnusedAssets.Add(Asset);
	}
	
	const double End = FPlatformTime::Seconds() - Start;
	SlowTask.EnterProgressFrame();

	UE_LOG(LogTemp, Warning, TEXT("%f"), End);
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
	// 1. Searching for circular assets, this assets must be deleted at first
	for (const auto& Asset : UnusedAssets)
	{
		if (ProjectCleanerDataManager::IsCircularAsset(Asset))
		{
			Chunk.AddUnique(Asset);
		}
	}

	if (Chunk.Num() > 0)
	{
		return;
	}

	// 2. If circular assets not detected, then searching for root assets	
	for (const auto& Asset : UnusedAssets)
	{
		if (ProjectCleanerDataManager::IsRootAsset(Asset))
		{
			Chunk.Add(Asset);
		}
	}

	if (Chunk.Num() > 0)
	{
		return;
	}

	// 3. If we did not detected circular or root assets, just deleting remaining
	for (const auto& Asset : UnusedAssets)
	{
		Chunk.Add(Asset);
	}
}

FText ProjectCleanerManager::GetDeletionProgressText(const int32 DeletedAssetNum, const int32 Total) const
{
	const int32 Percent = Total > 0 ? (DeletedAssetNum * 100.0f) / Total : 0;
	return FText::FromString(
		FString::Printf(
		TEXT("Deleted %d of %d assets. %d %%"),
			DeletedAssetNum,
			AllAssets.Num(),
			Percent
		)
	);
}

void ProjectCleanerManager::OnAssetAdded(const FAssetData& AssetData)
{
	bIsActualData = false;
}

void ProjectCleanerManager::OnAssetRemoved(const FAssetData& AssetData)
{
	bIsActualData = false;
}

void ProjectCleanerManager::OnAssetUpdated(const FAssetData& AssetData)
{
	bIsActualData = false;
}

void ProjectCleanerManager::OnAssetRenamed(const FAssetData& AssetData, const FString& Name)
{
	bIsActualData = false;
}

void ProjectCleanerManager::RegisterDirectoryWatcherDelegates()
{
	DirectoryWatcher->Get()->RegisterDirectoryChangedCallback_Handle(
		FPaths::GameSourceDir(),
		IDirectoryWatcher::FDirectoryChanged::CreateRaw(
			this,
			&ProjectCleanerManager::OnDirectoryChanged
		),
		SourceDirDelegateHandle,
		0
	);
	DirectoryWatcher->Get()->RegisterDirectoryChangedCallback_Handle(
		FPaths::ProjectConfigDir(),
		IDirectoryWatcher::FDirectoryChanged::CreateRaw(
			this,
			&ProjectCleanerManager::OnDirectoryChanged
		),
		ConfigDirDelegateHandle,
		0
	);
	DirectoryWatcher->Get()->RegisterDirectoryChangedCallback_Handle(
		FPaths::ProjectPluginsDir(),
		IDirectoryWatcher::FDirectoryChanged::CreateRaw(
			this,
			&ProjectCleanerManager::OnDirectoryChanged
		),
		PluginsDirDelegateHandle,
		0
	);
}

void ProjectCleanerManager::UnRegisterDirectoryWatcherDelegates() const
{
	if (!DirectoryWatcher) return;
	
	if (SourceDirDelegateHandle.IsValid())
	{
		DirectoryWatcher->Get()->UnregisterDirectoryChangedCallback_Handle(FPaths::GameSourceDir(), SourceDirDelegateHandle);
	}
	if (ConfigDirDelegateHandle.IsValid())
	{
		DirectoryWatcher->Get()->UnregisterDirectoryChangedCallback_Handle(FPaths::ProjectConfigDir(), ConfigDirDelegateHandle);
	}
	if (PluginsDirDelegateHandle.IsValid())
	{
		DirectoryWatcher->Get()->UnregisterDirectoryChangedCallback_Handle(FPaths::ProjectPluginsDir(), PluginsDirDelegateHandle);
	}
}

void ProjectCleanerManager::OnDirectoryChanged(const TArray<FFileChangeData>& InFileChanges)
{
	bIsActualData = false;
}

void ProjectCleanerManager::RegisterAssetRegistryDelegates()
{
	AssetAddedDelegate = AssetRegistry->Get().OnAssetAdded().AddRaw(this, &ProjectCleanerManager::OnAssetAdded);
	AssetRemovedDelegate = AssetRegistry->Get().OnAssetRemoved().AddRaw(this, &ProjectCleanerManager::OnAssetRemoved);
	AssetRenamedDelegate = AssetRegistry->Get().OnAssetRenamed().AddRaw(this, &ProjectCleanerManager::OnAssetRenamed);
	AssetUpdatedDelegate = AssetRegistry->Get().OnAssetUpdated().AddRaw(this, &ProjectCleanerManager::OnAssetUpdated);
}

void ProjectCleanerManager::UnRegisterAssetRegistryDelegates() const
{
	if (!AssetRegistry) return;
	
	if (AssetAddedDelegate.IsValid())
	{
		AssetRegistry->Get().OnAssetAdded().Remove(AssetAddedDelegate);
	}
	if (AssetRemovedDelegate.IsValid())
	{
		AssetRegistry->Get().OnAssetRemoved().Remove(AssetRemovedDelegate);
	}
	if (AssetUpdatedDelegate.IsValid())
	{
		AssetRegistry->Get().OnAssetUpdated().Remove(AssetUpdatedDelegate);
	}
	if (AssetRenamedDelegate.IsValid())
	{
		AssetRegistry->Get().OnAssetUpdated().Remove(AssetUpdatedDelegate);
	}
}

#undef LOCTEXT_NAMESPACE
