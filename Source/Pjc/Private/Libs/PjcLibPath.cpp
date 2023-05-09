// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Libs/PjcLibPath.h"
#include "PjcConstants.h"
#include "PjcTypes.h"
#include "Libs/PjcLibAsset.h"
// Engine Headers
#include "AssetRegistry/AssetRegistryModule.h"

FString FPjcLibPath::Normalize(const FString& InPath)
{
	if (InPath.IsEmpty()) return {};

	// Ensure the path starts with a slash or a disk drive letter
	if (!(InPath.StartsWith(TEXT("/")) || InPath.StartsWith(TEXT("\\")) || (InPath.Len() > 2 && InPath[1] == ':')))
	{
		return {};
	}

	FString Path = FPaths::ConvertRelativePathToFull(InPath).TrimStartAndEnd();
	FPaths::RemoveDuplicateSlashes(Path);

	// Collapse any ".." or "." references in the path
	FPaths::CollapseRelativeDirectories(Path);

	if (FPaths::GetExtension(Path).IsEmpty())
	{
		FPaths::NormalizeDirectoryName(Path);
	}
	else
	{
		FPaths::NormalizeFilename(Path);
	}

	// Ensure the path does not end with a trailing slash
	if (Path.EndsWith(TEXT("/")) || Path.EndsWith(TEXT("\\")))
	{
		Path = Path.LeftChop(1);
	}

	// if (Path.StartsWith(PjcConstants::PathRoot.ToString())) return Path;
	//
	// // Ensure that we are dealing with paths that are under the project directory
	// if (!Path.StartsWith(ToFullPath(FPaths::ProjectContentDir()))) return {};

	return Path;
}

FString FPjcLibPath::ToAbsolute(const FString& InPath)
{
	const FString PathNormalized = Normalize(InPath);
	const FString PathProjectContent = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir());

	if (PathNormalized.IsEmpty()) return {};
	if (PathNormalized.StartsWith(PathProjectContent)) return PathNormalized;
	if (PathNormalized.StartsWith(PjcConstants::PathRoot.ToString()))
	{
		FString Path = PathNormalized;
		Path.RemoveFromStart(PjcConstants::PathRoot.ToString());

		return PathProjectContent / Path;
	}

	return {};
}

FString FPjcLibPath::ToContentPath(const FString& InPath)
{
	const FString PathNormalized = Normalize(InPath);
	const FString PathProjectContent = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir());

	if (PathNormalized.IsEmpty()) return {};
	if (PathNormalized.StartsWith(PjcConstants::PathRoot.ToString())) return PathNormalized;
	if (PathNormalized.StartsWith(PathProjectContent))
	{
		FString Path = PathNormalized;
		Path.RemoveFromStart(PathProjectContent);

		return PjcConstants::PathRoot.ToString() / Path;
	}

	return {};
}

FString FPjcLibPath::ToObjectPath(const FString& InPath)
{
	if (FPaths::FileExists(InPath))
	{
		const FString FileName = FPaths::GetBaseFilename(InPath);
		const FString AssetPath = ToContentPath(FPaths::GetPath(InPath));

		return FString::Printf(TEXT("%s/%s.%s"), *AssetPath, *FileName, *FileName);
	}

	const FString ObjectPath = FPackageName::ExportTextPathToObjectPath(InPath);

	if (!ObjectPath.StartsWith(PjcConstants::PathRoot.ToString())) return {};

	TArray<FString> Parts;
	ObjectPath.ParseIntoArray(Parts, TEXT("/"), true);

	if (Parts.Num() > 0)
	{
		FString Left;
		FString Right;
		Parts.Last().Split(TEXT("."), &Left, &Right);

		if (!Left.IsEmpty() && !Right.IsEmpty() && Left.Equals(*Right))
		{
			return ObjectPath;
		}
	}

	return {};
}

bool FPjcLibPath::IsPathEmpty(const FString& InPath)
{
	// first we check if given path contains any assets, because AssetRegistry keeps cached info
	const FAssetRegistryModule& AssetRegistry = FPjcLibAsset::GetAssetRegistry();
	const FName PackagePath = FName{*ToContentPath(InPath)};

	if (AssetRegistry.Get().HasAssets(PackagePath, true)) return false;

	// if not then checking by FileSystem
	TArray<FString> Files;
	IFileManager::Get().FindFilesRecursive(Files, *ToAbsolute(InPath), TEXT("*"), true, false);

	return Files.Num() == 0 && !InPath.StartsWith(PjcConstants::PathDevelopers.ToString()) && !InPath.StartsWith(PjcConstants::PathCollections.ToString());
}

bool FPjcLibPath::IsPathExcluded(const FString& InPath)
{
	// const FString ContentPath = ToContentPath(InPath);
	//
	// const UPjcEditorSettings* EditorSettings = GetDefault<UPjcEditorSettings>();
	// if (!EditorSettings) return false;
	//
	// for (const auto& ExcludedPath : EditorSettings->ExcludedFolders)
	// {
	// 	if (ContentPath.StartsWith(ExcludedPath.Path))
	// 	{
	// 		return true;
	// 	}
	// }

	return false;
}

