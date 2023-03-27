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

FString FPjcLibPath::ToAssetPath(const FString& InPath)
{
	const FString PathNormalized = Normalize(InPath);

	if (PathNormalized.IsEmpty()) return {};
	if (PathNormalized.StartsWith(PjcConstants::PathRelRoot.ToString())) return PathNormalized;
	if (PathNormalized.StartsWith(ContentDir()))
	{
		FString Path = PathNormalized;
		Path.RemoveFromStart(ContentDir());

		return PjcConstants::PathRelRoot.ToString() + Path;
	}

	return {};
}

FName FPjcLibPath::ToObjectPath(const FString& InPath)
{
	const FString ObjectPath = FPackageName::ExportTextPathToObjectPath(InPath);

	if (!ObjectPath.StartsWith(PjcConstants::PathRelRoot.ToString())) return NAME_None;

	TArray<FString> Parts;
	ObjectPath.ParseIntoArray(Parts, TEXT("/"), true);

	if (Parts.Num() > 0)
	{
		FString Left;
		FString Right;
		Parts.Last().Split(TEXT("."), &Left, &Right);

		if (!Left.IsEmpty() && !Right.IsEmpty() && Left.Equals(*Right))
		{
			return FName{*ObjectPath};
		}
	}

	return NAME_None;
}

FString FPjcLibPath::GetFilePath(const FString& InPath)
{
	if (!IsFile(InPath)) return {};

	return FPaths::GetPath(Normalize(InPath));
}

FString FPjcLibPath::GetPathName(const FString& InPath)
{
	const FString Path = IsDir(InPath) ? Normalize(InPath) : GetFilePath(InPath);
	if (Path.IsEmpty()) return {};

	TArray<FString> Parts;
	Path.ParseIntoArray(Parts,TEXT("/"), true);

	return Parts.Num() > 0 ? Parts.Last() : TEXT("");
}

FString FPjcLibPath::GetFileExtension(const FString& InPath, const bool bIncludeDot)
{
	return FPaths::GetExtension(InPath, bIncludeDot);
}

bool FPjcLibPath::IsValid(const FString& InPath)
{
	return IsFile(InPath) ? FPaths::FileExists(Normalize(InPath)) : FPaths::DirectoryExists(Normalize(InPath));
}

bool FPjcLibPath::IsFile(const FString& InPath)
{
	return !GetFileExtension(InPath, false).IsEmpty();
}

bool FPjcLibPath::IsDir(const FString& InPath)
{
	return !IsFile(InPath);
}

int64 FPjcLibPath::GetFileSize(const FString& InPath)
{
	return IsValid(InPath) ? IFileManager::Get().FileSize(*Normalize(InPath)) : 0;
}

int64 FPjcLibPath::GetFilesSize(const TArray<FString>& InPaths)
{
	int64 Size = 0;

	for (const auto& InPath : InPaths)
	{
		if (!IsValid(InPath)) continue;

		Size += IFileManager::Get().FileSize(*Normalize(InPath));
	}

	return Size;
}
