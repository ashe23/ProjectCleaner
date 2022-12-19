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

void UProjectCleanerSubsystem::GetAssetsReferencers(const TArray<FAssetData>& Assets, TArray<FAssetData>& Referencers) const
{
	Referencers.Reset();

	TSet<FName> UsedAssetsRefs;
	TArray<FName> Stack;
	for (const auto& Asset : Assets)
	{
		UsedAssetsRefs.Add(Asset.PackageName);
		Stack.Add(Asset.PackageName);

		TArray<FName> Refs;
		while (Stack.Num() > 0)
		{
			const auto CurrentPackageName = Stack.Pop(false);
			Refs.Reset();

			ModuleAssetRegistry->Get().GetReferencers(CurrentPackageName, Refs);

			Refs.RemoveAllSwap([&](const FName& Ref)
			{
				return !Ref.ToString().StartsWith(*ProjectCleanerConstants::PathRelRoot.ToString());
			}, false);

			for (const auto& Ref : Refs)
			{
				bool bIsAlreadyInSet = false;
				UsedAssetsRefs.Add(Ref, &bIsAlreadyInSet);
				if (!bIsAlreadyInSet)
				{
					Stack.Add(Ref);
				}
			}
		}
	}

	FARFilter Filter;
	Filter.bRecursivePaths = true;
	Filter.PackagePaths.Add(ProjectCleanerConstants::PathRelRoot);

	for (const auto& Ref : UsedAssetsRefs)
	{
		Filter.PackageNames.Add(Ref);
	}

	ModuleAssetRegistry->Get().GetAssets(Filter, Referencers);
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

int64 UProjectCleanerSubsystem::GetFilesTotalSize(const TArray<FString>& Files)
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

bool UProjectCleanerSubsystem::FolderIsEmpty(const FString& InFolderPath)
{
	if (InFolderPath.IsEmpty()) return false;
	if (!FPaths::DirectoryExists(InFolderPath)) return false;

	TArray<FString> Files;
	IFileManager::Get().FindFilesRecursive(Files, *InFolderPath, TEXT("*.*"), true, false);

	return Files.Num() == 0;
}

void UProjectCleanerSubsystem::ProjectScan()
{
	const UProjectCleanerExcludeSettings* ExcludeSettings = GetDefault<UProjectCleanerExcludeSettings>();
	if (!ExcludeSettings) return;

	FProjectCleanerScanSettings NewScanSettings;

	for (const auto& ExcludedFolder : ExcludeSettings->ExcludedFolders)
	{
		if (ExcludedFolder.Path.IsEmpty()) continue;

		const FString PathAbs = PathConvertToAbs(ExcludedFolder.Path);
		if (!FPaths::DirectoryExists(PathAbs)) continue;

		NewScanSettings.ExcludedFolders.AddUnique(ExcludedFolder.Path);
	}

	for (const auto& ExcludedClasses : ExcludeSettings->ExcludedClasses)
	{
		if (!ExcludedClasses.LoadSynchronous()) continue;

		NewScanSettings.ExcludedClasses.AddUnique(ExcludedClasses.Get());
	}

	for (const auto& ExcludedAsset : ExcludeSettings->ExcludedAssets)
	{
		if (!ExcludedAsset.LoadSynchronous()) continue;

		NewScanSettings.ExcludedAssets.AddUnique(ExcludedAsset.Get());
	}

	ProjectScan(NewScanSettings);
}

void UProjectCleanerSubsystem::ProjectScan(const FProjectCleanerScanSettings& InScanSettings)
{
	ScanSettings = InScanSettings;

	ScanDataReset();

	if (AssetRegistryWorking())
	{
		ScanData.ScanResult = EProjectCleanerScanResult::AssetRegistryWorking;
		ScanData.ScanResultMsg = ScanResultToString(ScanData.ScanResult);
		return;
	}

	if (EditorInPlayMode())
	{
		ScanData.ScanResult = EProjectCleanerScanResult::EditorInPlayMode;
		ScanData.ScanResultMsg = ScanResultToString(ScanData.ScanResult);
		return;
	}

	if (bScanningProject)
	{
		ScanData.ScanResult = EProjectCleanerScanResult::ScanningInProgress;
		ScanData.ScanResultMsg = ScanResultToString(ScanData.ScanResult);
		return;
	}

	if (bCleaningProject)
	{
		ScanData.ScanResult = EProjectCleanerScanResult::CleaningInProgress;
		ScanData.ScanResultMsg = ScanResultToString(ScanData.ScanResult);
		return;
	}

	GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->CloseAllAssetEditors();
	FixupRedirectors();

	if (!FEditorFileUtils::SaveDirtyPackages(false, true, true, false, false, false))
	{
		ScanData.ScanResult = EProjectCleanerScanResult::FailedToSaveAssets;
		ScanData.ScanResultMsg = ScanResultToString(ScanData.ScanResult);
		return;
	}

	bScanningProject = true;

	FScopedSlowTask SlowTaskScan{
		9.0f,
		FText::FromString(TEXT("Scanning project..")),
		GIsEditor && !IsRunningCommandlet()
	};
	SlowTaskScan.MakeDialog();

	SlowTaskScan.EnterProgressFrame(1.0f);

	FindAssetsTotal();

	SlowTaskScan.EnterProgressFrame(1.0f);

	FindAssetsPrimary();

	SlowTaskScan.EnterProgressFrame(1.0f);

	FindAssetsIndirect();

	SlowTaskScan.EnterProgressFrame(1.0f);

	FindAssetsExcluded();

	SlowTaskScan.EnterProgressFrame(1.0f);

	FindAssetsUsed();

	SlowTaskScan.EnterProgressFrame(1.0f);

	FindAssetsUnused();

	SlowTaskScan.EnterProgressFrame(1.0f);

	FindFilesCorrupted();

	SlowTaskScan.EnterProgressFrame(1.0f);

	FindFilesNonEngine();

	SlowTaskScan.EnterProgressFrame(1.0f);

	FindFolders();

	bScanningProject = false;
	ScanData.ScanResult = EProjectCleanerScanResult::Success;

	if (DelegateProjectScanned.IsBound())
	{
		DelegateProjectScanned.Broadcast();
	}
}

const FProjectCleanerScanData& UProjectCleanerSubsystem::GetScanData() const
{
	return ScanData;
}

bool UProjectCleanerSubsystem::CanScanProject() const
{
	return !AssetRegistryWorking() && !EditorInPlayMode() && !ScanningInProgress() && !CleaningInProgress();
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

bool UProjectCleanerSubsystem::ScanningInProgress() const
{
	return bScanningProject;
}

bool UProjectCleanerSubsystem::CleaningInProgress() const
{
	return bCleaningProject;
}

bool UProjectCleanerSubsystem::EditorInPlayMode()
{
	if (!GEditor) return false;

	return GEditor->PlayWorld || GIsPlayInEditorWorld;
}

bool UProjectCleanerSubsystem::FolderIsExcluded(const FString& InFolderPath) const
{
	for (const auto& ExcludedFolder : GetDefault<UProjectCleanerExcludeSettings>()->ExcludedFolders)
	{
		if (FPaths::IsUnderDirectory(PathConvertToAbs(InFolderPath), PathConvertToAbs(ExcludedFolder.Path)))
		{
			return true;
		}
	}

	return false;
}

FProjectCleanerDelegateProjectScanned& UProjectCleanerSubsystem::OnProjectScanned()
{
	return DelegateProjectScanned;
}

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
		case EProjectCleanerScanResult::FailedToSaveAssets: return TEXT("Cant scan project because failed to save some assets");
		default: return TEXT("");
	}
}

