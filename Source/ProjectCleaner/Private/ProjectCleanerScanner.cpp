// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "ProjectCleanerScanner.h"
#include "ProjectCleaner.h"
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
#include "ProjectCleanerApi.h"
#include "Engine/AssetManager.h"
#include "Engine/MapBuildDataRegistry.h"
#include "Misc/ScopedSlowTask.h"

class FContentFolderVisitor final : public IPlatformFile::FDirectoryVisitor
{
public:
	TSet<FString>& FoldersAll;
	TSet<FString>& FoldersEmpty;
	TSet<FString>& FilesCorrupted;
	TSet<FString>& FilesNonEngine;
	const TSet<FString>& FoldersBlacklist;

	FContentFolderVisitor(
		TSet<FString>& InFoldersAll,
		TSet<FString>& InFoldersEmpty,
		TSet<FString>& InFilesCorrupted,
		TSet<FString>& InFilesNonEngine,
		const TSet<FString>& InFoldersBlacklist
	)
		: FDirectoryVisitor(EDirectoryVisitorFlags::None),
		  FoldersAll(InFoldersAll),
		  FoldersEmpty(InFoldersEmpty),
		  FilesCorrupted(InFilesCorrupted),
		  FilesNonEngine(InFilesNonEngine),
		  FoldersBlacklist(InFoldersBlacklist)
	{
	}

	virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory) override
	{
		const FString FullPath = FPaths::ConvertRelativePathToFull(FilenameOrDirectory);

		if (bIsDirectory)
		{
			if (UProjectCleanerLibrary::IsUnderAnyFolder(FullPath, FoldersBlacklist)) return true;

			FoldersAll.Add(FullPath);

			TArray<FString> Files;
			IFileManager::Get().FindFilesRecursive(Files, *FullPath, TEXT("*.*"), true, false);

			if (Files.Num() == 0)
			{
				FoldersEmpty.Add(FullPath);
			}

			return true;
		}

		FString FileName;
		FString FileExtension;
		FString Path;
		FPaths::Split(FullPath, Path, FileName, FileExtension);

		if (UProjectCleanerLibrary::IsUnderAnyFolder(Path, FoldersBlacklist)) return true;

		if (UProjectCleanerLibrary::IsEngineFileExtension(FileExtension))
		{
			if (UProjectCleanerLibrary::IsCorruptedEngineFile(FullPath))
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
};

FProjectCleanerScanner::FProjectCleanerScanner()
{
	ModuleAssetRegistry = &FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);
}

