// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Libs/ProjectCleanerLibAsset.h"
#include "Libs/ProjectCleanerLibPath.h"
#include "ProjectCleanerConstants.h"
// Engine Headers
#include "AssetToolsModule.h"
#include "Engine/AssetManager.h"
#include "Engine/MapBuildDataRegistry.h"
#include "EditorUtilityBlueprint.h"
#include "EditorUtilityWidget.h"
#include "EditorUtilityWidgetBlueprint.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Internationalization/Regex.h"
#include "Misc/FileHelper.h"
#include "Misc/ScopedSlowTask.h"
#include "Settings/ProjectCleanerExcludeSettings.h"

void UProjectCleanerLibAsset::ProjectScan(const FProjectCleanerScanSettings& ScanSettings, FProjectCleanerScanResult& ScanResult)
{
}

void UProjectCleanerLibAsset::ProjectClean()
{
}

void UProjectCleanerLibAsset::ProjectCleanEmptyFolders()
{
}

void UProjectCleanerLibAsset::FixupRedirectors()
{
	const auto ModuleAssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);
	const FAssetToolsModule& ModuleAssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
	
	FScopedSlowTask FixRedirectorsTask{
		1.0f,
		FText::FromString(ProjectCleanerConstants::MsgFixingRedirectors)
	};
	FixRedirectorsTask.MakeDialog();

	FARFilter Filter;
	Filter.bRecursivePaths = true;
	Filter.PackagePaths.Emplace(ProjectCleanerConstants::PathRelRoot);
	Filter.ClassNames.Emplace(UObjectRedirector::StaticClass()->GetFName());

	TArray<FAssetData> AssetList;
	ModuleAssetRegistry.Get().GetAssets(Filter, AssetList);

	if (AssetList.Num() == 0) return;

	FScopedSlowTask FixRedirectorsLoadingTask(
		AssetList.Num(),
		FText::FromString(ProjectCleanerConstants::MsgLoadingRedirectors)
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

	ModuleAssetTools.Get().FixupReferencers(Redirectors);

	FixRedirectorsTask.EnterProgressFrame(1.0f);
}

void UProjectCleanerLibAsset::GetAssetsAll(TArray<FAssetData>& AssetsAll)
{
	AssetsAll.Reset();

	const auto ModuleAssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);
	ModuleAssetRegistry.Get().GetAssetsByPath(ProjectCleanerConstants::PathRelRoot, AssetsAll);
}

void UProjectCleanerLibAsset::GetAssetsPrimary(TArray<FAssetData>& AssetsPrimary)
{
	AssetsPrimary.Reset();

	const auto ModuleAssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);

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
	Filter.PackagePaths.Add(FName{ProjectCleanerConstants::PathRelRoot});

	for (const auto& ClassName : PrimaryAssetClasses)
	{
		Filter.ClassNames.Add(ClassName);
	}
	ModuleAssetRegistry.Get().GetAssets(Filter, AssetsPrimary);

	FARFilter FilterBlueprint;
	FilterBlueprint.bRecursivePaths = true;
	FilterBlueprint.bRecursiveClasses = true;
	FilterBlueprint.PackagePaths.Add(ProjectCleanerConstants::PathRelRoot);
	FilterBlueprint.ClassNames.Add(UBlueprint::StaticClass()->GetFName());

	TArray<FAssetData> BlueprintAssets;
	ModuleAssetRegistry.Get().GetAssets(FilterBlueprint, BlueprintAssets);

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
	AssetsIndirect.Reset();

	TArray<FProjectCleanerIndirectAsset> AssetIndirectInfos;
	GetAssetsIndirectWithInfo(AssetIndirectInfos);

	for (const auto& AssetIndirectInfo : AssetIndirectInfos)
	{
		AssetsIndirect.Add(AssetIndirectInfo.AssetData);
	}
}

