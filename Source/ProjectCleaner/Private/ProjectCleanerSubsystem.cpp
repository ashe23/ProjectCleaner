// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "ProjectCleanerSubsystem.h"
#include "ProjectCleaner.h"
#include "ProjectCleanerConstants.h"
// Engine Headers
#include "AssetToolsModule.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Internationalization/Regex.h"
#include "Misc/FileHelper.h"
#include "Misc/ScopedSlowTask.h"
#include "EditorUtilityBlueprint.h"
#include "EditorUtilityWidget.h"
#include "EditorUtilityWidgetBlueprint.h"
#include "FileHelpers.h"
#include "Engine/MapBuildDataRegistry.h"
#include "Engine/AssetManager.h"
#include "Settings/ProjectCleanerExcludeSettings.h"

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

	ResetData();
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

void UProjectCleanerSubsystem::GetLinkedAssets(const TArray<FAssetData>& Assets, TArray<FAssetData>& LinkedAssets)
{
	LinkedAssets.Reset();

	TSet<FName> UsedAssetsDeps;
	TArray<FName> Stack;
	for (const auto& Asset : Assets)
	{
		UsedAssetsDeps.Add(Asset.PackageName);
		Stack.Add(Asset.PackageName);

		TArray<FName> Deps;
		while (Stack.Num() > 0)
		{
			const auto CurrentPackageName = Stack.Pop(false);
			Deps.Reset();

			ModuleAssetRegistry->Get().GetDependencies(CurrentPackageName, Deps);

			Deps.RemoveAllSwap([&](const FName& Dep)
			{
				return !Dep.ToString().StartsWith(*ProjectCleanerConstants::PathRelRoot.ToString());
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

	FARFilter Filter;
	Filter.bRecursivePaths = true;
	Filter.PackagePaths.Add(ProjectCleanerConstants::PathRelRoot);

	for (const auto& Dep : UsedAssetsDeps)
	{
		Filter.PackageNames.Add(Dep);
	}

	ModuleAssetRegistry->Get().GetAssets(Filter, LinkedAssets);
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

	const FString RelativePath = PathConvertToRel(InFilePathAbs);

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

bool UProjectCleanerSubsystem::FolderIsExcluded(const FString& InPath)
{
	const UProjectCleanerExcludeSettings* ExcludeSettings = GetDefault<UProjectCleanerExcludeSettings>();
	ensure(ExcludeSettings);

	const FString PathRelative = PathConvertToRel(InPath);

	if (PathRelative.IsEmpty()) return false;

	for (const auto& ExcludedFolder : ExcludeSettings->ExcludedFolders)
	{
		if (ExcludedFolder.Path.Equals(PathRelative, ESearchCase::CaseSensitive))
		{
			return true;
		}
	}

	return false;
}

bool UProjectCleanerSubsystem::FolderIsEmpty(const FString& InPath)
{
	return FoldersEmpty.Contains(PathConvertToAbs(InPath));
}

FString UProjectCleanerSubsystem::PathNormalize(const FString& InPath)
{
	FString Path = FPaths::ConvertRelativePathToFull(InPath);
	FPaths::RemoveDuplicateSlashes(Path);
	FPaths::NormalizeDirectoryName(Path);

	return Path;
}

FString UProjectCleanerSubsystem::PathConvertToAbs(const FString& InPath)
{
	const FString NormalizedPath = PathNormalize(InPath);

	return NormalizedPath.Replace(*ProjectCleanerConstants::PathRelRoot.ToString(), *(FPaths::ProjectDir() / TEXT("Content")), ESearchCase::CaseSensitive);
}

FString UProjectCleanerSubsystem::PathConvertToRel(const FString& InPath)
{
	const FString NormalizedPath = PathNormalize(InPath);

	return NormalizedPath.Replace(*(FPaths::ProjectDir() / TEXT("Content")), *ProjectCleanerConstants::PathRelRoot.ToString(), ESearchCase::CaseSensitive);
}

FString UProjectCleanerSubsystem::GetAssetClassName(const FAssetData& AssetData) const
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

UClass* UProjectCleanerSubsystem::GetAssetClass(const FAssetData& AssetData) const
{
	if (!AssetData.IsValid()) return nullptr;

	if (AssetData.AssetClass.IsEqual("Blueprint"))
	{
		const UBlueprint* BlueprintAsset = Cast<UBlueprint>(AssetData.GetAsset());
		if (!BlueprintAsset) return nullptr;

		return BlueprintAsset->GeneratedClass;
	}

	return AssetData.GetClass();
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
	if (!ModuleAssetTools) return;
	if (!PlatformFile) return;

	ResetData();
	FixupRedirectors();
	FEditorFileUtils::SaveDirtyPackages(false, true, true, false, false, false);

	bScanningProject = true;

	// todo:ashe23 add slow task

	FindFoldersForbidden();
	FindFoldersTotal();
	FindFilesCorrupted();
	FindFilesNonEngine();
	FindAssetsForbidden();
	FindAssetsAll();
	FindAssetsPrimary();
	FindAssetsIndirect();
	FindAssetsExcluded();
	FindAssetsUsed();
	FindAssetsUnused();
	FindFoldersEmpty();

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
	ModuleAssetRegistry->Get().GetAssetsByPath(ProjectCleanerConstants::PathRelRoot, AssetsAll, true);
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
	const UProjectCleanerExcludeSettings* ExcludeSettings = GetDefault<UProjectCleanerExcludeSettings>();
	ensure(ExcludeSettings);

	TArray<FAssetData> AssetsExcludedByPath;
	TArray<FAssetData> AssetsExcludedByClass;
	AssetsExcludedByPath.Reserve(AssetsAll.Num());
	AssetsExcludedByClass.Reserve(AssetsAll.Num());

	FARFilter FilterByPath;
	FilterByPath.bRecursivePaths = ExcludeSettings->ExcludedFolders.Num() > 0;
	for (const auto& ExcludedFolder : ExcludeSettings->ExcludedFolders)
	{
		if (ExcludedFolder.Path.IsEmpty()) continue;

		FilterByPath.PackagePaths.Add(FName{*ExcludedFolder.Path});
	}
	ModuleAssetRegistry->Get().GetAssets(FilterByPath, AssetsExcludedByPath);

	FARFilter FilterByClass;
	FilterByClass.bRecursivePaths = ExcludeSettings->ExcludedClasses.Num() > 0;
	FilterByClass.bRecursiveClasses = ExcludeSettings->ExcludedClasses.Num() > 0;

	if (ExcludeSettings->ExcludedClasses.Num() > 0)
	{
		FilterByClass.PackagePaths.Add(ProjectCleanerConstants::PathRelRoot);
	}
	for (const auto& ExcludedClass : ExcludeSettings->ExcludedClasses)
	{
		if (!ExcludedClass.LoadSynchronous()) continue;

		FilterByClass.ClassNames.Add(ExcludedClass->GetFName());
	}
	ModuleAssetRegistry->Get().GetAssets(FilterByClass, AssetsExcludedByClass); // todo:ashe23 not working correctly with blueprint assets
	
	AssetsExcludedByPath.Shrink();
	AssetsExcludedByClass.Shrink();

	AssetsExcluded.Reserve(AssetsExcludedByPath.Num() + AssetsExcludedByClass.Num() + ExcludeSettings->ExcludedAssets.Num());

	for (const auto& AssetExcludedByPath : AssetsExcludedByPath)
	{
		AssetsExcluded.AddUnique(AssetExcludedByPath);
	}

	for (const auto& AssetExcludedByClass : AssetsExcludedByClass)
	{
		AssetsExcluded.AddUnique(AssetExcludedByClass);
	}

	for (const auto& ExcludedAsset : ExcludeSettings->ExcludedAssets)
	{
		if (!ExcludedAsset.LoadSynchronous()) continue;

		const FName ObjectPath = ExcludedAsset.ToSoftObjectPath().GetAssetPathName();
		const FAssetData AssetData = ModuleAssetRegistry->Get().GetAssetByObjectPath(ObjectPath);
		if (!AssetData.IsValid()) continue;

		AssetsExcluded.AddUnique(AssetData);
	}
}

void UProjectCleanerSubsystem::FindAssetsUsed()
{
	AssetsUsed.Reserve(AssetsAll.Num());

	for (const auto& Asset : AssetsPrimary)
	{
		AssetsUsed.AddUnique(Asset);
	}

	for (const auto& Asset : AssetsIndirect)
	{
		AssetsUsed.AddUnique(Asset);
	}

	for (const auto& Asset : AssetsWithExternalRefs)
	{
		AssetsUsed.AddUnique(Asset);
	}

	for (const auto& Asset : AssetsForbidden)
	{
		AssetsUsed.AddUnique(Asset);
	}

	for (const auto& Asset : AssetsExcluded)
	{
		AssetsUsed.AddUnique(Asset);
	}

	if (FModuleManager::Get().IsModuleLoaded(TEXT("MegascansPlugin")))
	{
		TArray<FAssetData> AssetsMsPresets;
		ModuleAssetRegistry->Get().GetAssetsByPath(ProjectCleanerConstants::PathRelMSPresets, AssetsMsPresets, true);

		for (const auto& Asset : AssetsMsPresets)
		{
			AssetsUsed.AddUnique(Asset);
		}
	}

	TArray<FAssetData> LinkedAssets;
	GetLinkedAssets(AssetsUsed, LinkedAssets);

	for (const auto& Asset : LinkedAssets)
	{
		AssetsUsed.AddUnique(Asset);
	}

	AssetsUsed.Shrink();
}

void UProjectCleanerSubsystem::FindAssetsUnused()
{
	AssetsUnused.Reserve(AssetsAll.Num());

	for (const auto& Asset : AssetsAll)
	{
		if (AssetsUsed.Contains(Asset)) continue;

		AssetsUnused.AddUnique(Asset);
	}

	AssetsUnused.Shrink();
}

void UProjectCleanerSubsystem::FindAssetsForbidden()
{
	FARFilter Filter;
	Filter.bRecursivePaths = true;
	Filter.bRecursiveClasses = true;
	Filter.PackagePaths.Add(ProjectCleanerConstants::PathRelRoot);
	Filter.ClassNames.Add(UEditorUtilityWidget::StaticClass()->GetFName());
	Filter.ClassNames.Add(UEditorUtilityBlueprint::StaticClass()->GetFName());
	Filter.ClassNames.Add(UEditorUtilityWidgetBlueprint::StaticClass()->GetFName());
	Filter.ClassNames.Add(UMapBuildDataRegistry::StaticClass()->GetFName());

	ModuleAssetRegistry->Get().GetAssets(Filter, AssetsForbidden);
}

void UProjectCleanerSubsystem::FindAssetsPrimary()
{
	TArray<FName> PrimaryAssetClasses;

	// getting list of primary asset classes that are defined in AssetManager
	const auto& AssetManager = UAssetManager::Get();
	if (!AssetManager.IsValid()) return;

	TArray<FPrimaryAssetTypeInfo> AssetTypeInfos;
	AssetManager.Get().GetPrimaryAssetTypeInfoList(AssetTypeInfos);
	PrimaryAssetClasses.Reserve(AssetTypeInfos.Num());

	for (const auto& AssetTypeInfo : AssetTypeInfos)
	{
		if (!AssetTypeInfo.AssetBaseClassLoaded) continue;

		PrimaryAssetClasses.AddUnique(AssetTypeInfo.AssetBaseClassLoaded->GetFName());
	}

	// getting list of primary assets classes that are derived from main primary assets
	TSet<FName> DerivedFromPrimaryAssets;
	{
		const TSet<FName> ExcludedClassNames;
		ModuleAssetRegistry->Get().GetDerivedClassNames(PrimaryAssetClasses, ExcludedClassNames, DerivedFromPrimaryAssets);
	}

	for (const auto& DerivedClassName : DerivedFromPrimaryAssets)
	{
		PrimaryAssetClasses.AddUnique(DerivedClassName);
	}

	FARFilter Filter;
	Filter.bRecursiveClasses = true;
	Filter.bRecursivePaths = true;
	Filter.PackagePaths.Add(ProjectCleanerConstants::PathRelRoot);

	for (const auto& ClassName : PrimaryAssetClasses)
	{
		Filter.ClassNames.Add(ClassName);
	}
	ModuleAssetRegistry->Get().GetAssets(Filter, AssetsPrimary);

	FARFilter FilterBlueprint;
	FilterBlueprint.bRecursivePaths = true;
	FilterBlueprint.bRecursiveClasses = true;
	FilterBlueprint.PackagePaths.Add(ProjectCleanerConstants::PathRelRoot);
	FilterBlueprint.ClassNames.Add(UBlueprint::StaticClass()->GetFName());

	TArray<FAssetData> BlueprintAssets;
	ModuleAssetRegistry->Get().GetAssets(FilterBlueprint, BlueprintAssets);

	for (const auto& BlueprintAsset : BlueprintAssets)
	{
		const FName BlueprintClass = FName{*GetAssetClassName(BlueprintAsset)};
		if (PrimaryAssetClasses.Contains(BlueprintClass))
		{
			AssetsPrimary.AddUnique(BlueprintAsset);
		}
	}
}

void UProjectCleanerSubsystem::FindAssetsWithExternalRefs()
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

void UProjectCleanerSubsystem::FindFoldersForbidden()
{
	FoldersForbidden.Add(FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() / TEXT("Collections")));
	FoldersForbidden.Add(FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() / TEXT("Developers")));
	FoldersForbidden.Add(FPaths::ConvertRelativePathToFull(FPaths::GameDevelopersDir() / FPaths::GameUserDeveloperFolderName()));
	FoldersForbidden.Add(FPaths::ConvertRelativePathToFull(FPaths::GameUserDeveloperDir() / TEXT("Collections")));
	// todo:ashe23 for ue5 add __ExternalObject__ and __ExternalActors__ folders

	if (FModuleManager::Get().IsModuleLoaded(TEXT("MegascansPlugin")))
	{
		FoldersForbidden.Add(FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() / TEXT("MSPresets")));
	}
}

void UProjectCleanerSubsystem::FindFoldersTotal()
{
	TArray<FString> Folders;

	IFileManager::Get().FindFilesRecursive(Folders, *FPaths::ProjectContentDir(), TEXT("*.*"), false, true);

	FoldersTotal.Append(Folders);
}

void UProjectCleanerSubsystem::FindFoldersEmpty()
{
	TArray<FString> Files;
	for (const auto& Folder : FoldersTotal)
	{
		if (FoldersForbidden.Contains(Folder)) continue;
		if (FolderIsExcluded(Folder)) continue;

		IFileManager::Get().FindFilesRecursive(Files, *Folder, TEXT("*.*"), true, false);

		if (Files.Num() > 0) continue;

		FoldersEmpty.Add(Folder);
		Files.Reset();
	}
}

void UProjectCleanerSubsystem::FindFilesCorrupted()
{
	TArray<FString> Files;
	IFileManager::Get().FindFilesRecursive(Files, *FPaths::ProjectContentDir(), TEXT("*.*"), true, false);

	for (const auto& File : Files)
	{
		if (!FileHasEngineExtension(FPaths::GetExtension(File))) continue;
		if (!FileIsCorrupted(File)) continue;

		FilesCorrupted.Add(File);
	}
}

void UProjectCleanerSubsystem::FindFilesNonEngine()
{
	TArray<FString> Files;
	IFileManager::Get().FindFilesRecursive(Files, *FPaths::ProjectContentDir(), TEXT("*.*"), true, false);

	for (const auto& File : Files)
	{
		if (FileHasEngineExtension(FPaths::GetExtension(File))) continue;

		FilesNonEngine.Add(File);
	}
}

void UProjectCleanerSubsystem::ResetData()
{
	AssetsAll.Reset();
	AssetsIndirect.Reset();
	AssetsExcluded.Reset();
	AssetsUsed.Reset();
	AssetsUnused.Reset();
	AssetsForbidden.Reset();
	AssetsPrimary.Reset();
	AssetsWithExternalRefs.Reset();
	AssetsIndirectInfos.Reset();
	FoldersTotal.Reset();
	FoldersEmpty.Reset();
	FoldersForbidden.Reset();
	FilesCorrupted.Reset();
	FilesNonEngine.Reset();
}

void UProjectCleanerSubsystem::FixupRedirectors()
{
	FScopedSlowTask FixRedirectorsTask{
		1.0f,
		FText::FromString(TEXT("Fixing redirectors.."))
	};
	FixRedirectorsTask.MakeDialog();

	FARFilter Filter;
	Filter.bRecursivePaths = true;
	Filter.PackagePaths.Add(ProjectCleanerConstants::PathRelRoot);
	Filter.ClassNames.Add(UObjectRedirector::StaticClass()->GetFName());

	// Getting all redirectors in given path
	TArray<FAssetData> AssetList;
	ModuleAssetRegistry->Get().GetAssets(Filter, AssetList);

	if (AssetList.Num() == 0) return;

	FScopedSlowTask FixRedirectorsLoadingTask(
		AssetList.Num(),
		FText::FromString(TEXT("Loading redirectors..."))
	);
	FixRedirectorsLoadingTask.MakeDialog();

	TArray<UObjectRedirector*> Redirectors;
	Redirectors.Reserve(AssetList.Num());

	for (const auto& Asset : AssetList)
	{
		FixRedirectorsLoadingTask.EnterProgressFrame();

		UObject* AssetObj = Asset.GetAsset();
		if (!AssetObj) continue;

		UObjectRedirector* Redirector = CastChecked<UObjectRedirector>(AssetObj);
		if (!Redirector) continue;

		Redirectors.Add(Redirector);
	}

	Redirectors.Shrink();

	// Fix up all founded redirectors
	ModuleAssetTools->Get().FixupReferencers(Redirectors);

	FixRedirectorsTask.EnterProgressFrame(1.0f);
}
