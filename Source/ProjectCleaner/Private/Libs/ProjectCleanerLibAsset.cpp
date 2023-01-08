// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Libs/ProjectCleanerLibAsset.h"
#include "ProjectCleanerConstants.h"
#include "ProjectCleanerTypes.h"
// Engine Headers
#include "AssetToolsModule.h"
#include "EditorUtilityBlueprint.h"
#include "EditorUtilityWidget.h"
#include "EditorUtilityWidgetBlueprint.h"
#include "FileHelpers.h"
#include "ProjectCleaner.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/AssetManager.h"
#include "Engine/MapBuildDataRegistry.h"
#include "Internationalization/Regex.h"
#include "Libs/ProjectCleanerLibEditor.h"
#include "Libs/ProjectCleanerLibFile.h"
#include "Libs/ProjectCleanerLibPath.h"
#include "Misc/FileHelper.h"
#include "Misc/ScopedSlowTask.h"
#include "Settings/ProjectCleanerSettings.h"

bool UProjectCleanerLibAsset::AssetRegistryWorking()
{
	const FAssetRegistryModule& ModuleAssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);

	return ModuleAssetRegistry.Get().IsLoadingAssets();
}

FString UProjectCleanerLibAsset::GetAssetClassName(const FAssetData& AssetData)
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

UClass* UProjectCleanerLibAsset::GetAssetClass(const FAssetData& AssetData)
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

void UProjectCleanerLibAsset::GetAssetsDependencies(const TArray<FAssetData>& Assets, TArray<FAssetData>& Dependencies)
{
	Dependencies.Empty();

	const FAssetRegistryModule& ModuleAssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);

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

			ModuleAssetRegistry.Get().GetDependencies(CurrentPackageName, Deps);

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

	ModuleAssetRegistry.Get().GetAssets(Filter, Dependencies);
}

void UProjectCleanerLibAsset::GetAssetsReferencers(const TArray<FAssetData>& Assets, TArray<FAssetData>& Referencers)
{
	Referencers.Empty();

	const FAssetRegistryModule& ModuleAssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);

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

			ModuleAssetRegistry.Get().GetReferencers(CurrentPackageName, Refs);

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

	ModuleAssetRegistry.Get().GetAssets(Filter, Referencers);
}

void UProjectCleanerLibAsset::GetAssetsAll(TArray<FAssetData>& Assets)
{
	Assets.Empty();

	const FAssetRegistryModule& ModuleAssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);
	ModuleAssetRegistry.Get().GetAssetsByPath(ProjectCleanerConstants::PathRelRoot, Assets, true);

	// todo:ashe23 for ue5 exclude __External*_ folders
}

void UProjectCleanerLibAsset::GetAssetsPrimary(TArray<FAssetData>& AssetsPrimary)
{
	AssetsPrimary.Empty();

	const FAssetRegistryModule& ModuleAssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);

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
		ModuleAssetRegistry.Get().GetDerivedClassNames(PrimaryAssetClasses, ExcludedClassNames, DerivedFromPrimaryAssets);
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

	AssetsPrimary.Reserve(PrimaryAssetClasses.Num());
	ModuleAssetRegistry.Get().GetAssets(Filter, AssetsPrimary);

	// getting primary blueprint assets
	FARFilter FilterBlueprint;
	FilterBlueprint.bRecursivePaths = true;
	FilterBlueprint.bRecursiveClasses = true;
	FilterBlueprint.PackagePaths.Add(ProjectCleanerConstants::PathRelRoot);
	FilterBlueprint.ClassNames.Add(UBlueprint::StaticClass()->GetFName());

	TArray<FAssetData> BlueprintAssets;
	ModuleAssetRegistry.Get().GetAssets(FilterBlueprint, BlueprintAssets);

	AssetsPrimary.Reserve(AssetsPrimary.Num() + BlueprintAssets.Num());
	for (const auto& BlueprintAsset : BlueprintAssets)
	{
		const FName BlueprintClass = FName{*GetAssetClassName(BlueprintAsset)};
		if (PrimaryAssetClasses.Contains(BlueprintClass))
		{
			AssetsPrimary.AddUnique(BlueprintAsset);
		}
	}
}

