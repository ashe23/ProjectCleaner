// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Libs/PjcLibPath.h"
#include "PjcConstants.h"

FString FPjcLibPath::ProjectDir()
{
	return FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()).LeftChop(1);
}

FString FPjcLibPath::ContentDir()
{
	return FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir()).LeftChop(1);
}

FString FPjcLibPath::SourceDir()
{
	return FPaths::ConvertRelativePathToFull(FPaths::GameSourceDir()).LeftChop(1);
}

FString FPjcLibPath::ConfigDir()
{
	return FPaths::ConvertRelativePathToFull(FPaths::ProjectConfigDir()).LeftChop(1);
}

FString FPjcLibPath::PluginsDir()
{
	return FPaths::ConvertRelativePathToFull(FPaths::ProjectPluginsDir()).LeftChop(1);
}

FString FPjcLibPath::SavedDir()
{
	return FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()).LeftChop(1);
}

FString FPjcLibPath::DevelopersDir()
{
	return FPaths::ConvertRelativePathToFull(FPaths::GameDevelopersDir()).LeftChop(1);
}

FString FPjcLibPath::CollectionsDir()
{
	return ContentDir() / TEXT("Collections");
}

FString FPjcLibPath::CurrentUserDevelopersDir()
{
	return FPaths::ConvertRelativePathToFull(FPaths::GameUserDeveloperDir()).LeftChop(1);
}

FString FPjcLibPath::CurrentUserCollectionsDir()
{
	return CurrentUserDevelopersDir() / TEXT("Collections");
}

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

	if (Path.StartsWith(PjcConstants::PathRelRoot.ToString())) return Path;

	// Ensure that we are dealing with paths that are under the project directory
	if (!Path.StartsWith(ProjectDir())) return {};

	return Path;
}

FString FPjcLibPath::ToAbsolute(const FString& InPath)
{
	const FString PathNormalized = Normalize(InPath);

	if (PathNormalized.IsEmpty()) return {};
	if (PathNormalized.StartsWith(ProjectDir())) return PathNormalized;
	if (PathNormalized.StartsWith(PjcConstants::PathRelRoot.ToString()))
	{
		FString Path = PathNormalized;
		Path.RemoveFromStart(PjcConstants::PathRelRoot.ToString());

		return ContentDir() + Path;
	}

	return {};
}
