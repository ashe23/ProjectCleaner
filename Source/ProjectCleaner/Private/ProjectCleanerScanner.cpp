// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "ProjectCleanerScanner.h"
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
#include "ProjectCleaner.h"
#include "Engine/AssetManager.h"
#include "Engine/MapBuildDataRegistry.h"
#include "Misc/ScopedSlowTask.h"

class FContentFolderVisitor final : public IPlatformFile::FDirectoryVisitor
{
public:
	// TSet<FString>& FoldersAll;
	TSet<FString>& FoldersEmpty;
	TSet<FString>& FilesCorrupted;
	TSet<FString>& FilesNonEngine;
	const TSet<FString>& FoldersBlacklist;

	FContentFolderVisitor(
		// TSet<FString>& InFoldersAll,
		TSet<FString>& InFoldersEmpty,
		TSet<FString>& InFilesCorrupted,
		TSet<FString>& InFilesNonEngine,
		const TSet<FString>& InFoldersBlacklist
	)
		: FDirectoryVisitor(EDirectoryVisitorFlags::None),
		  // FoldersAll(InFoldersAll),
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

			// FoldersAll.Add(FullPath);

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

FProjectCleanerScanner::FProjectCleanerScanner(const TWeakObjectPtr<UProjectCleanerScanSettings>& InScanSettings)
	: ScanSettings(InScanSettings),
	  ModuleAssetRegistry(FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName))
{
	ScanSettings->OnChange().AddLambda([&]()
	{
		// Scan();
		UE_LOG(LogProjectCleaner, Warning, TEXT("Scanner: ScanSettings Changed!"));
	});
}

void FProjectCleanerScanner::Scan()
{

	return;
	if (UProjectCleanerLibrary::IsAssetRegistryWorking()) return;

	UProjectCleanerLibrary::FixupRedirectors();
	UProjectCleanerLibrary::SaveAllAssets();

	FScopedSlowTask SlowTask{2.0f, FText::FromString(ProjectCleanerConstants::MsgScanning)};
	SlowTask.MakeDialog();

	DataInit();

	FContentFolderVisitor ContentFolderVisitor{
		// FoldersAll,
		FoldersEmpty,
		FilesCorrupted,
		FilesNonEngine,
		FoldersBlacklist
	};
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	PlatformFile.IterateDirectoryRecursively(*FPaths::ProjectContentDir(), ContentFolderVisitor);

	SlowTask.EnterProgressFrame(1.0f);

	// 1. getting all assets in project
	ModuleAssetRegistry.Get().GetAssetsByPath(ProjectCleanerConstants::PathRelRoot, AssetsAll, true);

	// 2. getting all used assets in project
	UProjectCleanerLibrary::GetAssetsIndirectAdvanced(AssetsIndirectAdvanced);

	AssetsIndirect.Reserve(AssetsIndirectAdvanced.Num());
	for (const auto& IndirectAsset : AssetsIndirectAdvanced)
	{
		AssetsIndirect.AddUnique(IndirectAsset.AssetData);
	}

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
		if (ExcludedFolder.Path.IsEmpty()) continue;
		if (!FPaths::DirectoryExists(UProjectCleanerLibrary::PathConvertToAbs(ExcludedFolder.Path))) continue;

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

	ModuleAssetRegistry.Get().UseFilterToExcludeAssets(AssetsUnused, Filter);
	// ModuleAssetRegistry.Get().GetAssets(Filter, AssetsExcluded);
	// todo:ashe23 get excluded assets

	SlowTask.EnterProgressFrame(1.0f);

	if (DelegateScanFinished.IsBound())
	{
		DelegateScanFinished.Broadcast();
	}
}

void FProjectCleanerScanner::CleanProject()
{
	UE_LOG(LogProjectCleaner, Warning, TEXT("Cleaning project"));

	if (DelegateCleanFinished.IsBound())
	{
		DelegateCleanFinished.Broadcast();
	}
}

void FProjectCleanerScanner::DeleteEmptyFolders()
{
	UE_LOG(LogProjectCleaner, Warning, TEXT("Deleting empty folders"));

	if (DelegateEmptyFoldersDeleted.IsBound())
	{
		DelegateEmptyFoldersDeleted.Broadcast();
	}
}

