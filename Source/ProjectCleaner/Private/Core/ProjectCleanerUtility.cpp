// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#include "Core/ProjectCleanerUtility.h"
#include "StructsContainer.h"
#include "ProjectCleaner.h"
// Engine Headers
#include "ObjectTools.h"
#include "FileHelpers.h"
#include "AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "Engine/AssetManager.h"
#include "UObject/ObjectRedirector.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Misc/ScopedSlowTask.h"
#include "Engine/AssetManagerSettings.h"
#include "Engine/MapBuildDataRegistry.h"

void ProjectCleanerUtility::GetEmptyFolders(TArray<FString>& EmptyFolders, const bool bScanDeveloperContents)
{
	FindAllEmptyFolders(FPaths::ProjectContentDir() / TEXT("*"), EmptyFolders);

	if (bScanDeveloperContents)
	{
		EmptyFolders.RemoveAllSwap([&](const FString& Elem) {
			return
				Elem.Equals(FPaths::GameUserDeveloperDir()) ||
				Elem.Equals(FPaths::GameUserDeveloperDir() + TEXT("Collections/")) ||
				Elem.Equals(FPaths::ProjectContentDir() + TEXT("Collections/")) ||
				Elem.Equals(FPaths::GameDevelopersDir());
			});
	}
	else
	{
		EmptyFolders.RemoveAllSwap([&](const FString& Elem)
			{
				return
					Elem.StartsWith(FPaths::GameDevelopersDir()) ||
					Elem.StartsWith(FPaths::ProjectContentDir() + TEXT("Collections/"));
			});
	}
}

void ProjectCleanerUtility::GetInvalidFiles(TSet<FString>& CorruptedFiles, TSet<FString>& NonEngineFiles)
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::GetModuleChecked<FAssetRegistryModule>("AssetRegistry");
	
	TSet<FString> ProjectFilesFromDisk;
	GetProjectFilesFromDisk(ProjectFilesFromDisk);
	
	CorruptedFiles.Reserve(ProjectFilesFromDisk.Num());
	NonEngineFiles.Reserve(ProjectFilesFromDisk.Num());

	for (const auto& ProjectFile : ProjectFilesFromDisk)
	{
		if (IsEngineExtension(FPaths::GetExtension(ProjectFile, false)))
		{
			// here we got absolute path "C:/MyProject/Content/material.uasset"
			// we must first convert that path to In Engine Internal Path like "/Game/material.uasset"
			const FString InternalFilePath = ConvertAbsolutePathToInternal(ProjectFile);
			// Converting file path to object path (This is for searching in AssetRegistry)
			// example "/Game/Name.uasset" => "/Game/Name.Name"
			FString ObjectPath = InternalFilePath;
			ObjectPath.RemoveFromEnd(FPaths::GetExtension(InternalFilePath, true));
			ObjectPath.Append(TEXT(".") + FPaths::GetBaseFilename(InternalFilePath));

			// Trying to find that file in AssetRegistry
			const FAssetData AssetData = AssetRegistryModule.Get().GetAssetByObjectPath(FName{ *ObjectPath });
			// Adding to CorruptedFiles list, if we cant find it in AssetRegistry
			if (AssetData.IsValid()) continue;
			CorruptedFiles.Add(ProjectFile);
		}
		else
		{
			NonEngineFiles.Add(ProjectFile);
		}
	}
}

void ProjectCleanerUtility::GetCorruptedAssets(FProjectCleanerData& CleanerData)
{
	// CleanerData.CorruptedAssets.Reserve(CleanerData.ProjectAllAssetsFiles.Num());
	// for (const auto& File : CleanerData.ProjectAllAssetsFiles)
	// {
	// 	if (!IsEngineExtension(FPaths::GetExtension(File, false))) continue;
	// 	
	// 	// here we got absolute path "C:/MyProject/Content/material.uasset"
	// 	// we must first convert that path to In Engine Internal Path like "/Game/material.uasset"
	// 	const FString InternalFilePath = ConvertAbsolutePathToInternal(File);
	// 	// Converting file path to object path (This is for searching in AssetRegistry)
	// 	// example "/Game/Name.uasset" => "/Game/Name.Name"
	// 	FString ObjectPath = InternalFilePath;
	// 	ObjectPath.RemoveFromEnd(FPaths::GetExtension(InternalFilePath, true));
	// 	ObjectPath.Append(TEXT(".") + FPaths::GetBaseFilename(InternalFilePath));
	// 	
	// 	const bool IsInAssetRegistry = CleanerData.ProjectAllAssets.ContainsByPredicate([&](const FAssetData& Elem)
	// 	{
	// 		return Elem.ObjectPath.IsEqual(FName{ObjectPath});
	// 	});
	// 	if (!IsInAssetRegistry)
	// 	{
	// 		CleanerData.CorruptedAssets.Add(File);
	// 	}
	// }
	//
	// CleanerData.CorruptedAssets.Shrink();
}

