// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#include "Core/ProjectCleanerManager.h"
#include "StructsContainer.h"
#include "UI/ProjectCleanerNotificationManager.h"
// Engine Headers
#include "AssetRegistryModule.h"
#include "Misc/ScopedSlowTask.h"
#include "Misc/FileHelper.h"
#include "Engine/AssetManager.h"
#include "ShaderCompiler.h"

#define LOCTEXT_NAMESPACE "FProjectCleanerModule"

FProjectCleanerManager::FProjectCleanerManager()
{
	CleanerConfigs = GetMutableDefault<UCleanerConfigs>();
	
	ensure(CleanerConfigs);

	CleanerConfigs->LoadConfig();
}

FProjectCleanerManager::~FProjectCleanerManager()
{
}

void FProjectCleanerManager::Update()
{
	if (DataManager.IsLoadingAssets()) return;
	
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
	
	CleanerConfigs->PostEditChange();
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

bool FProjectCleanerManager::ExcludePath(const FString& InPath)
{
	if (!DataManager.ExcludePath(InPath))
	{
		return false;
	}
	
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

	return true;
}

bool FProjectCleanerManager::IncludePath(const FString& InPath)
{
	if (!DataManager.IncludePath(InPath))
	{
		ProjectCleanerNotificationManager::AddTransient(
			FText::FromString(FStandardCleanerText::CantIncludePath),
			SNotificationItem::CS_Fail,
			10.0f
		);
		return false;
	}
	
	CleanerConfigs->Paths.RemoveAll([&] (const FDirectoryPath& DirPath)
	{
		return DirPath.Path.Equals(InPath);
	});
	
	Update();

	return true;
}

bool FProjectCleanerManager::IncludeSelectedAssets(const TArray<FAssetData>& Assets)
{
	if (!DataManager.IncludeSelectedAssets(Assets))
	{
		ProjectCleanerNotificationManager::AddTransient(
			FText::FromString(FStandardCleanerText::CantIncludeSomeAssets),
			SNotificationItem::CS_Fail,
			10.0f
		);
		return false;
	}
	
	Update();

	return true;
}

int32 FProjectCleanerManager::DeleteSelectedAssets(const TArray<FAssetData>& Assets)
{
	const int32 AssetNum = Assets.Num();
	const int32 DeletedAssetsNum = DataManager.DeleteSelectedAssets(Assets);

	if (DeletedAssetsNum != 0 && DeletedAssetsNum != AssetNum)
	{
		ProjectCleanerNotificationManager::AddTransient(
			FText::FromString(FStandardCleanerText::FailedToDeleteSomeAssets),
			SNotificationItem::CS_Fail,
			3.0f
		);
	}

	return DeletedAssetsNum;
}

int32 FProjectCleanerManager::DeleteAllUnusedAssets()
{
	const int32 UnusedAssetsNum = DataManager.GetUnusedAssets().Num();
	const int32 DeleteAssetsNum = DataManager.DeleteAllUnusedAssets();

	if (UnusedAssetsNum != DeleteAssetsNum)
	{
		ProjectCleanerNotificationManager::AddTransient(
			FText::FromString(FStandardCleanerText::FailedToDeleteSomeAssets),
			SNotificationItem::CS_Fail,
			10.0f
		);
	}
	else
	{
		ProjectCleanerNotificationManager::AddTransient(
			FText::FromString(FStandardCleanerText::UnusedAssetsSuccessfullyDeleted),
			SNotificationItem::CS_Success,
			10.0f
		);
	}
	
	if (CleanerConfigs->bAutomaticallyDeleteEmptyFolders)
	{
		DeleteEmptyFolders();
	}
	
	// todo:ashe23 This part is hacky
	// Because assets loaded before deletion, in the cleanup end , we got a lot of shaders to compile,
	// I dig into engine codes and couldn't find any interface to pause/resume shader compilations, besides
	// GShaderCompilingManager->Shutdown(), which completely shut down current and future shader compilations.
	// So we just restarting editor after cleaning finished.
	// 
	// PS. If someone has better understanding about this, or has better solution, would much appreciate for help :)
	
	// show window to restart editor, if any shader compilation still exists
	if (GShaderCompilingManager && GShaderCompilingManager->IsCompiling())
	{
		if (GShaderCompilingManager->HasShaderJobs())
		{
			const auto ConfirmationWindowStatus = ProjectCleanerNotificationManager::ShowConfirmationWindow(
				FText::FromString(FStandardCleanerText::RestartEditorTitle),
				FText::FromString(FStandardCleanerText::RestartEditorContent)
			);
			if (!ProjectCleanerNotificationManager::IsConfirmationWindowCanceled(ConfirmationWindowStatus))
			{
				FUnrealEdMisc::Get().RestartEditor(true);
				return DeleteAssetsNum;
			}
		}
	}

	return DeleteAssetsNum;
}

int32 FProjectCleanerManager::DeleteEmptyFolders()
{
	const int32 DeletedFoldersNum = DataManager.DeleteEmptyFolders();

	if (DeletedFoldersNum > 0)
	{
		ProjectCleanerNotificationManager::AddTransient(
			FText::FromString(FStandardCleanerText::FoldersSuccessfullyDeleted),
			SNotificationItem::CS_Success,
			5.0f
		);
	}
	
	return DeletedFoldersNum;
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

#undef LOCTEXT_NAMESPACE