void UProjectCleanerLibAsset::GetAssetsIndirect(TArray<FAssetData>& AssetsIndirect)
{
	TArray<FProjectCleanerIndirectAssetInfo> AssetsIndirectInfos;
	GetAssetsIndirectInfo(AssetsIndirectInfos);

	AssetsIndirect.Empty();
	AssetsIndirect.Reserve(AssetsIndirectInfos.Num());

	for (const auto& Info : AssetsIndirectInfos)
	{
		AssetsIndirect.AddUnique(Info.AssetData);
	}

	AssetsIndirect.Shrink();
}

void UProjectCleanerLibAsset::GetAssetsIndirectInfo(TArray<FProjectCleanerIndirectAssetInfo>& AssetsIndirectInfos)
{
	AssetsIndirectInfos.Empty();

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	const FAssetRegistryModule& ModuleAssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);

	FScopedSlowTask SlowTask{
		2.0f,
		FText::FromString(TEXT("Scanning for indirect assets...")),
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
	PlatformFile.FindFilesRecursively(FilesToScan, *SourceDir, TEXT(".cs"));
	PlatformFile.FindFilesRecursively(FilesToScan, *SourceDir, TEXT(".cpp"));
	PlatformFile.FindFilesRecursively(FilesToScan, *SourceDir, TEXT(".h"));
	PlatformFile.FindFilesRecursively(FilesToScan, *ConfigDir, TEXT(".ini"));
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
	PlatformFile.IterateDirectory(*PluginsDir, Visitor);

	// 3) for every installed plugin we scanning only "Source" and "Config" folders
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

	FScopedSlowTask SlowTaskFiles{
		static_cast<float>(Files.Num()),
		FText::FromString(TEXT("Scanning files for indirect asset usages...")),
		GIsEditor && !IsRunningCommandlet()
	};
	SlowTaskFiles.MakeDialog();

	for (const auto& File : Files)
	{
		SlowTaskFiles.EnterProgressFrame(1.0f, FText::FromString(FString::Printf(TEXT("Scanning %s"), *File)));

		if (!PlatformFile.FileExists(*File)) continue;

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

			const FAssetData AssetData = ModuleAssetRegistry.Get().GetAssetByObjectPath(FName{*FoundedAssetObjectPath});
			if (!AssetData.IsValid()) continue;

			// if founded asset is ok, we loading file by lines to determine on what line its used
			TArray<FString> Lines;
			FFileHelper::LoadFileToStringArray(Lines, *File);
			for (int32 i = 0; i < Lines.Num(); ++i)
			{
				if (!Lines.IsValidIndex(i)) continue;
				if (!Lines[i].Contains(FoundedAssetObjectPath)) continue;

				FProjectCleanerIndirectAssetInfo IndirectAsset;
				IndirectAsset.AssetData = AssetData;
				IndirectAsset.FilePath = FPaths::ConvertRelativePathToFull(File);
				IndirectAsset.LineNum = i + 1;
				AssetsIndirectInfos.AddUnique(IndirectAsset);
			}
		}
	}

	SlowTask.EnterProgressFrame(1.0f);
}

void UProjectCleanerLibAsset::GetAssetsUsed(TArray<FAssetData>& AssetsUsed)
{
	AssetsUsed.Empty();

	TArray<FAssetData> AssetsAll;
	TArray<FAssetData> AssetsPrimary;
	TArray<FAssetData> AssetsIndirect;
	TArray<FAssetData> AssetsEditor;
	TArray<FAssetData> AssetsMegascans;
	TArray<FAssetData> AssetsWithExternalRefs;

	GetAssetsAll(AssetsAll);
	GetAssetsPrimary(AssetsPrimary);
	GetAssetsIndirect(AssetsIndirect);
	GetAssetsEditor(AssetsEditor);
	GetAssetsMegascans(AssetsMegascans);
	GetAssetsWithExternalRefs(AssetsWithExternalRefs, AssetsAll);

	AssetsUsed.Reserve(AssetsPrimary.Num() + AssetsPrimary.Num() + AssetsIndirect.Num() + AssetsEditor.Num() + AssetsMegascans.Num() + AssetsWithExternalRefs.Num());

	for (const auto& Asset : AssetsPrimary)
	{
		AssetsUsed.AddUnique(Asset);
	}

	for (const auto& Asset : AssetsIndirect)
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

	for (const auto& Asset : AssetsWithExternalRefs)
	{
		AssetsUsed.AddUnique(Asset);
	}

	TArray<FAssetData> UsedAssetsDependencies;
	GetAssetsDependencies(AssetsUsed, UsedAssetsDependencies);

	AssetsUsed.Reserve(AssetsUsed.Num() + UsedAssetsDependencies.Num());
	for (const auto& Asset : UsedAssetsDependencies)
	{
		AssetsUsed.AddUnique(Asset);
	}

	AssetsUsed.Shrink();
}