void FProjectCleanerScanner::Scan(const TWeakObjectPtr<UProjectCleanerScanSettings>& InScanSettings)
{
	if (!ModuleAssetRegistry) return;
	if (UProjectCleanerLibrary::IsAssetRegistryWorking()) return;
	if (!InScanSettings.IsValid()) return;

	ScanSettings = InScanSettings;

	UProjectCleanerLibrary::FixupRedirectors();
	UProjectCleanerLibrary::SaveAllAssets();

	FScopedSlowTask SlowTask{2.0f, FText::FromString(ProjectCleanerConstants::MsgScanning)};
	SlowTask.MakeDialog();
	
	DataInit();
	
	FContentFolderVisitor ContentFolderVisitor{
		FoldersAll,
		FoldersEmpty,
		FilesCorrupted,
		FilesNonEngine,
		FoldersBlacklist
	};
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	PlatformFile.IterateDirectoryRecursively(*FPaths::ProjectContentDir(), ContentFolderVisitor);

	SlowTask.EnterProgressFrame(1.0f);

	// 1. getting all assets in project
	ModuleAssetRegistry->Get().GetAssetsByPath(ProjectCleanerConstants::PathRelRoot, AssetsAll, true);

	// 2. getting all used assets in project
	UProjectCleanerLibrary::GetAssetsIndirect(AssetsIndirect);
	UProjectCleanerLibrary::GetAssetsPrimary(AssetsPrimary, true);
	UProjectCleanerLibrary::GetAssetsWithExternalRefs(AssetsWithExternalRefs);
	
	TArray<FAssetData> UsedAssets;
	UsedAssets.Append(AssetsPrimary);
	UsedAssets.Append(AssetsIndirect);
	UsedAssets.Append(AssetsWithExternalRefs);
	UsedAssets.Append(AssetsBlacklist);
	UsedAssets.Append(ScanSettings->ExcludedAssets);

	// 3. getting all used assets linked assets (refs and deps)
	TArray<FAssetData> LinkedAssets;
	UProjectCleanerLibrary::GetLinkedAssets(UsedAssets, LinkedAssets);
	for (const auto& LinkedAsset : LinkedAssets)
	{
		UsedAssets.AddUnique(LinkedAsset);
	}

	// 4. now filtering all used assets from all assets and we get unused assets in project
	AssetsUnused.Append(AssetsAll);
	
	FARFilter Filter;
	Filter.bRecursivePaths = true;

	// filtering blacklisted folder
	for (const auto& FolderBlacklist : FoldersBlacklist)
	{
		Filter.PackagePaths.AddUnique(FName{*UProjectCleanerLibrary::PathConvertToRel(FolderBlacklist)});
	}

	// filtering excluded by folders
	for (const auto& ExcludedFolder : ScanSettings->ExcludedFolders)
	{
		Filter.PackagePaths.AddUnique(FName{*ExcludedFolder.Path});
	}
	
	// filtering excluded by class
	for (const auto& ExcludedClass : ScanSettings->ExcludedClasses)
	{
		if (!ExcludedClass.LoadSynchronous()) continue;
			
		Filter.ClassNames.AddUnique(ExcludedClass->GetFName());
	}
	
	for (const auto& Asset : UsedAssets)
	{
		if (!Asset.IsValid()) continue;
	
		Filter.ObjectPaths.AddUnique(Asset.ObjectPath);
	}

	ModuleAssetRegistry->Get().UseFilterToExcludeAssets(AssetsUnused, Filter);
}

const TSet<FString>& FProjectCleanerScanner::GetFilesCorrupted() const
{
	return FilesCorrupted;
}

const TSet<FString>& FProjectCleanerScanner::GetFilesNonEngine() const
{
	return FilesNonEngine;
}

const TArray<FAssetData>& FProjectCleanerScanner::GetAssetsUnused() const
{
	return AssetsUnused;
}

void FProjectCleanerScanner::DataInit()
{
	// 1. reset old data
	DataReset();

	// 2. fill blacklist folders
	FoldersBlacklist.Add(FPaths::ProjectContentDir() / TEXT("Collections"));
	FoldersBlacklist.Add(FPaths::GameUserDeveloperDir() / TEXT("Collections"));
	// todo:ashe23 for ue5 add __ExternalObject__ and __ExternalActors__ folders
	
	if (FModuleManager::Get().IsModuleLoaded(ProjectCleanerConstants::PluginMegascans))
	{
		FoldersBlacklist.Add(FPaths::ProjectContentDir() / ProjectCleanerConstants::PluginMegascansMsPresetsFolder.ToString());
	}
	
	if (!ScanSettings->bScanDeveloperContents)
	{
		FoldersBlacklist.Add(FPaths::ProjectContentDir() / TEXT("Developers"));
	}

	// 3. fill blacklist assets
	FARFilter Filter;
	Filter.bRecursivePaths = true;
	Filter.bRecursiveClasses = true;
	Filter.PackagePaths.Add(ProjectCleanerConstants::PathRelRoot);
	Filter.ClassNames.Add(UEditorUtilityWidget::StaticClass()->GetFName());
	Filter.ClassNames.Add(UEditorUtilityBlueprint::StaticClass()->GetFName());
	Filter.ClassNames.Add(UEditorUtilityWidgetBlueprint::StaticClass()->GetFName());
	Filter.ClassNames.Add(UMapBuildDataRegistry::StaticClass()->GetFName());

	ModuleAssetRegistry->Get().GetAssets(Filter, AssetsBlacklist);
}

