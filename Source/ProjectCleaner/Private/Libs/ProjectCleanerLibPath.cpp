// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Libs/ProjectCleanerLibPath.h"
#include "ProjectCleanerConstants.h"
// Engine Headers
#include "AssetRegistry/AssetRegistryModule.h"
#include "Internationalization/Regex.h"

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

	const FString Path = Normalize(InPath);

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

bool UProjectCleanerLibPath::IsUnderFolder(const FString& InSearchFolderPath, const FString& InRootFolderPath)
{
	const FString SearchFolderPathAbs = Convert(InSearchFolderPath, EProjectCleanerPathType::Absolute);
	const FString RootFolderPathAbs = Convert(InRootFolderPath, EProjectCleanerPathType::Absolute);

	if (SearchFolderPathAbs.IsEmpty() || RootFolderPathAbs.IsEmpty()) return false;

	return SearchFolderPathAbs.Equals(RootFolderPathAbs) || FPaths::IsUnderDirectory(SearchFolderPathAbs, RootFolderPathAbs);
}

bool UProjectCleanerLibPath::IsUnderFolders(const FString& InSearchFolderPath, const TSet<FString>& Folders)
{
	if (InSearchFolderPath.IsEmpty() || Folders.Num() == 0) return false;

	for (const auto& Folder : Folders)
	{
		if (IsUnderFolder(InSearchFolderPath, Folder)) return true;
	}

	return false;
}

bool UProjectCleanerLibPath::FileHasEngineExtension(const FString& Extension)
{
	TSet<FString> EngineExtensions;
	EngineExtensions.Reserve(3);
	EngineExtensions.Add(TEXT("uasset"));
	EngineExtensions.Add(TEXT("umap"));
	EngineExtensions.Add(TEXT("collection"));

	return EngineExtensions.Contains(Extension.ToLower());
}

bool UProjectCleanerLibPath::FileIsCorrupted(const FString& InFilePathAbs)
{
	// here we got absolute path "C:/MyProject/Content/material.uasset"
	// we must first convert that path to In Engine Internal Path like "/Game/material.uasset"
	const FString RelativePath = Convert(InFilePathAbs, EProjectCleanerPathType::Relative);
	if (RelativePath.IsEmpty()) return false;

	// Converting file path to object path (This is for searching in AssetRegistry)
	// example "/Game/Name.uasset" => "/Game/Name.Name"
	FString ObjectPath = RelativePath;
	ObjectPath.RemoveFromEnd(FPaths::GetExtension(RelativePath, true));
	ObjectPath.Append(TEXT(".") + FPaths::GetBaseFilename(RelativePath));

	const FAssetRegistryModule& ModuleAssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);
	const FAssetData AssetData = ModuleAssetRegistry.Get().GetAssetByObjectPath(FName{*ObjectPath});

	// if its does not exist in asset registry, then something wrong with asset
	return !AssetData.IsValid();
}

bool UProjectCleanerLibPath::FileContainsIndirectAssets(const FString& FileContent)
{
	if (FileContent.IsEmpty()) return false;

	// search any sub string that has asset package path in it
	static FRegexPattern Pattern(TEXT(R"(\/Game([A-Za-z0-9_.\/]+)\b)"));
	FRegexMatcher Matcher(Pattern, FileContent);
	return Matcher.FindNext();
}

int64 UProjectCleanerLibPath::FilesGetTotalSize(const TArray<FString>& Files)
{
	if (Files.Num() == 0) return 0;

	int64 TotalSize = 0;
	for (const auto& File : Files)
	{
		if (File.IsEmpty() || !FPaths::FileExists(File)) continue;

		TotalSize += IFileManager::Get().FileSize(*File);
	}

	return TotalSize;
}

FString UProjectCleanerLibPath::Normalize(const FString& InPath)
{
	FString Path = InPath;
	FPaths::RemoveDuplicateSlashes(Path);
	FPaths::NormalizeDirectoryName(Path);

	return Path;
}
