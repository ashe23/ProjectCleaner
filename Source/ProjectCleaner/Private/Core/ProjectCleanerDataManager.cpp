// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#include "Core/ProjectCleanerDataManager.h"
#include "Core/ProjectCleanerUtility.h"
// Engine Headers
#include "Settings/ContentBrowserSettings.h"
#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/World.h"
#include "Engine/AssetManager.h"
#include "Engine/AssetManagerSettings.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "GenericPlatform/GenericPlatformFile.h"
#include "HAL/PlatformFilemanager.h"
// #include "Internationalization/Regex.h"

FString ProjectCleanerDataManager::GameUserDeveloperDir = FPaths::GameUserDeveloperDir();
FString ProjectCleanerDataManager::GameDevelopersDir = FPaths::GameUserDeveloperDir();
FString ProjectCleanerDataManager::GameUserDeveloperCollectionsDir = FPaths::GameUserDeveloperDir() + TEXT("Collections");
FString ProjectCleanerDataManager::CollectionsDir = FPaths::ProjectContentDir() + TEXT("Collections");
FString ProjectCleanerDataManager::ProjectContentDir = FPaths::ProjectContentDir();

ProjectCleanerDataManager::ProjectCleanerDataManager() :
	AssetRegistry(nullptr),
	DirectoryWatcher(nullptr),
	CleanerConfigs(nullptr),
	ExcludeOptions(nullptr),
	ContentBrowserSettings(nullptr)
{
	AssetRegistry = &FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	DirectoryWatcher = &FModuleManager::LoadModuleChecked<FDirectoryWatcherModule>(TEXT("DirectoryWatcher"));

	CleanerConfigs = GetMutableDefault<UCleanerConfigs>();
	ExcludeOptions = GetMutableDefault<UExcludeOptions>();
	ContentBrowserSettings = GetMutableDefault<UContentBrowserSettings>();

	ensure(CleanerConfigs);
	ensure(ExcludeOptions);
	ensure(AssetRegistry);
	ensure(DirectoryWatcher);

	AbsoluteRootFolder = FName{*FPaths::ProjectContentDir()};
	RelativeRootFolder = FName{*ProjectCleanerUtility::ConvertAbsolutePathToInternal(ProjectContentDir)};
}

void ProjectCleanerDataManager::Empty()
{
	AllAssets.Empty();
	UsedAssets.Empty();
	UnusedAssets.Empty();
	AllFiles.Empty();
	NonEngineFiles.Empty();
	CorruptedAssets.Empty();
	EmptyFolders.Empty();
	IndirectlyUsedAssets.Empty();
	PrimaryAssetClasses.Empty();
}

void ProjectCleanerDataManager::Update()
{
	Empty();

	FindPrimaryAssetClasses();
	AssetRegistry->Get().GetAssetsByPath(RelativeRootFolder, AllAssets, true);
	FindIndirectlyUsedAssets();
	FindUsedAssets();
	AnalyzeProjectDirectory();
	FindUnusedAssets();

	UE_LOG(LogTemp, Warning, TEXT("A"));
}

void ProjectCleanerDataManager::SetRootFolder(const FString& Folder)
{
	if (!FPaths::ValidatePath(Folder)) return;
	if (!FPlatformFileManager::Get().GetPlatformFile().DirectoryExists(*Folder)) return;
	
	AbsoluteRootFolder = FName{*Folder};
	RelativeRootFolder = FName{*ProjectCleanerUtility::ConvertAbsolutePathToInternal(Folder)};
}

void ProjectCleanerDataManager::FindPrimaryAssetClasses()
{
	const auto& AssetManager = UAssetManager::Get();
	if (!AssetManager.IsValid()) return;

	PrimaryAssetClasses.Reserve(10);
	
	const UAssetManagerSettings& Settings = AssetManager.GetSettings();
	TArray<FPrimaryAssetId> Ids;
	for (const auto& Type : Settings.PrimaryAssetTypesToScan)
	{
		AssetManager.Get().GetPrimaryAssetIdList(Type.PrimaryAssetType, Ids);
		for(const auto& Id : Ids)
		{
			FAssetData Data;
			AssetManager.Get().GetPrimaryAssetData(Id, Data);
			if(!Data.IsValid()) continue;
			PrimaryAssetClasses.Add(Data.AssetClass);
		}
		Ids.Reset();
	}

	PrimaryAssetClasses.Shrink();
}

