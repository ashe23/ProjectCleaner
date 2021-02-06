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
#include "HAL/PlatformFilemanager.h"
#include "Misc/FileHelper.h"
#include "Misc/ScopedSlowTask.h"
#include "FileHelpers.h"
#include "IContentBrowserSingleton.h"
#include "Editor/ContentBrowser/Public/ContentBrowserModule.h"

#pragma optimize("", off)

bool ProjectCleanerUtility::HasFiles(const FString& SearchPath)
{
	TArray<FString> Files;
	IFileManager::Get().FindFiles(Files, *SearchPath, true, false);

	return Files.Num() > 0;
}

bool ProjectCleanerUtility::GetAllEmptyDirectories(const FString& SearchPath,
                                                   TArray<FString>& Directories,
                                                   TArray<FString>& NonProjectFiles,
                                                   const bool bIsRootDirectory)
{
	bool AllSubDirsEmpty = true;
	TArray<FString> ChildDirectories;
	GetChildrenDirectories(SearchPath, ChildDirectories);

	FindNonProjectFiles(SearchPath, NonProjectFiles);

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
		if (GetAllEmptyDirectories(NewPath, Directories,NonProjectFiles, false))
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

void ProjectCleanerUtility::DeleteEmptyFolders(TArray<FString>& EmptyFolders)
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::GetModuleChecked<FAssetRegistryModule>("AssetRegistry");
	for (auto& EmptyFolder : EmptyFolders)
	{
		if (IFileManager::Get().DirectoryExists(*EmptyFolder))
		{
			IFileManager::Get().DeleteDirectory(*EmptyFolder, false, true);
			auto f = EmptyFolder.Replace(*FPaths::ProjectContentDir(), TEXT("/Game/"));
			AssetRegistryModule.Get().RemovePath(f);
		}
	}


	TArray<FString> Paths;
	Paths.Add("/Game");
	AssetRegistryModule.Get().ScanPathsSynchronous(Paths, true);
	AssetRegistryModule.Get().SearchAllAssets(true);

	FContentBrowserModule& CBModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	TArray<FString> FocusFolders;
	FocusFolders.Add("/Game");
	CBModule.Get().SetSelectedPaths(FocusFolders, true);

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
		return Val.AssetClass.ToString().Contains("MapBuildDataRegistry") || Val.AssetClass == UWorld::StaticClass()->
			GetFName();
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

int32 ProjectCleanerUtility::GetUnusedAssetsNum(TArray<FAssetData>& UnusedAssets, TArray<FString> AllSourceFiles)
{
	FScopedSlowTask SlowTask{3.0f, FText::FromString("Scanning project...")};
	SlowTask.MakeDialog();

	UnusedAssets.Empty();
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::GetModuleChecked<FAssetRegistryModule>("AssetRegistry");


	FindAllGameAssets(UnusedAssets);
	RemoveLevelAssets(UnusedAssets);

	SlowTask.EnterProgressFrame(1.0f);

	// Finding all assets and their dependencies that used in levels
	TSet<FName> LevelsDependencies;
	FARFilter Filter;
	Filter.ClassNames.Add(UWorld::StaticClass()->GetFName());
	GetAllDependencies(Filter, AssetRegistryModule.Get(), LevelsDependencies);

	SlowTask.EnterProgressFrame(1.0f);


	// Removing all assets that are used in any level
	UnusedAssets.RemoveAll([&](const FAssetData& Val)
	{
		return LevelsDependencies.Contains(Val.PackageName);
	});

	// Remove all assets that used in code( hard linked)
	FindAllSourceFiles(AllSourceFiles);

	SlowTask.EnterProgressFrame(1.0f);

	UnusedAssets.RemoveAll([&](const FAssetData& Val)
	{
		return UsedInSourceFiles(AllSourceFiles, Val.PackageName);
	});

	return UnusedAssets.Num();
}

int32 ProjectCleanerUtility::GetEmptyFoldersNum(TArray<FString>& EmptyFolders, TArray<FString>& NonProjectFiles)
{
	FScopedSlowTask SlowTask{1.0f, FText::FromString("Searching empty folders...")};
	SlowTask.MakeDialog();

	EmptyFolders.Empty();

	const auto ProjectRoot = FPaths::ProjectContentDir();
	GetAllEmptyDirectories(ProjectRoot / TEXT("*"), EmptyFolders, NonProjectFiles, true);

	SlowTask.EnterProgressFrame(1.0f);

	return EmptyFolders.Num();
}

int64 ProjectCleanerUtility::GetUnusedAssetsTotalSize(TArray<FAssetData>& UnusedAssets)
{
	FScopedSlowTask SlowTask{1.0f, FText::FromString("Calculating total size of unused assets...")};
	SlowTask.MakeDialog();

	int64 Size = 0;
	for (const auto& Asset : UnusedAssets)
	{
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::GetModuleChecked<FAssetRegistryModule>(
			"AssetRegistry");
		const auto AssetPackageData = AssetRegistryModule.Get().GetAssetPackageData(Asset.PackageName);
		if (!AssetPackageData) continue;
		Size += AssetPackageData->DiskSize;
	}

	SlowTask.EnterProgressFrame(1.0f);

	return Size;
}

void ProjectCleanerUtility::FixupRedirectors()
{
	FScopedSlowTask SlowTask{1.0f, FText::FromString("Fixing up Redirectors...")};
	SlowTask.MakeDialog();

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

	SlowTask.EnterProgressFrame(1.0f);
}

int32 ProjectCleanerUtility::DeleteAssets(TArray<FAssetData>& Assets)
{
	// first try to delete normally
	int32 DeletedAssets = ObjectTools::DeleteAssets(Assets, false);

	// if normally not working try to force delete
	if (DeletedAssets == 0)
	{
		TArray<UObject*> AssetObjects;
		AssetObjects.Reserve(Assets.Num());

		for (const auto& Asset : Assets)
		{
			AssetObjects.Add(Asset.GetAsset());
		}

		DeletedAssets = ObjectTools::ForceDeleteObjects(AssetObjects, false);
	}

	return DeletedAssets;
}

void ProjectCleanerUtility::GetRootAssets(TArray<FAssetData>& RootAssets,
                                          TArray<FAssetData>& AllAssets,
                                          const FCleaningStats& CleaningStats)
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::GetModuleChecked<FAssetRegistryModule>("AssetRegistry");
	for (const auto& Asset : AllAssets)
	{
		if (RootAssets.Num() >= CleaningStats.DeleteChunkSize) break;

		TArray<FName> Refs;
		AssetRegistryModule.Get().GetReferencers(Asset.PackageName, Refs);

		// Removing itself from list
		Refs.RemoveAll([&](const FName& Val)
		{
			return Val.Compare(Asset.PackageName) == 0;
		});

		if (Refs.Num() == 0)
		{
			RootAssets.AddUnique(Asset);
		}
	}
}