void UProjectCleanerSubsystem::FindAssetsTotal()
{
	ModuleAssetRegistry->Get().GetAssetsByPath(ProjectCleanerConstants::PathRelRoot, ScanData.AssetsAll, true);
	// todo:ashe23 for ue5 make sure we exclude __External*__ folders
}

void UProjectCleanerSubsystem::FindAssetsPrimary()
{
	FScopedSlowTask SlowTask{
		3.0f,
		FText::FromString(TEXT("Scanning for primary assets...")),
		GIsEditor && !IsRunningCommandlet()
	};
	SlowTask.MakeDialog();

	TArray<FName> PrimaryAssetClasses;
	// getting list of primary asset classes that are defined in AssetManager
	const auto& AssetManager = UAssetManager::Get();
	if (!AssetManager.IsValid()) return;

	SlowTask.EnterProgressFrame(1.0f, FText::FromString(TEXT("Searching assets in AssetManager...")));

	TArray<FPrimaryAssetTypeInfo> AssetTypeInfos;
	AssetManager.Get().GetPrimaryAssetTypeInfoList(AssetTypeInfos);
	PrimaryAssetClasses.Reserve(AssetTypeInfos.Num());

	for (const auto& AssetTypeInfo : AssetTypeInfos)
	{
		if (!AssetTypeInfo.AssetBaseClassLoaded) continue;

		PrimaryAssetClasses.AddUnique(AssetTypeInfo.AssetBaseClassLoaded->GetFName());
	}

	SlowTask.EnterProgressFrame(1.0f, FText::FromString(TEXT("Searching assets that derived from primary assets...")));

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
	ModuleAssetRegistry->Get().GetAssets(Filter, ScanData.AssetsPrimary);

	SlowTask.EnterProgressFrame(1.0f, FText::FromString(TEXT("Searching blueprint primary assets...")));

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
			ScanData.AssetsPrimary.AddUnique(BlueprintAsset);
		}
	}
}

