// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "ProjectCleaner/Public/ProjectCleanerLibrary.h"
#include "ProjectCleanerConstants.h"
// Engine Headers
#include "AssetToolsModule.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "FileHelpers.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/AssetManager.h"
#include "Engine/MapBuildDataRegistry.h"
#include "Internationalization/Regex.h"
#include "Misc/FileHelper.h"
#include "Misc/ScopedSlowTask.h"

// class FFindCorruptedFilesVisitor final : public IPlatformFile::FDirectoryVisitor
// {
// public:
// 	IPlatformFile& PlatformFile;
// 	TSet<FString>& CorruptedFiles;
//
// 	FFindCorruptedFilesVisitor(IPlatformFile& InPlatformFile, TSet<FString>& InCorruptedFiles)
// 		: FDirectoryVisitor(EDirectoryVisitorFlags::None),
// 		  PlatformFile(InPlatformFile),
// 		  CorruptedFiles(InCorruptedFiles)
// 	{
// 	}
//
// 	virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory) override
// 	{
// 		if (!bIsDirectory)
// 		{
// 			const FString FullPath = FPaths::ConvertRelativePathToFull(FilenameOrDirectory);
// 			if (UProjectCleanerLibrary::IsEngineExtension(FPaths::GetExtension(FullPath, false)))
// 			{
// 				// here we got absolute path "C:/MyProject/Content/material.uasset"
// 				// we must first convert that path to In Engine Internal Path like "/Game/material.uasset"
// 				const FString InternalFilePath = UProjectCleanerLibrary::PathConvertToRel(FullPath);
// 				// Converting file path to object path (This is for searching in AssetRegistry)
// 				// example "/Game/Name.uasset" => "/Game/Name.Name"
// 				FString ObjectPath = InternalFilePath;
// 				ObjectPath.RemoveFromEnd(FPaths::GetExtension(InternalFilePath, true));
// 				ObjectPath.Append(TEXT(".") + FPaths::GetBaseFilename(InternalFilePath));
//
// 				const FName ObjectPathName = FName{*ObjectPath};
//
// 				const FAssetRegistryModule& ModuleAssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
//
// 				// if its does not exist in asset registry, then something wrong with asset
// 				const FAssetData AssetData = ModuleAssetRegistry.Get().GetAssetByObjectPath(ObjectPathName);
// 				if (!AssetData.IsValid())
// 				{
// 					CorruptedFiles.Add(FullPath);
// 				}
// 			}
// 		}
//
// 		return true;
// 	}
// };
//
// class FFindNonEngineFilesVisitor final : public IPlatformFile::FDirectoryVisitor
// {
// public:
// 	IPlatformFile& PlatformFile;
// 	FRWLock FoundFilesLock;
// 	TSet<FString>& NonEngineFiles;
//
// 	FFindNonEngineFilesVisitor(IPlatformFile& InPlatformFile, TSet<FString>& InNonEngineFiles)
// 		: FDirectoryVisitor(EDirectoryVisitorFlags::ThreadSafe),
// 		  PlatformFile(InPlatformFile),
// 		  NonEngineFiles(InNonEngineFiles)
// 	{
// 	}
//
// 	virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory) override
// 	{
// 		if (!bIsDirectory)
// 		{
// 			const FString FullPath = FPaths::ConvertRelativePathToFull(FilenameOrDirectory);
// 			if (!UProjectCleanerLibrary::IsEngineExtension(FPaths::GetExtension(FullPath, false)))
// 			{
// 				FRWScopeLock ScopeLock(FoundFilesLock, SLT_Write);
// 				NonEngineFiles.Add(FullPath);
// 			}
// 		}
//
// 		return true;
// 	}
// };
//
// class FFindEmptyDirectoriesVisitor final : public IPlatformFile::FDirectoryVisitor
// {
// public:
// 	IPlatformFile& PlatformFile;
// 	FRWLock FoundFilesLock;
// 	TSet<FString>& EmptyDirectories;
// 	const TSet<FString>& ExcludedDirectories;
//
// 	FFindEmptyDirectoriesVisitor(IPlatformFile& InPlatformFile, TSet<FString>& InEmptyDirectories, const TSet<FString>& InExcludedDirectories)
// 		: FDirectoryVisitor(EDirectoryVisitorFlags::ThreadSafe)
// 		  , PlatformFile(InPlatformFile)
// 		  , EmptyDirectories(InEmptyDirectories)
// 		  , ExcludedDirectories(InExcludedDirectories)
// 	{
// 	}
//
// 	virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory) override
// 	{
// 		if (bIsDirectory)
// 		{
// 			// in order to directory be empty it must apply following cases
// 			// 1. Must not contain any files
// 			// 2. Can contain other empty folders
//
// 			const FString CurrentDirectory = FString::Printf(TEXT("%s/"), FilenameOrDirectory);
// 			const FString CurrentDirectoryWithAsterisk = FString::Printf(TEXT("%s/*"), FilenameOrDirectory);
//
// 			bool bFiltered = false;
// 			for (const auto& ExcludedDir : ExcludedDirectories)
// 			{
// 				if (CurrentDirectory.Equals(ExcludedDir) || FPaths::IsUnderDirectory(CurrentDirectory, ExcludedDir))
// 				{
// 					bFiltered = true;
// 				}
// 			}
//
// 			if (!bFiltered)
// 			{
// 				TArray<FString> Files;
// 				IFileManager::Get().FindFilesRecursive(Files, *CurrentDirectory, TEXT("*.*"), true, false);
//
// 				if (Files.Num() == 0)
// 				{
// 					FRWScopeLock ScopeLock(FoundFilesLock, SLT_Write);
// 					EmptyDirectories.Emplace(CurrentDirectory);
// 				}
// 			}
// 		}
//
// 		return true;
// 	}
// };
//
// class FFindDirectoriesVisitor final : public IPlatformFile::FDirectoryVisitor
// {
// public:
// 	IPlatformFile& PlatformFile;
// 	FRWLock FoundFilesLock;
// 	TSet<FString>& FoundDirectories;
// 	const TSet<FString>& ExcludedDirectories;
//
// 	FFindDirectoriesVisitor(IPlatformFile& InPlatformFile, TSet<FString>& InDirectories, const TSet<FString>& InExcludedDirectories)
// 		: FDirectoryVisitor(EDirectoryVisitorFlags::ThreadSafe)
// 		  , PlatformFile(InPlatformFile)
// 		  , FoundDirectories(InDirectories)
// 		  , ExcludedDirectories(InExcludedDirectories)
// 	{
// 	}
//
// 	virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory) override
// 	{
// 		if (bIsDirectory)
// 		{
// 			const FString CurrentDirectory = FString::Printf(TEXT("%s/"), FilenameOrDirectory);
//
// 			// if current directory is in excluded list or under of any excluded directory list , then we ignore it
// 			bool bFiltered = false;
// 			for (const auto& ExcludedDir : ExcludedDirectories)
// 			{
// 				if (CurrentDirectory.Equals(ExcludedDir) || FPaths::IsUnderDirectory(CurrentDirectory, ExcludedDir))
// 				{
// 					bFiltered = true;
// 				}
// 			}
//
// 			if (!bFiltered)
// 			{
// 				FRWScopeLock ScopeLock(FoundFilesLock, SLT_Write);
// 				FoundDirectories.Emplace(CurrentDirectory);
// 			}
// 		}
// 		return true;
// 	}
// };
//
// void UProjectCleanerLibrary::GetSubDirectories(const FString& RootDir, const bool bRecursive, TSet<FString>& SubDirectories, const TSet<FString>& ExcludeDirectories)
// {
// 	if (RootDir.IsEmpty()) return;
// 	if (!FPaths::DirectoryExists(RootDir)) return;
//
// 	SubDirectories.Reset();
//
// 	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
// 	FFindDirectoriesVisitor FindDirectoriesVisitor(PlatformFile, SubDirectories, ExcludeDirectories);
//
// 	if (bRecursive)
// 	{
// 		PlatformFile.IterateDirectoryRecursively(*RootDir, FindDirectoriesVisitor);
// 	}
// 	else
// 	{
// 		PlatformFile.IterateDirectory(*RootDir, FindDirectoriesVisitor);
// 	}
// }
//
// void UProjectCleanerLibrary::GetEmptyDirectories(const FString& InAbsPath, TSet<FString>& EmptyDirectories, const TSet<FString>& ExcludeDirectories)
// {
// 	if (InAbsPath.IsEmpty()) return;
// 	if (!FPaths::DirectoryExists(InAbsPath)) return;
//
// 	EmptyDirectories.Reset();
//
// 	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
// 	FFindEmptyDirectoriesVisitor FindEmptyDirectoriesVisitor(PlatformFile, EmptyDirectories, ExcludeDirectories);
//
// 	PlatformFile.IterateDirectoryRecursively(*InAbsPath, FindEmptyDirectoriesVisitor);
// }

