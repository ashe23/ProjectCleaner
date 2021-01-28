// Fill out your copyright notice in the Description page of Project Settings.


#include "Public/ProjectCleanerUtility.h"
#include "FileManager.h"
#include "AssetRegistry/Public/AssetData.h"
#include "AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "Engine/World.h"
#include "Misc/Paths.h"
#include "ObjectTools.h"
#include "UObject/ObjectRedirector.h"

bool ProjectCleanerUtility::HasFiles(const FString& SearchPath)
{
	TArray<FString> Files;
	IFileManager::Get().FindFiles(Files, *SearchPath, true, false);

	return Files.Num() > 0;
}

bool ProjectCleanerUtility::HasDirectories(const FString& SearchPath)
{
	TArray<FString> Directories;
	IFileManager::Get().FindFiles(Directories, *SearchPath, false, true);

	return Directories.Num() > 0;
}

bool ProjectCleanerUtility::GetAllEmptyDirectories(const FString& SearchPath, TArray<FString>& Directories,
                                                   const bool bIsRootDirectory)
{
	bool AllSubDirsEmpty = true;
	TArray<FString> ChildDirectories;
	GetChildrenDirectories(SearchPath, ChildDirectories);

	// Your Project Root directory (<Your Project>/Content) also contains "Collections" and "Developers" folders
	// we dont need them
	if (bIsRootDirectory)
	{
		RemoveDevsAndCollectionsDirectories(ChildDirectories);
	}


	for (const auto& Dir : ChildDirectories)
	{
		// "*" needed for unreal`s IFileManager class, without it , its not working.  
		auto NewPath = SearchPath;
		NewPath.RemoveFromEnd(TEXT("*"));
		NewPath += Dir / TEXT("*");
		if (GetAllEmptyDirectories(NewPath, Directories, false))
		{
			NewPath.RemoveFromEnd(TEXT("*"));
			Directories.Add(NewPath);
		}
		else
		{
			AllSubDirsEmpty = false;
		}
	}

	if (AllSubDirsEmpty && !HasFiles(SearchPath))
	{
		return true;
	}

	return false;
}

void ProjectCleanerUtility::GetChildrenDirectories(const FString& SearchPath, TArray<FString>& Output)
{
	IFileManager::Get().FindFiles(Output, *SearchPath, false, true);
}

void ProjectCleanerUtility::RemoveDevsAndCollectionsDirectories(TArray<FString>& Directories)
{
	Directories.RemoveAll([&](const FString& Val)
	{
		return Val.Contains("Developers") || Val.Contains("Collections");
	});
}

int32 ProjectCleanerUtility::DeleteUnusedAssets(TArray<FAssetData>& AssetsToDelete)
{
	// todo:ashe23 try to delete in chunks for performance purposes
	// todo:ashe23 after deleting lot of files content browser not updates its content, but in reality files are deleted
	for (const auto& Asset : AssetsToDelete)
	{
		// StreamableManager.Unload(Asset.GetAsset());
	}
	
	if (AssetsToDelete.Num() > 0)
	{
		return ObjectTools::DeleteAssets(AssetsToDelete);
	}

	return 0;
}

void ProjectCleanerUtility::DeleteEmptyFolders(TArray<FString>& EmptyFolders)
{
	for (const auto& EmptyFolder : EmptyFolders)
	{
		if (IFileManager::Get().DirectoryExists(*EmptyFolder))
		{
			IFileManager::Get().DeleteDirectory(*EmptyFolder, false, true);
		}
	}

	EmptyFolders.Empty();
}

void ProjectCleanerUtility::FindAllGameAssets(TArray<FAssetData>& GameAssetsContainer)
{
	// todo fix up redirectors before finding all assets and filling them in array
	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::GetModuleChecked<FAssetRegistryModule>("AssetRegistry");
	AssetRegistryModule.Get().GetAssetsByPath(FName{"/Game"}, GameAssetsContainer, true);
	// AssetRegistryModule.Get().GetAssetAvailability()

	// for(const auto& Asset: GameAssetsContainer)
	// {
	// 	Asset.IsRedirector();
	// }
}