void FPjcLibPath::GetFilesInPath(const FString& InSearchPath, const bool bSearchRecursive, TSet<FString>& OutFiles)
{
	OutFiles.Empty();

	struct FFindFilesVisitor : IPlatformFile::FDirectoryVisitor
	{
		TSet<FString>& Files;

		explicit FFindFilesVisitor(TSet<FString>& InFiles) : Files(InFiles) { }

		virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory) override
		{
			if (!bIsDirectory)
			{
				Files.Emplace(FPaths::ConvertRelativePathToFull(FilenameOrDirectory));
			}

			return true;
		}
	};

	FFindFilesVisitor FindFilesVisitor{OutFiles};

	if (bSearchRecursive)
	{
		FPlatformFileManager::Get().GetPlatformFile().IterateDirectoryRecursively(*InSearchPath, FindFilesVisitor);
	}
	else
	{
		FPlatformFileManager::Get().GetPlatformFile().IterateDirectory(*InSearchPath, FindFilesVisitor);
	}
}

void FPjcLibPath::GetFilesInPathByExt(const FString& InSearchPath, const bool bSearchRecursive, const bool bExtSearchInvert, const TSet<FString>& InExtensions, TSet<FString>& OutFiles)
{
	OutFiles.Empty();

	struct FFindFilesVisitor : IPlatformFile::FDirectoryVisitor
	{
		const bool bSearchInvert;
		TSet<FString>& Files;
		const TSet<FString>& Extensions;

		explicit FFindFilesVisitor(const bool bInSearchInvert, TSet<FString>& InFiles, const TSet<FString>& InExtensions)
			: bSearchInvert(bInSearchInvert),
			  Files(InFiles),
			  Extensions(InExtensions) { }

		virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory) override
		{
			if (!bIsDirectory)
			{
				const FString FullPath = FPaths::ConvertRelativePathToFull(FilenameOrDirectory);

				if (Extensions.Num() == 0)
				{
					Files.Emplace(FullPath);
					return true;
				}

				const FString Ext = FPaths::GetExtension(FullPath, false);
				const bool bExistsInSearchList = Extensions.Contains(Ext);

				if (
					bExistsInSearchList && !bSearchInvert ||
					!bExistsInSearchList && bSearchInvert
				)
				{
					Files.Emplace(FullPath);
				}
			}

			return true;
		}
	};

	TSet<FString> ExtensionsNormalized;
	ExtensionsNormalized.Reserve(InExtensions.Num());

	for (const auto& Ext : InExtensions)
	{
		const FString ExtNormalized = Ext.Replace(TEXT("."), TEXT(""));
		ExtensionsNormalized.Emplace(ExtNormalized);
	}

	FFindFilesVisitor FindFilesVisitor{bExtSearchInvert, OutFiles, ExtensionsNormalized};
	if (bSearchRecursive)
	{
		FPlatformFileManager::Get().GetPlatformFile().IterateDirectoryRecursively(*InSearchPath, FindFilesVisitor);
	}
	else
	{
		FPlatformFileManager::Get().GetPlatformFile().IterateDirectory(*InSearchPath, FindFilesVisitor);
	}
}

void FPjcLibPath::GetFoldersInPath(const FString& InSearchPath, const bool bSearchRecursive, TSet<FString>& OutFolders)
{
	OutFolders.Empty();

	struct FFindFoldersVisitor : IPlatformFile::FDirectoryVisitor
	{
		TSet<FString>& Folders;

		explicit FFindFoldersVisitor(TSet<FString>& InFolders) : Folders(InFolders) { }

		virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory) override
		{
			if (bIsDirectory)
			{
				Folders.Emplace(FPaths::ConvertRelativePathToFull(FilenameOrDirectory));
			}

			return true;
		}
	};

	FFindFoldersVisitor FindFoldersVisitor{OutFolders};
	if (bSearchRecursive)
	{
		FPlatformFileManager::Get().GetPlatformFile().IterateDirectoryRecursively(*InSearchPath, FindFoldersVisitor);
	}
	else
	{
		FPlatformFileManager::Get().GetPlatformFile().IterateDirectory(*InSearchPath, FindFoldersVisitor);
	}
}

int32 FPjcLibPath::DeleteFiles(const TSet<FString>& InFiles)
{
	int32 NumFilesDeleted = 0;

	for (const auto& File : InFiles)
	{
		const FString FilePathAbs = ToAbsolute(File);
		if (FilePathAbs.IsEmpty()) continue;
		if (!IFileManager::Get().Delete(*FilePathAbs, true)) continue;

		++NumFilesDeleted;
	}

	return NumFilesDeleted;
}

int32 FPjcLibPath::DeleteFolders(const TSet<FString>& InFolders)
{
	int32 NumFoldersDeleted = 0;

	for (const auto& Folder : InFolders)
	{
		const FString FolderPathAbs = ToAbsolute(Folder);
		if (FolderPathAbs.IsEmpty()) continue;
		if (!IFileManager::Get().DeleteDirectory(*FolderPathAbs, true, true)) continue;

		++NumFoldersDeleted;
	}

	return NumFoldersDeleted;
}
