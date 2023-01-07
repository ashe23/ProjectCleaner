// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Libs/ProjectCleanerLibPath.h"
#include "ProjectCleanerConstants.h"

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
		*GetFolderContent(),
		ESearchCase::CaseSensitive
	);
}

FString UProjectCleanerLibPath::ConvertToRel(const FString& InPath)
{
	const FString PathNormalized = Normalize(InPath);

	if (!PathIsUnderContentFolder(PathNormalized)) return {};

	return PathNormalized.Replace(
		*GetFolderContent(),
		*ProjectCleanerConstants::PathRelRoot.ToString(),
		ESearchCase::CaseSensitive
	);
}

FString UProjectCleanerLibPath::GetFolderContent()
{
	return FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("Content"));
}

FString UProjectCleanerLibPath::GetFolderDevelopers()
{
	return FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() / TEXT("Developers"));
}

FString UProjectCleanerLibPath::GetFolderCollections()
{
	return FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() / TEXT("Collections"));
}

FString UProjectCleanerLibPath::GetFolderDevelopersUser()
{
	return FPaths::ConvertRelativePathToFull(FPaths::GameDevelopersDir() / FPaths::GameUserDeveloperFolderName());
}

FString UProjectCleanerLibPath::GetFolderCollectionsUser()
{
	return FPaths::ConvertRelativePathToFull(FPaths::GameUserDeveloperDir() / TEXT("Collections"));
}

bool UProjectCleanerLibPath::PathIsUnderContentFolder(const FString& InPath)
{
	const FString PathNormalized = Normalize(InPath);

	if (
		!InPath.StartsWith(ProjectCleanerConstants::PathRelRoot.ToString()) &&
		!InPath.StartsWith(GetFolderContent())
	)
	{
		return false;
	}

	return true;
}