// void UProjectCleanerLibrary::GetPrimaryAssetClasses(TArray<UClass*>& PrimaryAssetClasses)
// {
// 	PrimaryAssetClasses.Reset();
//
// 	const auto& AssetManager = UAssetManager::Get();
// 	if (!AssetManager.IsValid()) return;
//
// 	TArray<FPrimaryAssetTypeInfo> AssetTypeInfos;
// 	AssetManager.Get().GetPrimaryAssetTypeInfoList(AssetTypeInfos);
//
// 	for (const auto& AssetTypeInfo : AssetTypeInfos)
// 	{
// 		if (!AssetTypeInfo.AssetBaseClassLoaded) continue;
//
// 		PrimaryAssetClasses.Add(AssetTypeInfo.AssetBaseClassLoaded);
// 	}
// }
//
// void UProjectCleanerLibrary::GetPrimaryAssetClassNames(TArray<FName>& ClassNames)
// {
// 	ClassNames.Reset();
// 	
// 	TArray<UClass*> Classes;
// 	GetPrimaryAssetClasses(Classes);
//
// 	for (const auto& AssetClass : Classes)
// 	{
// 		if (!AssetClass) continue;
// 		
// 		ClassNames.Add(AssetClass->GetFName());
// 	}
// }
//
// void UProjectCleanerLibrary::GetPrimaryAssets(TArray<FAssetData>& PrimaryAssets)
// {
// 	PrimaryAssets.Reset();
//
// 	TArray<UClass*> PrimaryAssetClasses;
// 	GetPrimaryAssetClasses(PrimaryAssetClasses);
//
// 	FARFilter Filter;
// 	Filter.bRecursivePaths = true;
// 	Filter.bRecursiveClasses = true;
// 	Filter.PackagePaths.Add(FName{*ProjectCleanerConstants::PathRelativeRoot});
// 	// Filter.ClassNames.Add(UMapBuildDataRegistry::StaticClass()->GetFName());
//
// 	for (const auto& PrimaryAssetClass : PrimaryAssetClasses)
// 	{
// 		Filter.ClassNames.Add(PrimaryAssetClass->GetFName());
// 	}
//
// 	const FAssetRegistryModule& ModuleAssetRegistry = FModuleManager::GetModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);
// 	ModuleAssetRegistry.Get().GetAssets(Filter, PrimaryAssets);
// }

