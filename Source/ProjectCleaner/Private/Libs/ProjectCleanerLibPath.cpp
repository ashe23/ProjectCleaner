// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Libs/ProjectCleanerLibPath.h"
#include "ProjectCleanerConstants.h"

FString UProjectCleanerLibPath::FolderContent(const EProjectCleanerPathType PathType)
{
	// {project_dir}/Content

	if (PathType == EProjectCleanerPathType::Absolute)
	{
		return FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / ProjectCleanerConstants::FolderContent.ToString());
	}

	return ProjectCleanerConstants::PathRelRoot.ToString();
}

FString UProjectCleanerLibPath::FolderDevelopers(const EProjectCleanerPathType PathType)
{
	// {project_dir}/Content/Developers

	if (PathType == EProjectCleanerPathType::Absolute)
	{
		return FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() / ProjectCleanerConstants::FolderDevelopers.ToString());
	}

	return ProjectCleanerConstants::PathRelDevelopers.ToString();
}

FString UProjectCleanerLibPath::FolderDeveloper(const EProjectCleanerPathType PathType)
{
	// {project_dir}/Content/Developers/{current_user}

	if (PathType == EProjectCleanerPathType::Absolute)
	{
		return FPaths::ConvertRelativePathToFull(FolderDevelopers(PathType) / FPaths::GameUserDeveloperFolderName());
	}

	return FString::Printf(TEXT("%s/%s"), *ProjectCleanerConstants::PathRelDevelopers.ToString(), *FPaths::GameUserDeveloperFolderName());
}

FString UProjectCleanerLibPath::FolderCollections(const EProjectCleanerPathType PathType)
{
	// {project_dir}/Content/Collections

	if (PathType == EProjectCleanerPathType::Absolute)
	{
		return FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() / ProjectCleanerConstants::FolderCollections.ToString());
	}

	return ProjectCleanerConstants::PathRelCollections.ToString();
}

FString UProjectCleanerLibPath::FolderDeveloperCollections(const EProjectCleanerPathType PathType)
{
	// {project_dir}/Content/Developers/{current_user}/Collections

	if (PathType == EProjectCleanerPathType::Absolute)
	{
		return FPaths::ConvertRelativePathToFull(FolderDeveloper(PathType) / ProjectCleanerConstants::FolderCollections.ToString());
	}

	return FString::Printf(TEXT("%s/%s/%s"), *ProjectCleanerConstants::PathRelDevelopers.ToString(), *FPaths::GameUserDeveloperFolderName(), *ProjectCleanerConstants::FolderCollections.ToString());
}

FString UProjectCleanerLibPath::FolderMsPresets(const EProjectCleanerPathType PathType)
{
	// {project_dir}/Content/MSPresets

	if (PathType == EProjectCleanerPathType::Absolute)
	{
		return FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() / ProjectCleanerConstants::FolderMsPresets.ToString());
	}

	return ProjectCleanerConstants::PathRelMegascansPresets.ToString();
}

FString UProjectCleanerLibPath::Convert(const FString& InPath, const EProjectCleanerPathType ToPathType)
{
	if (InPath.IsEmpty())
	{
		return {};
	}

	FString Path = InPath;
	FPaths::RemoveDuplicateSlashes(Path);
	FPaths::NormalizeDirectoryName(Path);

	if (
		!Path.StartsWith(ProjectCleanerConstants::PathRelRoot.ToString()) &&
		!Path.StartsWith(FolderContent(EProjectCleanerPathType::Absolute)))
	{
		return {};
	}

	const FString From = ToPathType == EProjectCleanerPathType::Absolute ? ProjectCleanerConstants::PathRelRoot.ToString() : FolderContent(EProjectCleanerPathType::Absolute);
	const FString To = ToPathType == EProjectCleanerPathType::Absolute ? FolderContent(EProjectCleanerPathType::Absolute) : ProjectCleanerConstants::PathRelRoot.ToString();

	return Path.Replace(*From, *To, ESearchCase::CaseSensitive);
}
