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

class FContentFolderVisitor final : public IPlatformFile::FDirectoryVisitor
{
public:
	TSet<FString>& FilesCorrupted;
	TSet<FString>& FilesNonEngine;
	TSet<FString>& FoldersAll;
	TSet<FString>& FoldersEmpty;
	const TSet<FString>& FoldersForbiddenToDelete;
	const TSet<FString>& FoldersForbiddenToScan;

	FContentFolderVisitor(
		TSet<FString>& InFilesCorrupted,
		TSet<FString>& InFilesNonEngine,
		TSet<FString>& InFoldersAll,
		TSet<FString>& InFoldersEmpty,
		const TSet<FString>& InFoldersForbiddenToDelete,
		const TSet<FString>& InFoldersForbiddenToScan
	)
		: FDirectoryVisitor(EDirectoryVisitorFlags::None),
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
			if (UProjectCleanerLibrary::IsUnderForbiddenFolders(FullPath, FoldersForbiddenToScan)) return true;

			FoldersAll.Add(FullPath);

			TArray<FString> Files;
			IFileManager::Get().FindFilesRecursive(Files, *FullPath, TEXT("*.*"), true, false);

			// {
			// 	// filling folder tree as we progress
			// 	TArray<FString> AllSubFolders;
			// 	IFileManager::Get().FindFilesRecursive(AllSubFolders, *FullPath, TEXT("*.*"), false, true);
			// 	
			// 	TArray<FString> SubFolders;
			// 	IFileManager::Get().FindFiles(SubFolders, *FullPath, false, true);
			//
			// 	
			// 	FProjectCleanerFolder CleanerFolder;
			// 	CleanerFolder.DirPathAbs = FullPath;
			// 	
			// 	for (const auto& Folder : AllSubFolders)
			// 	{
			// 		if (UProjectCleanerLibrary::IsUnderForbiddenFolders(Folder, FoldersForbiddenToScan)) continue;
			// 		
			// 		CleanerFolder.SubFoldersAll.Add(Folder);	
			// 	}
			// 	for (const auto& Folder : SubFolders)
			// 	{
			// 		if (UProjectCleanerLibrary::IsUnderForbiddenFolders(Folder, FoldersForbiddenToScan)) continue;
			// 		
			// 		CleanerFolder.SubFolders.Add(Folder);	
			// 	}
			// 	
			// 	FoldersTree.Add(FullPath, CleanerFolder);
			// }

			if (Files.Num() == 0)
			{
				if (!UProjectCleanerLibrary::IsUnderForbiddenFolders(FullPath, FoldersForbiddenToDelete))
				{
					FoldersEmpty.Add(FullPath);
				}
			}