// void UProjectCleanerLibrary::GetAssetsInPath(const FString& InRelPath, const bool bRecursive, TArray<FAssetData>& Assets)
// {
// 	if (InRelPath.IsEmpty()) return;
//
// 	Assets.Reset();
//
// 	const FAssetRegistryModule& ModuleAssetRegistry = FModuleManager::GetModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);
//
// 	// in order for asset registry work correctly
// 	FString PackagePath = PathConvertToRel(InRelPath);
// 	PackagePath.RemoveFromEnd(TEXT("/"));
//
// 	FARFilter Filter;
// 	Filter.bRecursivePaths = bRecursive;
// 	Filter.PackagePaths.Add(FName{*PackagePath});
//
// 	ModuleAssetRegistry.Get().GetAssets(Filter, Assets);
// }

int64 UProjectCleanerLibrary::GetAssetsTotalSize(const TArray<FAssetData>& Assets)
{
	const FAssetRegistryModule& ModuleAssetRegistry = FModuleManager::GetModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);
	int64 Size = 0;
	for (const auto& Asset : Assets)
	{
		const auto AssetPackageData = ModuleAssetRegistry.Get().GetAssetPackageData(Asset.PackageName);
		if (!AssetPackageData) continue;
		Size += AssetPackageData->DiskSize;
	}

	return Size;
}

