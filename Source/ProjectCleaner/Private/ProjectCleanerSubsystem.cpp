// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "ProjectCleanerSubsystem.h"
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
}

#if WITH_EDITOR
void UProjectCleanerSubsystem::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	SaveConfig();
}
#endif

FString UProjectCleanerSubsystem::ScanResultToString(const EProjectCleanerScanResult ScanResult)
{
	switch (ScanResult)
	{
		case EProjectCleanerScanResult::None: return TEXT("");
		case EProjectCleanerScanResult::Success: return TEXT("");
		case EProjectCleanerScanResult::AssetRegistryWorking: return TEXT("Cant scan project because AssetRegistry still working");
		case EProjectCleanerScanResult::EditorInPlayMode: return TEXT("Cant scan project because Editor is in Play mode");
		case EProjectCleanerScanResult::ScanningInProgress: return TEXT("Scanning in progress");
		case EProjectCleanerScanResult::CleaningInProgress: return TEXT("Cleaning in progress");
		// case EProjectCleanerScanResult::InvalidScanSettings: return TEXT("Invalid Scan Settings");
		case EProjectCleanerScanResult::FailedToSaveAssets: return TEXT("Cant scan project because failed to save some assets");
		default: return TEXT("");
	}
}

void UProjectCleanerSubsystem::GetAssetsPrimary(TArray<FAssetData>& AssetsPrimary) const
{
	AssetsPrimary.Reset();

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

	// getting primary blueprint assets
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

void UProjectCleanerSubsystem::GetAssetsIndirect(TArray<FAssetData>& AssetsIndirect) const
{
	TArray<FProjectCleanerIndirectAssetInfo> Infos;
	GetAssetsIndirectInfo(Infos);

	AssetsIndirect.Reset();
	AssetsIndirect.Reserve(Infos.Num());

	for (const auto& Info : Infos)
	{
		AssetsIndirect.AddUnique(Info.AssetData);
	}
}

void UProjectCleanerSubsystem::GetAssetsUsed(TArray<FAssetData>& AssetsUsed) const
{
	AssetsUsed.Reset();

	FScopedSlowTask SlowTask{
		7.0f,
		FText::FromString(TEXT("Searching for used assets..")),
		GIsEditor && !IsRunningCommandlet()
	};
	SlowTask.MakeDialog();

	SlowTask.EnterProgressFrame(1.0f, FText::FromString(TEXT("Searching for primary assets...")));

	TArray<FAssetData> AssetsPrimary;
	GetAssetsPrimary(AssetsPrimary);

	SlowTask.EnterProgressFrame(1.0f, FText::FromString(TEXT("Searching for indirect assets...")));

	TArray<FAssetData> AssetsIndirect;
	GetAssetsIndirect(AssetsIndirect);

	SlowTask.EnterProgressFrame(1.0f, FText::FromString(TEXT("Searching assets with external referencers...")));

	TArray<FAssetData> AssetsWithExternalRefs;
	GetAssetsWithExternalRefs(AssetsWithExternalRefs);

	SlowTask.EnterProgressFrame(1.0f, FText::FromString(TEXT("Searching editor or plugin specific assets...")));

	TArray<FAssetData> AssetsEditor;
	TArray<FAssetData> AssetsMegascans;

	FARFilter FilterEditorAssets;
	FilterEditorAssets.bRecursivePaths = true;
	FilterEditorAssets.bRecursiveClasses = true;
	FilterEditorAssets.PackagePaths.Add(ProjectCleanerConstants::PathRelRoot);
	FilterEditorAssets.ClassNames.Add(UEditorUtilityWidget::StaticClass()->GetFName());
	FilterEditorAssets.ClassNames.Add(UEditorUtilityBlueprint::StaticClass()->GetFName());
	FilterEditorAssets.ClassNames.Add(UEditorUtilityWidgetBlueprint::StaticClass()->GetFName());
	FilterEditorAssets.ClassNames.Add(UMapBuildDataRegistry::StaticClass()->GetFName());
	ModuleAssetRegistry->Get().GetAssets(FilterEditorAssets, AssetsEditor);

	if (FModuleManager::Get().IsModuleLoaded(TEXT("MegascansPlugin")))
	{
		ModuleAssetRegistry->Get().GetAssetsByPath(ProjectCleanerConstants::PathRelMSPresets, AssetsMegascans, true);
	}

	SlowTask.EnterProgressFrame(1.0f, FText::FromString(TEXT("Searching for excluded assets...")));

	TArray<FAssetData> AssetsExcluded;
	GetAssetsExcluded(AssetsExcluded);

	SlowTask.EnterProgressFrame(1.0f, FText::FromString(TEXT("Filling used assets container...")));

	AssetsUsed.Reserve(AssetsPrimary.Num() + AssetsIndirect.Num() + AssetsWithExternalRefs.Num() + AssetsEditor.Num() + AssetsMegascans.Num() + AssetsExcluded.Num());

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

	for (const auto& Asset : AssetsEditor)
	{
		AssetsUsed.AddUnique(Asset);
	}

	for (const auto& Asset : AssetsMegascans)
	{
		AssetsUsed.AddUnique(Asset);
	}

	for (const auto& Asset : AssetsExcluded)
	{
		AssetsUsed.AddUnique(Asset);
	}

	SlowTask.EnterProgressFrame(1.0f, FText::FromString(TEXT("Searching for used assets dependencies...")));

	TArray<FAssetData> AssetsDependencies;
	GetAssetsDependencies(AssetsUsed, AssetsDependencies);

	AssetsUsed.Reserve(AssetsUsed.Num() + AssetsDependencies.Num());

	for (const auto& Asset : AssetsDependencies)
	{
		AssetsUsed.AddUnique(Asset);
	}

	AssetsUsed.Shrink();
}

void UProjectCleanerSubsystem::GetAssetsUnused(TArray<FAssetData>& AssetsUnused) const
{
	TArray<FAssetData> AssetsAll;
	TArray<FAssetData> AssetsUsed;

	FScopedSlowTask SlowTask{
		2.0f,
		FText::FromString(TEXT("Searching for unused assets..")),
		GIsEditor && !IsRunningCommandlet()
	};
	SlowTask.MakeDialog();

	SlowTask.EnterProgressFrame(1.0f, FText::FromString(TEXT("Searching for used assets...")));

	ModuleAssetRegistry->Get().GetAssetsByPath(ProjectCleanerConstants::PathRelRoot, AssetsAll, true);
	GetAssetsUsed(AssetsUsed);

	// todo:ashe23 filter assets inside Collections or external actors folders?

	SlowTask.EnterProgressFrame(1.0f, FText::FromString(TEXT("Filtering unused assets...")));

	AssetsUnused.Reset();
	AssetsUnused.Reserve(AssetsAll.Num());

	for (const auto& Asset : AssetsAll)
	{
		if (!AssetsUsed.Contains(Asset))
		{
			AssetsUnused.AddUnique(Asset);
		}
	}

	AssetsUnused.Shrink();
}

void UProjectCleanerSubsystem::GetAssetsIndirectInfo(TArray<FProjectCleanerIndirectAssetInfo>& Infos) const
{
	Infos.Reset();

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

	// 4) getting all assets
	TArray<FAssetData> AssetsAll;
	ModuleAssetRegistry->Get().GetAssetsByPath(ProjectCleanerConstants::PathRelRoot, AssetsAll, true);

	Files.Append(ProjectPluginsFiles);
	Files.Shrink();

	for (const auto& File : Files)
	{
		if (!PlatformFile->FileExists(*File)) continue;

		FString FileContent;
		FFileHelper::LoadFileToString(FileContent, *File);

		if (FileContent.IsEmpty()) continue;

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

				FProjectCleanerIndirectAssetInfo IndirectAsset;
				IndirectAsset.AssetData = *AssetData;
				IndirectAsset.FilePath = FPaths::ConvertRelativePathToFull(File);
				IndirectAsset.LineNum = i + 1;
				Infos.AddUnique(IndirectAsset);
			}
		}
	}
}

