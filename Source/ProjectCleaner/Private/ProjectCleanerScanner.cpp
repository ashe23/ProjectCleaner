// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "ProjectCleanerScanner.h"
#include "ProjectCleaner.h"
#include "ProjectCleanerTypes.h"
#include "ProjectCleanerLibrary.h"
#include "ProjectCleanerConstants.h"
#include "ProjectCleanerScanSettings.h"
// Engine Headers
#include "AssetRegistry/AssetRegistryModule.h"
#include "Internationalization/Regex.h"
#include "Misc/FileHelper.h"
#include "EditorUtilityBlueprint.h"
#include "EditorUtilityWidget.h"
#include "EditorUtilityWidgetBlueprint.h"
#include "Engine/AssetManager.h"
#include "Engine/MapBuildDataRegistry.h"

class FContentFolderVisitor final : public IPlatformFile::FDirectoryVisitor
{
public:
	IPlatformFile& PlatformFile;
	TSet<FString>& FilesAll;
	TSet<FString>& FilesCorrupted;
	TSet<FString>& FilesNonEngine;
	TSet<FString>& FoldersAll;
	TSet<FString>& FoldersEmpty;
	TSet<FString>& FoldersForbiddenToDelete;
	TSet<FString>& FoldersForbiddenToScan;

	FContentFolderVisitor(
		IPlatformFile& InPlatformFile,
		TSet<FString>& InFilesAll,
		TSet<FString>& InFilesCorrupted,
		TSet<FString>& InFilesNonEngine,
		TSet<FString>& InFoldersAll,
		TSet<FString>& InFoldersEmpty,
		TSet<FString>& InFoldersForbiddenToDelete,
		TSet<FString>& InFoldersForbiddenToScan
	)
		: FDirectoryVisitor(EDirectoryVisitorFlags::None),
		  PlatformFile(InPlatformFile),
		  FilesAll(InFilesAll),
		  FilesCorrupted(InFilesCorrupted),
		  FilesNonEngine(InFilesNonEngine),
		  FoldersAll(InFoldersAll),
		  FoldersEmpty(InFoldersEmpty),
		  FoldersForbiddenToDelete(InFoldersForbiddenToDelete),
		  FoldersForbiddenToScan(InFoldersForbiddenToScan)
	{
	}

	virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory) override
	{
		const FString FullPath = FPaths::ConvertRelativePathToFull(FilenameOrDirectory);

		if (bIsDirectory)
		{
			if (!CanScanFolder(FullPath)) return true;

			FoldersAll.Add(FullPath);

			TArray<FString> Files;
			IFileManager::Get().FindFilesRecursive(Files, *FullPath, TEXT("*.*"), true, false);

			if (Files.Num() == 0)
			{
				if (CanDeleteFolder(FullPath))
				{
					FoldersEmpty.Add(FullPath);
				}
			}

			return true;
		}

		FString FileName;
		FString FileExtension;
		FString FilePath;
		FPaths::Split(FullPath, FilePath, FileName, FileExtension);

		if (!CanScanFolder(FilePath)) return true;

		FilesAll.Add(FullPath);

		if (FileExtension.ToLower().Equals(TEXT("uasset")) || FileExtension.ToLower().Equals(TEXT("umap")))
		{
			if (IsFileCorrupted(FullPath))
			{
				FilesCorrupted.Add(FullPath);
			}
		}
		else
		{
			FilesNonEngine.Add(FullPath);
		}

		return true;
	}