void ProjectCleanerUtility::GetNonEngineFiles(TSet<FString>& NonEngineFiles,const TSet<FString>& ProjectFiles)
{
	for (const auto& File : ProjectFiles)
	{
		if (!IsEngineExtension(FPaths::GetExtension(File, false)))
		{
			NonEngineFiles.Add(File);
		}
	}
}

void ProjectCleanerUtility::GetAllAssets(FProjectCleanerData& CleanerData)
{
	// CleanerData.ProjectAllAssets.Reserve(CleanerData.ProjectAllAssetsFiles.Num());
	// FAssetRegistryModule& AssetRegistry = FModuleManager::GetModuleChecked<FAssetRegistryModule>("AssetRegistry");
	// AssetRegistry.Get().GetAssetsByPath(FName{ "/Game" }, CleanerData.ProjectAllAssets, true);
}

void ProjectCleanerUtility::GetUnusedAssets(FProjectCleanerData& CleanerData)
{
	// getting all used assets
	TSet<FName> UsedAssets;
	UsedAssets.Reserve(CleanerData.ProjectAllAssets.Num());
	GetUsedAssets(UsedAssets);
	
	CleanerData.UnusedAssets.Reserve(CleanerData.ProjectAllAssets.Num());
	for (const auto& Asset : CleanerData.ProjectAllAssets)
	{
		if (UsedAssets.Contains(Asset.PackageName) || !Asset.PackagePath.ToString().StartsWith("/Game")) continue;
		CleanerData.UnusedAssets.AddUnique(Asset);
	}

	// todo:ashe23 make as option?
	const bool IsMegascansLoaded = FModuleManager::Get().IsModuleLoaded("MegascansPlugin");
	if (!IsMegascansLoaded) return;
	
	CleanerData.UnusedAssets.RemoveAllSwap([&](const FAssetData& Elem)
	{
		return FPaths::IsUnderDirectory(Elem.PackagePath.ToString(), TEXT("/Game/MSPresets"));
	}, false);
	CleanerData.UnusedAssets.Shrink();
}

int64 ProjectCleanerUtility::GetTotalSize(const TArray<FAssetData>& Assets)
{
	int64 Size = 0;
	for (const auto& Asset : Assets)
	{
		FAssetRegistryModule& AssetRegistry = FModuleManager::GetModuleChecked<FAssetRegistryModule>("AssetRegistry");
		const auto AssetPackageData = AssetRegistry.Get().GetAssetPackageData(Asset.PackageName);
		if (!AssetPackageData) continue;
		Size += AssetPackageData->DiskSize;
	}

	return Size;
}

FString ProjectCleanerUtility::ConvertAbsolutePathToInternal(const FString& InPath)
{
	FString Path = InPath;
	FPaths::NormalizeFilename(Path);
	const FString ProjectContentDirAbsPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir());
	return ConvertPathInternal(ProjectContentDirAbsPath, FString{ "/Game/" }, Path);
}

FString ProjectCleanerUtility::ConvertInternalToAbsolutePath(const FString& InPath)
{
	FString Path = InPath;
	FPaths::NormalizeFilename(Path);
	const FString ProjectContentDirAbsPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir());
	return ConvertPathInternal(FString{ "/Game/" }, ProjectContentDirAbsPath, Path);
}

bool ProjectCleanerUtility::DeleteEmptyFolders(TArray<FString>& EmptyFolders)
{
	if (EmptyFolders.Num() == 0) return false;

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::GetModuleChecked<FAssetRegistryModule>("AssetRegistry");
	
	bool ErrorWhileDeleting = false;
	for (auto& EmptyFolder : EmptyFolders)
	{
		if (!IFileManager::Get().DirectoryExists(*EmptyFolder)) continue;

		if (!IFileManager::Get().DeleteDirectory(*EmptyFolder, false, true))
		{
			ErrorWhileDeleting = true;
			UE_LOG(LogProjectCleaner, Error, TEXT("Failed to delete %s folder."), *EmptyFolder);
			continue;
		}

		auto FolderPath = EmptyFolder.Replace(*FPaths::ProjectContentDir(), TEXT("/Game/"));

		// removing folder path from asset registry
		AssetRegistryModule.Get().RemovePath(FolderPath);
	}

	if (!ErrorWhileDeleting)
	{
		EmptyFolders.Empty();
	}

	return !ErrorWhileDeleting;
}

