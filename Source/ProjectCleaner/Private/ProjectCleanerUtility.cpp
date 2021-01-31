// Fill out your copyright notice in the Description page of Project Settings.


#include "Public/ProjectCleanerUtility.h"
#include "FileManager.h"
#include "AssetRegistry/Public/AssetData.h"
#include "AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "Engine/World.h"
#include "Misc/Paths.h"
#include "ObjectTools.h"
#include "Materials/Material.h"
#include "UObject/ObjectRedirector.h"

#pragma optimize("", off)


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

	if (AssetsToDelete.Num() > 0)
	{
		return ObjectTools::DeleteAssets(AssetsToDelete, true);
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
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::GetModuleChecked<FAssetRegistryModule>("AssetRegistry");
	AssetRegistryModule.Get().GetAssetsByPath(FName{"/Game"}, GameAssetsContainer, true);
}

void ProjectCleanerUtility::RemoveLevelAssets(TArray<FAssetData>& GameAssetsContainer)
{
	GameAssetsContainer.RemoveAll([](FAssetData Val)
	{
		return Val.AssetClass.ToString().Contains("MapBuildDataRegistry") || Val.AssetClass == UWorld::StaticClass()->GetFName();
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
			if (bIsAlreadyInSet == false && Dependency.IsValid())
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

void ProjectCleanerUtility::FixupRedirectors()
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::GetModuleChecked<FAssetRegistryModule>("AssetRegistry");

	const FName RootPath = TEXT("/Game");

	FARFilter Filter;
	Filter.bRecursivePaths = true;
	Filter.PackagePaths.Emplace(RootPath);
	Filter.ClassNames.Emplace(TEXT("ObjectRedirector"));

	// Query for a list of assets
	TArray<FAssetData> AssetList;
	AssetRegistryModule.Get().GetAssets(Filter, AssetList);

	if (AssetList.Num() > 0)
	{
		TArray<UObject*> Objects;
		// loading asset if needed
		for (const auto& Asset : AssetList)
		{
			Objects.Add(Asset.GetAsset());
		}


		// converting them to redirectors
		TArray<UObjectRedirector*> Redirectors;
		for (auto Object : Objects)
		{
			const auto Redirector = CastChecked<UObjectRedirector>(Object);
			Redirectors.Add(Redirector);
		}

		// Fix up all founded redirectors
		FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
		AssetToolsModule.Get().FixupReferencers(Redirectors);
	}
}

void ProjectCleanerUtility::FindAndCreateAssetTree(TArray<FAssetData>& UnusedAssets,
                                                   TArray<FAssetChunk>& AssetChunks)
{
	// find root assets
	TArray<FName> RootAssets;
	FindAllAssetsWithNoDependencies(RootAssets, UnusedAssets);

	for (const auto& RootAsset : RootAssets)
	{
		TArray<FName> Refs;
		DepResolve(RootAsset, Refs);

		FAssetChunk Chunk;
		for (const auto& Ref : Refs)
		{
			const auto FoundedAssetData = UnusedAssets.FindByPredicate([&](const FAssetData& SingleAsset)
			{
				return SingleAsset.PackageName == Ref;
			});
			if (FoundedAssetData && FoundedAssetData->IsValid())
			{
				Chunk.Dependencies.Add(*FoundedAssetData);
			}
		}

		if (Chunk.Dependencies.Num() > 0)
		{
			AssetChunks.Add(Chunk);
		}
	}


	// while (UnusedAssets.Num() > 0)
	// {
	// 	const auto Elem = UnusedAssets.Pop(false);
	// 	AssetRegistryModule.Get().GetReferencers(Elem.PackageName, Refs);
	// 	for (const auto& Ref : Refs)
	// 	{
	// 		Output.AddUnique(Ref);
	// 	}
	// 	Refs.Reset();
	// }


	UE_LOG(LogTemp, Warning, TEXT("A"));

	// ========= OLD Recursive VERSION =========
	// // 1) Finding all assets of projects
	// TArray<FAssetData> AllAssets;
	// AllAssets.Reserve(1000);
	//
	// FAssetRegistryModule& AssetRegistryModule = FModuleManager::GetModuleChecked<FAssetRegistryModule>("AssetRegistry");
	// AssetRegistryModule.Get().GetAssetsByPath(FName{"/Game"}, AllAssets, true);
	//
	// // 2) Finding unused assets
	// TArray<FName> RootAssets;
	// FindAllAssetsWithNoDependencies(RootAssets, AllAssets);
	//
	// for (const auto& Asset : RootAssets)
	// {
	// 	{
	// 		TArray<FName> Resolved;
	// 		Resolved.Reserve(20);
	// 		DepResolve(Asset, Resolved);
	// 		if (Resolved.Num() > 0)
	// 		{
	// 			FAssetChunk Chunk;
	// 			for (const auto& Elem : Resolved)
	// 			{
	// const auto FoundedAssetData = AllAssets.FindByPredicate([&](const FAssetData& SingleAsset)
	// {
	// 	return SingleAsset.PackageName == Elem;
	// });
	//
	// 				if (FoundedAssetData && FoundedAssetData->IsValid())
	// 				{
	// 					Chunk.Dependencies.Add(*FoundedAssetData);
	// 				}
	// 			}
	//
	// 			if(Chunk.Dependencies.Num() > 0)
	// 			{
	// 				AssetChunks.Add(Asset, Chunk);					
	// 			}
	// 		}
	// 	}
	// }
}

bool ProjectCleanerUtility::DepResolve(const FName& Asset, TArray<FName>& Resolved)
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::GetModuleChecked<FAssetRegistryModule>("AssetRegistry");

	if (IsLevelAsset(Asset)) return false;

	bool NoLevelsFound = true;

	TArray<FName> Referencers;
	AssetRegistryModule.Get().GetReferencers(Asset, Referencers, EAssetRegistryDependencyType::Hard);

	// remove itself from list
	Referencers.RemoveAll([&](const FName Val)
	{
		return Val.ToString().Compare(Asset.ToString()) == 0;
	});


	for (const auto& Ref : Referencers)
	{
		if (!DepResolve(Ref, Resolved))
		{
			NoLevelsFound = false;
		}
	}

	if (NoLevelsFound)
	{
		Resolved.AddUnique(Asset);
		return true;
	}

	return false;
}

bool ProjectCleanerUtility::IsLevelAsset(const FName& Asset)
{
	TArray<FAssetData> Levels;
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::GetModuleChecked<FAssetRegistryModule>("AssetRegistry");
	FARFilter Filter;
	Filter.bRecursivePaths = true;
	Filter.PackagePaths.Add(FName{"/Game"});
	Filter.ClassNames.Add(UWorld::StaticClass()->GetFName());
	AssetRegistryModule.Get().GetAssets(Filter, Levels);

	for (const auto& Level : Levels)
	{
		if (Level.PackageName.Compare(Asset) == 0)
		{
			return true;
		}
	}

	return false;
}

void ProjectCleanerUtility::FindAllAssetsWithNoDependencies(TArray<FName>& Assets, const TArray<FAssetData>& AllAssets)
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::GetModuleChecked<FAssetRegistryModule>("AssetRegistry");
	for (const auto& Asset : AllAssets)
	{
		if (Asset.AssetClass.Compare(UWorld::StaticClass()->GetFName()) == 0 || Asset.AssetClass.Compare(FName{
			"MapBuildDataRegistry"
		}) == 0)
		{
			continue;
		}

		TArray<FName> Dependencies;
		AssetRegistryModule.Get().GetDependencies(Asset.PackageName, Dependencies);
		// remove itself from list
		Dependencies.RemoveAll([&](const FName Val)
		{
			return Val.ToString().Compare(Asset.PackageName.ToString()) == 0;
		});

		if (Dependencies.Num() == 0)
		{
			Assets.Add(Asset.PackageName);
		}
	}
}


void ProjectCleanerUtility::FindAllRefs(const FName& Root)
{
	TArray<FName> Stack;
}

void ProjectCleanerUtility::DeleteAssetChunks(TArray<FAssetChunk>& AssetChunks)
{
	// todo:ashe23 add progress calculation here
	for (const auto& Chunk : AssetChunks)
	{
		ObjectTools::DeleteAssets(Chunk.Dependencies, false);
	}
}

#pragma optimize("", on)