private:
	// Checks if given folder path can be scanned
	bool CanScanFolder(const FString& InPath) const
	{
		for (const auto& ForbiddenFolder : FoldersForbiddenToScan)
		{
			if (InPath.Equals(ForbiddenFolder) || FPaths::IsUnderDirectory(InPath, ForbiddenFolder))
			{
				return false;
			}
		}

		return true;
	}

	// Checks if given folder can be deleted. For example Developers folder can be scanned, but cant be deleted
	bool CanDeleteFolder(const FString& InPath) const
	{
		for (const auto& ForbiddenFolder : FoldersForbiddenToDelete)
		{
			if (InPath.Equals(ForbiddenFolder) || FPaths::IsUnderDirectory(InPath, ForbiddenFolder))
			{
				return false;
			}
		}

		return true;
	}

	bool IsFileCorrupted(const FString& InFilePath) const
	{
		// here we got absolute path "C:/MyProject/Content/material.uasset"
		// we must first convert that path to In Engine Internal Path like "/Game/material.uasset"
		const FString RelativePath = UProjectCleanerLibrary::PathConvertToRel(InFilePath);
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
};

void FProjectCleanerScanner::Scan(const TWeakObjectPtr<UProjectCleanerScanSettings>& InScanSettings)
{
	if (UProjectCleanerLibrary::IsAssetRegistryWorking())
	{
		return;
	}

	if (!InScanSettings.IsValid())
	{
		UE_LOG(LogProjectCleaner, Error, TEXT("Invalid ScanSettings"));
		return;
	}

	ScanSettings = InScanSettings;

	Reset();

	FoldersForbiddenToDelete.Add(FPaths::ProjectContentDir());
	FoldersForbiddenToDelete.Add(FPaths::ProjectContentDir() / TEXT("Developers"));
	FoldersForbiddenToDelete.Add(FPaths::ProjectContentDir() / TEXT("Collections"));
	FoldersForbiddenToDelete.Add(FPaths::GameUserDeveloperDir() / TEXT("Collections"));
	// todo:ashe23 for ue5 add __ExternalObject__ and __ExternalActors__ folders
	
	if (FModuleManager::Get().IsModuleLoaded(FName{*ProjectCleanerConstants::PluginMegascans}))
	{
		FoldersForbiddenToDelete.Add(FPaths::ProjectContentDir() / ProjectCleanerConstants::PluginMegascansMsPresetsFolder);
	}

	FoldersForbiddenToScan.Add(FPaths::ProjectContentDir() / TEXT("Collections"));
	FoldersForbiddenToScan.Add(FPaths::GameUserDeveloperDir() / TEXT("Collections"));
	// todo:ashe23 for ue5 add __ExternalObject__ and __ExternalActors__ folders
	if (!ScanSettings->bScanDeveloperContents)
	{
		FoldersForbiddenToScan.Add(FPaths::ProjectContentDir() / TEXT("Developers"));
	}

	// we interested only in "Content" (/Game) folder of project

	// 0. before scanning project we should make sure redirectors are fixed and assets saved
	UProjectCleanerLibrary::FixupRedirectors();
	UProjectCleanerLibrary::SaveAllAssets();

	// const FAssetRegistryModule& ModuleAssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	//
	//
	// // todo:ashe23 add slow task
	//
	// // 1. getting all assets inside Content folder from AssetRegistry
	// ModuleAssetRegistry.Get().GetAssetsByPath(FName{*ProjectCleanerConstants::PathRelativeRoot}, AssetsAll, true);
	//
	// // 2. scanning Content folder

	const FString PathProjectContent = FPaths::ProjectContentDir();
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	FContentFolderVisitor ContentFolderVisitor(
		PlatformFile,
		FilesAll,
		FilesCorrupted,
		FilesNonEngine,
		FoldersAll,
		FoldersEmpty,
		FoldersForbiddenToDelete,
		FoldersForbiddenToScan
	);
	PlatformFile.IterateDirectoryRecursively(*PathProjectContent, ContentFolderVisitor);
	//
	// // 3. getting all used assets
	// 	// asset considered used if
	// 	// 3.1. is primary asset or linked to it
	// 	// 3.2. is indirect asset (used in source code or config files) or linked to it
	// 	// 3.3. has external refs outside Content folder
	// 	// 3.4. is in blacklist (list of asset classes, that we wont touch, things like EditorWidgetUtility assets, they used only in editor and we cant really determine its used or not)
	// 	// 3.5. excluded by user
	//
	//
	// FindAssetsPrimary();
	// FindAssetsIndirect();
	// FindAssetsWithExternalRefs();
	// FindAssetsBlackListed();
	// // FindAssetsExcluded();
	// FindAssetsUsed();
	//
	// // 4. filtering all used assets from all assets gives us all unused assets in project
	// AssetsUnused.Reserve(AssetsAll.Num());
	//
	// for (const auto& Asset : AssetsAll)
	// {
	// 	if (AssetsUsed.Contains(Asset)) continue;
	//
	// 	AssetsUnused.AddUnique(Asset);
	// }
	//
	// AssetsUnused.Shrink();

	// FindAssetsUnused();
	// ModuleAssetRegistry.Get().UseFilterToExcludeAssets();


	UE_LOG(LogProjectCleaner, Warning, TEXT("A"));
}