void UProjectCleanerSubsystem::GetAssetsDependencies(const TArray<FAssetData>& Assets, TArray<FAssetData>& Dependencies) const
{
	Dependencies.Reset();

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

	ModuleAssetRegistry->Get().GetAssets(Filter, Dependencies);
}

void UProjectCleanerSubsystem::GetAssetsWithExternalRefs(TArray<FAssetData>& Assets) const
{
	Assets.Reset();

	TArray<FAssetData> AssetsAll;
	ModuleAssetRegistry->Get().GetAssetsByPath(ProjectCleanerConstants::PathRelRoot, AssetsAll, true);

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
			Assets.AddUnique(Asset);
		}

		Refs.Reset();
	}
}

void UProjectCleanerSubsystem::GetFilesCorrupted(TArray<FString>& FilesCorrupted) const
{
	TArray<FString> Files;
	IFileManager::Get().FindFilesRecursive(Files, *FPaths::ProjectContentDir(), TEXT("*.*"), true, false);

	FilesCorrupted.Reset();
	FilesCorrupted.Reserve(Files.Num());

	for (const auto& File : Files)
	{
		const FString FilePathAbs = FPaths::ConvertRelativePathToFull(File);
		if (!FileHasEngineExtension(FilePathAbs)) continue;
		if (!FileIsCorrupted(FilePathAbs)) continue;

		FilesCorrupted.Add(FilePathAbs);
	}

	FilesCorrupted.Shrink();
}

