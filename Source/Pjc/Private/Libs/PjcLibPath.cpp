// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Libs/PjcLibPath.h"
#include "PjcConstants.h"

TOptional<FString> FPjcLibPath::Normalize(const FString& InPath)
{
	if (InPath.IsEmpty()) return TOptional<FString>{};

	// Ensure the path starts with a slash or a disk drive letter
	if (!(InPath.StartsWith(TEXT("/")) || InPath.StartsWith(TEXT("\\")) || (InPath.Len() > 2 && InPath[1] == ':')))
	{
		return TOptional<FString>{};
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

	if (Path.StartsWith(PjcConstants::PathRelRoot.ToString())) return TOptional<FString>{Path};

	// Ensure that we are dealing with paths that are under the project directory
	const FString ProjectPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());
	if (!Path.StartsWith(ProjectPath)) return TOptional<FString>{}; // Return an empty string if the path is outside the project directory

	return TOptional<FString>{Path};
}