void FProjectCleanerScanner::Reset()
{
	FoldersAll.Reset();
	FoldersEmpty.Reset();
	FoldersForbiddenToDelete.Reset();
	FoldersForbiddenToScan.Reset();

	FilesAll.Reset();
	FilesCorrupted.Reset();
	FilesNonEngine.Reset();

	AssetsAll.Reset();
	AssetsUsed.Reset();
	AssetsUnused.Reset();
	AssetsExcluded.Reset();
	AssetsPrimary.Reset();
	AssetsWithExternalRefs.Reset();
	AssetsBlackListed.Reset();
	AssetsIndirect.Reset();
}

const TSet<FString>& FProjectCleanerScanner::GetFoldersAll() const
{
	return FoldersAll;
}

const TSet<FString>& FProjectCleanerScanner::GetFoldersEmpty() const
{
	return FoldersEmpty;
}

const TSet<FString>& FProjectCleanerScanner::GetFoldersForbiddenToDelete() const
{
	return FoldersForbiddenToDelete;
}

const TSet<FString>& FProjectCleanerScanner::GetFoldersForbiddenToScan() const
{
	return FoldersForbiddenToScan;
}

const TArray<FAssetData>& FProjectCleanerScanner::GetAssetsAll() const
{
	return AssetsAll;
}

const TArray<FAssetData>& FProjectCleanerScanner::GetAssetsUnused() const
{
	return AssetsUnused;
}