void UProjectCleanerSubsystem::GetFilesNonEngine(TArray<FString>& FilesNonEngine) const
{
	TArray<FString> Files;
	IFileManager::Get().FindFilesRecursive(Files, *FPaths::ProjectContentDir(), TEXT("*.*"), true, false);

	FilesNonEngine.Reset();
	FilesNonEngine.Reserve(Files.Num());

	for (const auto& File : Files)
	{
		const FString FilePathAbs = FPaths::ConvertRelativePathToFull(File);
		if (FileHasEngineExtension(FilePathAbs)) continue;

		FilesNonEngine.Add(FilePathAbs);
	}

	FilesNonEngine.Shrink();
}

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

FString UProjectCleanerSubsystem::PathNormalize(const FString& InPath) const
{
	FString Path = FPaths::ConvertRelativePathToFull(InPath);
	FPaths::RemoveDuplicateSlashes(Path);
	FPaths::NormalizeDirectoryName(Path);

	return Path;
}

FString UProjectCleanerSubsystem::PathConvertToAbs(const FString& InPath) const
{
	const FString NormalizedPath = PathNormalize(InPath);

	return NormalizedPath.Replace(*ProjectCleanerConstants::PathRelRoot.ToString(), *FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("Content")), ESearchCase::CaseSensitive);
}

FString UProjectCleanerSubsystem::PathConvertToRel(const FString& InPath) const
{
	const FString NormalizedPath = PathNormalize(InPath);

	return NormalizedPath.Replace(*FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("Content")), *ProjectCleanerConstants::PathRelRoot.ToString(), ESearchCase::CaseSensitive);
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

bool UProjectCleanerSubsystem::FileHasEngineExtension(const FString& InFilePath) const
{
	const FString Extension = FPaths::GetExtension(InFilePath).ToLower();

	TSet<FString> EngineExtensions;
	EngineExtensions.Reserve(3);
	EngineExtensions.Add(TEXT("uasset"));
	EngineExtensions.Add(TEXT("umap"));
	EngineExtensions.Add(TEXT("collection"));

	return EngineExtensions.Contains(Extension);
}

bool UProjectCleanerSubsystem::FileIsCorrupted(const FString& InFilePathAbs) const
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

