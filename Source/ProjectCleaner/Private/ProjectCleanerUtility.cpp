// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#include "ProjectCleanerUtility.h"
#include "UI/ProjectCleanerSourceCodeAssetsUI.h"
#include "UI/ProjectCleanerDirectoryExclusionUI.h"
#include "Graph/AssetRelationalMap.h"
// Engine Headers
#include "ObjectTools.h"
#include "FileHelpers.h"
#include "AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "Engine/AssetManager.h"
#include "UObject/ObjectRedirector.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformFilemanager.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Misc/ScopedSlowTask.h"
#include "Engine/AssetManagerSettings.h"
#include "Engine/MapBuildDataRegistry.h"

void ProjectCleanerUtility::GetAllAssets(const FAssetRegistryModule* AssetRegistry, TArray<FAssetData>& Assets)
{
	if (!AssetRegistry) return;
	AssetRegistry->Get().GetAssetsByPath(FName{ "/Game" }, Assets, true);
}

void ProjectCleanerUtility::GetAllProjectFiles(TArray<FName>& AllProjectFiles)
{
	struct DirectoryVisitor : IPlatformFile::FDirectoryVisitor
	{
		DirectoryVisitor(TArray<FName>& Files) : AllFiles(Files) {}
		virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory) override
		{
			if (!bIsDirectory)
			{
				AllFiles.Add(ConvertAbsolutePathToRelative(FilenameOrDirectory));
			}
			return true;
		}

		TArray<FName>& AllFiles;
	};

	DirectoryVisitor Visitor{AllProjectFiles};
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	PlatformFile.IterateDirectoryRecursively(*FPaths::ProjectContentDir(), Visitor);
}

void ProjectCleanerUtility::GetInvalidProjectFiles(const FAssetRegistryModule* AssetRegistry, const TArray<FName>& AllProjectFiles, TSet<FName>& CorruptedFiles, TSet<FName>& NonUAssetFiles)
{
	if (!AssetRegistry) return;

	CorruptedFiles.Reserve(AllProjectFiles.Num());
	NonUAssetFiles.Reserve(AllProjectFiles.Num());
	
	for (const auto& ProjectFile : AllProjectFiles)
    {
    	auto FilePath = ProjectFile.ToString();
    	if (IsEngineExtension(FPaths::GetExtension(FilePath, false)))
    	{
    		// Converting file path to objectpath
    		// example "/Game/Name.uasset" => "/Game/Name.Name"
    		auto FileName = FPaths::GetBaseFilename(FilePath);
    		FilePath.RemoveFromEnd(FPaths::GetExtension(FilePath, true));
    		FilePath.Append(TEXT(".") + FileName);
    		const FName ObjectPath = FName{*FilePath};

    		// Trying to find that file in AssetRegistry
    		const auto AssetData = AssetRegistry->Get().GetAssetByObjectPath(ObjectPath);
    		// Adding to CorruptedFiles list, if we cant find it in AssetRegistry
    		if (AssetData.IsValid()) continue;
    		CorruptedFiles.Add(ProjectFile);
    	}
    	else
    	{
    		NonUAssetFiles.Add(ProjectFile);
    	}
    }
}

void ProjectCleanerUtility::GetAllPrimaryAssetClasses(UAssetManager& AssetManager, TSet<FName>& PrimaryAssetClasses)
{
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
}

void ProjectCleanerUtility::RemovePrimaryAssets(TArray<FAssetData>& UnusedAssets, TSet<FName>& PrimaryAssetClasses)
{
	UnusedAssets.RemoveAll([&](const FAssetData& Elem)
	{
		return
			PrimaryAssetClasses.Contains(Elem.AssetClass) ||
			Elem.AssetClass.IsEqual(UMapBuildDataRegistry::StaticClass()->GetFName());
	});
}

int32 ProjectCleanerUtility::GetEmptyFolders(TSet<FName>& EmptyFolders)
{
	FScopedSlowTask SlowTask{ 1.0f, FText::FromString("Searching empty folders...") };
	SlowTask.MakeDialog();

	const auto ProjectRoot = FPaths::ProjectContentDir();
	GetAllEmptyDirectories(
		ProjectRoot / TEXT("*"),
		EmptyFolders,
		true
	);

	SlowTask.EnterProgressFrame(1.0f);

	return EmptyFolders.Num();
}