void UProjectCleanerSubsystem::FindAssetsIndirect()
{
	FScopedSlowTask SlowTask{
		2.0f,
		FText::FromString(TEXT("Scanning for indirect assets..")),
		GIsEditor && !IsRunningCommandlet()
	};
	SlowTask.MakeDialog();
	SlowTask.EnterProgressFrame(1.0f, FText::FromString(TEXT("Preparing scan directories...")));

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

	SlowTask.EnterProgressFrame(1.0f, FText::FromString(TEXT("Scanning files for indirect asset usages...")));

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


			const FAssetData* AssetData = ScanData.AssetsAll.FindByPredicate([&](const FAssetData& Elem)
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
				ScanData.AssetsIndirectInfo.AddUnique(IndirectAsset);
				ScanData.AssetsIndirect.AddUnique(*AssetData);
			}
		}
	}
}

void UProjectCleanerSubsystem::FindAssetsExcluded()
{
	FScopedSlowTask SlowTask{
		static_cast<float>(ScanData.AssetsAll.Num()),
		FText::FromString(TEXT("Scanning for excluded assets based on specified settings...")),
		GIsEditor && !IsRunningCommandlet()
	};
	SlowTask.MakeDialog();

	ScanData.AssetsExcluded.Reserve(ScanData.AssetsAll.Num());

	for (const auto& Asset : ScanData.AssetsAll)
	{
		SlowTask.EnterProgressFrame(1.0f, FText::FromString(FString::Printf(TEXT("%s"), *Asset.AssetName.ToString())));

		if (AssetIsExcluded(Asset))
		{
			ScanData.AssetsExcluded.AddUnique(Asset);
		}
	}

	ScanData.AssetsExcluded.Shrink();
}

