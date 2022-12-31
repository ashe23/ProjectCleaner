// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Libs/ProjectCleanerLibPath.h"
#include "ProjectCleanerConstants.h"
#include "Settings/ProjectCleanerExcludeSettings.h"
// Engine Headers
#include "AssetRegistry/AssetRegistryModule.h"

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

bool UProjectCleanerLibPath::FileIsCorrupted(const FString& FilePathAbs)
{
	if (!FPaths::FileExists(FilePathAbs)) return false;

	const FString RelativePath = UProjectCleanerLibPath::ConvertToRel(FilePathAbs);

	// here we got absolute path "C:/MyProject/Content/material.uasset"
	// we must first convert that path to In Engine Internal Path like "/Game/material.uasset"
	// const FString RelativePath = Convert(InFilePathAbs, EProjectCleanerPathType::Relative);
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

bool UProjectCleanerLibPath::FileHasEngineExtension(const FString& FilePathAbs)
{
	const FString Extension = FPaths::GetExtension(FilePathAbs).ToLower();

	TSet<FString> EngineExtensions;
	EngineExtensions.Reserve(3);
	EngineExtensions.Add(TEXT("uasset"));
	EngineExtensions.Add(TEXT("umap"));
	EngineExtensions.Add(TEXT("collection"));

	return EngineExtensions.Contains(Extension);
}

bool UProjectCleanerLibPath::FolderIsEngineGenerated(const FString& InPath)
{
	TSet<FString> Folders;
	Folders.Reserve(4);
	Folders.Add(GetFolderCollections());
	Folders.Add(GetFolderDevelopers());
	Folders.Add(GetFolderDevelopersUser());
	Folders.Add(GetFolderCollectionsUser());

	return Folders.Contains(InPath);
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