void UProjectCleanerLibAsset::GetAssetsIndirectWithInfo(TArray<FProjectCleanerIndirectAsset>& AssetsIndirect)
{
	AssetsIndirect.Reset();

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

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

	for (const auto& File : Files)
	{
		if (!PlatformFile.FileExists(*File)) continue;

		FString FileContent;
		FFileHelper::LoadFileToString(FileContent, *File);

		if (!UProjectCleanerLibPath::FileContainsIndirectAssets(FileContent)) continue;

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

			TArray<FAssetData> AssetsAll;
			GetAssetsAll(AssetsAll); // todo:ashe23 optimize

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
				AssetsIndirect.AddUnique(IndirectAsset);
			}
		}
	}
}

void UProjectCleanerLibAsset::GetAssetsWithExternalRefs(TArray<FAssetData>& AssetsWithExternalRefs)
{
	AssetsWithExternalRefs.Reset();

	TArray<FAssetData> AssetsAll;
	GetAssetsAll(AssetsAll);// todo:ashe23 optimize

	const auto ModuleAssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);

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
			AssetsWithExternalRefs.AddUnique(Asset);
		}

		Refs.Reset();
	}
}

void UProjectCleanerLibAsset::GetAssetsBlacklisted(TArray<FAssetData>& AssetsBlacklisted)
{
	AssetsBlacklisted.Reset();

	const auto ModuleAssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);

	FARFilter FilterBlacklistAssets;
	FilterBlacklistAssets.bRecursivePaths = true;
	FilterBlacklistAssets.bRecursiveClasses = true;
	FilterBlacklistAssets.PackagePaths.Add(ProjectCleanerConstants::PathRelRoot);
	FilterBlacklistAssets.ClassNames.Add(UEditorUtilityWidget::StaticClass()->GetFName());
	FilterBlacklistAssets.ClassNames.Add(UEditorUtilityBlueprint::StaticClass()->GetFName());
	FilterBlacklistAssets.ClassNames.Add(UEditorUtilityWidgetBlueprint::StaticClass()->GetFName());
	FilterBlacklistAssets.ClassNames.Add(UMapBuildDataRegistry::StaticClass()->GetFName());
	ModuleAssetRegistry.Get().GetAssets(FilterBlacklistAssets, AssetsBlacklisted);
}

void UProjectCleanerLibAsset::GetAssetsLinked(const TArray<FAssetData>& InAssets, TArray<FAssetData>& OutLinkedAssets)
{
	OutLinkedAssets.Reset();

	const auto ModuleAssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);

	TSet<FName> UsedAssetsDeps;
	TArray<FName> Stack;
	for (const auto& Asset : InAssets)
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

	ModuleAssetRegistry.Get().GetAssets(Filter, OutLinkedAssets);
}

void UProjectCleanerLibAsset::GetAssetsUsed(TArray<FAssetData>& AssetsUsed)
{
	AssetsUsed.Reset();

	TArray<FAssetData> AssetsAll;
	TArray<FAssetData> AssetsPrimary;
	TArray<FAssetData> AssetsIndirect;
	TArray<FAssetData> AssetsWithExternalRefs;
	TArray<FAssetData> AssetsBlacklisted;
	TArray<FAssetData> AssetsExcluded;

	GetAssetsAll(AssetsAll);
	GetAssetsPrimary(AssetsPrimary);
	GetAssetsIndirect(AssetsIndirect);
	GetAssetsWithExternalRefs(AssetsWithExternalRefs);
	GetAssetsBlacklisted(AssetsBlacklisted);
	GetAssetsExcluded(AssetsExcluded);
	
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

	for (const auto& Asset : AssetsBlacklisted)
	{
		AssetsUsed.AddUnique(Asset);
	}

	for (const auto& Asset : AssetsExcluded)
	{
		AssetsUsed.AddUnique(Asset);
	}

	TArray<FAssetData> LinkedAssets;
	GetAssetsLinked(AssetsUsed, LinkedAssets);
	
	for (const auto& LinkedAsset : LinkedAssets)
	{
		AssetsUsed.AddUnique(LinkedAsset);
	}
}