void UProjectCleanerSubsystem::FindAssetsUsed()
{
	FScopedSlowTask SlowTask{
		3.0f,
		FText::FromString(TEXT("Searching for used assets..")),
		GIsEditor && !IsRunningCommandlet()
	};
	SlowTask.MakeDialog();

	SlowTask.EnterProgressFrame(1.0f, FText::FromString(TEXT("Searching assets with external referencers..")));

	TArray<FAssetData> AssetsWithExternalRefs;
	AssetsWithExternalRefs.Reserve(ScanData.AssetsAll.Num());

	TArray<FName> Refs;
	for (const auto& Asset : ScanData.AssetsAll)
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

	AssetsWithExternalRefs.Shrink();

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

	SlowTask.EnterProgressFrame(1.0f, FText::FromString(TEXT("Searching for used assets dependencies...")));

	ScanData.AssetsUsed.Reserve(
		ScanData.AssetsPrimary.Num() + ScanData.AssetsIndirect.Num() + AssetsWithExternalRefs.Num() + AssetsEditor.Num() + AssetsMegascans.Num() + ScanData.AssetsExcluded.Num()
	);

	for (const auto& Asset : ScanData.AssetsPrimary)
	{
		ScanData.AssetsUsed.AddUnique(Asset);
	}

	for (const auto& Asset : ScanData.AssetsIndirect)
	{
		ScanData.AssetsUsed.AddUnique(Asset);
	}

	for (const auto& Asset : AssetsWithExternalRefs)
	{
		ScanData.AssetsUsed.AddUnique(Asset);
	}

	for (const auto& Asset : AssetsEditor)
	{
		ScanData.AssetsUsed.AddUnique(Asset);
	}

	for (const auto& Asset : AssetsMegascans)
	{
		ScanData.AssetsUsed.AddUnique(Asset);
	}

	for (const auto& Asset : ScanData.AssetsExcluded)
	{
		ScanData.AssetsUsed.AddUnique(Asset);
	}

	TArray<FAssetData> AssetsDependencies;
	GetAssetsDependencies(ScanData.AssetsUsed, AssetsDependencies);

	ScanData.AssetsUsed.Reserve(ScanData.AssetsUsed.Num() + AssetsDependencies.Num());

	for (const auto& Asset : AssetsDependencies)
	{
		ScanData.AssetsUsed.AddUnique(Asset);
	}

	ScanData.AssetsUsed.Shrink();
}

void UProjectCleanerSubsystem::FindAssetsUnused()
{
	FScopedSlowTask SlowTask{
		static_cast<float>(ScanData.AssetsAll.Num()),
		FText::FromString(TEXT("Searching for unused assets...")),
		GIsEditor && !IsRunningCommandlet()
	};
	SlowTask.MakeDialog();

	ScanData.AssetsUnused.Reserve(ScanData.AssetsAll.Num());

	for (const auto& Asset : ScanData.AssetsAll)
	{
		SlowTask.EnterProgressFrame(1.0f, FText::FromString(FString::Printf(TEXT("%s"), *Asset.AssetName.ToString())));

		if (!ScanData.AssetsUsed.Contains(Asset))
		{
			ScanData.AssetsUnused.AddUnique(Asset);
		}
	}

	ScanData.AssetsUnused.Shrink();
}

void UProjectCleanerSubsystem::FindFilesCorrupted()
{
	TArray<FString> Files;
	IFileManager::Get().FindFilesRecursive(Files, *FPaths::ProjectContentDir(), TEXT("*.*"), true, false);

	FScopedSlowTask SlowTask{
		static_cast<float>(Files.Num()),
		FText::FromString(TEXT("Searching for corrupted files...")),
		GIsEditor && !IsRunningCommandlet()
	};
	SlowTask.MakeDialog();

	ScanData.FilesCorrupted.Reserve(Files.Num());

	for (const auto& File : Files)
	{
		SlowTask.EnterProgressFrame(1.0f, FText::FromString(FString::Printf(TEXT("%s"), *File)));

		const FString FilePathAbs = FPaths::ConvertRelativePathToFull(File);
		if (!FileHasEngineExtension(FilePathAbs)) continue;
		if (!FileIsCorrupted(FilePathAbs)) continue;

		ScanData.FilesCorrupted.AddUnique(FilePathAbs);
	}

	ScanData.FilesCorrupted.Shrink();
}