bool ProjectCleanerUtility::GetAllEmptyDirectories(const FString& SearchPath,
	TSet<FName>& Directories,
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
			Directories.Add(*NewPath);
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

void ProjectCleanerUtility::RemoveMegascansPluginAssetsIfActive(TArray<FAssetData>& UnusedAssets)
{
	const bool IsMegascansLoaded = FModuleManager::Get().IsModuleLoaded("MegascansPlugin");
	if (!IsMegascansLoaded) return;
	
	UnusedAssets.RemoveAll([&](const FAssetData& Elem)
	{
		return FPaths::IsUnderDirectory(Elem.PackagePath.ToString(), TEXT("/Game/MSPresets"));
	});
}

void ProjectCleanerUtility::RemoveAssetsUsedIndirectly(
	TArray<FAssetData>& UnusedAssets,
	AssetRelationalMap& RelationalMap,
	TArray<TWeakObjectPtr<USourceCodeAsset>>& SourceCodeAssets)
{
	// assets used indirectly are those used in source code files or config files
	
	// 1) find all source and config files
	TArray<FString> AllFiles;
	AllFiles.Reserve(200);

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

	// 2) finding all source files in main project "Source" directory (<yourproject>/Source/*)
	TArray<FString> FilesToScan;
	PlatformFile.FindFilesRecursively(FilesToScan, *FPaths::GameSourceDir(), TEXT(".cs"));
	PlatformFile.FindFilesRecursively(FilesToScan, *FPaths::GameSourceDir(), TEXT(".cpp"));
	PlatformFile.FindFilesRecursively(FilesToScan, *FPaths::GameSourceDir(), TEXT(".h"));
	PlatformFile.FindFilesRecursively(FilesToScan, *FPaths::ProjectConfigDir(), TEXT(".ini"));
	AllFiles.Append(FilesToScan);

	// 3) we should find all source files in plugins folder (<yourproject>/Plugins/*)
	// But we should include only "Source" directories in our scanning
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

	// 4) for every installed plugin we scanning only "Source" directories
	for (const auto& Dir : Visitor.InstalledPlugins)
	{
		const FString PluginSourcePathDir = Dir + "/Source";
		const FString PluginConfigPathDir = Dir + "/Config";
		
		PlatformFile.FindFilesRecursively(ProjectPluginsFiles, *PluginSourcePathDir, TEXT(".cs"));
		PlatformFile.FindFilesRecursively(ProjectPluginsFiles, *PluginSourcePathDir, TEXT(".cpp"));
		PlatformFile.FindFilesRecursively(ProjectPluginsFiles, *PluginSourcePathDir, TEXT(".h"));
		PlatformFile.FindFilesRecursively(ProjectPluginsFiles, *PluginConfigPathDir, TEXT(".ini"));
	}

	AllFiles.Append(ProjectPluginsFiles);

	// 5) loading file contents
	TArray<FSourceCodeFile> SourceCodeFiles;
	SourceCodeFiles.Reserve(AllFiles.Num());

	for (const auto& File : AllFiles)
	{
		if (PlatformFile.FileExists(*File))
		{
			FSourceCodeFile SourceCodeFile;
			SourceCodeFile.Name = FName{ FPaths::GetCleanFilename(File) };
			SourceCodeFile.AbsoluteFilePath = File;
			FFileHelper::LoadFileToString(SourceCodeFile.Content, *File);
			SourceCodeFiles.Add(SourceCodeFile);
		}
	}

	// 6) parsing files and checking if assets used there
	TSet<FName> FoundedAssets;
	FoundedAssets.Reserve(UnusedAssets.Num());
	
	for (const auto& UnusedAsset : UnusedAssets)
	{
		const auto File = GetFileWhereAssetUsed(UnusedAsset, SourceCodeFiles);
		if(!File) continue;

		FoundedAssets.Add(UnusedAsset.PackageName);
		
		auto Obj = NewObject<USourceCodeAsset>();
		Obj->AssetName = UnusedAsset.AssetName.ToString();
		Obj->AssetPath = UnusedAsset.PackageName.ToString();
		Obj->SourceCodePath = File->AbsoluteFilePath;
		SourceCodeAssets.Add(Obj);
	}

	TSet<FName> FilteredAssets;
	FilteredAssets.Reserve(UnusedAssets.Num());
	
	// 7) for founded assets find all linked assets
	for (const auto& FoundedAsset : FoundedAssets)
	{
		const auto AssetNode = RelationalMap.FindByPackageName(FoundedAsset);
		if (!AssetNode) continue;

		FilteredAssets.Add(FoundedAsset);
		for (const auto& LinkedAsset : AssetNode->LinkedAssetsData)
		{
			FilteredAssets.Add(LinkedAsset->PackageName);
		}
	}
	
	// 8) remove founded assets from unused assets list
	UnusedAssets.RemoveAll([&](const FAssetData& Elem)
	{
		return FilteredAssets.Contains(Elem.PackageName);
	});
}

void ProjectCleanerUtility::RemoveAssetsExcludedByUser(
	const FAssetRegistryModule* AssetRegistry,
	TArray<FAssetData>& UnusedAssets,
	TSet<FAssetData>& ExcludedAssets,
	AssetRelationalMap& RelationalMap,
	const UExcludeDirectoriesFilterSettings* DirectoryFilterSettings)
{
	if (!AssetRegistry) return;
	if (!DirectoryFilterSettings) return;

	// todo:ashe23 maybe excluded assets should be keep old assets on refresh
	TArray<FAssetData> FilteredAssets;
	FilteredAssets.Reserve(UnusedAssets.Num());
	
	TArray<FAssetData> IterationAssets;
	for (const auto& FilterPath : DirectoryFilterSettings->Paths)
	{
		AssetRegistry->Get().GetAssetsByPath(
			FName{ *FilterPath.Path },
			IterationAssets,
			true
		);
		FilteredAssets.Append(IterationAssets);
		IterationAssets.Reset();
	}

	for (const auto& FilteredAsset : FilteredAssets)
	{
		ExcludedAssets.Add(FilteredAsset);
		
		const auto Node = RelationalMap.FindByPackageName(FilteredAsset.PackageName);
		if (!Node) continue;
		for (const auto& LinkedAsset : Node->LinkedAssetsData)
		{
			ExcludedAssets.Add(*LinkedAsset);
		}
	}

	UnusedAssets.RemoveAll([&](const FAssetData& Elem)
	{
		return ExcludedAssets.Contains(Elem);
	});
}

FName ProjectCleanerUtility::ConvertRelativeToAbsPath(const FName& InPath)
{
	return ConvertPath(InPath, TEXT("/Game/"), *FPaths::ProjectContentDir());
}

FName ProjectCleanerUtility::ConvertAbsToRelativePath(const FName& InPath)
{
	return ConvertPath(InPath, *FPaths::ProjectContentDir(), TEXT("/Game/"));
}

FName ProjectCleanerUtility::ConvertPath(FName Path, const FName& From, const FName& To)
{
	FString ConvertedPath = Path.ToString();
	FPaths::NormalizeFilename(ConvertedPath);
	
	const auto Result = ConvertedPath.Replace(
		*From.ToString(),
		*To.ToString()
	);
	return FName{ *Result };
}

bool ProjectCleanerUtility::HasFiles(const FString& SearchPath)
{
	TArray<FString> Files;
	IFileManager::Get().FindFiles(Files, *SearchPath, true, false);

	return Files.Num() > 0;
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

void ProjectCleanerUtility::DeleteEmptyFolders(TSet<FName>& EmptyFolders)
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::GetModuleChecked<FAssetRegistryModule>("AssetRegistry");
	for (auto& EmptyFolder : EmptyFolders)
	{
		if (IFileManager::Get().DirectoryExists(*EmptyFolder.ToString()))
		{
			IFileManager::Get().DeleteDirectory(*EmptyFolder.ToString(), false, true);
			auto DirPath = EmptyFolder.ToString().Replace(*FPaths::ProjectContentDir(), TEXT("/Game/"));
			AssetRegistryModule.Get().RemovePath(DirPath);
		}
	}
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
	// Filter.ClassNames.Emplace(TEXT("ObjectRedirector"));
	Filter.ClassNames.Emplace(UObjectRedirector::StaticClass()->GetFName());

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

//void ProjectCleanerUtility::GetRootAssets(TArray<FAssetData>& RootAssets, TArray<FAssetData>& Assets,
//                                          TArray<FNode>& List)
//{
//	// first we deleting cycle assets
//	// like Skeletal mesh, skeleton, and physical assets
//	// those assets must not be deleted separately
//	// TSet<FName> CircularAssets;
//	// for (const auto& Node : List)
//	// {
//	// 	if (Node.IsCircular())
//	// 	{
//	// 		CircularAssets.Add(Node.Asset);
//	// 	}
//	// }
//	//
//	// if (CircularAssets.Num() > 0)
//	// {
//	// 	for (const auto& CircularAsset : CircularAssets)
//	// 	{
//	// 		FAssetData* AssetData = GetAssetData(CircularAsset, Assets);
//	// 		if (!AssetData) continue;
//	// 		RootAssets.Add(*AssetData);
//	// 	}
//	// }
//	// else
//	// {
//	// 	constexpr int32 ChunkSize = 20;
//	// 	for (const auto& Node : List)
//	// 	{
//	// 		if (RootAssets.Num() > ChunkSize) break;
//	//
//	// 		if (Node.Refs.Num() == 0)
//	// 		{
//	// 			FAssetData* AssetData = GetAssetData(Node.Asset, Assets);
//	// 			if (!AssetData) continue;
//	// 			RootAssets.Add(*AssetData);
//	// 		}
//	// 	}
//	// }
//	//
//	// // todo:ashe23 not a good solution
//	// if (RootAssets.Num() == 0 && Assets.Num() != 0)
//	// {
//	// 	RootAssets = Assets;
//	// }
//}

int64 ProjectCleanerUtility::GetTotalSize(const TArray<FAssetData>& AssetContainer)
{
	int64 Size = 0;
	for (const auto& Asset : AssetContainer)
	{
		FAssetRegistryModule& AssetRegistry = FModuleManager::GetModuleChecked<FAssetRegistryModule>("AssetRegistry");
		const auto AssetPackageData = AssetRegistry.Get().GetAssetPackageData(Asset.PackageName);
		if (!AssetPackageData) continue;
		Size += AssetPackageData->DiskSize;
	}

	return Size;
}

bool ProjectCleanerUtility::IsEngineExtension(const FString& Extension)
{
	return Extension.Equals("uasset") || Extension.Equals("umap");
}

const FSourceCodeFile* ProjectCleanerUtility::GetFileWhereAssetUsed(const FAssetData& Asset, const TArray<FSourceCodeFile>& SourceCodeFiles)
{
	for (const auto& File : SourceCodeFiles)
	{
		//	todo:ashe23 BUG with similar names

		// Wrapping in quotes AssetName => "AssetName"
		FString QuotedAssetName = Asset.AssetName.ToString();
		QuotedAssetName.InsertAt(0, TEXT("\""));
		QuotedAssetName.Append(TEXT("\""));

		if (
			File.Content.Contains(Asset.PackageName.ToString()) ||
			File.Content.Contains(QuotedAssetName)
			)
		{
			return &File;
		}
	}

	return nullptr;
}


FString ProjectCleanerUtility::ConvertRelativeToAbsolutePath(const FName& PackageName)
{	
	FString PackageFileName;
	FString PackageFile;
	if (
		FPackageName::TryConvertLongPackageNameToFilename(PackageName.ToString(), PackageFileName) &&
		FPackageName::FindPackageFileWithoutExtension(PackageFileName, PackageFile)
	)
	{
		return FPaths::ConvertRelativePathToFull(PackageFile);
	}

	return FString{};
}

FName ProjectCleanerUtility::ConvertAbsolutePathToRelative(const FName& InPath)
{
	FString Path = InPath.ToString();
	FPaths::NormalizeFilename(Path);
	const auto Result = Path.Replace(*FPaths::ProjectContentDir(), TEXT("/Game/"));
	return FName{*Result};
}

void ProjectCleanerUtility::RemoveUsedAssets(TArray<FAssetData>& Assets, const TSet<FName>& PrimaryAssetClasses)
{
	FAssetRegistryModule& AssetRegistry = FModuleManager::GetModuleChecked<FAssetRegistryModule>("AssetRegistry");
	
	FARFilter Filter;
	Filter.ClassNames.Add(UWorld::StaticClass()->GetFName());
	for (const auto& AssetClass : PrimaryAssetClasses)
	{
		Filter.ClassNames.Add(AssetClass);
	}
	Filter.PackagePaths.Add("/Game");
	Filter.bRecursivePaths = true;

	TSet<FName> UsedAssets;
	UsedAssets.Reserve(Assets.Num());
	TArray<FName> PackageNamesToProcess;
	{
		TArray<FAssetData> FoundAssets;
		AssetRegistry.Get().GetAssets(Filter, FoundAssets);
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
		AssetRegistry.Get().GetDependencies(FAssetIdentifier(PackageName), AssetDependencies);
		for (const FAssetIdentifier& Dependency : AssetDependencies)
		{
			bool bIsAlreadyInSet = false;
			UsedAssets.Add(Dependency.PackageName, &bIsAlreadyInSet);
			if (bIsAlreadyInSet == false && Dependency.IsValid())
			{
				PackageNamesToProcess.Add(Dependency.PackageName);
			}
		}
	}

	Assets.RemoveAll([&](const FAssetData& Elem)
	{
		return UsedAssets.Contains(Elem.PackageName);
	});
}

//void ProjectCleanerUtility::RemoveAssetsWithExternalDependencies(TArray<FAssetData>& Assets, TArray<FNode>& List)
//{
//	// CreateAdjacencyList(Assets,List, false);
//	//
//	// TSet<FName> AssetsToRemove;
//	// AssetsToRemove.Reserve(Assets.Num());
//	//
//	// for(const auto& Node : List)
//	// {
//	// 	if (Node.HasLinkedAssetsOutsideGameFolder())
//	// 	{
//	// 		AssetsToRemove.Add(Node.Asset);	
//	// 	}
//	// }
//	//
//	// Assets.RemoveAll([&](const FAssetData& Asset)
//	// {
//	// 	return AssetsToRemove.Contains(Asset.PackageName);
//	// });
//}

//void ProjectCleanerUtility::RemoveAssetsExcludedByUser(
//	TArray<FAssetData>& Assets,
//	TArray<FNode>& List,
//	UExcludeDirectoriesFilterSettings* DirectoryFilterSettings)
//{
//	// if(!DirectoryFilterSettings) return;
//	//
//	// // updating adjacency list
//	// CreateAdjacencyList(Assets, List, true);
//	//
//	// FAssetRegistryModule& AssetRegistry = FModuleManager::GetModuleChecked<FAssetRegistryModule>("AssetRegistry");
//	// TSet<FAssetData> FilteredAssets;
//	// FilteredAssets.Reserve(Assets.Num());
//	//
//	// TArray<FAssetData> AssetsInPath;
//	// AssetsInPath.Reserve(100);
//	// for (const auto& FilterPath : DirectoryFilterSettings->Paths)
//	// {
//	// 	AssetRegistry.Get().GetAssetsByPath(*FilterPath.Path, AssetsInPath, true);
//	// 	FilteredAssets.Append(AssetsInPath);
//	// 	AssetsInPath.Reset();
//	// }
//	//
//	// // for filtered assets finding related assets
//	// TSet<FName> RelatedAssets;
//	// RelatedAssets.Reset();
//	// RelatedAssets.Reserve(Assets.Num());
//	// for(const auto& FilteredAsset : FilteredAssets)
//	// {
//	// 	const auto Node = List.FindByPredicate([&](const FNode& Elem)
//	// 	{
//	// 		return Elem.Asset.IsEqual(FilteredAsset.PackageName);
//	// 	});
//	// 	if (Node)
//	// 	{
//	// 		FindAllRelatedAssets(*Node, RelatedAssets, List);
//	// 	}
//	// }
//	//
//	// Assets.RemoveAll([&] (const FAssetData& Elem)
//	// {
//	// 	return RelatedAssets.Contains(Elem.PackageName);
//	// });
//}