void UProjectCleanerLibAsset::ProjectScan(const FProjectCleanerScanSettings& ScanSettings, FProjectCleanerScanData& ScanData)
{
	ScanData.Empty();

	if (AssetRegistryWorking())
	{
		ScanData.ScanResult = EProjectCleanerScanResult::AssetRegistryWorking;
		ScanData.ScanResultMsg = TEXT("Cant scan project because AssetRegistry still working");
		UE_LOG(LogProjectCleaner, Error, TEXT("%s"), *ScanData.ScanResultMsg);

		return;
	}

	if (UProjectCleanerLibEditor::EditorInPlayMode())
	{
		ScanData.ScanResult = EProjectCleanerScanResult::EditorInPlayMode;
		ScanData.ScanResultMsg = TEXT("Cant scan project because Editor is in Play mode");
		UE_LOG(LogProjectCleaner, Error, TEXT("%s"), *ScanData.ScanResultMsg);

		return;
	}

	if (!IsRunningCommandlet())
	{
		GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->CloseAllAssetEditors();
		FixupRedirectors();
	}

	if (!FEditorFileUtils::SaveDirtyPackages(false, true, true, false, false, false))
	{
		ScanData.ScanResult = EProjectCleanerScanResult::FailedToSaveAssets;
		ScanData.ScanResultMsg = TEXT("Cant scan project because failed to save some assets");
		UE_LOG(LogProjectCleaner, Error, TEXT("%s"), *ScanData.ScanResultMsg);

		return;
	}

	FScopedSlowTask SlowTask{
		6.0f,
		FText::FromString(TEXT("Scanning project...")),
		GIsEditor && !IsRunningCommandlet()
	};
	SlowTask.MakeDialog();

	SlowTask.EnterProgressFrame(1.0f, FText::FromString(TEXT("Scanning Content folder...")));
	UProjectCleanerLibFile::GetFolders(UProjectCleanerLibPath::GetFolderContent(), ScanData.FoldersAll, true);

	SlowTask.EnterProgressFrame(1.0f, FText::FromString(TEXT("Searching Empty folders...")));
	UProjectCleanerLibFile::GetFoldersEmpty(ScanData.FoldersEmpty);

	SlowTask.EnterProgressFrame(1.0f, FText::FromString(TEXT("Searching Corrupted files...")));
	UProjectCleanerLibFile::GetFilesCorrupted(ScanData.FilesCorrupted);

	SlowTask.EnterProgressFrame(1.0f, FText::FromString(TEXT("Searching NonEngine files...")));
	UProjectCleanerLibFile::GetFilesNonEngine(ScanData.FilesNonEngine);

	SlowTask.EnterProgressFrame(1.0f, FText::FromString(TEXT("Searching Primary assets...")));
	GetAssetsPrimary(ScanData.AssetsPrimary);

	SlowTask.EnterProgressFrame(1.0f, FText::FromString(TEXT("Searching Indirect assets...")));
	GetAssetsIndirect(ScanData.AssetsIndirect);
	GetAssetsIndirectInfo(ScanData.AssetsIndirectInfo);

	// SlowTask.EnterProgressFrame(1.0f, FText::FromString(TEXT("Searching Used assets...")));
	// GetAssetsUsed(ScanData.AssetsUsed);
}

int64 UProjectCleanerLibAsset::GetAssetsTotalSize(const TArray<FAssetData>& Assets)
{
	int64 Size = 0;

	const FAssetRegistryModule& ModuleAssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);

	for (const auto& Asset : Assets)
	{
		if (!Asset.IsValid()) continue;

		const auto AssetPackageData = ModuleAssetRegistry.Get().GetAssetPackageData(Asset.PackageName);
		if (!AssetPackageData) continue;

		Size += AssetPackageData->DiskSize;
	}

	return Size;
}

