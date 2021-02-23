#include "ProjectCleanerUtility.h"

// Engine Headers
#include "HAL/FileManager.h"
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
#include "FileHelpers.h"
#include "Misc/ScopedSlowTask.h"
#include "IContentBrowserSingleton.h"
#include "Editor/ContentBrowser/Public/ContentBrowserModule.h"

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
		if (GetAllEmptyDirectories(NewPath, Directories, NonProjectFiles, false))
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

int32 ProjectCleanerUtility::GetEmptyFoldersAndNonProjectFiles(TArray<FString>& EmptyFolders,
                                                               TArray<FString>& NonProjectFiles)
{
	FScopedSlowTask SlowTask{1.0f, FText::FromString("Searching empty folders...")};
	SlowTask.MakeDialog();

	EmptyFolders.Empty();
	NonProjectFiles.Empty();

	const auto ProjectRoot = FPaths::ProjectContentDir();
	GetAllEmptyDirectories(
		ProjectRoot / TEXT("*"),
		EmptyFolders,
		NonProjectFiles,
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

void ProjectCleanerUtility::CreateAdjacencyList(TArray<FAssetData>& Assets, TArray<FNode>& List)
{
	if (Assets.Num() == 0) return;

	FAssetRegistryModule& AssetRegistry = FModuleManager::GetModuleChecked<FAssetRegistryModule>("AssetRegistry");

	for (const auto& Asset : Assets)
	{
		FNode Node;
		Node.Asset = Asset.PackageName;
		TArray<FName> Deps;
		TArray<FName> Refs;
		AssetRegistry.Get().GetDependencies(Asset.PackageName, Deps);
		AssetRegistry.Get().GetReferencers(Asset.PackageName, Refs);

		for (const auto& Dep : Deps)
		{
			FAssetData* UnusedAsset = Assets.FindByPredicate([&](const FAssetData& Elem)
			{
				return Elem.PackageName == Dep;
			});
			if (UnusedAsset && UnusedAsset->PackageName != Asset.PackageName)
			{
				Node.AdjacentAssets.AddUnique(Dep);
			}
		}

		for (const auto& Ref : Refs)
		{
			FAssetData* UnusedAsset = Assets.FindByPredicate([&](const FAssetData& Elem)
			{
				return Elem.PackageName == Ref;
			});
			if (UnusedAsset && UnusedAsset->PackageName != Asset.PackageName)
			{
				Node.AdjacentAssets.AddUnique(Ref);
			}
		}

		List.Add(Node);
	}
}

void ProjectCleanerUtility::FindAllRelatedAssets(const FNode& Node,
                                                 TArray<FName>& RelatedAssets,
                                                 const TArray<FNode>& List)
{
	RelatedAssets.AddUnique(Node.Asset);
	for (const auto& Adj : Node.AdjacentAssets)
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
