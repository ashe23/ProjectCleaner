// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "ProjectCleaner/Public/ProjectCleanerLibrary.h"

class FFindDirectoriesVisitor final : public IPlatformFile::FDirectoryVisitor
{
public:
	IPlatformFile& PlatformFile;
	FRWLock FoundFilesLock;
	TSet<FString>& FoundDirectories;

	FFindDirectoriesVisitor(IPlatformFile& InPlatformFile, TSet<FString>& InDirectories)
		: FDirectoryVisitor(EDirectoryVisitorFlags::ThreadSafe)
		  , PlatformFile(InPlatformFile)
		  , FoundDirectories(InDirectories)
	{
	}

	virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory) override
	{
		if (bIsDirectory)
		{
			FRWScopeLock ScopeLock(FoundFilesLock, SLT_Write);
			FoundDirectories.Emplace(FString::Printf(TEXT("%s/"), FilenameOrDirectory));
		}
		return true;
	}
};

void UProjectCleanerLibrary::GetSubDirectories(const FString& RootDir, TSet<FString>& SubDirectories, TSet<FString>& ExcludeDirectories)
{
	if (RootDir.IsEmpty()) return;
	if (!FPaths::DirectoryExists(RootDir)) return;

	SubDirectories.Reset();

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	FFindDirectoriesVisitor FindDirectoriesVisitor(PlatformFile, SubDirectories);
	PlatformFile.IterateDirectory(*RootDir, FindDirectoriesVisitor);

	if (ExcludeDirectories.Num() > 0)
	{
		SubDirectories = SubDirectories.Difference(ExcludeDirectories); 
	}
}
