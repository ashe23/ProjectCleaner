// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "ProjectCleaner/Public/ProjectCleanerLibrary.h"

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