void FProjectCleanerScanner::FindAssetsPrimary()
{
	const FAssetRegistryModule& ModuleAssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

	TSet<FString> PrimaryAssetClassList;
	{
		// getting list of primary asset classes that are defined in AssetManager
		TArray<FName> PrimaryAssetClassNames;
		const auto& AssetManager = UAssetManager::Get();
		if (!AssetManager.IsValid()) return;

		TArray<FPrimaryAssetTypeInfo> AssetTypeInfos;
		AssetManager.Get().GetPrimaryAssetTypeInfoList(AssetTypeInfos);

		for (const auto& AssetTypeInfo : AssetTypeInfos)
		{
			if (!AssetTypeInfo.AssetBaseClassLoaded) continue;

			PrimaryAssetClassNames.AddUnique(AssetTypeInfo.AssetBaseClassLoaded->GetFName());
		}

		// getting list of primary assets classes that are derived from main primary assets
		TSet<FName> DerivedFromPrimaryAssets;
		{
			const TSet<FName> ExcludedClassNames;
			ModuleAssetRegistry.Get().GetDerivedClassNames(PrimaryAssetClassNames, ExcludedClassNames, DerivedFromPrimaryAssets);
		}

		for (const auto& DerivedClassName : DerivedFromPrimaryAssets)
		{
			PrimaryAssetClassList.Add(DerivedClassName.ToString());
		}
	}

	for (const auto& Asset : AssetsAll)
	{
		const FString AssetClassName = UProjectCleanerLibrary::GetAssetClassName(Asset);
		if (PrimaryAssetClassList.Contains(AssetClassName))
		{
			AssetsPrimary.Add(Asset);
		}
	}


	//
	// FARFilter Filter_BP;
	// Filter_BP.ClassNames.Add(UBlueprint::StaticClass()->GetFName());
	// Filter_BP.PackagePaths.Add(FName{*ProjectCleanerConstants::PathRelativeRoot});
	// Filter_BP.bRecursiveClasses = true;
	// Filter_BP.bRecursivePaths = true;
	//
	// TArray<FAssetData> BlueprintAssets;
	// ModuleAssetRegistry.Get().GetAssets(Filter_BP, BlueprintAssets);
	//
	// for (const auto& BP_Asset : BlueprintAssets)
	// {
	// 	const FName BP_ClassName = FName{*UProjectCleanerLibrary::GetAssetClassName(BP_Asset)};
	// 	if (DerivedFromPrimaryAssets.Contains(BP_ClassName))
	// 	{
	// 		AssetsPrimary.AddUnique(BP_Asset);
	// 	}
	// }

	// FARFilter Filter;
	// Filter.bRecursiveClasses = true;
	// Filter.bRecursivePaths = true;
	// Filter.PackagePaths.Add(FName{*ProjectCleanerConstants::PathRelativeRoot});
	// Filter.ClassNames.Append(PrimaryAssetClasses);
	// Filter.ClassNames.Add(UMapBuildDataRegistry::StaticClass()->GetFName());
	//
	// ModuleAssetRegistry.Get().GetAssets(Filter, AssetsPrimary);
	//
	// 	for (const auto& Asset : PrimaryAssets)
	// 	{
	// 		UsedAssets.Add(Asset.PackageName);
	// 	}
	//
	// 	for (const auto& Asset : IndirectAssets)
	// 	{
	// 		UsedAssets.Add(Asset.Key.PackageName);
	// 	}
	//
	// 	for (const auto& Asset : AssetsWithExternalRefs)
	// 	{
	// 		UsedAssets.Add(Asset.PackageName);
	// 	}
	//
	// 	if (!bScanDeveloperContents)
	// 	{
	// 		TArray<FAssetData> AssetsInDeveloperFolder;
	// 		AssetRegistry->Get().GetAssetsByPath(TEXT("/Game/Developers"), AssetsInDeveloperFolder, true);
	//
	// 		for (const auto& Asset : AssetsInDeveloperFolder)
	// 		{
	// 			UsedAssets.Add(Asset.PackageName);
	// 		}
	// 	}
}