void UProjectCleanerLibAsset::GetAssetsUnused(TArray<FAssetData>& AssetsUnused)
{
	// AssetsUnused.Append(AssetsAll);
	//
	// FARFilter Filter;
	// Filter.bRecursivePaths = true;
	//
	// for (const auto& FolderBlacklist : FoldersBlacklisted)
	// {
	// 	const FString FolderPathRel = UProjectCleanerLibPath::Convert(FolderBlacklist, EProjectCleanerPathType::Relative);
	// 	Filter.PackagePaths.AddUnique(FName{*FolderPathRel});
	// }
	//
	// for (const auto& Asset : AssetsUsed)
	// {
	// 	Filter.ObjectPaths.AddUnique(Asset.ObjectPath);
	// }
	//
	// ModuleAssetRegistry->Get().UseFilterToExcludeAssets(AssetsUnused, Filter);
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

int64 UProjectCleanerLibAsset::GetAssetsTotalSize(const TArray<FAssetData>& Assets)
{
	const auto ModuleAssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);

	int64 Size = 0;

	for (const auto& Asset : Assets)
	{
		const auto AssetPackageData = ModuleAssetRegistry.Get().GetAssetPackageData(Asset.PackageName);
		if (!AssetPackageData) continue;
		Size += AssetPackageData->DiskSize;
	}

	return Size;
}

void UProjectCleanerLibAsset::GetFoldersBlacklisted(TSet<FString>& FoldersBlacklisted)
{
	FoldersBlacklisted.Reset();

	FoldersBlacklisted.Add(UProjectCleanerLibPath::FolderCollections(EProjectCleanerPathType::Absolute));
	FoldersBlacklisted.Add(UProjectCleanerLibPath::FolderDeveloperCollections(EProjectCleanerPathType::Absolute));
	// todo:ashe23 for ue5 add __ExternalObject__ and __ExternalActors__ folders

	if (FModuleManager::Get().IsModuleLoaded(ProjectCleanerConstants::PluginNameMegascans))
	{
		FoldersBlacklisted.Add(UProjectCleanerLibPath::FolderMsPresets(EProjectCleanerPathType::Absolute));
	}
}

void UProjectCleanerLibAsset::GetAssetsExcluded(TArray<FAssetData>& AssetsExcluded)
{
	const UProjectCleanerExcludeSettings* ExcludeSettings = GetDefault<UProjectCleanerExcludeSettings>();
	if (!ExcludeSettings) return;

	AssetsExcluded.Reset();
	
	const auto ModuleAssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);

	TArray<FAssetData> AssetsAll;
	GetAssetsAll(AssetsAll); // todo:ashe23 optimize
	
	for (const auto& Asset : AssetsAll)
	{
		// excluded by path
		const FString PackagePathAbs = UProjectCleanerLibPath::Convert(Asset.PackagePath.ToString(), EProjectCleanerPathType::Absolute);
		for (const auto& ExcludedFolder : ExcludeSettings->ExcludedFolders)
		{
			const FString ExcludedFolderPathAbs = UProjectCleanerLibPath::Convert(ExcludedFolder.Path, EProjectCleanerPathType::Absolute);

			if (UProjectCleanerLibPath::IsUnderFolder(PackagePathAbs, ExcludedFolderPathAbs))
			{
				AssetsExcluded.AddUnique(Asset);
			}
		}

		// excluded by class
		const FString AssetClassName = GetAssetClassName(Asset);
		for (const auto& ExcludedClass : ExcludeSettings->ExcludedClasses)
		{
			if (!ExcludedClass.LoadSynchronous()) continue;

			const FString ExcludedClassName = ExcludedClass->GetName();

			if (ExcludedClassName.Equals(AssetClassName))
			{
				AssetsExcluded.AddUnique(Asset);
			}
		}
	}

	for (const auto& ExcludedAsset : ExcludeSettings->ExcludedAssets)
	{
		if (!ExcludedAsset.LoadSynchronous()) continue;

		const FName AssetObjectPath = ExcludedAsset.ToSoftObjectPath().GetAssetPathName();
		const FAssetData AssetData = ModuleAssetRegistry.Get().GetAssetByObjectPath(AssetObjectPath);
		if (!AssetData.IsValid()) continue;

		AssetsExcluded.AddUnique(AssetData);
	}
}