			return true;
		}

		FString FileName;
		FString FileExtension;
		FString Path;
		FPaths::Split(FullPath, Path, FileName, FileExtension);

		if (UProjectCleanerLibrary::IsUnderForbiddenFolders(Path, FoldersForbiddenToScan)) return true;

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
	if (UProjectCleanerLibrary::IsAssetRegistryWorking())
	{
		UE_LOG(LogProjectCleaner, Warning, TEXT("Cant start scaning, becayse AssetRegistry still working"));
		return;
	}

	if (!ModuleAssetRegistry) return;

	if (!InScanSettings.IsValid())
	{
		UE_LOG(LogProjectCleaner, Error, TEXT("Invalid ScanSettings"));
		return;
	}

	ScanSettings = InScanSettings;

	Reset();

	// todo:ashe23 add slow task here

	// // 0. before we start scanning project, we need to make sure its assets saved and all redirectors are fixed
	// UProjectCleanerLibrary::FixupRedirectors();
	// UProjectCleanerLibrary::SaveAllAssets(!bSilentMode);
	//
	// // 1. filling forbidden folders
	// FoldersForbiddenToScan.Add(FPaths::ProjectContentDir() / TEXT("Collections"));
	// FoldersForbiddenToScan.Add(FPaths::GameUserDeveloperDir() / TEXT("Collections"));
	// // todo:ashe23 for ue5 add __ExternalObject__ and __ExternalActors__ folders
	//
	// FoldersForbiddenToDelete.Add(FPaths::ProjectContentDir() / TEXT("Collections"));
	// FoldersForbiddenToDelete.Add(FPaths::GameUserDeveloperDir() / TEXT("Collections"));
	//
	// // if Megascans plugin enabled, we must not delete MSPresets folder either or any of its assets, its contains important base assets for other megascans library assets
	// if (FModuleManager::Get().IsModuleLoaded(ProjectCleanerConstants::PluginMegascans))
	// {
	// 	FoldersForbiddenToScan.Add(FPaths::ProjectContentDir() / ProjectCleanerConstants::PluginMegascansMsPresetsFolder.ToString());
	// 	FoldersForbiddenToDelete.Add(UProjectCleanerLibrary::PathConvertToAbs(ProjectCleanerConstants::PathRelMegascansPresets.ToString()));
	// }
	//
	// // we can scan Developers folder if user specified so, but we must not delete it (only children folders)
	// FoldersForbiddenToDelete.Add(FPaths::ProjectContentDir() / TEXT("Developers"));
	// if (!ScanSettings->bScanDeveloperContents)
	// {
	// 	FoldersForbiddenToScan.Add(FPaths::ProjectContentDir() / TEXT("Developers"));
	// }
	//
	//
	// // 2. scanning Content folder
	// FContentFolderVisitor ContentFolderVisitor{
	// 	FilesCorrupted,
	// 	FilesNonEngine,
	// 	FoldersAll,
	// 	FoldersEmpty,
	// 	FoldersForbiddenToDelete,
	// 	FoldersForbiddenToScan
	// };
	// IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	// PlatformFile.IterateDirectoryRecursively(*FPaths::ProjectContentDir(), ContentFolderVisitor);
	//
	// // 3. searching for used assets
	// ModuleAssetRegistry->Get().GetAssetsByPath(ProjectCleanerConstants::PathRelRoot, AssetsAll, true);
	// FindBlacklistAssets();
	// FindPrimaryAssets();
	// FindIndirectAssets();
	// FindWithExternalRefsAssets();
	// FindExcludedAssets();
	//
	// AssetsUsed.Reserve(AssetsAll.Num());
	//
	// for (const auto& Asset : AssetsPrimary)
	// {
	// 	AssetsUsed.AddUnique(Asset);
	// }
	// for (const auto& Asset : AssetsIndirect)
	// {
	// 	AssetsUsed.AddUnique(Asset);
	// }
	// for (const auto& Asset : AssetsWithExternalRefs)
	// {
	// 	AssetsUsed.AddUnique(Asset);
	// }
	// for (const auto& Asset : AssetsExcluded)
	// {
	// 	AssetsUsed.AddUnique(Asset);
	// }
	// for (const auto& Asset : AssetsBlacklist)
	// {
	// 	AssetsUsed.AddUnique(Asset);
	// }
	//
	// TArray<FAssetData> LinkedAssets;
	// UProjectCleanerLibrary::GetLinkedAssets(AssetsUsed, LinkedAssets);
	// for (const auto& Asset : LinkedAssets)
	// {
	// 	AssetsUsed.Add(Asset);
	// }
	//
	// AssetsUsed.Shrink();
	//
	// AssetsUnused.Reserve(AssetsAll.Num());
	//
	// // 4. excluding used assets from all assets and we have unused assets in project
	// for (const auto& Asset : AssetsAll)
	// {
	// 	if (AssetsUsed.Contains(Asset)) continue;
	//
	// 	const FString AbsPath = UProjectCleanerLibrary::PathConvertToAbs(Asset.PackagePath.ToString());
	// 	if (UProjectCleanerLibrary::IsUnderForbiddenFolders(AbsPath, FoldersForbiddenToScan)) continue;
	//
	// 	AssetsUnused.AddUnique(Asset);
	// }
	//
	// AssetsUnused.Shrink();
	//
	// // 5. creating folders tree data
	// CreateFoldersInfoTree();

	UProjectCleanerApi::GetAssetsUnused(TEXT("/Game/StarterContent/Materials"), ScanSettings.Get(), AssetsUnused);

	return;
}

