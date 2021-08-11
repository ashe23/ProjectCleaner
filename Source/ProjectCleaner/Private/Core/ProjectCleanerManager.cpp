// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#include "Core/ProjectCleanerManager.h"
#include "StructsContainer.h"
// Engine Headers
#include "AssetRegistryModule.h"
#include "Misc/ScopedSlowTask.h"
#include "Misc/FileHelper.h"
#include "Engine/AssetManagerSettings.h"
#include "Engine/AssetManager.h"

#define LOCTEXT_NAMESPACE "FProjectCleanerModule"

FProjectCleanerManager::FProjectCleanerManager()
{
	CleanerConfigs = GetMutableDefault<UCleanerConfigs>();
	
	ensure(CleanerConfigs);
}

FProjectCleanerManager::~FProjectCleanerManager()
{
}

void FProjectCleanerManager::Update()
{
	FScopedSlowTask UpdateTask(1.0f, FText::FromString(FStandardCleanerText::Scanning));
	UpdateTask.MakeDialog();

	DataManager.SetCleanerConfigs(CleanerConfigs);
	DataManager.AnalyzeProject();

	UpdateTask.EnterProgressFrame();
	
	// Broadcast to all bounded objects that data is updated
	if (OnCleanerManagerUpdated.IsBound())
	{
		OnCleanerManagerUpdated.Execute();
	}
}

void FProjectCleanerManager::ExcludeSelectedAssets(const TArray<FAssetData>& Assets)
{
	DataManager.ExcludeSelectedAssets(Assets);
	
	Update();
}

void FProjectCleanerManager::ExcludeSelectedAssetsByType(const TArray<FAssetData>& Assets)
{
	for (const auto& Asset : Assets)
	{
		if (Asset.AssetClass.IsEqual(TEXT("Blueprint")))
		{
			const auto LoadedAsset = Asset.GetAsset();
			if (!LoadedAsset) continue;
			
			const auto BlueprintAsset = Cast<UBlueprint>(LoadedAsset);
			if (BlueprintAsset && BlueprintAsset->GeneratedClass)
			{
				CleanerConfigs->Classes.AddUnique(BlueprintAsset->GeneratedClass);
			}
		}
		else
		{
			CleanerConfigs->Classes.AddUnique(Asset.GetClass());
		}
	}
	
	Update();
}

void FProjectCleanerManager::ExcludePath(const FString& InPath)
{
	if (InPath.IsEmpty()) return;
	
	FDirectoryPath DirectoryPath;
	DirectoryPath.Path = InPath;
	
	const bool bAlreadyExcluded = CleanerConfigs->Paths.ContainsByPredicate([&] (const FDirectoryPath& DirPath)
	{
		return DirPath.Path.Equals(InPath); 
	});
	
	if (!bAlreadyExcluded)
	{
		CleanerConfigs->Paths.Add(DirectoryPath);
	}
	
	Update();
}

void FProjectCleanerManager::IncludePath(const FString& InPath)
{
	if (InPath.IsEmpty()) return;
	
	CleanerConfigs->Paths.RemoveAll([&] (const FDirectoryPath& DirPath)
	{
		return DirPath.Path.Equals(InPath);
	});
	
	Update();
}

void FProjectCleanerManager::IncludeSelectedAssets(const TArray<FAssetData>& Assets)
{
	DataManager.IncludeSelectedAssets(Assets);
	
	Update();
}

int32 FProjectCleanerManager::DeleteSelectedAssets(const TArray<FAssetData>& Assets)
{
	const int32 DeletedAssets = DataManager.DeleteSelectedAssets(Assets);
	if (DeletedAssets > 0)
	{
		Update();
	}

	return DeletedAssets;
}

void FProjectCleanerManager::DeleteAllUnusedAssets()
{
	DataManager.DeleteAllUnusedAssets();
}

void FProjectCleanerManager::DeleteEmptyFolders()
{
	DataManager.DeleteEmptyFolders();
}

const FProjectCleanerDataManager& FProjectCleanerManager::GetDataManager() const
{
	return DataManager;
}

const TArray<FAssetData>& FProjectCleanerManager::GetAllAssets() const
{
	return DataManager.GetAllAssets();
}

const TArray<FAssetData>& FProjectCleanerManager::GetUnusedAssets() const
{
	return DataManager.GetUnusedAssets();
}

const TSet<FName>& FProjectCleanerManager::GetExcludedAssets() const
{
	return DataManager.GetExcludedAssets();
}

const TSet<FName>& FProjectCleanerManager::GetCorruptedAssets() const
{
	return DataManager.GetCorruptedAssets();
}

const TSet<FName>& FProjectCleanerManager::GetNonEngineFiles() const
{
	return DataManager.GetNonEngineFiles();
}

const TMap<FAssetData, FIndirectAsset>& FProjectCleanerManager::GetIndirectAssets() const
{
	return DataManager.GetIndirectAssets();
}

const TSet<FName>& FProjectCleanerManager::GetEmptyFolders() const
{
	return DataManager.GetEmptyFolders();
}

const TSet<FName>& FProjectCleanerManager::GetPrimaryAssetClasses() const
{
	return DataManager.GetPrimaryAssetClasses();
}

UCleanerConfigs* FProjectCleanerManager::GetCleanerConfigs() const
{
	return CleanerConfigs;
}

float FProjectCleanerManager::GetUnusedAssetsPercent() const
{
	if (DataManager.GetAllAssets().Num() == 0) return 0.0f;

	return DataManager.GetUnusedAssets().Num() * 100.0f / DataManager.GetAllAssets().Num();
}

void FProjectCleanerManager::IncludeAllAssets()
{
	CleanerConfigs->Classes.Empty();
	CleanerConfigs->Paths.Empty();
	DataManager.IncludeAllAssets();

	Update();
}
//
// const TArray<FAssetData>& ProjectCleanerManager::GetAllAssets() const
// {
// 	return AllAssets;
// }
//
// const TSet<FName>& ProjectCleanerManager::GetCorruptedAssets() const
// {
// 	return CorruptedAssets;
// }
//
// const TSet<FName>& ProjectCleanerManager::GetNonEngineFiles() const
// {
// 	return NonEngineFiles;
// }
//
// const TMap<FAssetData, FIndirectAsset>& ProjectCleanerManager::GetIndirectAssets() const
// {
// 	return IndirectAssets;
// }
//
// const TSet<FName>& ProjectCleanerManager::GetEmptyFolders() const
// {
// 	return EmptyFolders;
// }
//
// const TSet<FName>& ProjectCleanerManager::GetPrimaryAssetClasses() const
// {
// 	return PrimaryAssetClasses;
// }
//
// const TArray<FAssetData>& ProjectCleanerManager::GetUnusedAssets() const
// {
// 	return UnusedAssets;
// }
//
// const TSet<FName>& ProjectCleanerManager::GetExcludedAssets() const
// {
// 	return ExcludedAssets;
// }
//
// UCleanerConfigs* ProjectCleanerManager::GetCleanerConfigs() const
// {
// 	return CleanerConfigs;
// }
//
// UExcludeOptions* ProjectCleanerManager::GetExcludeOptions() const
// {
// 	return ExcludeOptions;
// }



// const FAssetRegistryModule* ProjectCleanerManager::GetAssetRegistry() const
// {
// 	return AssetRegistry;
// }

#undef LOCTEXT_NAMESPACE