void ProjectCleanerDataManager::FindUsedAssets()
{
	UsedAssets.Reserve(AllAssets.Num());
	
	FARFilter Filter;
	Filter.ClassNames.Add(UWorld::StaticClass()->GetFName());
	Filter.ClassNames.Append(PrimaryAssetClasses.Array());
	Filter.PackagePaths.Add(RelativeRootFolder);
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

	TArray<FAssetIdentifier> AssetDependencies;
	while (PackageNamesToProcess.Num() > 0)
	{
		const FName PackageName = PackageNamesToProcess.Pop(false);
		AssetDependencies.Reset();
		AssetRegistry->Get().GetDependencies(FAssetIdentifier(PackageName), AssetDependencies);
		for (const FAssetIdentifier& Dependency : AssetDependencies)
		{
			bool bIsAlreadyInSet = false;
			if (!Dependency.PackageName.ToString().StartsWith(RelativeRootFolder.ToString())) continue;
			
			UsedAssets.Add(Dependency.PackageName, &bIsAlreadyInSet);
			if (!bIsAlreadyInSet && Dependency.IsValid())
			{
				PackageNamesToProcess.Add(Dependency.PackageName);
			}
		}
	}
}

void ProjectCleanerDataManager::FindUnusedAssets()
{
	TSet<FName> LinkedAssets;
	FindLinkedAssets(LinkedAssets);
	
	for (const auto& AssetData : AllAssets)
	{
		if (UsedAssets.Contains(AssetData.PackageName)) continue;
		if (!AssetData.PackageName.ToString().StartsWith(RelativeRootFolder.ToString())) continue;
		if (IndirectlyUsedAssets.Contains(AssetData)) continue;
		if (LinkedAssets.Contains(AssetData.PackageName)) continue;
		// todo:ashe23 assets with external referencers
		// todo:ashe23 excluded by path
		// todo:ashe23 excluded by class

		UnusedAssets.Add(AssetData);
	}
}

void ProjectCleanerDataManager::FindIndirectlyUsedAssets()
{
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	TSet<FString> Files;
	Files.Reserve(200); // reserving some space
	
	// 1) finding all source files in main project "Source" directory (<yourproject>/Source/*)
	TArray<FString> FilesToScan;
	PlatformFile.FindFilesRecursively(FilesToScan, *FPaths::GameSourceDir(), TEXT(".cs"));
	PlatformFile.FindFilesRecursively(FilesToScan, *FPaths::GameSourceDir(), TEXT(".cpp"));
	PlatformFile.FindFilesRecursively(FilesToScan, *FPaths::GameSourceDir(), TEXT(".h"));
	PlatformFile.FindFilesRecursively(FilesToScan, *FPaths::ProjectConfigDir(), TEXT(".ini"));
	Files.Append(FilesToScan);
	
	// 2) we should find all source files in plugins folder (<yourproject>/Plugins/*)
	TArray<FString> ProjectPluginsFiles;
	// finding all installed plugins in "Plugins" directory
	struct DirectoryVisitor : public IPlatformFile::FDirectoryVisitor
	{
		virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory) override
		{
			if (bIsDirectory)
			{
				InstalledPlugins.Add(FilenameOrDirectory);
			}
	
			return true;
		}
	
		TArray<FString> InstalledPlugins;
	};
	
	DirectoryVisitor Visitor;
	PlatformFile.IterateDirectory(*FPaths::ProjectPluginsDir(), Visitor);
	
	// 3) for every installed plugin we scanning only "Source" and "Config" folders
	for (const auto& Dir : Visitor.InstalledPlugins)
	{
		const FString PluginSourcePathDir = Dir + "/Source";
		const FString PluginConfigPathDir = Dir + "/Config";
	
		PlatformFile.FindFilesRecursively(ProjectPluginsFiles, *PluginSourcePathDir, TEXT(".cs"));
		PlatformFile.FindFilesRecursively(ProjectPluginsFiles, *PluginSourcePathDir, TEXT(".cpp"));
		PlatformFile.FindFilesRecursively(ProjectPluginsFiles, *PluginSourcePathDir, TEXT(".h"));
		PlatformFile.FindFilesRecursively(ProjectPluginsFiles, *PluginConfigPathDir, TEXT(".ini"));
	}
	
	Files.Append(ProjectPluginsFiles);
	Files.Shrink();

	// for (const auto& File : Files)
	// {
	// 	if (!PlatformFile.FileExists(*File)) continue;
	//
	// 	FString FileContent;
	// 	FFileHelper::LoadFileToString(FileContent, *File);
	//
	// 	// search any sub string that has asset package path in it
	// 	static FRegexPattern Pattern(TEXT(R"(\/Game(.*)\b)"));
	// 	FRegexMatcher Matcher(Pattern, FileContent);
	// 	if (!Matcher.FindNext()) continue;
	// 	const FName FoundedAssetPackageName =  FName{Matcher.GetCaptureGroup(0)};
	//
	// 	// if founded asset exist in AssetRegistry database, then we finding line number where its used
	// 	// if (!ObjectPaths.Contains(FoundedAssetPackageName)) continue;
	// 	const FAssetData* AssetData = AllAssets.FindByPredicate([&] (const FAssetData& Elem)
	// 	{
	// 		return Elem.ObjectPath.IsEqual(FoundedAssetPackageName);
	// 	});
	// 	if (!AssetData) continue;
	//
	// 	TArray<FString> Lines;
	// 	FFileHelper::LoadFileToStringArray(Lines, *File);
	// 	for (int32 i = 0; i < Lines.Num(); ++i)
	// 	{
	// 		if (!Lines.IsValidIndex(i)) continue;
	// 		if (!Lines[i].Contains(FoundedAssetPackageName.ToString())) continue;
	//
	// 		FIndirectAsset IndirectAsset;
	// 		IndirectAsset.File = FPaths::ConvertRelativePathToFull(File);
	// 		IndirectAsset.Line = i + 1;
	// 		IndirectlyUsedAssets.Add(*AssetData, IndirectAsset);
	// 	}
	// }
}

