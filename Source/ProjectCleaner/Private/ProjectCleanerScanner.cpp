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

const TArray<FProjectCleanerIndirectAsset>& FProjectCleanerScanner::GetAssetsIndirectAdvanced() const
{
	return AssetsIndirectAdvanced;
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
	AssetsIndirectAdvanced.Reset();
	AssetsWithExternalRefs.Reset();
	AssetsBlacklist.Reset();
	AssetsUnused.Reset();
}