void UProjectCleanerSubsystem::FindFilesNonEngine()
{
	TArray<FString> Files;
	IFileManager::Get().FindFilesRecursive(Files, *FPaths::ProjectContentDir(), TEXT("*.*"), true, false);

	FScopedSlowTask SlowTask{
		static_cast<float>(Files.Num()),
		FText::FromString(TEXT("Searching for non engine files...")),
		GIsEditor && !IsRunningCommandlet()
	};
	SlowTask.MakeDialog();

	ScanData.FilesNonEngine.Reserve(Files.Num());

	for (const auto& File : Files)
	{
		SlowTask.EnterProgressFrame(1.0f, FText::FromString(FString::Printf(TEXT("%s"), *File)));

		const FString FilePathAbs = FPaths::ConvertRelativePathToFull(File);
		if (FileHasEngineExtension(FilePathAbs)) continue;

		ScanData.FilesNonEngine.AddUnique(FilePathAbs);
	}

	ScanData.FilesNonEngine.Shrink();
}

void UProjectCleanerSubsystem::FindFolders()
{
	TArray<FString> Folders;
	IFileManager::Get().FindFilesRecursive(Folders, *FPaths::ProjectContentDir(), TEXT("*.*"), false, true);

	for (const auto& Folder : Folders)
	{
		const FString FolderPathAbs = FPaths::ConvertRelativePathToFull(Folder);

		ScanData.FoldersAll.AddUnique(FolderPathAbs);

		if (FolderIsEmpty(FolderPathAbs) && !FolderIsExcluded(FolderPathAbs))
		{
			ScanData.FoldersEmpty.AddUnique(FolderPathAbs);
		}
	}
}

void UProjectCleanerSubsystem::ScanDataReset()
{
	ScanData.ScanResult = EProjectCleanerScanResult::None;
	ScanData.ScanResultMsg.Reset();
	ScanData.AssetsAll.Reset();
	ScanData.AssetsUsed.Reset();
	ScanData.AssetsPrimary.Reset();
	ScanData.AssetsIndirect.Reset();
	ScanData.AssetsIndirectInfo.Reset();
	ScanData.AssetsExcluded.Reset();
	ScanData.AssetsUnused.Reset();
	ScanData.FoldersAll.Reset();
	ScanData.FoldersEmpty.Reset();
	ScanData.FilesCorrupted.Reset();
	ScanData.FilesNonEngine.Reset();
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

bool UProjectCleanerSubsystem::AssetExcludedByPath(const FAssetData& AssetData) const
{
	for (const auto& ExcludedFolder : ScanSettings.ExcludedFolders)
	{
		const FString FolderPathAbs = PathConvertToAbs(ExcludedFolder);
		const FString AssetPathAbs = PathConvertToAbs(AssetData.PackagePath.ToString());

		if (FolderPathAbs.IsEmpty() || AssetPathAbs.IsEmpty()) continue;
		if (FPaths::IsUnderDirectory(AssetPathAbs, FolderPathAbs))
		{
			return true;
		}
	}

	return false;
}

bool UProjectCleanerSubsystem::AssetExcludedByClass(const FAssetData& AssetData) const
{
	const FString AssetClassName = GetAssetClassName(AssetData);

	for (const auto& ExcludedClass : ScanSettings.ExcludedClasses)
	{
		if (!ExcludedClass) continue;

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
	for (const auto& ExcludedAsset : ScanSettings.ExcludedAssets)
	{
		if (!ExcludedAsset.LoadSynchronous()) continue;

		if (ExcludedAsset.ToSoftObjectPath() == AssetData.ToSoftObjectPath())
		{
			return true;
		}
	}

	return false;
}

bool UProjectCleanerSubsystem::FileHasEngineExtension(const FString& InFilePath)
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