void ProjectCleanerUtility::GetUsedAssets(TSet<FName>& UsedAssets)
{
	FAssetRegistryModule& AssetRegistry = FModuleManager::GetModuleChecked<FAssetRegistryModule>("AssetRegistry");
	TSet<FName> PrimaryAssetClasses;
	GetPrimaryAssetClasses(PrimaryAssetClasses);
	
	FARFilter Filter;
	Filter.ClassNames.Add(UWorld::StaticClass()->GetFName());
	Filter.ClassNames.Append(PrimaryAssetClasses.Array());
	Filter.PackagePaths.Add("/Game");
	Filter.bRecursivePaths = true;
	Filter.bRecursiveClasses = true;
	
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
			if (!bIsAlreadyInSet && Dependency.IsValid())
			{
				PackageNamesToProcess.Add(Dependency.PackageName);
			}
		}
	}
}

// Private
void ProjectCleanerUtility::GetPrimaryAssetClasses(TSet<FName>& PrimaryAssetClasses)
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
}

void ProjectCleanerUtility::GetProjectFilesFromDisk(TSet<FString>& ProjectFiles)
{
	struct DirectoryVisitor : IPlatformFile::FDirectoryVisitor
	{
		DirectoryVisitor(TSet<FString>& Files) : AllFiles(Files) {}
		virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory) override
		{
			if (!bIsDirectory)
			{
				AllFiles.Add(FPaths::ConvertRelativePathToFull(FilenameOrDirectory));
			}

			return true;
		}

		TSet<FString>& AllFiles;
	};

	DirectoryVisitor Visitor{ ProjectFiles };
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	PlatformFile.IterateDirectoryRecursively(*FPaths::ProjectContentDir(), Visitor);
}

// void ProjectCleanerUtility::FindAssetsUsedIndirectly(const TArray<FAssetData>& UnusedAssets, TArray<FIndirectFileInfo>& IndirectFileInfos)
// {
// 	/* YourProjectDirectory
// 		Config +
// 		Content
// 		Plugins
// 			SomePlugin
// 				Binaries
// 				Config +
// 				Intermediate
// 				Resources
// 				Source +
// 			SomeOtherPlugin
// 				...
// 		Intermediate
// 		Saved
// 		Script
// 		Source +
// 	*/
// 	// So we scanning only folders marked with +
// 	
// 	// 1) Find files with this extensions(.cpp, .h, .cs, .ini)
// 	// 2) for every unused asset check if that assets used in that file
// 	//	2.1) if so add to Indirectly used assets
// 	TArray<FString> AllFiles;
// 	GetSourceAndConfigFiles(AllFiles);
// 	
// 	if (AllFiles.Num() == 0) return;
//
// 	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
// 	for (const auto& File : AllFiles)
// 	{
// 		if (!PlatformFile.FileExists(*File)) continue;
//
// 		TArray<FString> Lines;
// 		FFileHelper::LoadFileToStringArray(Lines, *File);
//
// 		for (int32 LineNum = 0; LineNum < Lines.Num(); ++LineNum)
// 		{
// 			if (!Lines.IsValidIndex(LineNum)) continue;
//
// 			for (const auto& UnusedAsset : UnusedAssets)
// 			{
// 				FString QuotedAssetName = UnusedAsset.AssetName.ToString();
// 				QuotedAssetName.InsertAt(0, TEXT("\""));
// 				QuotedAssetName.Append(TEXT("\""));
// 				if (Lines[LineNum].Contains(UnusedAsset.PackageName.ToString()) || Lines[LineNum].Contains(QuotedAssetName))
// 				{
// 					FIndirectFileInfo Info;
// 					Info.AssetData = UnusedAsset;
// 					Info.FileName = FPaths::GetCleanFilename(File);
// 					Info.FilePath = FPaths::ConvertRelativePathToFull(File);
// 					Info.LineNum = LineNum + 1;
// 					
// 					IndirectFileInfos.Add(Info);
// 				}
// 			}
// 		}
// 	}
// }

bool ProjectCleanerUtility::IsEngineExtension(const FString& Extension)
{
	return Extension.Equals("uasset") || Extension.Equals("umap");
}

bool ProjectCleanerUtility::FindAllEmptyFolders(const FString& FolderPath, TArray<FString>& EmptyFolders)
{
	bool IsSubFoldersEmpty = true;
	TArray<FString> SubFolders;
	IFileManager::Get().FindFiles(SubFolders, *FolderPath, false, true);

	for (const auto& SubFolder : SubFolders)
	{
		// "*" needed for unreal`s IFileManager class, without it , its not working.
		auto NewPath = FolderPath;
		NewPath.RemoveFromEnd(TEXT("*"));
		NewPath += SubFolder / TEXT("*");
		if (FindAllEmptyFolders(NewPath, EmptyFolders))
		{
			NewPath.RemoveFromEnd(TEXT("*"));
			EmptyFolders.AddUnique(*NewPath);
		}
		else
		{
			IsSubFoldersEmpty = false;
		}
	}

	TArray<FString> FilesInFolder;
	IFileManager::Get().FindFiles(FilesInFolder, *FolderPath, true, false);

	if (IsSubFoldersEmpty && FilesInFolder.Num() == 0)
	{
		return true;
	}

	return false;
}