void ProjectCleanerUtility::RemoveLevelAssets(TArray<FAssetData>& GameAssetsContainer)
{
	GameAssetsContainer.RemoveAll([](FAssetData Val)
	{
		return Val.AssetName.ToString().Contains("_BuiltData") || Val.AssetClass == UWorld::StaticClass()->GetFName();
	});
}

void ProjectCleanerUtility::GetAllDependencies(const FARFilter& InAssetRegistryFilter,
	const IAssetRegistry& AssetRegistry, TSet<FName>& OutDependencySet)
{
	TArray<FName> PackageNamesToProcess;
	{
		TArray<FAssetData> FoundAssets;
		AssetRegistry.GetAssets(InAssetRegistryFilter, FoundAssets);
		for (const FAssetData& AssetData : FoundAssets)
		{
			PackageNamesToProcess.Add(AssetData.PackageName);
			OutDependencySet.Add(AssetData.PackageName);
		}
	}

	TArray<FAssetIdentifier> AssetDependencies;
	while (PackageNamesToProcess.Num() > 0)
	{
		const FName PackageName = PackageNamesToProcess.Pop(false);
		AssetDependencies.Reset();
		AssetRegistry.GetDependencies(FAssetIdentifier(PackageName), AssetDependencies);
		for (const FAssetIdentifier& Dependency : AssetDependencies)
		{
			bool bIsAlreadyInSet = false;
			OutDependencySet.Add(Dependency.PackageName, &bIsAlreadyInSet);
			if (bIsAlreadyInSet == false)
			{
				PackageNamesToProcess.Add(Dependency.PackageName);
			}
		}
	}
}

int32 ProjectCleanerUtility::GetUnusedAssetsNum(TArray<FAssetData>& UnusedAssets)
{
	UnusedAssets.Empty();
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::GetModuleChecked<FAssetRegistryModule>("AssetRegistry");

	FindAllGameAssets(UnusedAssets);
	RemoveLevelAssets(UnusedAssets);

	// finding redirectors
	TArray<FAssetData> Redirs;
	FARFilter RedirectorFilter;
	RedirectorFilter.ClassNames.Add(UObjectRedirector::StaticClass()->GetFName());
	AssetRegistryModule.Get().GetAssets(RedirectorFilter,Redirs);
	
	// Finding all assets and their dependencies that used in levels
	TSet<FName> LevelsDependencies;
	FARFilter Filter;
	Filter.ClassNames.Add(UWorld::StaticClass()->GetFName());
	GetAllDependencies(Filter, AssetRegistryModule.Get(), LevelsDependencies);

	// Removing all assets that are used in any level
	UnusedAssets.RemoveAll([&](const FAssetData& Val)
	{
		return LevelsDependencies.Contains(Val.PackageName);
	});

	return UnusedAssets.Num();
}

int32 ProjectCleanerUtility::GetEmptyFoldersNum(TArray<FString>& EmptyFolders)
{
	EmptyFolders.Empty();

	const auto ProjectRoot = FPaths::ProjectContentDir();
	GetAllEmptyDirectories(ProjectRoot / TEXT("*"), EmptyFolders, true);

	return EmptyFolders.Num();
}

int64 ProjectCleanerUtility::GetUnusedAssetsTotalSize(TArray<FAssetData>& UnusedAssets)
{
	int64 Size = 0;
	for (const auto& Asset : UnusedAssets)
	{
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::GetModuleChecked<FAssetRegistryModule>(
            "AssetRegistry");
		const auto AssetPackageData = AssetRegistryModule.Get().GetAssetPackageData(Asset.PackageName);
		if (!AssetPackageData) continue;
		Size += AssetPackageData->DiskSize;
	}

	return Size;
}

void ProjectCleanerUtility::GetRedirectors()
{
	
}