void ProjectCleanerUtility::FindNonProjectFiles(const FString& SearchPath, TArray<FString>& NonProjectFilesList)
{
	// Project Directories may contain non .uasset files, which wont be shown in content browser,
	// Or there also case when assets saved in old engine versions not showing in new engine version content browser,
	// those asset must be tracked and informed to user , so they can handle them manually
	TArray<FString> NonUAssetFiles;
	IFileManager::Get().FindFiles(NonUAssetFiles, *SearchPath, true, false);

	for (const auto& NonUAssetFile : NonUAssetFiles)
	{
		const auto Extension = FPaths::GetExtension(NonUAssetFile);
		if (!Extension.Equals("uasset") && !Extension.Equals("umap"))
		{
			FString Path = SearchPath;
			Path.RemoveFromEnd("*");
			Path.Append(NonUAssetFile);
			Path = FPaths::ConvertRelativePathToFull(Path);
			NonProjectFilesList.AddUnique(Path);
		}
	}
}

void ProjectCleanerUtility::FindAllSourceFiles(TArray<FString>& AllFiles)
{
	const auto ProjectSourceDir = FPaths::GameSourceDir();
	const auto ProjectPluginsDir = FPaths::ProjectPluginsDir();
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	TArray<FString> ProjectSourceFiles;
	TArray<FString> ProjectPluginsFiles;

	PlatformFile.FindFilesRecursively(ProjectSourceFiles, *ProjectSourceDir, TEXT(".cpp"));
	PlatformFile.FindFilesRecursively(ProjectSourceFiles, *ProjectSourceDir, TEXT(".h"));
	PlatformFile.FindFilesRecursively(ProjectPluginsFiles, *ProjectPluginsDir, TEXT(".cpp"));
	PlatformFile.FindFilesRecursively(ProjectPluginsFiles, *ProjectPluginsDir, TEXT(".h"));

	AllFiles.Append(ProjectSourceFiles);
	AllFiles.Append(ProjectPluginsFiles);
}

bool ProjectCleanerUtility::UsedInSourceFiles(TArray<FString>& AllFiles, const FName& Asset)
{
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

	for (const auto& File : AllFiles)
	{
		if (PlatformFile.FileExists(*File))
		{
			FString FileContent;
			FFileHelper::LoadFileToString(FileContent, *File);
			if (FileContent.Find(Asset.ToString()) != -1)
			{
				return true;
			}
		}
	}

	return false;
}

void ProjectCleanerUtility::SaveAllAssets()
{
	FEditorFileUtils::SaveDirtyPackages(
		true,
		true,
		true,
		false,
		false,
		false
	);
}

#pragma optimize("", on)
