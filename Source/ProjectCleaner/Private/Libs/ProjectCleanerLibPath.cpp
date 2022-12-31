// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Libs/ProjectCleanerLibPath.h"
#include "ProjectCleanerConstants.h"
#include "Settings/ProjectCleanerExcludeSettings.h"


FString UProjectCleanerLibPath::Normalize(const FString& InPath)
{
	if (InPath.IsEmpty()) return InPath;

	FString Path = FPaths::ConvertRelativePathToFull(InPath);
	FPaths::RemoveDuplicateSlashes(Path);

	if (FPaths::GetExtension(Path).IsEmpty())
	{
		FPaths::NormalizeDirectoryName(Path);
	}
	else
	{
		FPaths::NormalizeFilename(Path);
	}

	return Path;
}

FString UProjectCleanerLibPath::ConvertToAbs(const FString& InPath)
{
	const FString PathNormalized = Normalize(InPath);

	if (!PathIsUnderContentFolder(PathNormalized)) return {};

	return PathNormalized.Replace(
		*ProjectCleanerConstants::PathRelRoot.ToString(),
		*FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("Content")),
		ESearchCase::CaseSensitive
	);
}

FString UProjectCleanerLibPath::ConvertToRel(const FString& InPath)
{
	const FString PathNormalized = Normalize(InPath);

	if (!PathIsUnderContentFolder(PathNormalized)) return {};

	return PathNormalized.Replace(
		*FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("Content")),
		*ProjectCleanerConstants::PathRelRoot.ToString(),
		ESearchCase::CaseSensitive
	);
}

FString UProjectCleanerLibPath::GetContentFolder()
{
	return FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("Content"));
}

bool UProjectCleanerLibPath::FolderIsEmpty(const FString& InPath)
{
	if (InPath.IsEmpty()) return false;
	if (!FPaths::DirectoryExists(InPath)) return false;

	TArray<FString> Files;
	IFileManager::Get().FindFilesRecursive(Files, *InPath, TEXT("*.*"), true, false);

	return Files.Num() == 0;
}

bool UProjectCleanerLibPath::FolderIsExcluded(const FString& InPath)
{
	for (const auto& ExcludedFolder : GetDefault<UProjectCleanerExcludeSettings>()->ExcludedFolders)
	{
		if (FPaths::IsUnderDirectory(ConvertToAbs(InPath), ConvertToAbs(ExcludedFolder.Path)))
		{
			return true;
		}
	}

	return false;
}

bool UProjectCleanerLibPath::PathIsUnderContentFolder(const FString& InPath)
{
	const FString PathNormalized = Normalize(InPath);

	if (
		!InPath.StartsWith(ProjectCleanerConstants::PathRelRoot.ToString()) &&
		!InPath.StartsWith(FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("Content"))))
	{
		return false;
	}

	return true;
}