bool UProjectCleanerSubsystem::FolderIsEmpty(const FString& InFolderPath) const
{
	if (InFolderPath.IsEmpty()) return false;
	if (!FPaths::DirectoryExists(InFolderPath)) return false;

	TArray<FString> Files;
	IFileManager::Get().FindFilesRecursive(Files, *InFolderPath, TEXT("*.*"), true, false);

	return Files.Num() == 0;
}

FProjectCleanerScanData UProjectCleanerSubsystem::ProjectScan(const FProjectCleanerScanSettings& ScanSettings)
{
	FProjectCleanerScanData ScanData;

	if (AssetRegistryWorking())
	{
		ScanData.ScanResult = EProjectCleanerScanResult::AssetRegistryWorking;
		return ScanData;
	}

	if (EditorInPlayMode())
	{
		ScanData.ScanResult = EProjectCleanerScanResult::EditorInPlayMode;
		return ScanData;
	}

	if (bScanningProject)
	{
		ScanData.ScanResult = EProjectCleanerScanResult::ScanningInProgress;
		return ScanData;
	}

	if (bCleaningProject)
	{
		ScanData.ScanResult = EProjectCleanerScanResult::CleaningInProgress;
		return ScanData;
	}

	// if (!ScanSettings.IsValid())
	// {
	// 	ScanData.ScanResult = EProjectCleanerScanResult::InvalidScanSettings;
	// 	return ScanData;
	// }

	GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->CloseAllAssetEditors();
	FixupRedirectors();

	if (!FEditorFileUtils::SaveDirtyPackages(false, true, true, false, false, false))
	{
		ScanData.ScanResult = EProjectCleanerScanResult::FailedToSaveAssets;
		return ScanData;
	}

	bScanningProject = true;

	// FScopedSlowTask SlowTaskScan{
	// 	2.0f,
	// 	FText::FromString(TEXT("Scanning project..")),
	// 	GIsEditor && !IsRunningCommandlet()
	// };
	// SlowTaskScan.MakeDialog();


	// todo:ashe23 new version of scanner, think about some caching methods
	// - make sure asset registry is not working
	// - make sure editor is not in play mode
	// - make sure we are not scanning project currently
	// - make sure we are not cleaning project currently
	// - make sure all opened asset editors are closed
	// - OnScanFailed delegate must be called here, with text message what really failed

	// 0. pre scan actions
	//		- make sure all redirectors are fixed
	//		- make sure all assets are saved
	//		- cache all required assets??
	//		- query general file and folders number and reserve some space

	// 1. gathering all assets from asset registry that are inside Content folder

	// 2. traversing every asset and checking if its used.
	//		Below used asset cases:
	//		- primary asset or its dependency
	//		- indirect asset or its dependency
	//		- has external refs outside "/Game" folder
	//		- forbidden asset or its dependency
	//		- excluded asset or its dependency
	//		- megascans asset (if plugin enabled), but those assets are in category of external ref, maybe merge them??

	// 3. All - Used = Unused assets in project

	// 4. post scan actions
	//		- shrink data containers
	//		- broadcast OnScanFinished delegate with results

	bScanningProject = false;

	return ScanData;
}

void UProjectCleanerSubsystem::GetAssetsByPath(const FString& InFolderPathRel, const bool bRecursive, TArray<FAssetData>& Assets) const
{
	if (InFolderPathRel.IsEmpty()) return;
	if (!InFolderPathRel.StartsWith(ProjectCleanerConstants::PathRelRoot.ToString())) return;

	Assets.Reset();

	ModuleAssetRegistry->Get().GetAssetsByPath(FName{*InFolderPathRel}, Assets, bRecursive);
}

int32 UProjectCleanerSubsystem::GetAssetsByPathNum(const FString& InFolderPathRel, const bool bRecursive) const
{
	TArray<FAssetData> Assets;
	GetAssetsByPath(InFolderPathRel, bRecursive, Assets);

	return Assets.Num();
}