// void UProjectCleanerLibrary::GetAssetsIndirect(TArray<FProjectCleanerIndirectAsset>& IndirectAssets)
// {
// 	IndirectAssets.Reset();
//
// 	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
// 	const FAssetRegistryModule& ModuleAssetRegistry = FModuleManager::GetModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);
// 	
// 	const FString SourceDir = FPaths::ProjectDir() + TEXT("Source/");
// 	const FString ConfigDir = FPaths::ProjectDir() + TEXT("Config/");
// 	const FString PluginsDir = FPaths::ProjectDir() + TEXT("Plugins/");
// 	
// 	TSet<FString> Files;
// 	Files.Reserve(200); // reserving some space
// 	
// 	// 1) finding all source files in main project "Source" directory (<yourproject>/Source/*)
// 	TArray<FString> FilesToScan;
// 	PlatformFile.FindFilesRecursively(FilesToScan, *SourceDir, TEXT(".cs"));
// 	PlatformFile.FindFilesRecursively(FilesToScan, *SourceDir, TEXT(".cpp"));
// 	PlatformFile.FindFilesRecursively(FilesToScan, *SourceDir, TEXT(".h"));
// 	PlatformFile.FindFilesRecursively(FilesToScan, *ConfigDir, TEXT(".ini"));
// 	Files.Append(FilesToScan);
// 	
// 	// 2) we should find all source files in plugins folder (<yourproject>/Plugins/*)
// 	TArray<FString> ProjectPluginsFiles;
// 	// finding all installed plugins in "Plugins" directory
// 	struct DirectoryVisitor : public IPlatformFile::FDirectoryVisitor
// 	{
// 		virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory) override
// 		{
// 			if (bIsDirectory)
// 			{
// 				InstalledPlugins.Add(FilenameOrDirectory);
// 			}
// 	
// 			return true;
// 		}
// 	
// 		TArray<FString> InstalledPlugins;
// 	};
// 	
// 	DirectoryVisitor Visitor;
// 	PlatformFile.IterateDirectory(*PluginsDir, Visitor);
// 	
// 	// 3) for every installed plugin we scanning only "Source" and "Config" folders
// 	for (const auto& Dir : Visitor.InstalledPlugins)
// 	{
// 		const FString PluginSourcePathDir = Dir + "/Source";
// 		const FString PluginConfigPathDir = Dir + "/Config";
// 	
// 		PlatformFile.FindFilesRecursively(ProjectPluginsFiles, *PluginSourcePathDir, TEXT(".cs"));
// 		PlatformFile.FindFilesRecursively(ProjectPluginsFiles, *PluginSourcePathDir, TEXT(".cpp"));
// 		PlatformFile.FindFilesRecursively(ProjectPluginsFiles, *PluginSourcePathDir, TEXT(".h"));
// 		PlatformFile.FindFilesRecursively(ProjectPluginsFiles, *PluginConfigPathDir, TEXT(".ini"));
// 	}
// 	
// 	Files.Append(ProjectPluginsFiles);
// 	Files.Shrink();
//
// 	TArray<FAssetData> AllAssets;
// 	AllAssets.Reserve(ModuleAssetRegistry.Get().GetAllocatedSize());
// 	ModuleAssetRegistry.Get().GetAssetsByPath(FName{*ProjectCleanerConstants::PathRelativeRoot}, AllAssets, true);
//
// 	for (const auto& File : Files)
// 	{
// 		if (!PlatformFile.FileExists(*File)) continue;
// 	
// 		FString FileContent;
// 		FFileHelper::LoadFileToString(FileContent, *File);
// 		
// 		if (!HasIndirectlyUsedAssets(FileContent)) continue;
//
// 		static FRegexPattern Pattern(TEXT(R"(\/Game([A-Za-z0-9_.\/]+)\b)"));
// 		FRegexMatcher Matcher(Pattern, FileContent);
// 		while (Matcher.FindNext())
// 		{
// 			FString FoundedAssetObjectPath =  Matcher.GetCaptureGroup(0);
// 		
//
// 			// if ObjectPath ends with "_C" , then its probably blueprint, so we trim that
// 			if (FoundedAssetObjectPath.EndsWith("_C"))
// 			{
// 				FString TrimmedObjectPath = FoundedAssetObjectPath;
// 				TrimmedObjectPath.RemoveFromEnd("_C");
// 				
// 				FoundedAssetObjectPath = TrimmedObjectPath;
// 			}
//
// 			
// 			const FAssetData* AssetData = AllAssets.FindByPredicate([&] (const FAssetData& Elem)
// 			{
// 				return
// 					Elem.ObjectPath.ToString()==(FoundedAssetObjectPath) ||
// 					Elem.PackageName.ToString()==(FoundedAssetObjectPath);
// 			});
//
// 			if (!AssetData) continue;
// 			
// 			// if founded asset is ok, we loading file by lines to determine on what line its used
// 			TArray<FString> Lines;
// 			FFileHelper::LoadFileToStringArray(Lines, *File);
// 			for (int32 i = 0; i < Lines.Num(); ++i)
// 			{
// 				if (!Lines.IsValidIndex(i)) continue;
// 				if (!Lines[i].Contains(FoundedAssetObjectPath)) continue;
// 			
// 				FProjectCleanerIndirectAsset IndirectAsset;
// 				IndirectAsset.AssetData = *AssetData;
// 				IndirectAsset.FilePath = FPaths::ConvertRelativePathToFull(File);
// 				IndirectAsset.LineNum = i + 1;
// 				IndirectAssets.Add(IndirectAsset);
// 			}
// 		}
// 	}
// }

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