bool ProjectCleanerUtility::IsUnderDeveloperFolder(const FString& PackagePath)
{
	const FString InPath = ConvertInternalToAbsolutePath(PackagePath);
	const FString DeveloperFolderPath = FPaths::ConvertRelativePathToFull(FPaths::GameDevelopersDir());
	const FString CollectionsFolderPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() + TEXT("Collections/"));

	return
		FPaths::IsUnderDirectory(InPath, DeveloperFolderPath) ||
		FPaths::IsUnderDirectory(InPath, CollectionsFolderPath);
}

FString ProjectCleanerUtility::ConvertPathInternal(const FString& From, const FString To, const FString& Path)
{
	return Path.Replace(*From, *To, ESearchCase::IgnoreCase);
}

// void ProjectCleanerUtility::RemoveMegascansPluginAssetsIfActive(TArray<FAssetData>& UnusedAssets)
// {
// 	const bool IsMegascansLoaded = FModuleManager::Get().IsModuleLoaded("MegascansPlugin");
// 	if (!IsMegascansLoaded) return;
// 	
// 	UnusedAssets.RemoveAllSwap([&](const FAssetData& Elem)
// 	{
// 		return FPaths::IsUnderDirectory(Elem.PackagePath.ToString(), TEXT("/Game/MSPresets"));
// 	});
// }

void ProjectCleanerUtility::GetSourceAndConfigFiles(TArray<FString>& AllFiles)
{
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

	// 1) find all source and config files
	AllFiles.Reserve(200); // reserving some space

	// 2) finding all source files in main project "Source" directory (<yourproject>/Source/*)
	TArray<FString> FilesToScan;
	PlatformFile.FindFilesRecursively(FilesToScan, *FPaths::GameSourceDir(), TEXT(".cs"));
	PlatformFile.FindFilesRecursively(FilesToScan, *FPaths::GameSourceDir(), TEXT(".cpp"));
	PlatformFile.FindFilesRecursively(FilesToScan, *FPaths::GameSourceDir(), TEXT(".h"));
	PlatformFile.FindFilesRecursively(FilesToScan, *FPaths::ProjectConfigDir(), TEXT(".ini"));
	AllFiles.Append(FilesToScan);

	// 3) we should find all source files in plugins folder (<yourproject>/Plugins/*)
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

	// 4) for every installed plugin we scanning only "Source" and "Config" folders
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
	AllFiles.Shrink();
}

void ProjectCleanerUtility::FixupRedirectors()
{
	FScopedSlowTask SlowTask{1.0f, FText::FromString("Fixing up Redirectors...")};
	SlowTask.MakeDialog();

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::GetModuleChecked<FAssetRegistryModule>("AssetRegistry");

	FARFilter Filter;
	Filter.bRecursivePaths = true;
	Filter.PackagePaths.Emplace(TEXT("/Game"));
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
			const auto AssetObj = Asset.GetAsset();
			if (!AssetObj) continue;
			Objects.Add(AssetObj);
		}

		// converting them to redirectors
		TArray<UObjectRedirector*> Redirectors;
		for (auto Object : Objects)
		{
			const auto Redirector = CastChecked<UObjectRedirector>(Object);
			if (!Redirector) continue;
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
			const auto AssetObj = Asset.GetAsset();
			if(!AssetObj) continue;
			AssetObjects.Add(AssetObj);
		}

		DeletedAssets = ObjectTools::ForceDeleteObjects(AssetObjects, false);
	}

	return DeletedAssets;
}

void ProjectCleanerUtility::SaveAllAssets(const bool PromptUser = true)
{
	FEditorFileUtils::SaveDirtyPackages(
		PromptUser,
		true,
		true,
		false,
		false,
		false
	);
}

void ProjectCleanerUtility::UpdateAssetRegistry(bool bSyncScan = false)
{
	FAssetRegistryModule& AssetRegistry = FModuleManager::GetModuleChecked<FAssetRegistryModule>("AssetRegistry");
	
	TArray<FString> ScanFolders;
	ScanFolders.Add("/Game");

	AssetRegistry.Get().ScanPathsSynchronous(ScanFolders, true);
	AssetRegistry.Get().SearchAllAssets(bSyncScan);
}