void FProjectCleanerScanner::Reset()
{
	FoldersAll.Reset();
	FoldersEmpty.Reset();
	FoldersForbiddenToScan.Reset();
	FoldersForbiddenToDelete.Reset();
	FoldersTree.Reset();

	FilesCorrupted.Reset();
	FilesNonEngine.Reset();

	AssetsAll.Reset();
	AssetsUsed.Reset();
	AssetsUnused.Reset();
	AssetsPrimary.Reset();
	AssetsExcluded.Reset();
	AssetsIndirect.Reset();
	AssetsIndirectInfo.Reset();
	AssetsWithExternalRefs.Reset();
}

void FProjectCleanerScanner::GetSubFolders(const FString& InFolderPathAbs, TSet<FString>& SubFolders)
{
	SubFolders.Reset();
	
	const auto Elem = FoldersTree.Find(InFolderPathAbs);
	if (!Elem) return;

	SubFolders.Append(Elem->SubFolders);
}

int32 FProjectCleanerScanner::GetFoldersTotalNum(const FString& InFolderPathAbs) const
{
	const auto Elem = FoldersTree.Find(InFolderPathAbs);
	if (!Elem) return 0;

	return Elem->SubFoldersAll.Num();
}

int32 FProjectCleanerScanner::GetFoldersEmptyNum(const FString& InFolderPathAbs) const
{
	const auto Elem = FoldersTree.Find(InFolderPathAbs);
	if (!Elem) return 0;

	return Elem->SubFoldersEmpty.Num();
}

int32 FProjectCleanerScanner::GetAssetsTotalNum(const FString& InFolderPathAbs) const
{
	const auto Elem = FoldersTree.Find(InFolderPathAbs);
	if (!Elem) return 0;

	return Elem->AssetsTotal.Num();
}

int32 FProjectCleanerScanner::GetAssetsUnusedNum(const FString& InFolderPathAbs) const
{
	const auto Elem = FoldersTree.Find(InFolderPathAbs);
	if (!Elem) return 0;

	return Elem->AssetsUnused.Num();
}

int64 FProjectCleanerScanner::GetSizeTotal(const FString& InFolderPathAbs) const
{
	const auto Elem = FoldersTree.Find(InFolderPathAbs);
	if (!Elem) return 0;

	return UProjectCleanerLibrary::GetAssetsTotalSize(Elem->AssetsTotal);
}

int64 FProjectCleanerScanner::GetSizeUnused(const FString& InFolderPathAbs) const
{
	const auto Elem = FoldersTree.Find(InFolderPathAbs);
	if (!Elem) return 0;

	return UProjectCleanerLibrary::GetAssetsTotalSize(Elem->AssetsUnused);
}

bool FProjectCleanerScanner::IsEmptyFolder(const FString& InFolderPathAbs) const
{
	return FoldersEmpty.Contains(InFolderPathAbs);
}

bool FProjectCleanerScanner::IsExcludedFolder(const FString& InFolderPathAbs) const
{
	if (!ScanSettings.IsValid()) return false;

	for (const auto& ExcludedFolder : ScanSettings->ExcludedFolders)
	{
		if (InFolderPathAbs.Equals(ExcludedFolder.Path))
		{
			return true;
		}
	}

	return false;
}

const TMap<FString, FProjectCleanerFolderInfo>& FProjectCleanerScanner::GetFoldersTreeInfo()
{
	return FoldersTree;
}

const TSet<FString>& FProjectCleanerScanner::GetForbiddenFoldersToScan()
{
	return FoldersForbiddenToScan;
}