FString UProjectCleanerLibrary::GetAssetClassName(const FAssetData& AssetData)
{
	if (!AssetData.IsValid()) return {};

	if (AssetData.AssetClass.IsEqual("Blueprint"))
	{
		const auto GeneratedClassName = AssetData.TagsAndValues.FindTag(TEXT("GeneratedClass")).GetValue();
		const FString ClassObjectPath = FPackageName::ExportTextPathToObjectPath(*GeneratedClassName);
		return FPackageName::ObjectPathToObjectName(ClassObjectPath);
	}

	return AssetData.AssetClass.ToString();
}

// void UProjectCleanerLibrary::GetNonEngineFiles(TSet<FString>& NonEngineFiles)
// {
// 	NonEngineFiles.Reset();
//
// 	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
// 	FFindNonEngineFilesVisitor FindNonEngineFilesVisitor(PlatformFile, NonEngineFiles);
//
// 	PlatformFile.IterateDirectoryRecursively(*FPaths::ProjectContentDir(), FindNonEngineFilesVisitor);
// }

// void UProjectCleanerLibrary::GetAssetsCorrupted(TSet<FString>& CorruptedAssets)
// {
// 	CorruptedAssets.Reset();
//
// 	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
// 	FFindCorruptedFilesVisitor FindCorruptedFilesVisitor(PlatformFile, CorruptedAssets);
//
// 	PlatformFile.IterateDirectoryRecursively(*FPaths::ProjectContentDir(), FindCorruptedFilesVisitor);
// }

