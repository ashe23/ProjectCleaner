// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#include "ProjectCleanerUtility.h"
#include "UI/ProjectCleanerSourceCodeAssetsUI.h"
#include "UI/ProjectCleanerDirectoryExclusionUI.h"
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

void ProjectCleanerUtility::FindAllProjectFiles(TArray<FName>& AllProjectFiles)
{
	struct DirectoryVisitor : IPlatformFile::FDirectoryVisitor
	{
		DirectoryVisitor(TArray<FName>& Files) : AllFiles(Files) {}
		virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory) override
		{
			if(!bIsDirectory)
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

void ProjectCleanerUtility::FindInvalidProjectFiles(const FAssetRegistryModule* AssetRegistry, const TArray<FName>& AllProjectFiles, TSet<FName>& CorruptedFiles, TSet<FName>& NonUAssetFiles)
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
    		NonUAssetFiles.Add(FName{*ConvertRelativeToAbsolutePath(ProjectFile)});
    	}
    }
}

void ProjectCleanerUtility::FindAllPrimaryAssetClasses(UAssetManager& AssetManager, TSet<FName>& PrimaryAssetClasses)
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

bool ProjectCleanerUtility::HasFiles(const FString& SearchPath)
{
	TArray<FString> Files;
	IFileManager::Get().FindFiles(Files, *SearchPath, true, false);

	return Files.Num() > 0;
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

int32 ProjectCleanerUtility::GetEmptyFolders(TSet<FName>& EmptyFolders)
{
	FScopedSlowTask SlowTask{1.0f, FText::FromString("Searching empty folders...")};
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

void ProjectCleanerUtility::FindAllSourceFiles(TArray<FSourceCodeFile>& SourceFiles)
{
	TArray<FString> AllFiles;
	AllFiles.Reserve(200);

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

	// 1) finding all source files in main project "Source" directory (<yourproject>/Source/*)
	const auto ProjectSourceDir = FPaths::GameSourceDir();
	TArray<FString> ProjectSourceFiles;
	PlatformFile.FindFilesRecursively(ProjectSourceFiles, *ProjectSourceDir, TEXT(".cpp"));
	PlatformFile.FindFilesRecursively(ProjectSourceFiles, *ProjectSourceDir, TEXT(".h"));
	AllFiles.Append(ProjectSourceFiles);

	// 2) we should find all source files in plugins folder (<yourproject>/Plugins/*)
	// But we should include only "Source" directories in our scanning
	const auto ProjectPluginsDir = FPaths::ProjectPluginsDir();
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
	PlatformFile.IterateDirectory(*ProjectPluginsDir, Visitor);

	// for every installed plugin we scanning only "Source" directories
	for (const auto& Dir : Visitor.InstalledPlugins)
	{
		const FString PluginSourcePathDir = Dir + "/Source";
		PlatformFile.FindFilesRecursively(ProjectPluginsFiles, *PluginSourcePathDir, TEXT(".cpp"));
		PlatformFile.FindFilesRecursively(ProjectPluginsFiles, *PluginSourcePathDir, TEXT(".h"));
	}

	AllFiles.Append(ProjectPluginsFiles);

	SourceFiles.Reserve(AllFiles.Num());

	for (const auto& File : AllFiles)
	{
		if (PlatformFile.FileExists(*File))
		{
			FSourceCodeFile SourceCodeFile;
			SourceCodeFile.Name = FName{FPaths::GetCleanFilename(File)};
			SourceCodeFile.RelativeFilePath = File;
			SourceCodeFile.AbsoluteFilePath = FPaths::ConvertRelativePathToFull(File);
			FFileHelper::LoadFileToString(SourceCodeFile.Content, *File);
			SourceFiles.Add(SourceCodeFile);
		}
	}
}

void ProjectCleanerUtility::LoadSourceCodeFilesContent(TArray<FString>& AllSourceFiles,
														TArray<FString>& SourceCodeFilesContent)
{
	SourceCodeFilesContent.Reserve(AllSourceFiles.Num());

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	for (const auto& File : AllSourceFiles)
	{
		if (!PlatformFile.FileExists(*File)) continue;

		FString FileContent;
		FFileHelper::LoadFileToString(FileContent, *File);
		SourceCodeFilesContent.Add(FileContent);
	}
}

bool ProjectCleanerUtility::UsedInSourceFiles(const TArray<FString>& AllFiles, const FAssetData& Asset)
{
	for (const auto& File : AllFiles)
	{
		if (
			(File.Find(Asset.PackageName.ToString()) != -1) ||
			File.Find(Asset.PackagePath.ToString()) != -1
		)
		{
			return true;
		}
	}

	return false;
}

void ProjectCleanerUtility::GetAllAssets(TArray<FAssetData>& Assets)
{
	FAssetRegistryModule& AssetRegistry = FModuleManager::GetModuleChecked<FAssetRegistryModule>("AssetRegistry");
	AssetRegistry.Get().GetAssetsByPath(FName{"/Game"}, Assets, true);
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

void ProjectCleanerUtility::CreateAdjacencyList(TArray<FAssetData>& Assets, TArray<FNode>& List,
												const bool OnlyProjectFiles)
{
	if (Assets.Num() == 0) return;
	
	List.Reset();
	List.Reserve(Assets.Num());

	FAssetRegistryModule& AssetRegistry = FModuleManager::GetModuleChecked<FAssetRegistryModule>("AssetRegistry");
	TArray<FName> Deps;
	TArray<FName> Refs;
	for (const auto& Asset : Assets)
	{
		FNode Node;
		Node.Asset = Asset.PackageName;
		const auto AssetData = GetAssetData(Asset.PackageName, Assets);
		Node.AssetData = AssetData ? *AssetData : nullptr;
		AssetRegistry.Get().GetDependencies(Asset.PackageName, Deps);
		AssetRegistry.Get().GetReferencers(Asset.PackageName, Refs);

		// if flag given, removing all assets that are outside "Game" Folder
		if (OnlyProjectFiles)
		{
			for (const auto& Dep : Deps)
			{
				const auto AssetRef = Assets.FindByPredicate([&](const FAssetData& Elem)
				{
					return Elem.PackageName.IsEqual(Dep);
				});
				if (AssetRef && !AssetRef->PackageName.IsEqual(Asset.PackageName))
				{
					Node.LinkedAssets.Add(Dep);
					Node.Deps.Add(Dep);
				}
			}

			for (const auto& Ref : Refs)
			{
				const auto AssetRef = Assets.FindByPredicate([&](const FAssetData& Elem)
				{
					return Elem.PackageName.IsEqual(Ref);
				});
				if (AssetRef && !AssetRef->PackageName.IsEqual(Asset.PackageName))
				{
					Node.LinkedAssets.Add(Ref);
					Node.Refs.Add(Ref);
				}
			}
		}
		else
		{
			Node.LinkedAssets.Append(Deps);
			Node.LinkedAssets.Append(Refs);
			Node.Refs.Append(Refs);
			Node.Deps.Append(Deps);
		}
		List.Add(Node);
		Deps.Empty();
		Refs.Empty();
	}
}

void ProjectCleanerUtility::FindAllRelatedAssets(const FNode& Node,
												 TSet<FName>& RelatedAssets,
												 const TArray<FNode>& List)
{
	RelatedAssets.Add(Node.Asset);
	for (const auto& Adj : Node.LinkedAssets)
	{
		if (!RelatedAssets.Contains(Adj))
		{
			const FNode* NodeRef = List.FindByPredicate([&](const FNode& Elem)
			{
				return Elem.Asset == Adj;
			});

			if (NodeRef)
			{
				FindAllRelatedAssets(*NodeRef, RelatedAssets, List);
			}
		}
	}
}

void ProjectCleanerUtility::FindAllUassetFiles(TSet<FName>& UassetFiles, TSet<FName>& NonUassetFiles)
{
	struct FDirectoryVisitor : IPlatformFile::FDirectoryVisitor
	{
		FDirectoryVisitor(TSet<FName>& NonUFiles, TSet<FName>& UFiles) :
			NonUassetFiles(NonUFiles),
			UassetFiles(UFiles)
		{}
		virtual bool Visit(const TCHAR* FileName, bool bIsDirectory) override
		{
			if (!bIsDirectory)
			{
				if (IsEngineExtension(FPaths::GetExtension(FileName)))
				{
					UassetFiles.Add(FileName);					
				}
				else
				{
					NonUassetFiles.Add(FileName);
				}				
			}			
			
			return true;
		}
		
		TSet<FName>& NonUassetFiles;
		TSet<FName>& UassetFiles;
	};

	FDirectoryVisitor Visitor{NonUassetFiles, UassetFiles};
	IFileManager::Get().IterateDirectoryRecursively(*FPaths::ProjectContentDir(), Visitor);
}

void ProjectCleanerUtility::GetRootAssets(TArray<FAssetData>& RootAssets, TArray<FAssetData>& Assets,
                                          TArray<FNode>& List)
{
	// first we deleting cycle assets
	// like Skeletal mesh, skeleton, and physical assets
	// those assets must not be deleted separately
	TSet<FName> CircularAssets;
	for (const auto& Node : List)
	{
		if (Node.IsCircular())
		{
			CircularAssets.Add(Node.Asset);
		}
	}

	if (CircularAssets.Num() > 0)
	{
		for (const auto& CircularAsset : CircularAssets)
		{
			FAssetData* AssetData = GetAssetData(CircularAsset, Assets);
			if (!AssetData) continue;
			RootAssets.Add(*AssetData);
		}
	}
	else
	{
		constexpr int32 ChunkSize = 20;
		for (const auto& Node : List)
		{
			if (RootAssets.Num() > ChunkSize) break;

			if (Node.Refs.Num() == 0)
			{
				FAssetData* AssetData = GetAssetData(Node.Asset, Assets);
				if (!AssetData) continue;
				RootAssets.Add(*AssetData);
			}
		}
	}

	// todo:ashe23 not a good solution
	if (RootAssets.Num() == 0 && Assets.Num() != 0)
	{
		RootAssets = Assets;
	}
}

bool ProjectCleanerUtility::IsCycle(const FName& Referencer, const TArray<FName>& Deps, const FAssetData& CurrentAsset)
{
	return Deps.Contains(Referencer) && Referencer != CurrentAsset.PackageName;
}

FAssetData* ProjectCleanerUtility::GetAssetData(const FName& AssetName, TArray<FAssetData>& AssetContainer)
{
	return AssetContainer.FindByPredicate([&](const FAssetData& Val)
	{
		return Val.PackageName == AssetName;
	});
}

FAssetData* ProjectCleanerUtility::GetAssetData(const FName& AssetName, TArray<FNode>& List)
{
	const auto Node = List.FindByPredicate([&](const FNode& Elem)
	{
		return Elem.Asset.IsEqual(AssetName);
	});

	return (Node) ? &Node->AssetData : nullptr;
}

void ProjectCleanerUtility::GetLinkedAssetsChain(
	TSet<FName>& LinkedAssets,
	const TArray<FAssetData>& Assets,
	TArray<FNode>& List
)
{
	TSet<FName> RelatedAssets;
	LinkedAssets.Reserve(List.Num());
	RelatedAssets.Reserve(List.Num());

	for (const auto& Asset : Assets)
	{
		const auto Node = List.FindByPredicate([&](const FNode& Elem)
		{
			return Elem.Asset.IsEqual(Asset.PackageName);
		});
		if(!Node) continue;
		FindAllRelatedAssets(*Node,RelatedAssets, List);
		LinkedAssets.Append(RelatedAssets);
		RelatedAssets.Reset();
	}
}

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

bool ProjectCleanerUtility::IsEmptyDirectory(const FString& Path)
{
	TArray<FString> FilesAndDirs;
	IFileManager::Get().FindFiles(FilesAndDirs, *Path,true, true);

	return FilesAndDirs.Num() == 0;
}

bool ProjectCleanerUtility::IsEngineExtension(const FString& Extension)
{
	return Extension.Equals("uasset") || Extension.Equals("umap");
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

bool ProjectCleanerUtility::UsedInSourceFiles(
	const FAssetData& Asset,
	TArray<FSourceCodeFile>& SourceFiles,
	TArray<TWeakObjectPtr<USourceCodeAsset>>& SourceCodeFiles)
{
	for (const auto& File : SourceFiles)
	{
		//	todo:ashe23 BUG with similar names
		//	todo:ashe23 what about config files?
		
		// Wrapping in quotes AssetName => "AssetName"
		FString QuotedAssetName = Asset.AssetName.ToString();
		QuotedAssetName.InsertAt(0, TEXT("\""));
		QuotedAssetName.Append(TEXT("\""));
		
		if (
			File.Content.Contains(Asset.PackageName.ToString()) ||
			File.Content.Contains(QuotedAssetName)
		)
		{
			auto Obj = NewObject<USourceCodeAsset>();
			Obj->AssetName = Asset.AssetName.ToString();
			Obj->AssetPath = Asset.PackageName.ToString();
			Obj->SourceCodePath = File.AbsoluteFilePath;
			SourceCodeFiles.Add(Obj);
			return true;
		}
	}

	return false;
}

void ProjectCleanerUtility::CheckForCorruptedFiles(TArray<FAssetData>& Assets, TSet<FName>& UassetFiles, TSet<FName>& CorruptedFiles)
{
	CorruptedFiles.Reset();
	CorruptedFiles.Reserve(Assets.Num());
	
	TSet<FString> ConvertedAssets;
	ConvertedAssets.Reserve(Assets.Num());
	for(const auto& Asset : Assets)
	{
		ConvertedAssets.Add(ConvertRelativeToAbsolutePath(Asset.PackageName));
	}
	
	for (const auto& File : UassetFiles)
	{
		if (!ConvertedAssets.Contains(File.ToString()))
		{
			CorruptedFiles.Add(File);
		}
	}
}

void ProjectCleanerUtility::FindCorruptedFiles(
	const TArray<FAssetData>& RegistryAssets,
	const TSet<FName>& AllUassetFiles,
	TSet<FName>& CorruptedFiles
)
{
	// todo:ashe23 check asset engine version?
	TSet<FName> RelativeAssetPaths;
	RelativeAssetPaths.Reserve(AllUassetFiles.Num());
	for (const auto& UassetFile : AllUassetFiles)
	{
		// keeping only relative path without extension
		const auto Extension = FPaths::GetExtension(UassetFile.ToString(), true);
		auto PurePath = UassetFile.ToString();
		PurePath.RemoveFromEnd(Extension);
		RelativeAssetPaths.Add(ConvertAbsolutePathToRelative(FName{*PurePath}));
	}

	CorruptedFiles.Reserve(RelativeAssetPaths.Num());
	for (const auto& Path : RelativeAssetPaths)
	{
		const bool IsInList = RegistryAssets.ContainsByPredicate([&](const FAssetData& Elem)
		{
			return Elem.PackageName.IsEqual(Path);
		});
		if (!IsInList)
		{
			CorruptedFiles.Add(Path);
		}
	}	
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

void ProjectCleanerUtility::RemoveAssetsWithExternalDependencies(TArray<FAssetData>& Assets, TArray<FNode>& List)
{
	CreateAdjacencyList(Assets,List, false);
	
	TSet<FName> AssetsToRemove;
	AssetsToRemove.Reserve(Assets.Num());
	
	for(const auto& Node : List)
	{
		if (Node.HasLinkedAssetsOutsideGameFolder())
		{
			AssetsToRemove.Add(Node.Asset);	
		}
	}

	Assets.RemoveAll([&](const FAssetData& Asset)
	{
		return AssetsToRemove.Contains(Asset.PackageName);
	});
}

void ProjectCleanerUtility::RemoveAssetsUsedInSourceCode(
	TArray<FAssetData>& Assets,
	TArray<FNode>& List,
	TArray<FSourceCodeFile>& SourceCodeFiles,
	TArray<TWeakObjectPtr<USourceCodeAsset>>& SourceCodeAssets)
{
	CreateAdjacencyList(Assets, List, true);
	FindAllSourceFiles(SourceCodeFiles);

	TSet<FName> RelatedAssets;
	RelatedAssets.Reserve(Assets.Num());
	for (const auto& Asset : Assets)
	{
		if (UsedInSourceFiles(Asset, SourceCodeFiles, SourceCodeAssets))
		{
			const auto Node = List.FindByPredicate([&](const FNode& Elem)
			{
				return Elem.Asset.IsEqual(Asset.PackageName);
			});

			if (Node)
			{
				FindAllRelatedAssets(*Node,RelatedAssets, List);
			}
		}
	}

	Assets.RemoveAll([&](const FAssetData& Elem)
	{
		return RelatedAssets.Contains(Elem.PackageName);
	});
}

void ProjectCleanerUtility::RemoveAssetsExcludedByUser(
	TArray<FAssetData>& Assets,
	TArray<FNode>& List,
	UExcludeDirectoriesFilterSettings* DirectoryFilterSettings)
{
	if(!DirectoryFilterSettings) return;

	// updating adjacency list
	CreateAdjacencyList(Assets, List, true);

	FAssetRegistryModule& AssetRegistry = FModuleManager::GetModuleChecked<FAssetRegistryModule>("AssetRegistry");
	TSet<FAssetData> FilteredAssets;
	FilteredAssets.Reserve(Assets.Num());
	
	TArray<FAssetData> AssetsInPath;
	AssetsInPath.Reserve(100);
	for (const auto& FilterPath : DirectoryFilterSettings->Paths)
	{
		AssetRegistry.Get().GetAssetsByPath(*FilterPath.Path, AssetsInPath, true);
		FilteredAssets.Append(AssetsInPath);
		AssetsInPath.Reset();
	}
	
	// for filtered assets finding related assets
	TSet<FName> RelatedAssets;
	RelatedAssets.Reset();
	RelatedAssets.Reserve(Assets.Num());
	for(const auto& FilteredAsset : FilteredAssets)
	{
		const auto Node = List.FindByPredicate([&](const FNode& Elem)
		{
			return Elem.Asset.IsEqual(FilteredAsset.PackageName);
		});
		if (Node)
		{
			FindAllRelatedAssets(*Node, RelatedAssets, List);
		}
	}
	
	Assets.RemoveAll([&] (const FAssetData& Elem)
	{
		return RelatedAssets.Contains(Elem.PackageName);
	});
}