void FProjectCleanerScanner::FindAssetsIndirect()
{
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	const FAssetRegistryModule& ModuleAssetRegistry = FModuleManager::GetModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);

	const FString SourceDir = FPaths::ProjectDir() + TEXT("Source/");
	const FString ConfigDir = FPaths::ProjectDir() + TEXT("Config/");
	const FString PluginsDir = FPaths::ProjectDir() + TEXT("Plugins/");

	TSet<FString> Files;
	Files.Reserve(200); // reserving some space

	// 1. finding all source files in main project "Source" directory (<yourproject>/Source/*)
	TArray<FString> FilesToScan;
	PlatformFile.FindFilesRecursively(FilesToScan, *SourceDir, TEXT(".cs"));
	PlatformFile.FindFilesRecursively(FilesToScan, *SourceDir, TEXT(".cpp"));
	PlatformFile.FindFilesRecursively(FilesToScan, *SourceDir, TEXT(".h"));
	PlatformFile.FindFilesRecursively(FilesToScan, *ConfigDir, TEXT(".ini"));
	Files.Append(FilesToScan);

	// 2. we should find all source files in plugins folder (<yourproject>/Plugins/*)
	TArray<FString> ProjectPluginsFiles;
	// finding all installed plugins in "Plugins" directory
	struct DirectoryVisitor : public IPlatformFile::FDirectoryVisitor
	{
		virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory) override
		{
			if (bIsDirectory)
			{
				InstalledPlugins.Add(FilenameOrDirectory);
			}

			return true;
		}

		TArray<FString> InstalledPlugins;
	};

	DirectoryVisitor Visitor;
	PlatformFile.IterateDirectory(*PluginsDir, Visitor);

	// 3. for every installed plugin we scanning only "Source" and "Config" folders
	for (const auto& Dir : Visitor.InstalledPlugins)
	{
		const FString PluginSourcePathDir = Dir + "/Source";
		const FString PluginConfigPathDir = Dir + "/Config";

		PlatformFile.FindFilesRecursively(ProjectPluginsFiles, *PluginSourcePathDir, TEXT(".cs"));
		PlatformFile.FindFilesRecursively(ProjectPluginsFiles, *PluginSourcePathDir, TEXT(".cpp"));
		PlatformFile.FindFilesRecursively(ProjectPluginsFiles, *PluginSourcePathDir, TEXT(".h"));
		PlatformFile.FindFilesRecursively(ProjectPluginsFiles, *PluginConfigPathDir, TEXT(".ini"));
	}

	Files.Append(ProjectPluginsFiles);
	Files.Shrink();

	TArray<FAssetData> AllAssets;
	AllAssets.Reserve(ModuleAssetRegistry.Get().GetAllocatedSize());
	ModuleAssetRegistry.Get().GetAssetsByPath(FName{*ProjectCleanerConstants::PathRelRoot}, AllAssets, true);

	for (const auto& File : Files)
	{
		if (!PlatformFile.FileExists(*File)) continue;

		FString FileContent;
		FFileHelper::LoadFileToString(FileContent, *File);

		if (!HasIndirectAsset(FileContent)) continue;

		static FRegexPattern Pattern(TEXT(R"(\/Game([A-Za-z0-9_.\/]+)\b)"));
		FRegexMatcher Matcher(Pattern, FileContent);
		while (Matcher.FindNext())
		{
			FString FoundedAssetObjectPath = Matcher.GetCaptureGroup(0);


			// if ObjectPath ends with "_C" , then its probably blueprint, so we trim that
			if (FoundedAssetObjectPath.EndsWith("_C"))
			{
				FString TrimmedObjectPath = FoundedAssetObjectPath;
				TrimmedObjectPath.RemoveFromEnd("_C");

				FoundedAssetObjectPath = TrimmedObjectPath;
			}


			const FAssetData* AssetData = AllAssets.FindByPredicate([&](const FAssetData& Elem)
			{
				return
					Elem.ObjectPath.ToString() == (FoundedAssetObjectPath) ||
					Elem.PackageName.ToString() == (FoundedAssetObjectPath);
			});

			if (!AssetData) continue;

			// if founded asset is ok, we loading file by lines to determine on what line its used
			TArray<FString> Lines;
			FFileHelper::LoadFileToStringArray(Lines, *File);
			for (int32 i = 0; i < Lines.Num(); ++i)
			{
				if (!Lines.IsValidIndex(i)) continue;
				if (!Lines[i].Contains(FoundedAssetObjectPath)) continue;

				FProjectCleanerIndirectAsset IndirectAsset;
				IndirectAsset.AssetData = *AssetData;
				IndirectAsset.FilePath = FPaths::ConvertRelativePathToFull(File);
				IndirectAsset.LineNum = i + 1;
				AssetsIndirect.AddUnique(IndirectAsset);
			}
		}
	}
}

void FProjectCleanerScanner::FindAssetsWithExternalRefs()
{
	const FAssetRegistryModule& ModuleAssetRegistry = FModuleManager::GetModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);

	TArray<FName> Refs;
	for (const auto& Asset : AssetsAll)
	{
		ModuleAssetRegistry.Get().GetReferencers(Asset.PackageName, Refs);

		const bool HasExternalRefs = Refs.ContainsByPredicate([](const FName& Ref)
		{
			return !Ref.ToString().StartsWith(ProjectCleanerConstants::PathRelRoot);
		});

		if (HasExternalRefs)
		{
			AssetsWithExternalRefs.AddUnique(Asset);
		}

		Refs.Reset();
	}
}

