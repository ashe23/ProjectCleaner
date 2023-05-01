// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Libs/PjcLibPath.h"

#include "PjcConstants.h"
#include "PjcTypes.h"
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

	if (Path.StartsWith(PjcConstants::PathRoot.ToString())) return Path;

	// Ensure that we are dealing with paths that are under the project directory
	if (!Path.StartsWith(FPaths::ProjectContentDir())) return {};

	return Path;
}

FString FPjcLibPath::ToAbsolute(const FString& InPath)
{
	const FString PathNormalized = Normalize(InPath);

	if (PathNormalized.IsEmpty()) return {};
	if (PathNormalized.StartsWith(FPaths::ProjectContentDir())) return PathNormalized;
	if (PathNormalized.StartsWith(PjcConstants::PathRoot.ToString()))
	{
		FString Path = PathNormalized;
		Path.RemoveFromStart(PjcConstants::PathRoot.ToString());

		return FPaths::ProjectContentDir().LeftChop(1) + Path;
	}

	return {};
}

FString FPjcLibPath::ToContentPath(const FString& InPath)
{
	const FString PathNormalized = Normalize(InPath);

	if (PathNormalized.IsEmpty()) return {};
	if (PathNormalized.StartsWith(PjcConstants::PathRoot.ToString())) return PathNormalized;
	if (PathNormalized.StartsWith(FPaths::ProjectContentDir()))
	{
		FString Path = PathNormalized;
		Path.RemoveFromStart(FPaths::ProjectContentDir());

		return PjcConstants::PathRoot.ToString() + Path;
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
	const FAssetRegistryModule& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(PjcConstants::ModuleAssetRegistry);
	const FName PackagePath = FName{*ToContentPath(InPath)};

	if (AssetRegistry.Get().HasAssets(PackagePath, true)) return false;

	// if not then checking by FileSystem
	TArray<FString> Files;
	IFileManager::Get().FindFilesRecursive(Files, *ToAbsolute(InPath), TEXT("*"), true, false);

	return Files.Num() == 0 && !InPath.StartsWith(PjcConstants::PathDevelopers.ToString()) && !InPath.StartsWith(PjcConstants::PathCollections.ToString());
}

bool FPjcLibPath::IsPathExcluded(const FString& InPath)
{
	const FString ContentPath = ToContentPath(InPath);

	const UPjcEditorAssetExcludeSettings* EditorAssetExcludeSettings = GetDefault<UPjcEditorAssetExcludeSettings>();
	if (!EditorAssetExcludeSettings) return false;

	for (const auto& ExcludedPath : EditorAssetExcludeSettings->ExcludedFolders)
	{
		if (ContentPath.StartsWith(ExcludedPath.Path))
		{
			return true;
		}
	}

	return false;
}
