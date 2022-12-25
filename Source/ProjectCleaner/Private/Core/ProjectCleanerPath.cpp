// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Core/ProjectCleanerPath.h"
#include "ProjectCleanerConstants.h"

FProjectCleanerPath::FProjectCleanerPath(const FString& InPath)
{
	if (InPath.IsEmpty()) return;

	// we must ensure that given path is under Content folder, anything outside we are not interested in
	// so given path must be in relative form : /Game/Folder/AnotherFolder
	// or in absolute form: {Path_To_Project_Content_Dir}/Folder/AnotherFolder
	const FString PathProjectContentDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("Content"));
	if (!InPath.StartsWith(ProjectCleanerConstants::PathRelRoot.ToString()) && !InPath.StartsWith(PathProjectContentDir)) return;

	// todo:ashe23 for ue5 we must also exclude __ExternalActors__ and __ExternalObject__ folders

	// normalizing given path
	FString FullPath = FPaths::ConvertRelativePathToFull(InPath);
	FPaths::RemoveDuplicateSlashes(FullPath);
	FPaths::NormalizeDirectoryName(FullPath);
	FullPath.RemoveFromEnd(TEXT("/"));

	// generating absolute and relative versions
	const FString PathNormalized = FullPath;
	const FString PathAbsolute = PathNormalized.Replace(
		*ProjectCleanerConstants::PathRelRoot.ToString(),
		*PathProjectContentDir,
		ESearchCase::CaseSensitive
	);
	const FString PathRelative = PathNormalized.Replace(
		*PathProjectContentDir,
		*ProjectCleanerConstants::PathRelRoot.ToString(),
		ESearchCase::CaseSensitive
	);

	// determining given path is directory or file and making sure in both cases file or directory exists
	FString PartPath;
	FString PartName;
	FString PartExt;
	FPaths::Split(PathAbsolute, PartPath, PartName, PartExt);

	if (PartExt.IsEmpty() && !FPaths::DirectoryExists(PathAbsolute)) return;
	if (!PartExt.IsEmpty() && !FPaths::FileExists(PathAbsolute)) return;

	// if everything ok , filling data
	PathAbs = PathAbsolute;
	PathRel = PathRelative;
	Name = PartName;
	Extension = PartExt;
}

bool FProjectCleanerPath::IsValid() const
{
	return PathAbs.IsEmpty() == false;
}

bool FProjectCleanerPath::IsDirectory() const
{
	return Extension.IsEmpty();
}

const FString& FProjectCleanerPath::GetPathAbs() const
{
	return PathAbs;
}

const FString& FProjectCleanerPath::GetPathRel() const
{
	return PathRel;
}

const FString& FProjectCleanerPath::GetExtension() const
{
	return Extension;
}

const FString& FProjectCleanerPath::GetName() const
{
	return Name;
}

bool FProjectCleanerPath::IsFile() const
{
	return Extension.IsEmpty() == false;
}