void FProjectCleanerScanner::DataReset()
{
	FoldersAll.Reset();
	FoldersEmpty.Reset();
	FoldersBlacklist.Reset();

	FilesCorrupted.Reset();
	FilesNonEngine.Reset();

	AssetsAll.Reset();
	AssetsPrimary.Reset();
	AssetsIndirect.Reset();
	AssetsWithExternalRefs.Reset();
	AssetsBlacklist.Reset();
	AssetsUnused.Reset();
}

// void FProjectCleanerScanner::GetSubFolders(const FString& InFolderPathAbs, TSet<FString>& SubFolders)
// {
// 	SubFolders.Reset();
// 	
// 	const auto Elem = FoldersTree.Find(InFolderPathAbs);
// 	if (!Elem) return;
//
// 	SubFolders.Append(Elem->SubFolders);
// }
//
// int32 FProjectCleanerScanner::GetFoldersTotalNum(const FString& InFolderPathAbs) const
// {
// 	const auto Elem = FoldersTree.Find(InFolderPathAbs);
// 	if (!Elem) return 0;
//
// 	return Elem->SubFoldersAll.Num();
// }
//
// int32 FProjectCleanerScanner::GetFoldersEmptyNum(const FString& InFolderPathAbs) const
// {
// 	const auto Elem = FoldersTree.Find(InFolderPathAbs);
// 	if (!Elem) return 0;
//
// 	return Elem->SubFoldersEmpty.Num();
// }
//
// int32 FProjectCleanerScanner::GetAssetsTotalNum(const FString& InFolderPathAbs) const
// {
// 	const auto Elem = FoldersTree.Find(InFolderPathAbs);
// 	if (!Elem) return 0;
//
// 	return Elem->AssetsTotal.Num();
// }
//
// int32 FProjectCleanerScanner::GetAssetsUnusedNum(const FString& InFolderPathAbs) const
// {
// 	const auto Elem = FoldersTree.Find(InFolderPathAbs);
// 	if (!Elem) return 0;
//
// 	return Elem->AssetsUnused.Num();
// }
//
// int64 FProjectCleanerScanner::GetSizeTotal(const FString& InFolderPathAbs) const
// {
// 	const auto Elem = FoldersTree.Find(InFolderPathAbs);
// 	if (!Elem) return 0;
//
// 	return UProjectCleanerLibrary::GetAssetsTotalSize(Elem->AssetsTotal);
// }
//
// int64 FProjectCleanerScanner::GetSizeUnused(const FString& InFolderPathAbs) const
// {
// 	const auto Elem = FoldersTree.Find(InFolderPathAbs);
// 	if (!Elem) return 0;
//
// 	return UProjectCleanerLibrary::GetAssetsTotalSize(Elem->AssetsUnused);
// }
//
// bool FProjectCleanerScanner::IsEmptyFolder(const FString& InFolderPathAbs) const
// {
// 	return FoldersEmpty.Contains(InFolderPathAbs);
// }
//
// bool FProjectCleanerScanner::IsExcludedFolder(const FString& InFolderPathAbs) const
// {
// 	if (!ScanSettings.IsValid()) return false;
//
// 	for (const auto& ExcludedFolder : ScanSettings->ExcludedFolders)
// 	{
// 		if (InFolderPathAbs.Equals(ExcludedFolder.Path))
// 		{
// 			return true;
// 		}
// 	}
//
// 	return false;
// }
//
// const TMap<FString, FProjectCleanerFolderInfo>& FProjectCleanerScanner::GetFoldersTreeInfo()
// {
// 	return FoldersTree;
// }
//
// const TSet<FString>& FProjectCleanerScanner::GetForbiddenFoldersToScan()
// {
// 	return FoldersForbiddenToScan;
// }
//
// void FProjectCleanerScanner::FindBlacklistAssets()
// {
// 	FARFilter Filter;
// 	Filter.bRecursivePaths = true;
// 	Filter.bRecursiveClasses = true;
// 	Filter.PackagePaths.Add(FName{ProjectCleanerConstants::PathRelRoot});
// 	Filter.ClassNames.Add(UEditorUtilityWidget::StaticClass()->GetFName());
// 	Filter.ClassNames.Add(UEditorUtilityBlueprint::StaticClass()->GetFName());
// 	Filter.ClassNames.Add(UEditorUtilityWidgetBlueprint::StaticClass()->GetFName());
// 	Filter.ClassNames.Add(UMapBuildDataRegistry::StaticClass()->GetFName());
//
// 	ModuleAssetRegistry->Get().GetAssets(Filter, AssetsBlacklist);
// }
//
// void FProjectCleanerScanner::FindPrimaryAssets()
// {
// 	AssetsPrimary.Reset();
//
// 	TArray<FName> PrimaryAssetClasses;
// 	UProjectCleanerLibrary::GetPrimaryAssetClasses(PrimaryAssetClasses, true);
//
// 	FARFilter Filter;
// 	Filter.bRecursiveClasses = true;
// 	Filter.bRecursivePaths = true;
// 	Filter.PackagePaths.Add(FName{ProjectCleanerConstants::PathRelRoot});
//
// 	for (const auto& ClassName : PrimaryAssetClasses)
// 	{
// 		Filter.ClassNames.Add(ClassName);
// 	}
// 	ModuleAssetRegistry->Get().GetAssets(Filter, AssetsPrimary);
//
// 	FARFilter FilterBlueprint;
// 	FilterBlueprint.bRecursivePaths = true;
// 	FilterBlueprint.bRecursiveClasses = true;
// 	FilterBlueprint.PackagePaths.Add(FName{ProjectCleanerConstants::PathRelRoot});
// 	FilterBlueprint.ClassNames.Add(UBlueprint::StaticClass()->GetFName());
//
// 	TArray<FAssetData> BlueprintAssets;
// 	ModuleAssetRegistry->Get().GetAssets(FilterBlueprint, BlueprintAssets);
//
// 	for (const auto& BlueprintAsset : BlueprintAssets)
// 	{
// 		const FName BlueprintClass = FName{*UProjectCleanerLibrary::GetAssetClassName(BlueprintAsset)};
// 		if (PrimaryAssetClasses.Contains(BlueprintClass))
// 		{
// 			AssetsPrimary.AddUnique(BlueprintAsset);
// 		}
// 	}
// }
//
// void FProjectCleanerScanner::FindExcludedAssets()
// {
// 	if (ScanSettings->ExcludedFolders.Num() == 0 && ScanSettings->ExcludedClasses.Num() == 0 && ScanSettings->ExcludedAssets.Num() == 0) return;
//
// 	FARFilter Filter;
//
// 	if (ScanSettings->ExcludedFolders.Num() > 0)
// 	{
// 		for (const auto& ExcludedFolder : ScanSettings->ExcludedFolders)
// 		{
// 			if (!FPaths::DirectoryExists(UProjectCleanerLibrary::PathConvertToAbs(ExcludedFolder.Path))) continue;
//
// 			Filter.PackagePaths.Add(FName{*ExcludedFolder.Path});
// 		}
// 	}
//
// 	if (ScanSettings->ExcludedClasses.Num() > 0)
// 	{
// 		for (const auto& ExcludedClass : ScanSettings->ExcludedClasses)
// 		{
// 			if (!ExcludedClass.LoadSynchronous()) continue;
//
// 			Filter.ClassNames.Add(ExcludedClass->GetFName());
// 		}
// 	}
//
// 	if (!Filter.IsEmpty())
// 	{
// 		Filter.PackagePaths.Add(ProjectCleanerConstants::PathRelRoot);
// 		Filter.bRecursivePaths = true;
// 		Filter.bRecursiveClasses = true;
// 		ModuleAssetRegistry->Get().GetAssets(Filter, AssetsExcluded);
// 	}
//
// 	for (const auto& ExcludedAsset : ScanSettings->ExcludedAssets)
// 	{
// 		AssetsExcluded.AddUnique(ExcludedAsset);
// 	}
// }
//
// void FProjectCleanerScanner::FindIndirectAssets()
// {
// 	UProjectCleanerLibrary::GetAssetsIndirectAdvanced(AssetsIndirectInfo);
//
// 	AssetsIndirect.Reserve(AssetsIndirectInfo.Num());
// 	for (const auto& Info : AssetsIndirectInfo)
// 	{
// 		AssetsIndirect.AddUnique(Info.AssetData);
// 	}
// }
//
// void FProjectCleanerScanner::FindWithExternalRefsAssets()
// {
// 	TArray<FName> Refs;
// 	for (const auto& Asset : AssetsAll)
// 	{
// 		ModuleAssetRegistry->Get().GetReferencers(Asset.PackageName, Refs);
//
// 		const bool HasExternalRefs = Refs.ContainsByPredicate([](const FName& Ref)
// 		{
// 			return !Ref.ToString().StartsWith(ProjectCleanerConstants::PathRelRoot.ToString());
// 		});
//
// 		if (HasExternalRefs)
// 		{
// 			AssetsWithExternalRefs.AddUnique(Asset);
// 		}
//
// 		Refs.Reset();
// 	}
// }
//
// void FProjectCleanerScanner::CreateFoldersInfoTree()
// {
// 	if (!ScanSettings.IsValid()) return;
//
// 	TSet<FString> AllFolders;
// 	AllFolders.Append(FoldersAll);
// 	AllFolders.Add(FPaths::ProjectContentDir());
// 	
// 	FoldersTree.Reserve(FoldersAll.Num());
//
// 	for (const auto& Folder : FoldersAll)
// 	{
// 		FProjectCleanerFolderInfo FolderInfo;
// 		FolderInfo.FolderPathAbs = Folder;
// 		FolderInfo.FolderPathRel = UProjectCleanerLibrary::PathConvertToRel(Folder);
// 		FolderInfo.FolderName = FPaths::GetPathLeaf(Folder);
//
// 		// getting 1st level subfolders
// 		TArray<FString> SubFolders;
// 		IFileManager::Get().FindFiles(SubFolders, *(Folder / TEXT("*")), false, true);
//
// 		for (const auto& SubFolder : SubFolders)
// 		{
// 			if (UProjectCleanerLibrary::IsUnderForbiddenFolders(SubFolder, FoldersForbiddenToScan)) continue;
//
// 			FolderInfo.SubFolders.Add(Folder / SubFolder);
//
// 			if (FoldersEmpty.Contains(SubFolder))
// 			{
// 				FolderInfo.SubFoldersEmpty.Add(SubFolder);
// 			}
// 		}
//
// 		// getting all subfolders
// 		TArray<FString> AllSubFolders;
// 		IFileManager::Get().FindFilesRecursive(AllSubFolders, *Folder, TEXT("*.*"), false, true);
//
// 		for (const auto& SubFolder : AllSubFolders)
// 		{
// 			if (UProjectCleanerLibrary::IsUnderForbiddenFolders(SubFolder, FoldersForbiddenToScan)) continue;
//
// 			FolderInfo.SubFoldersAll.Add(SubFolder);
//
// 			if (FoldersEmpty.Contains(SubFolder))
// 			{
// 				FolderInfo.SubFoldersEmpty.Add(SubFolder);
// 			}
// 		}
//
// 		for (const auto& Asset : AssetsAll)
// 		{
// 			const auto AssetAbsPath = UProjectCleanerLibrary::PathConvertToAbs(Asset.PackagePath.ToString());
// 			
// 			if (UProjectCleanerLibrary::IsUnderFolder(AssetAbsPath, Folder))
// 			{
// 				FolderInfo.AssetsTotal.AddUnique(Asset);
// 				
// 				if (AssetsUnused.Contains(Asset))
// 				{
// 					FolderInfo.AssetsUnused.AddUnique(Asset);
// 				}
// 			}
// 		}
//
// 		FolderInfo.bExcluded = false;
// 		for (const auto& ExcludedFolder : ScanSettings->ExcludedFolders)
// 		{
// 			if (Folder.Equals(ExcludedFolder.Path))
// 			{
// 				FolderInfo.bExcluded = true;
// 				break;
// 			}
// 		}
// 		
// 		FolderInfo.bDevFolder = Folder.Equals(FPaths::ProjectContentDir() / TEXT("Developers"));
// 		FolderInfo.bEmpty = FoldersEmpty.Contains(Folder);
//
// 		FoldersTree.Add(Folder, FolderInfo);
// 	}
// }