void UProjectCleanerLibrary::FixupRedirectors()
{
	const FAssetRegistryModule& ModuleAssetRegistry = FModuleManager::GetModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);
	const FAssetToolsModule& ModuleAssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));

	FScopedSlowTask FixRedirectorsTask{
		1.0f,
		FText::FromString(ProjectCleanerConstants::MsgFixingRedirectors)
	};
	FixRedirectorsTask.MakeDialog();

	FARFilter Filter;
	Filter.bRecursivePaths = true;
	Filter.PackagePaths.Emplace(ProjectCleanerConstants::PathRelRoot);
	Filter.ClassNames.Emplace(UObjectRedirector::StaticClass()->GetFName());

	// Getting all redirectors in given path
	TArray<FAssetData> AssetList;
	ModuleAssetRegistry.Get().GetAssets(Filter, AssetList);

	if (AssetList.Num() > 0)
	{
		FScopedSlowTask FixRedirectorsLoadingTask(
			AssetList.Num(),
			FText::FromString(ProjectCleanerConstants::MsgLoadingRedirectors)
		);
		FixRedirectorsLoadingTask.MakeDialog();

		TArray<UObjectRedirector*> Redirectors;
		Redirectors.Reserve(AssetList.Num());

		for (const auto& Asset : AssetList)
		{
			FixRedirectorsLoadingTask.EnterProgressFrame();

			UObject* AssetObj = Asset.GetAsset();
			if (!AssetObj) continue;

			const auto Redirector = CastChecked<UObjectRedirector>(AssetObj);
			if (!Redirector) continue;

			Redirectors.Add(Redirector);
		}

		Redirectors.Shrink();

		// Fix up all founded redirectors
		ModuleAssetTools.Get().FixupReferencers(Redirectors);
	}

	FixRedirectorsTask.EnterProgressFrame(1.0f);
}

void UProjectCleanerLibrary::SaveAllAssets(const bool bPromptUser)
{
	FEditorFileUtils::SaveDirtyPackages(
		bPromptUser,
		true,
		true,
		false,
		false,
		false
	);
}

void UProjectCleanerLibrary::UpdateAssetRegistry(const bool bSyncScan)
{
	const FAssetRegistryModule& ModuleAssetRegistry = FModuleManager::GetModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);

	TArray<FString> ScanFolders;
	ScanFolders.Add(ProjectCleanerConstants::PathRelRoot);

	ModuleAssetRegistry.Get().ScanPathsSynchronous(ScanFolders, true);
	ModuleAssetRegistry.Get().SearchAllAssets(bSyncScan);
}

void UProjectCleanerLibrary::FocusOnDirectory(const FString& InRelPath)
{
	if (InRelPath.IsEmpty()) return;

	TArray<FString> FocusFolders;
	FocusFolders.Add(InRelPath);

	const FContentBrowserModule& ModuleContentBrowser = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	ModuleContentBrowser.Get().SyncBrowserToFolders(FocusFolders);
}

// bool UProjectCleanerLibrary::IsEngineExtension(const FString& Extension)
// {
// 	return Extension.ToLower().Equals(TEXT("uasset")) || Extension.ToLower().Equals(TEXT("umap"));
// }

bool UProjectCleanerLibrary::IsUnderMegascansFolder(const FString& AssetPackagePath)
{
	return AssetPackagePath.StartsWith(ProjectCleanerConstants::PathRelMegascans) || AssetPackagePath.StartsWith(ProjectCleanerConstants::PathRelMegascansPresets);
}

bool UProjectCleanerLibrary::IsAssetRegistryWorking()
{
	return FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName).Get().IsLoadingAssets();
}

// bool UProjectCleanerLibrary::HasIndirectlyUsedAssets(const FString& FileContent)
// {
// 	if (FileContent.IsEmpty()) return false;
//
// 	// search any sub string that has asset package path in it
// 	static FRegexPattern Pattern(TEXT(R"(\/Game([A-Za-z0-9_.\/]+)\b)"));
// 	FRegexMatcher Matcher(Pattern, FileContent);
// 	return Matcher.FindNext();
// }

FString UProjectCleanerLibrary::ConvertPathInternal(const FString& From, const FString& To, const FString& Path)
{
	return Path.Replace(*From, *To, ESearchCase::IgnoreCase);
}