void ProjectCleanerDataManager::AnalyzeProjectDirectory()
{
	struct ProjectCleanerRootDirVisitor : IPlatformFile::FDirectoryVisitor
	{
		ProjectCleanerRootDirVisitor(
			TArray<FAssetData>& Assets,
			TSet<FName>& Files,
			TSet<FName>& NewNonEngineFiles,
			TSet<FName>& NewEmptyFolders,
			TSet<FName>& NewCorruptedAssets
		) :
		AllAssets(Assets),
		AllFiles(Files),
		NonEngineFiles(NewNonEngineFiles),
		EmptyFolders(NewEmptyFolders),
		CorruptedAssets(NewCorruptedAssets) {}
		
		
		virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory) override
		{
			const FString FullPath = FPaths::ConvertRelativePathToFull(FilenameOrDirectory);
			if (bIsDirectory)
			{
				if (ProjectCleanerUtility::IsEmptyFolder(FullPath))
				{
					EmptyFolders.Add(FName{FullPath});
				}
			}
			else
			{
				if (ProjectCleanerUtility::IsEngineExtension(FPaths::GetExtension(FullPath, false)))
				{
					// here we got absolute path "C:/MyProject/Content/material.uasset"
					// we must first convert that path to In Engine Internal Path like "/Game/material.uasset"
					const FString InternalFilePath = ProjectCleanerUtility::ConvertAbsolutePathToInternal(FullPath);
					// Converting file path to object path (This is for searching in AssetRegistry)
					// example "/Game/Name.uasset" => "/Game/Name.Name"
					FString ObjectPath = InternalFilePath;
					ObjectPath.RemoveFromEnd(FPaths::GetExtension(InternalFilePath, true));
					ObjectPath.Append(TEXT(".") + FPaths::GetBaseFilename(InternalFilePath));

					const FName ObjectPathName = FName{*ObjectPath};
					const bool IsInAssetRegistry = AllAssets.ContainsByPredicate([&] (const FAssetData& Elem)
					{
						return Elem.ObjectPath.IsEqual(ObjectPathName);
					});
					if (!IsInAssetRegistry)
					{
						CorruptedAssets.Add(ObjectPathName);
					}
				}
				else
				{
					NonEngineFiles.Add(FName{FullPath});
				}
				
				AllFiles.Add(FName{FullPath});
			}

			return true;
		}
		TArray<FAssetData>& AllAssets;
		TSet<FName>& AllFiles;
		TSet<FName>& NonEngineFiles;
		TSet<FName>& EmptyFolders;
		TSet<FName>& CorruptedAssets;
	};

	ProjectCleanerRootDirVisitor Visitor{AllAssets, AllFiles, NonEngineFiles, EmptyFolders, CorruptedAssets};
	FPlatformFileManager::Get().GetPlatformFile().IterateDirectoryRecursively(*AbsoluteRootFolder.ToString(), Visitor);

	// finding empty folders trees
	EmptyFolders.Remove(FName{*GameUserDeveloperDir});
	EmptyFolders.Remove(FName{*GameUserDeveloperCollectionsDir});
	EmptyFolders.Remove(FName{*GameDevelopersDir});
	EmptyFolders.Remove(FName{*CollectionsDir});
	
	TSet<FName> NewEmptyFolders;
	for (const auto& EmptyFolder : EmptyFolders)
	{
		FString EmptyFolderPath = EmptyFolder.ToString();

		FString CurrentPath;
		EmptyFolderPath.Split(TEXT("/"), &CurrentPath, nullptr, ESearchCase::IgnoreCase, ESearchDir::FromEnd);
		CurrentPath += TEXT("/");

		while (!ProjectCleanerUtility::HasFiles(CurrentPath) || !CurrentPath.Equals(AbsoluteRootFolder.ToString()))
		{
			CurrentPath.RemoveFromEnd(TEXT("/"));
			NewEmptyFolders.Add(FName{*CurrentPath});
			
			FString Parent;
			CurrentPath.Split(TEXT("/"), &Parent, nullptr, ESearchCase::IgnoreCase, ESearchDir::FromEnd);
			CurrentPath = Parent;
			CurrentPath += TEXT("/");
		}
	}

	for (const auto& Folder : NewEmptyFolders)
	{
		EmptyFolders.Add(Folder);
	}
}

