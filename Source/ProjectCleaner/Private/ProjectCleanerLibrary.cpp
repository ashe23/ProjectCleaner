// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "ProjectCleaner/Public/ProjectCleanerLibrary.h"
// Engine Headers
#include "AssetRegistry/AssetRegistryModule.h"

class FFindEmptyDirectoriesVisitor final : public IPlatformFile::FDirectoryVisitor
{
public:
	IPlatformFile& PlatformFile;
	FRWLock FoundFilesLock;
	TSet<FString>& EmptyDirectories;
	const TSet<FString>& ExcludedDirectories;
public:
	FFindEmptyDirectoriesVisitor(IPlatformFile& InPlatformFile, TSet<FString>& InEmptyDirectories, const TSet<FString>& InExcludedDirectories)
		: FDirectoryVisitor(EDirectoryVisitorFlags::ThreadSafe)
		  , PlatformFile(InPlatformFile)
		  , EmptyDirectories(InEmptyDirectories)
		  , ExcludedDirectories(InExcludedDirectories)
	{
	}

	virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory) override
	{
		if (bIsDirectory)
		{
			// in order to directory be empty it must apply following cases
			// 1. Must not contain any files
			// 2. Can contain other empty folders

			const FString CurrentDirectory = FString::Printf(TEXT("%s/"), FilenameOrDirectory);
			const FString CurrentDirectoryWithAsterisk = FString::Printf(TEXT("%s/*"), FilenameOrDirectory);

			bool bFiltered = false;
			for (const auto& ExcludedDir : ExcludedDirectories)
			{
				if (CurrentDirectory.Equals(ExcludedDir) || FPaths::IsUnderDirectory(CurrentDirectory, ExcludedDir))
				{
					bFiltered = true;
				}
			}

			if (!bFiltered)
			{
				TArray<FString> Files;
				IFileManager::Get().FindFilesRecursive(Files, *CurrentDirectory, TEXT("*.*"), true, false);

				if (Files.Num() == 0)
				{
					FRWScopeLock ScopeLock(FoundFilesLock, SLT_Write);
					EmptyDirectories.Emplace(CurrentDirectory);
				}
			}
		}

		return true;
	}
};

class FFindDirectoriesVisitor final : public IPlatformFile::FDirectoryVisitor
{
public:
	IPlatformFile& PlatformFile;
	FRWLock FoundFilesLock;
	TSet<FString>& FoundDirectories;
	const TSet<FString>& ExcludedDirectories;

	FFindDirectoriesVisitor(IPlatformFile& InPlatformFile, TSet<FString>& InDirectories, const TSet<FString>& InExcludedDirectories)
		: FDirectoryVisitor(EDirectoryVisitorFlags::ThreadSafe)
		  , PlatformFile(InPlatformFile)
		  , FoundDirectories(InDirectories)
		  , ExcludedDirectories(InExcludedDirectories)
	{
	}

	virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory) override
	{
		if (bIsDirectory)
		{
			const FString CurrentDirectory = FString::Printf(TEXT("%s/"), FilenameOrDirectory);

			// if current directory is in excluded list or under of any excluded directory list , then we ignore it
			bool bFiltered = false;
			for (const auto& ExcludedDir : ExcludedDirectories)
			{
				if (CurrentDirectory.Equals(ExcludedDir) || FPaths::IsUnderDirectory(CurrentDirectory, ExcludedDir))
				{
					bFiltered = true;
				}
			}

			if (!bFiltered)
			{
				FRWScopeLock ScopeLock(FoundFilesLock, SLT_Write);
				FoundDirectories.Emplace(CurrentDirectory);
			}
		}
		return true;
	}
};

void UProjectCleanerLibrary::GetSubDirectories(const FString& RootDir, const bool bRecursive, TSet<FString>& SubDirectories, const TSet<FString>& ExcludeDirectories)
{
	if (RootDir.IsEmpty()) return;
	if (!FPaths::DirectoryExists(RootDir)) return;

	SubDirectories.Reset();

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	FFindDirectoriesVisitor FindDirectoriesVisitor(PlatformFile, SubDirectories, ExcludeDirectories);

	if (bRecursive)
	{
		PlatformFile.IterateDirectoryRecursively(*RootDir, FindDirectoriesVisitor);
	}
	else
	{
		PlatformFile.IterateDirectory(*RootDir, FindDirectoriesVisitor);
	}
}

void UProjectCleanerLibrary::GetEmptyDirectories(const FString& InAbsPath, TSet<FString>& EmptyDirectories, const TSet<FString>& ExcludeDirectories)
{
	if (InAbsPath.IsEmpty()) return;
	if (!FPaths::DirectoryExists(InAbsPath)) return;

	EmptyDirectories.Reset();

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	FFindEmptyDirectoriesVisitor FindEmptyDirectoriesVisitor(PlatformFile, EmptyDirectories, ExcludeDirectories);

	PlatformFile.IterateDirectoryRecursively(*InAbsPath, FindEmptyDirectoriesVisitor);
}

void UProjectCleanerLibrary::GetAssetsInPath(const FString& InRelPath, const bool bRecursive, TArray<FAssetData>& Assets)
{
	if (InRelPath.IsEmpty()) return;

	Assets.Reset();

	const FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

	// in order for asset registry work correctly
	FString PackagePath = InRelPath;
	PackagePath.RemoveFromEnd(TEXT("/"));
	
	FARFilter Filter;
	Filter.bRecursivePaths = bRecursive;
	Filter.PackagePaths.Add(FName{*PackagePath});

	AssetRegistryModule.Get().GetAssets(Filter, Assets);
}

int64 UProjectCleanerLibrary::GetAssetsTotalSize(const TArray<FAssetData>& Assets)
{
	const FAssetRegistryModule& AssetRegistryModule = FModuleManager::GetModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);
	int64 Size = 0;
	for (const auto& Asset : Assets)
	{
		const auto AssetPackageData = AssetRegistryModule.Get().GetAssetPackageData(Asset.PackageName);
		if (!AssetPackageData) continue;
		Size += AssetPackageData->DiskSize;
	}

	return Size;
}

FString UProjectCleanerLibrary::PathConvertToAbs(const FString& InRelPath)
{
	FString Path = InRelPath;
	FPaths::NormalizeFilename(Path);
	const FString ProjectContentDirAbsPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir());
	return ConvertPathInternal(FString{"/Game/"}, ProjectContentDirAbsPath, Path);
}

FString UProjectCleanerLibrary::PathConvertToRel(const FString& InAbsPath)
{
	FString Path = InAbsPath;
	FPaths::NormalizeFilename(Path);
	const FString ProjectContentDirAbsPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir());
	return ConvertPathInternal(ProjectContentDirAbsPath, FString{"/Game/"}, Path);
}

FString UProjectCleanerLibrary::ConvertPathInternal(const FString& From, const FString& To, const FString& Path)
{
	return Path.Replace(*From, *To, ESearchCase::IgnoreCase);
}
