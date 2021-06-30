// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#include "ProjectCleanerHelper.h"
#include "StructsContainer.h"

// Engine Headers
#include "AssetRegistryModule.h"
#include "Misc/FileHelper.h"
#include "Hal/FileManager.h"
#include "Hal/FileManagerGeneric.h"

//void ProjectCleanerHelper::GetEmptyFolders(TArray<FString>& EmptyFolders, const bool bScanDeveloperContents)
//{
//	FindAllEmptyFolders(FPaths::ProjectContentDir() / TEXT("*"), EmptyFolders);
//
//	if (bScanDeveloperContents)
//	{
//		EmptyFolders.RemoveAllSwap([&](const FString& Elem) {
//			return
//				Elem.Equals(FPaths::GameUserDeveloperDir()) ||
//				Elem.Equals(FPaths::GameUserDeveloperDir() + TEXT("Collections/")) ||
//				Elem.Equals(FPaths::ProjectContentDir() + TEXT("Collections/")) ||
//				Elem.Equals(FPaths::GameDevelopersDir());
//		});
//	}
//	else
//	{
//		EmptyFolders.RemoveAllSwap([&] (const FString& Elem)
//		{
//			return
//				Elem.StartsWith(FPaths::GameDevelopersDir()) ||
//				Elem.StartsWith(FPaths::ProjectContentDir() + TEXT("Collections/"));
//		});
//	}
//}

//void ProjectCleanerHelper::GetProjectFilesFromDisk(TSet<FString>& ProjectFiles)
//{
//	struct DirectoryVisitor : IPlatformFile::FDirectoryVisitor
//	{
//		DirectoryVisitor(TSet<FString>& Files) : AllFiles(Files) {}
//		virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory) override
//		{
//			if (!bIsDirectory)
//			{
//				AllFiles.Add(FPaths::ConvertRelativePathToFull(FilenameOrDirectory));
//			}
//			
//			return true;
//		}
//
//		TSet<FString>& AllFiles;
//	};
//
//	DirectoryVisitor Visitor{ ProjectFiles };
//	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
//	PlatformFile.IterateDirectoryRecursively(*FPaths::ProjectContentDir(), Visitor);
//}

void ProjectCleanerHelper::GetSourceCodeFilesFromDisk(TArray<FSourceCodeFile>& SourceCodeFiles)
{
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

	// 1) find all source and config files
	TArray<FString> AllFiles;
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

	// 5) loading file contents
	SourceCodeFiles.Reserve(AllFiles.Num());

	for (const auto& File : AllFiles)
	{
		if (PlatformFile.FileExists(*File))
		{
			FSourceCodeFile SourceCodeFile;
			SourceCodeFile.Name = FName{ FPaths::GetCleanFilename(File) };
			SourceCodeFile.AbsoluteFilePath = FPaths::ConvertRelativePathToFull(File);
			FFileHelper::LoadFileToString(SourceCodeFile.Content, *File);
			SourceCodeFiles.Add(SourceCodeFile);
		}
	}
}

//FString ProjectCleanerHelper::ConvertAbsolutePathToInternal(const FString& InPath)
//{
//	FString Path = InPath;
//	FPaths::NormalizeFilename(Path);
//	const FString ProjectContentDirAbsPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir());
//	return ConvertPathInternal(ProjectContentDirAbsPath, FString{ "/Game/" }, Path);
//}
//
//FString ProjectCleanerHelper::ConvertInternalToAbsolutePath(const FString& InPath)
//{
//	FString Path = InPath;
//	FPaths::NormalizeFilename(Path);
//	const FString ProjectContentDirAbsPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir());
//	return ConvertPathInternal(FString{ "/Game/" }, ProjectContentDirAbsPath, Path);
//}

//bool ProjectCleanerHelper::DeleteEmptyFolders(const FAssetRegistryModule* AssetRegistry, TArray<FString>& EmptyFolders)
//{
//	if (EmptyFolders.Num() == 0) return false;
//
//	bool ErrorWhileDeleting = false;
//	for (auto& EmptyFolder : EmptyFolders)
//	{
//		if (!IFileManager::Get().DirectoryExists(*EmptyFolder)) continue;
//
//		if (!IFileManager::Get().DeleteDirectory(*EmptyFolder, false, true))
//		{
//			ErrorWhileDeleting = true;
//			UE_LOG(LogTemp, Error, TEXT("Failed to delete %s folder."), *EmptyFolder);
//			continue;
//		}
//
//		auto FolderPath = EmptyFolder.Replace(*FPaths::ProjectContentDir(), TEXT("/Game/"));
//
//		// removing folder path from asset registry
//		if (!AssetRegistry) continue;
//		AssetRegistry->Get().RemovePath(FolderPath);
//	}
//
//	if (!ErrorWhileDeleting)
//	{
//		EmptyFolders.Empty();
//	}
//
//	return !ErrorWhileDeleting;
//}

// private
//bool ProjectCleanerHelper::FindAllEmptyFolders(const FString& FolderPath, TArray<FString>& EmptyFolders)
//{
//	bool IsSubFoldersEmpty = true;
//	TArray<FString> SubFolders;
//	IFileManager::Get().FindFiles(SubFolders, *FolderPath, false, true);
//
//	for (const auto& SubFolder : SubFolders)
//	{
//		// "*" needed for unreal`s IFileManager class, without it , its not working.
//		auto NewPath = FolderPath;
//		NewPath.RemoveFromEnd(TEXT("*"));
//		NewPath += SubFolder / TEXT("*");
//		if (FindAllEmptyFolders(NewPath, EmptyFolders))
//		{
//			NewPath.RemoveFromEnd(TEXT("*"));
//			EmptyFolders.AddUnique(*NewPath);
//		}
//		else
//		{
//			IsSubFoldersEmpty = false;
//		}
//	}
//
//	TArray<FString> FilesInFolder;
//	IFileManager::Get().FindFiles(FilesInFolder, *FolderPath, true, false);
//
//	if (IsSubFoldersEmpty && FilesInFolder.Num() == 0)
//	{
//		return true;
//	}
//
//	return false;
//}

//FString ProjectCleanerHelper::ConvertPathInternal(const FString& From, const FString To, const FString& Path)
//{
//	return Path.Replace(*From, *To, ESearchCase::IgnoreCase);
//}