int64 UProjectCleanerSubsystem::GetAssetsByPathSize(const FString& InFolderPathRel, const bool bRecursive) const
{
	TArray<FAssetData> Assets;
	GetAssetsByPath(InFolderPathRel, bRecursive, Assets);

	return GetAssetsTotalSize(Assets);
}

void UProjectCleanerSubsystem::GetAssetsExcluded(TArray<FAssetData>& AssetsExcluded) const
{
	const UProjectCleanerExcludeSettings* ExcludeSettings = GetDefault<UProjectCleanerExcludeSettings>();
	if (!ExcludeSettings) return;

	TArray<FAssetData> AssetsAll;
	ModuleAssetRegistry->Get().GetAssetsByPath(ProjectCleanerConstants::PathRelRoot, AssetsAll, true);

	AssetsExcluded.Reset();
	AssetsExcluded.Reserve(AssetsAll.Num());

	for (const auto& Asset : AssetsAll)
	{
		if (AssetIsExcluded(Asset))
		{
			AssetsExcluded.AddUnique(Asset);
		}
	}

	AssetsExcluded.Shrink();
}

bool UProjectCleanerSubsystem::AssetIsExcluded(const FAssetData& AssetData) const
{
	return AssetExcludedByPath(AssetData) || AssetExcludedByClass(AssetData) || AssetExcludedByObject(AssetData);
}

bool UProjectCleanerSubsystem::AssetRegistryWorking() const
{
	if (!ModuleAssetRegistry) return false;

	return ModuleAssetRegistry->Get().IsLoadingAssets();
}

bool UProjectCleanerSubsystem::EditorInPlayMode()
{
	if (!GEditor) return false;

	return GEditor->PlayWorld || GIsPlayInEditorWorld;
}

bool UProjectCleanerSubsystem::AssetExcludedByPath(const FAssetData& AssetData) const
{
	const UProjectCleanerExcludeSettings* ExcludeSettings = GetDefault<UProjectCleanerExcludeSettings>();
	if (!ExcludeSettings) return false;

	for (const auto& ExcludedFolder : ExcludeSettings->ExcludedFolders)
	{
		const FString FolderPathRel = PathConvertToAbs(ExcludedFolder.Path);
		const FString AssetPathRel = PathConvertToAbs(AssetData.PackagePath.ToString());

		if (FolderPathRel.IsEmpty() || AssetPathRel.IsEmpty()) continue;
		if (FPaths::IsUnderDirectory(AssetPathRel, FolderPathRel))
		{
			return true;
		}
	}

	return false;
}

bool UProjectCleanerSubsystem::AssetExcludedByClass(const FAssetData& AssetData) const
{
	const UProjectCleanerExcludeSettings* ExcludeSettings = GetDefault<UProjectCleanerExcludeSettings>();
	if (!ExcludeSettings) return false;

	const FString AssetClassName = GetAssetClassName(AssetData);

	for (const auto& ExcludedClass : ExcludeSettings->ExcludedClasses)
	{
		if (!ExcludedClass.LoadSynchronous()) continue;

		const FString ExcludedClassName = ExcludedClass->GetName();
		if (AssetClassName.Equals(ExcludedClassName))
		{
			return true;
		}
	}

	return false;
}

bool UProjectCleanerSubsystem::AssetExcludedByObject(const FAssetData& AssetData) const
{
	const UProjectCleanerExcludeSettings* ExcludeSettings = GetDefault<UProjectCleanerExcludeSettings>();
	if (!ExcludeSettings) return false;

	for (const auto& ExcludedAsset : ExcludeSettings->ExcludedAssets)
	{
		if (!ExcludedAsset.LoadSynchronous()) continue;

		if (ExcludedAsset.ToSoftObjectPath() == AssetData.ToSoftObjectPath())
		{
			return true;
		}
	}

	return false;
}

void UProjectCleanerSubsystem::FixupRedirectors() const
{
	FScopedSlowTask FixRedirectorsTask{
		1.0f,
		FText::FromString(TEXT("Fixing redirectors..")),
		GIsEditor && !IsRunningCommandlet()
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
