// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "ProjectCleanerSubsystem.h"
#include "ProjectCleaner.h"
#include "ProjectCleanerConstants.h"
// Engine Headers
#include "AssetToolsModule.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Internationalization/Regex.h"
#include "Misc/FileHelper.h"

UProjectCleanerSubsystem::UProjectCleanerSubsystem()
	:
	PlatformFile(&FPlatformFileManager::Get().GetPlatformFile()),
	ModuleAssetRegistry(&FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName)),
	ModuleAssetTools(&FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")))
{
}

void UProjectCleanerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UProjectCleanerSubsystem::Deinitialize()
{
	Super::Deinitialize();

	// todo:ashe23 clean cached data
}

#if WITH_EDITOR
void UProjectCleanerSubsystem::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	SaveConfig();
}
#endif

int64 UProjectCleanerSubsystem::GetAssetsTotalSize(const TArray<FAssetData>& Assets) const
{
	if (!ModuleAssetRegistry) return 0;

	int64 Size = 0;

	for (const auto& Asset : Assets)
	{
		const auto AssetPackageData = ModuleAssetRegistry->Get().GetAssetPackageData(Asset.PackageName);
		if (!AssetPackageData) continue;
		Size += AssetPackageData->DiskSize;
	}

	return Size;
}

int64 UProjectCleanerSubsystem::GetFilesTotalSize(const TSet<FString>& Files) const
{
	if (Files.Num() == 0) return 0;

	int64 TotalSize = 0;
	for (const auto& File : Files)
	{
		if (File.IsEmpty() || !FPaths::FileExists(File)) continue;

		TotalSize += IFileManager::Get().FileSize(*File);
	}

	return TotalSize;
}

bool UProjectCleanerSubsystem::FileContainsIndirectAssets(const FString& FileContent)
{
	if (FileContent.IsEmpty()) return false;

	// search any sub string that has asset package path in it
	static FRegexPattern Pattern(TEXT(R"(\/Game([A-Za-z0-9_.\/]+)\b)"));
	FRegexMatcher Matcher(Pattern, FileContent);
	return Matcher.FindNext();
}

bool UProjectCleanerSubsystem::FileHasEngineExtension(const FString& Extension)
{
	TSet<FString> EngineExtensions;
	EngineExtensions.Reserve(3);
	EngineExtensions.Add(TEXT("uasset"));
	EngineExtensions.Add(TEXT("umap"));
	EngineExtensions.Add(TEXT("collection"));

	return EngineExtensions.Contains(Extension.ToLower());
}

bool UProjectCleanerSubsystem::FileIsCorrupted(const FString& InFilePathAbs)
{
	if (!FPaths::FileExists(InFilePathAbs)) return false;

	const FString NormalizedPath = PathNormalize(InFilePathAbs);
	const FString RelativePath = NormalizedPath.Replace(*(FPaths::ProjectDir() / TEXT("Content")), *ProjectCleanerConstants::PathRelRoot.ToString(), ESearchCase::CaseSensitive);

	// here we got absolute path "C:/MyProject/Content/material.uasset"
	// we must first convert that path to In Engine Internal Path like "/Game/material.uasset"
	// const FString RelativePath = Convert(InFilePathAbs, EProjectCleanerPathType::Relative);
	if (RelativePath.IsEmpty()) return false;

	// Converting file path to object path (This is for searching in AssetRegistry)
	// example "/Game/Name.uasset" => "/Game/Name.Name"
	FString ObjectPath = RelativePath;
	ObjectPath.RemoveFromEnd(FPaths::GetExtension(RelativePath, true));
	ObjectPath.Append(TEXT(".") + FPaths::GetBaseFilename(RelativePath));

	const FAssetData AssetData = ModuleAssetRegistry->Get().GetAssetByObjectPath(FName{*ObjectPath});

	// if its does not exist in asset registry, then something wrong with asset
	return !AssetData.IsValid();
}

FString UProjectCleanerSubsystem::PathNormalize(const FString& InPath)
{
	FString Path = InPath;
	FPaths::RemoveDuplicateSlashes(Path);
	FPaths::NormalizeDirectoryName(Path);

	return Path;
}

const TArray<FAssetData>& UProjectCleanerSubsystem::GetAssetsAll() const
{
	return AssetsAll;
}

const TArray<FAssetData>& UProjectCleanerSubsystem::GetAssetsIndirect() const
{
	return AssetsIndirect;
}

const TArray<FAssetData>& UProjectCleanerSubsystem::GetAssetsExcluded() const
{
	return AssetsExcluded;
}

const TArray<FAssetData>& UProjectCleanerSubsystem::GetAssetsUnused() const
{
	return AssetsUnused;
}

const TSet<FString>& UProjectCleanerSubsystem::GetFoldersTotal() const
{
	return FoldersTotal;
}

const TSet<FString>& UProjectCleanerSubsystem::GetFoldersEmpty() const
{
	return FoldersEmpty;
}

const TSet<FString>& UProjectCleanerSubsystem::GetFilesCorrupted() const
{
	return FilesCorrupted;
}

const TSet<FString>& UProjectCleanerSubsystem::GetFilesNonEngine() const
{
	return FilesNonEngine;
}