void FProjectCleanerScanner::GetSubFolders(const FString& InFolderPathAbs, TSet<FString>& SubFolders) const
{
	TArray<FString> Folders;
	IFileManager::Get().FindFiles(Folders, *(InFolderPathAbs / TEXT("*")), false, true);

	for (const auto& Folder : Folders)
	{
		const FString FolderAbsPath = InFolderPathAbs / Folder;
		if (UProjectCleanerLibrary::IsUnderAnyFolder(FolderAbsPath, FoldersBlacklist)) continue;

		SubFolders.Add(FolderAbsPath);
	}
}

bool FProjectCleanerScanner::IsFolderEmpty(const FString& InFolderPathAbs) const
{
	return FoldersEmpty.Contains(InFolderPathAbs);
}

bool FProjectCleanerScanner::IsFolderExcluded(const FString& InFolderPathAbs) const
{
	if (InFolderPathAbs.IsEmpty()) return false;
	if (!FPaths::DirectoryExists(InFolderPathAbs)) return false;
	if (!ScanSettings.IsValid()) return false;

	for (const auto& ExcludedFolder : ScanSettings->ExcludedFolders)
	{
		if (ExcludedFolder.Path.IsEmpty()) continue;

		const FString ExcludedAbsPath = UProjectCleanerLibrary::PathConvertToAbs(ExcludedFolder.Path);
		if (!FPaths::DirectoryExists(ExcludedAbsPath)) continue;
		if (UProjectCleanerLibrary::IsUnderFolder(InFolderPathAbs, ExcludedAbsPath))
		{
			return true;
		}
	}

	return false;
}

int32 FProjectCleanerScanner::GetFoldersTotalNum(const FString& InFolderPathAbs) const
{
	if (InFolderPathAbs.IsEmpty()) return 0;
	if (!FPaths::DirectoryExists(InFolderPathAbs)) return 0;

	TArray<FString> AllFolders;
	IFileManager::Get().FindFilesRecursive(AllFolders, *InFolderPathAbs, TEXT("*.*"), false, true);

	int Num = 0;
	for (const auto& Folder : AllFolders)
	{
		if (UProjectCleanerLibrary::IsUnderAnyFolder(Folder, FoldersBlacklist)) continue;

		++Num;
	}

	return Num;
}

int32 FProjectCleanerScanner::GetFoldersEmptyNum(const FString& InFolderPathAbs) const
{
	if (InFolderPathAbs.IsEmpty()) return 0;
	if (!FPaths::DirectoryExists(InFolderPathAbs)) return 0;

	int32 Num = 0;
	for (const auto& EmptyFolder : FoldersEmpty)
	{
		if (EmptyFolder.Equals(InFolderPathAbs)) continue;
		if (UProjectCleanerLibrary::IsUnderFolder(EmptyFolder, InFolderPathAbs))
		{
			++Num;
		}
	}

	return Num;
}

int32 FProjectCleanerScanner::GetAssetTotalNum(const FString& InFolderPathAbs) const
{
	return GetNumFor(InFolderPathAbs, AssetsAll);
}

int32 FProjectCleanerScanner::GetAssetUnusedNum(const FString& InFolderPathAbs) const
{
	return GetNumFor(InFolderPathAbs, AssetsUnused);
}

int64 FProjectCleanerScanner::GetSizeTotal(const FString& InFolderPathAbs) const
{
	return GetSizeFor(InFolderPathAbs, AssetsAll);
}

int64 FProjectCleanerScanner::GetSizeUnused(const FString& InFolderPathAbs) const
{
	return GetSizeFor(InFolderPathAbs, AssetsUnused);
}

const TSet<FString>& FProjectCleanerScanner::GetFoldersEmpty() const
{
	return FoldersEmpty;
}