void ProjectCleanerDataManager::FindLinkedAssets(TSet<FName>& LinkedAssets)
{
	TArray<FName> Stack;
	for (const auto& Asset : IndirectlyUsedAssets)
	{
		FAssetNode Node;
		AssetRegistry->Get().GetReferencers(Asset.Key.PackageName, Node.Refs);
		AssetRegistry->Get().GetDependencies(Asset.Key.PackageName, Node.Deps);
	
		Node.Refs.Remove(Asset.Key.PackageName);
		Node.Deps.Remove(Asset.Key.PackageName);
	
		Stack.Add(Asset.Key.PackageName);
		
		TArray<FAssetIdentifier> AssetDependencies;
		TArray<FAssetIdentifier> AssetReferencers;
		
		while (Stack.Num() > 0)
		{
			const FName PackageName = Stack.Pop(false);
	
			AssetDependencies.Reset();
			AssetReferencers.Reset();
			AssetRegistry->Get().GetDependencies(FAssetIdentifier(PackageName), AssetDependencies);
			AssetRegistry->Get().GetReferencers(FAssetIdentifier(PackageName), AssetReferencers);
	
			AssetDependencies.RemoveAllSwap([&](const FAssetIdentifier& Elem)
			{
				return !Elem.PackageName.ToString().StartsWith(RelativeRootFolder.ToString());
			});
			AssetReferencers.RemoveAllSwap([&](const FAssetIdentifier& Elem)
			{
				return !Elem.PackageName.ToString().StartsWith(RelativeRootFolder.ToString());
			});
			
			for (const auto Dep : AssetDependencies)
			{
				bool bIsAlreadyInSet = false;
				LinkedAssets.Add(Dep.PackageName, &bIsAlreadyInSet);
				if (!bIsAlreadyInSet && Dep.IsValid())
				{
					Stack.Add(Dep.PackageName);
				}
			}
			for (const auto Ref : AssetReferencers)
			{
				bool bIsAlreadyInSet = false;
				LinkedAssets.Add(Ref.PackageName, &bIsAlreadyInSet);
				if (!bIsAlreadyInSet && Ref.IsValid())
				{
					Stack.Add(Ref.PackageName);
				}
			}
		}
	}
}