void FProjectCleanerScanner::FindBlacklistAssets()
{
	FARFilter Filter;
	Filter.bRecursivePaths = true;
	Filter.bRecursiveClasses = true;
	Filter.PackagePaths.Add(FName{ProjectCleanerConstants::PathRelRoot});
	Filter.ClassNames.Add(UEditorUtilityWidget::StaticClass()->GetFName());
	Filter.ClassNames.Add(UEditorUtilityBlueprint::StaticClass()->GetFName());
	Filter.ClassNames.Add(UEditorUtilityWidgetBlueprint::StaticClass()->GetFName());
	Filter.ClassNames.Add(UMapBuildDataRegistry::StaticClass()->GetFName());

	ModuleAssetRegistry->Get().GetAssets(Filter, AssetsBlacklist);
}

void FProjectCleanerScanner::FindPrimaryAssets()
{
	AssetsPrimary.Reset();

	TArray<FName> PrimaryAssetClasses;
	UProjectCleanerLibrary::GetPrimaryAssetClasses(PrimaryAssetClasses, true);

	FARFilter Filter;
	Filter.bRecursiveClasses = true;
	Filter.bRecursivePaths = true;
	Filter.PackagePaths.Add(FName{ProjectCleanerConstants::PathRelRoot});

	for (const auto& ClassName : PrimaryAssetClasses)
	{
		Filter.ClassNames.Add(ClassName);
	}
	ModuleAssetRegistry->Get().GetAssets(Filter, AssetsPrimary);

	FARFilter FilterBlueprint;
	FilterBlueprint.bRecursivePaths = true;
	FilterBlueprint.bRecursiveClasses = true;
	FilterBlueprint.PackagePaths.Add(FName{ProjectCleanerConstants::PathRelRoot});
	FilterBlueprint.ClassNames.Add(UBlueprint::StaticClass()->GetFName());

	TArray<FAssetData> BlueprintAssets;
	ModuleAssetRegistry->Get().GetAssets(FilterBlueprint, BlueprintAssets);

	for (const auto& BlueprintAsset : BlueprintAssets)
	{
		const FName BlueprintClass = FName{*UProjectCleanerLibrary::GetAssetClassName(BlueprintAsset)};
		if (PrimaryAssetClasses.Contains(BlueprintClass))
		{
			AssetsPrimary.AddUnique(BlueprintAsset);
		}
	}
}

void FProjectCleanerScanner::FindExcludedAssets()
{
	if (ScanSettings->ExcludedFolders.Num() == 0 && ScanSettings->ExcludedClasses.Num() == 0 && ScanSettings->ExcludedAssets.Num() == 0) return;

	FARFilter Filter;

	if (ScanSettings->ExcludedFolders.Num() > 0)
	{
		for (const auto& ExcludedFolder : ScanSettings->ExcludedFolders)
		{
			if (!FPaths::DirectoryExists(UProjectCleanerLibrary::PathConvertToAbs(ExcludedFolder.Path))) continue;

			Filter.PackagePaths.Add(FName{*ExcludedFolder.Path});
		}
	}

	if (ScanSettings->ExcludedClasses.Num() > 0)
	{
		for (const auto& ExcludedClass : ScanSettings->ExcludedClasses)
		{
			if (!ExcludedClass.LoadSynchronous()) continue;

			Filter.ClassNames.Add(ExcludedClass->GetFName());
		}
	}

	if (!Filter.IsEmpty())
	{
		Filter.PackagePaths.Add(ProjectCleanerConstants::PathRelRoot);
		Filter.bRecursivePaths = true;
		Filter.bRecursiveClasses = true;
		ModuleAssetRegistry->Get().GetAssets(Filter, AssetsExcluded);
	}

	for (const auto& ExcludedAsset : ScanSettings->ExcludedAssets)
	{
		AssetsExcluded.AddUnique(ExcludedAsset);
	}
}