void FProjectCleanerScanner::FindAssetsBlackListed()
{
	const FAssetRegistryModule& ModuleAssetRegistry = FModuleManager::GetModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);

	AssetsBlackListed.Reserve(AssetsAll.Num());

	FARFilter Filter;
	Filter.bRecursivePaths = true;
	Filter.bRecursiveClasses = true;
	Filter.PackagePaths.Add(FName{ProjectCleanerConstants::PathRelRoot});
	Filter.ClassNames.Add(UEditorUtilityWidget::StaticClass()->GetFName());
	Filter.ClassNames.Add(UEditorUtilityBlueprint::StaticClass()->GetFName());
	Filter.ClassNames.Add(UMapBuildDataRegistry::StaticClass()->GetFName());

	ModuleAssetRegistry.Get().GetAssets(Filter, AssetsBlackListed);

	AssetsBlackListed.Shrink();
}

void FProjectCleanerScanner::FindAssetsExcluded()
{
	const FAssetRegistryModule& ModuleAssetRegistry = FModuleManager::GetModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);

	AssetsExcluded.Reserve(AssetsAll.Num());

	FARFilter Filter;
	Filter.bRecursivePaths = true;
	Filter.bRecursiveClasses = true;
	Filter.PackagePaths.Add(FName{*ProjectCleanerConstants::PathRelRoot});

	for (const auto& ExcludedPath : ScanSettings->ExcludedDirectories)
	{
		Filter.PackagePaths.Add(FName{*ExcludedPath.Path});
	}

	// todo:ashe23 not sure about this
	// for (const auto& ExcludedAsset : ScanSettings->ExcludedAssets)
	// {
	// 	if (!ExcludedAsset.LoadSynchronous()) continue;
	// 	
	// 	const FString PackageName = ExcludedAsset.GetLongPackageName();
	// 	const FString Name = ExcludedAsset.GetAssetName();
	// }

	for (const auto& ExcludedClass : ScanSettings->ExcludedClasses)
	{
		if (!ExcludedClass.LoadSynchronous()) continue;

		Filter.ClassNames.Add(ExcludedClass->GetFName());
	}

	ModuleAssetRegistry.Get().GetAssets(Filter, AssetsExcluded);

	AssetsExcluded.Shrink();
}

void FProjectCleanerScanner::FindAssetsUsed()
{
	const FAssetRegistryModule& ModuleAssetRegistry = FModuleManager::GetModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);

	AssetsUsed.Reserve(AssetsAll.Num());

	for (const auto& PrimaryAsset : AssetsPrimary)
	{
		AssetsUsed.AddUnique(PrimaryAsset);
	}

	for (const auto& IndirectAsset : AssetsIndirect)
	{
		AssetsUsed.AddUnique(IndirectAsset.AssetData);
	}

	for (const auto& AssetWithExternalRef : AssetsWithExternalRefs)
	{
		AssetsUsed.AddUnique(AssetWithExternalRef);
	}

	for (const auto& BlackListedAsset : AssetsBlackListed)
	{
		AssetsUsed.AddUnique(BlackListedAsset);
	}

	// for (const auto& ExcludedAsset : AssetsExcluded)
	// {
	// 	AssetsUsed.AddUnique(ExcludedAsset);
	// }

	if (!ScanSettings->bScanDeveloperContents)
	{
		TArray<FAssetData> DeveloperContentAssets;
		ModuleAssetRegistry.Get().GetAssetsByPath(FName{*ProjectCleanerConstants::PathRelDevelopers}, DeveloperContentAssets, true);

		for (const auto& DeveloperContentAsset : DeveloperContentAssets)
		{
			AssetsUsed.AddUnique(DeveloperContentAsset);
		}
	}

	const bool IsMegascansLoaded = FModuleManager::Get().IsModuleLoaded(FName{*ProjectCleanerConstants::PluginMegascans});
	if (IsMegascansLoaded)
	{
		TArray<FAssetData> MegascansPresetAssets;
		ModuleAssetRegistry.Get().GetAssetsByPath(FName{*ProjectCleanerConstants::PathRelMegascansPresets}, MegascansPresetAssets, true);

		for (const auto& MegascansAsset : MegascansPresetAssets)
		{
			AssetsUsed.AddUnique(MegascansAsset);
		}
	}

	// finding all linked assets (refs and deps) of used assets
	TSet<FName> UsedAssetsDeps;
	TArray<FName> Stack;
	for (const auto& Asset : AssetsUsed)
	{
		UsedAssetsDeps.Add(Asset.PackageName);
		Stack.Add(Asset.PackageName);

		TArray<FName> Deps;
		while (Stack.Num() > 0)
		{
			const auto CurrentPackageName = Stack.Pop(false);
			Deps.Reset();

			ModuleAssetRegistry.Get().GetDependencies(CurrentPackageName, Deps);

			Deps.RemoveAllSwap([&](const FName& Dep)
			{
				return !Dep.ToString().StartsWith(*ProjectCleanerConstants::PathRelRoot);
			}, false);

			for (const auto& Dep : Deps)
			{
				bool bIsAlreadyInSet = false;
				UsedAssetsDeps.Add(Dep, &bIsAlreadyInSet);
				if (!bIsAlreadyInSet)
				{
					Stack.Add(Dep);
				}
			}
		}
	}

	for (const auto& Asset : AssetsAll)
	{
		if (UsedAssetsDeps.Contains(Asset.PackageName))
		{
			AssetsUsed.AddUnique(Asset);
		}
	}

	AssetsUsed.Shrink();
}