void UProjectCleanerLibAsset::ProjectScan(const UProjectCleanerSettings* Settings, FProjectCleanerScanData& ScanData)
{
	if (!Settings)
	{
		UE_LOG(LogProjectCleaner, Error, TEXT("Invalid Settings"));
		return;
	}

	FProjectCleanerScanSettings ScanSettings;

	for (const auto& ScanPath : Settings->ScanPaths)
	{
		const FString PathAbs = UProjectCleanerLibPath::ConvertToAbs(ScanPath.Path);
		if (PathAbs.IsEmpty()) continue;

		ScanSettings.ScanPaths.Add(UProjectCleanerLibPath::ConvertToRel(PathAbs));
	}

	for (const auto& ExcludePath : Settings->ExcludePaths)
	{
		const FString PathAbs = UProjectCleanerLibPath::ConvertToAbs(ExcludePath.Path);
		if (PathAbs.IsEmpty()) continue;

		ScanSettings.ExcludePaths.Add(UProjectCleanerLibPath::ConvertToRel(PathAbs));
	}

	for (const auto& ScanClass : Settings->ScanClasses)
	{
		if (!ScanClass.LoadSynchronous()) continue;

		ScanSettings.ScanClasses.Add(ScanClass.Get());
	}

	for (const auto& ExcludeClass : Settings->ExcludeClasses)
	{
		if (!ExcludeClass.LoadSynchronous()) continue;

		ScanSettings.ExcludeClasses.Add(ExcludeClass.Get());
	}

	for (const auto& ExcludeAsset : Settings->ExcludeAssets)
	{
		if (!ExcludeAsset.LoadSynchronous()) continue;

		ScanSettings.ExcludeAssets.Add(ExcludeAsset.Get());
	}

	ProjectScan(ScanSettings, ScanData);
}

void UProjectCleanerLibAsset::FixupRedirectors()
{
	const FAssetRegistryModule& ModuleAssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);
	const FAssetToolsModule& ModuleAssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));

	FScopedSlowTask FixRedirectorsTask{
		1.0f,
		FText::FromString(TEXT("Fixing redirectors...")),
		GIsEditor && !IsRunningCommandlet()
	};
	FixRedirectorsTask.MakeDialog();

	FARFilter Filter;
	Filter.bRecursivePaths = true;
	Filter.PackagePaths.Add(ProjectCleanerConstants::PathRelRoot);
	Filter.ClassNames.Add(UObjectRedirector::StaticClass()->GetFName());

	TArray<FAssetData> AssetList;
	ModuleAssetRegistry.Get().GetAssets(Filter, AssetList);

	if (AssetList.Num() == 0) return;

	FScopedSlowTask FixRedirectorsLoadingTask(
		AssetList.Num(),
		FText::FromString(TEXT("Loading redirectors...")),
		GIsEditor && !IsRunningCommandlet()
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

	ModuleAssetTools.Get().FixupReferencers(Redirectors, false);

	FixRedirectorsTask.EnterProgressFrame(1.0f);
}

void UProjectCleanerLibAsset::GetAssetsEditor(TArray<FAssetData>& AssetsEditor)
{
	AssetsEditor.Empty();

	const FAssetRegistryModule& ModuleAssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);

	FARFilter Filter;
	Filter.bRecursivePaths = true;
	Filter.bRecursiveClasses = true;
	Filter.PackagePaths.Add(ProjectCleanerConstants::PathRelRoot);
	Filter.ClassNames.Add(UEditorUtilityWidget::StaticClass()->GetFName());
	Filter.ClassNames.Add(UEditorUtilityBlueprint::StaticClass()->GetFName());
	Filter.ClassNames.Add(UEditorUtilityWidgetBlueprint::StaticClass()->GetFName());
	Filter.ClassNames.Add(UMapBuildDataRegistry::StaticClass()->GetFName());

	ModuleAssetRegistry.Get().GetAssets(Filter, AssetsEditor);
}

void UProjectCleanerLibAsset::GetAssetsMegascans(TArray<FAssetData>& AssetsMegascans)
{
	AssetsMegascans.Empty();

	if (FModuleManager::Get().IsModuleLoaded(TEXT("MegascansPlugin")))
	{
		const FAssetRegistryModule& ModuleAssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);
		ModuleAssetRegistry.Get().GetAssetsByPath(ProjectCleanerConstants::PathRelMSPresets, AssetsMegascans, true);
	}
}

void UProjectCleanerLibAsset::GetAssetsWithExternalRefs(TArray<FAssetData>& Assets, const TArray<FAssetData>& AssetsAll)
{
	Assets.Empty();
	Assets.Reserve(AssetsAll.Num());

	const FAssetRegistryModule& ModuleAssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);

	TArray<FName> Refs;
	for (const auto& Asset : AssetsAll)
	{
		ModuleAssetRegistry.Get().GetReferencers(Asset.PackageName, Refs);

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

	Assets.Shrink();
}