bool UProjectCleanerSubsystem::IsAssetRegistryWorking() const
{
	if (!ModuleAssetRegistry) return false;

	return ModuleAssetRegistry->Get().IsLoadingAssets();
}

bool UProjectCleanerSubsystem::IsEditorInPlayMode() const
{
	if (!GEditor) return false;

	return GEditor->PlayWorld || GIsPlayInEditorWorld;
}

bool UProjectCleanerSubsystem::IsScanningProject() const
{
	return bScanningProject;
}

bool UProjectCleanerSubsystem::IsCleaningProject() const
{
	return bCleaningProject;
}

void UProjectCleanerSubsystem::ProjectScan()
{
	if (!CanScanProject()) return;
	if (!ModuleAssetRegistry) return;

	ResetData();
	// todo;ashe23 fixup redirectors
	// save all assets
	bScanningProject = true;

	FindFoldersTotal();
	FindFoldersEmpty();
	FindFilesCorrupted();
	FindFilesNonEngine();
	FindAssetsAll();
	FindAssetsIndirect();
	FindAssetsExcluded();
	FindAssetsUnused();

	bScanningProject = false;

	if (DelegateProjectScanned.IsBound())
	{
		DelegateProjectScanned.Broadcast();
	}
}

FProjectCleanerDelegateProjectScanned& UProjectCleanerSubsystem::OnProjectScanned()
{
	return DelegateProjectScanned;
}

bool UProjectCleanerSubsystem::CanScanProject() const
{
	return !IsAssetRegistryWorking() && !IsEditorInPlayMode() && !IsScanningProject() && !IsCleaningProject();
}

void UProjectCleanerSubsystem::FindAssetsAll()
{
	ModuleAssetRegistry->Get().GetAssetsByPath(ProjectCleanerConstants::PathRelRoot, AssetsAll);
}

void UProjectCleanerSubsystem::FindAssetsIndirect()
{
	const FString SourceDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() + TEXT("Source/"));
	const FString ConfigDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() + TEXT("Config/"));
	const FString PluginsDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() + TEXT("Plugins/"));

	TSet<FString> Files;
	Files.Reserve(200); // reserving some space

	// 1) finding all source files in main project "Source" directory (<yourproject>/Source/*)
	TArray<FString> FilesToScan;
	PlatformFile->FindFilesRecursively(FilesToScan, *SourceDir, TEXT(".cs"));
	PlatformFile->FindFilesRecursively(FilesToScan, *SourceDir, TEXT(".cpp"));
	PlatformFile->FindFilesRecursively(FilesToScan, *SourceDir, TEXT(".h"));
	PlatformFile->FindFilesRecursively(FilesToScan, *ConfigDir, TEXT(".ini"));
	Files.Append(FilesToScan);

	// 2) we should find all source files in plugins folder (<yourproject>/Plugins/*)
	TArray<FString> ProjectPluginsFiles;
	// finding all installed plugins in "Plugins" directory
	struct FDirectoryVisitor : IPlatformFile::FDirectoryVisitor
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

	FDirectoryVisitor Visitor;
	PlatformFile->IterateDirectory(*PluginsDir, Visitor);

	// 3) for every installed plugin we scanning only "Source" and "Config" folders
	for (const auto& Dir : Visitor.InstalledPlugins)
	{
		const FString PluginSourcePathDir = Dir + "/Source";
		const FString PluginConfigPathDir = Dir + "/Config";

		PlatformFile->FindFilesRecursively(ProjectPluginsFiles, *PluginSourcePathDir, TEXT(".cs"));
		PlatformFile->FindFilesRecursively(ProjectPluginsFiles, *PluginSourcePathDir, TEXT(".cpp"));
		PlatformFile->FindFilesRecursively(ProjectPluginsFiles, *PluginSourcePathDir, TEXT(".h"));
		PlatformFile->FindFilesRecursively(ProjectPluginsFiles, *PluginConfigPathDir, TEXT(".ini"));
	}

	Files.Append(ProjectPluginsFiles);
	Files.Shrink();

	for (const auto& File : Files)
	{
		if (!PlatformFile->FileExists(*File)) continue;

		FString FileContent;
		FFileHelper::LoadFileToString(FileContent, *File);

		if (!FileContainsIndirectAssets(FileContent)) continue;

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


			const FAssetData* AssetData = AssetsAll.FindByPredicate([&](const FAssetData& Elem)
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
				AssetsIndirectInfos.AddUnique(IndirectAsset);
				AssetsIndirect.AddUnique(*AssetData);
			}
		}
	}
}

void UProjectCleanerSubsystem::FindAssetsExcluded()
{
}

void UProjectCleanerSubsystem::FindAssetsUnused()
{
}

void UProjectCleanerSubsystem::FindFoldersTotal()
{
}

void UProjectCleanerSubsystem::FindFoldersEmpty()
{
}

void UProjectCleanerSubsystem::FindFilesCorrupted()
{
}

void UProjectCleanerSubsystem::FindFilesNonEngine()
{
}

void UProjectCleanerSubsystem::ResetData()
{
	AssetsAll.Reset();
	AssetsIndirect.Reset();
	AssetsExcluded.Reset();
	AssetsUnused.Reset();
	AssetsIndirectInfos.Reset();
	FoldersTotal.Reset();
	FoldersEmpty.Reset();
	FilesCorrupted.Reset();
	FilesNonEngine.Reset();
}