void FProjectCleanerScanner::FindAssetsUnused()
{
	const FAssetRegistryModule& ModuleAssetRegistry = FModuleManager::GetModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);

	AssetsUnused.Reserve(AssetsAll.Num());

	// // finding all dependency assets (refs and deps) of used assets
	// TSet<FName> UsedAssetsDeps;
	// TArray<FName> Stack;
	// for (const auto& Asset : AssetsUsed)
	// {
	// 	UsedAssetsDeps.Add(Asset.PackageName);
	// 	Stack.Add(Asset.PackageName);
	//
	// 	TArray<FName> Deps;
	// 	while (Stack.Num() > 0)
	// 	{
	// 		const auto CurrentPackageName = Stack.Pop(false);
	// 		Deps.Reset();
	//
	// 		ModuleAssetRegistry.Get().GetDependencies(CurrentPackageName, Deps);
	//
	// 		Deps.RemoveAllSwap([&] (const FName& Dep)
	// 		{
	// 			return !Dep.ToString().StartsWith(*ProjectCleanerConstants::PathRelativeRoot);
	// 		}, false);
	//
	// 		for (const auto& Dep : Deps)
	// 		{
	// 			bool bIsAlreadyInSet = false;
	// 			UsedAssetsDeps.Add(Dep, &bIsAlreadyInSet);
	// 			if (!bIsAlreadyInSet)
	// 			{
	// 				Stack.Add(Dep);
	// 			}
	// 		}
	// 	}
	// }
	const bool IsMegascansLoaded = FModuleManager::Get().IsModuleLoaded("MegascansPlugin");

	for (const auto& Asset : AssetsAll)
	{
		if (AssetsUsed.Contains(Asset)) continue;
		// if (UsedAssetsDeps.Contains(Asset.PackageName)) continue;
		if (IsMegascansLoaded && UProjectCleanerLibrary::IsUnderMegascansFolder(Asset.PackagePath.ToString())) continue;
		// assets under developers folder

		AssetsUnused.Add(Asset);
	}

	AssetsUnused.Shrink();
}

bool FProjectCleanerScanner::HasIndirectAsset(const FString& FileContent) const
{
	if (FileContent.IsEmpty()) return false;

	// search any sub string that has asset package path in it
	static FRegexPattern Pattern(TEXT(R"(\/Game([A-Za-z0-9_.\/]+)\b)"));
	FRegexMatcher Matcher(Pattern, FileContent);
	return Matcher.FindNext();
}