void FProjectCleanerScanner::FindIndirectAssets()
{
	UProjectCleanerLibrary::GetAssetsIndirectAdvanced(AssetsIndirectInfo);

	AssetsIndirect.Reserve(AssetsIndirectInfo.Num());
	for (const auto& Info : AssetsIndirectInfo)
	{
		AssetsIndirect.AddUnique(Info.AssetData);
	}
}

void FProjectCleanerScanner::FindWithExternalRefsAssets()
{
	TArray<FName> Refs;
	for (const auto& Asset : AssetsAll)
	{
		ModuleAssetRegistry->Get().GetReferencers(Asset.PackageName, Refs);

		const bool HasExternalRefs = Refs.ContainsByPredicate([](const FName& Ref)
		{
			return !Ref.ToString().StartsWith(ProjectCleanerConstants::PathRelRoot.ToString());
		});

		if (HasExternalRefs)
		{
			AssetsWithExternalRefs.AddUnique(Asset);
		}

		Refs.Reset();
	}
}

void FProjectCleanerScanner::CreateFoldersInfoTree()
{
	if (!ScanSettings.IsValid()) return;

	TSet<FString> AllFolders;
	AllFolders.Append(FoldersAll);
	AllFolders.Add(FPaths::ProjectContentDir());
	
	FoldersTree.Reserve(FoldersAll.Num());

	for (const auto& Folder : FoldersAll)
	{
		FProjectCleanerFolderInfo FolderInfo;
		FolderInfo.FolderPathAbs = Folder;
		FolderInfo.FolderPathRel = UProjectCleanerLibrary::PathConvertToRel(Folder);
		FolderInfo.FolderName = FPaths::GetPathLeaf(Folder);

		// getting 1st level subfolders
		TArray<FString> SubFolders;
		IFileManager::Get().FindFiles(SubFolders, *(Folder / TEXT("*")), false, true);

		for (const auto& SubFolder : SubFolders)
		{
			if (UProjectCleanerLibrary::IsUnderForbiddenFolders(SubFolder, FoldersForbiddenToScan)) continue;

			FolderInfo.SubFolders.Add(Folder / SubFolder);

			if (FoldersEmpty.Contains(SubFolder))
			{
				FolderInfo.SubFoldersEmpty.Add(SubFolder);
			}
		}

		// getting all subfolders
		TArray<FString> AllSubFolders;
		IFileManager::Get().FindFilesRecursive(AllSubFolders, *Folder, TEXT("*.*"), false, true);

		for (const auto& SubFolder : AllSubFolders)
		{
			if (UProjectCleanerLibrary::IsUnderForbiddenFolders(SubFolder, FoldersForbiddenToScan)) continue;

			FolderInfo.SubFoldersAll.Add(SubFolder);

			if (FoldersEmpty.Contains(SubFolder))
			{
				FolderInfo.SubFoldersEmpty.Add(SubFolder);
			}
		}

		for (const auto& Asset : AssetsAll)
		{
			const auto AssetAbsPath = UProjectCleanerLibrary::PathConvertToAbs(Asset.PackagePath.ToString());
			
			if (UProjectCleanerLibrary::IsUnderFolder(AssetAbsPath, Folder))
			{
				FolderInfo.AssetsTotal.AddUnique(Asset);
				
				if (AssetsUnused.Contains(Asset))
				{
					FolderInfo.AssetsUnused.AddUnique(Asset);
				}
			}
		}

		FolderInfo.bExcluded = false;
		for (const auto& ExcludedFolder : ScanSettings->ExcludedFolders)
		{
			if (Folder.Equals(ExcludedFolder.Path))
			{
				FolderInfo.bExcluded = true;
				break;
			}
		}
		
		FolderInfo.bDevFolder = Folder.Equals(FPaths::ProjectContentDir() / TEXT("Developers"));
		FolderInfo.bEmpty = FoldersEmpty.Contains(Folder);

		FoldersTree.Add(Folder, FolderInfo);
	}
}