const TSet<FString>& FProjectCleanerScanner::GetFoldersBlacklist() const
{
	return FoldersBlacklist;
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

const TArray<FAssetData>& FProjectCleanerScanner::GetAssetsIndirect() const
{
	return AssetsIndirect;
}

const TArray<FAssetData>& FProjectCleanerScanner::GetAssetsExcluded() const
{
	return AssetsExcluded;
}

const TArray<FProjectCleanerIndirectAsset>& FProjectCleanerScanner::GetAssetsIndirectAdvanced() const
{
	return AssetsIndirectAdvanced;
}

FProjectCleanerDelegateScanFinished& FProjectCleanerScanner::OnScanFinished()
{
	return DelegateScanFinished;
}

void FProjectCleanerScanner::ScannerInit()
{
	if (!ScanSettings.IsValid()) return;

	ScanSettings->OnChange().AddLambda([&]()
	{
		Scan();
		UE_LOG(LogProjectCleaner, Warning, TEXT("Scanner: ScanSettings Changed!"));
	});
}

void FProjectCleanerScanner::DataInit()
{
	// 1. reset old data
	DataReset();

	// 2. fill blacklist folders
	FoldersBlacklist.Add(FPaths::ProjectContentDir() / ProjectCleanerConstants::FolderCollections.ToString());
	FoldersBlacklist.Add(FPaths::GameUserDeveloperDir() / ProjectCleanerConstants::FolderCollections.ToString());
	// todo:ashe23 for ue5 add __ExternalObject__ and __ExternalActors__ folders

	if (FModuleManager::Get().IsModuleLoaded(ProjectCleanerConstants::PluginNameMegascans))
	{
		FoldersBlacklist.Add(FPaths::ProjectContentDir() / ProjectCleanerConstants::FolderMsPresets.ToString());
	}

	if (!ScanSettings->bScanDeveloperContents)
	{
		FoldersBlacklist.Add(FPaths::ProjectContentDir() / ProjectCleanerConstants::FolderDevelopers.ToString());
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

	ModuleAssetRegistry.Get().GetAssets(Filter, AssetsBlacklist);
}

void FProjectCleanerScanner::DataReset()
{
	FoldersEmpty.Reset();
	FoldersBlacklist.Reset();

	FilesCorrupted.Reset();
	FilesNonEngine.Reset();

	AssetsAll.Reset();
	AssetsPrimary.Reset();
	AssetsIndirect.Reset();
	AssetsExcluded.Reset();
	AssetsIndirectAdvanced.Reset();
	AssetsWithExternalRefs.Reset();
	AssetsBlacklist.Reset();
	AssetsUnused.Reset();
}

int32 FProjectCleanerScanner::GetNumFor(const FString& InFolderPathAbs, const TArray<FAssetData>& Assets)
{
	if (InFolderPathAbs.IsEmpty()) return 0;
	if (!FPaths::DirectoryExists(InFolderPathAbs)) return 0;

	int32 Num = 0;
	for (const auto& Asset : Assets)
	{
		const FString AssetPackagePathAbs = UProjectCleanerLibrary::PathConvertToAbs(Asset.PackagePath.ToString());
		if (!UProjectCleanerLibrary::IsUnderFolder(AssetPackagePathAbs, InFolderPathAbs)) continue;

		++Num;
	}

	return Num;
}

int64 FProjectCleanerScanner::GetSizeFor(const FString& InFolderPathAbs, const TArray<FAssetData>& Assets)
{
	if (InFolderPathAbs.IsEmpty()) return 0;
	if (!FPaths::DirectoryExists(InFolderPathAbs)) return 0;

	const FAssetRegistryModule& ModuleAssetRegistry = FModuleManager::GetModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);

	int64 Size = 0;
	for (const auto& Asset : Assets)
	{
		const FString AssetPackagePathAbs = UProjectCleanerLibrary::PathConvertToAbs(Asset.PackagePath.ToString());
		if (!UProjectCleanerLibrary::IsUnderFolder(AssetPackagePathAbs, InFolderPathAbs)) continue;

		const auto AssetPackageData = ModuleAssetRegistry.Get().GetAssetPackageData(Asset.PackageName);
		if (!AssetPackageData) continue;
		Size += AssetPackageData->DiskSize;
	}

	return Size;
}
