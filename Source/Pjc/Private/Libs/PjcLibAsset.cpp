// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Libs/PjcLibAsset.h"
#include "Libs/PjcLibPath.h"
#include "PjcConstants.h"
// Engine Headers
#include "AssetToolsModule.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/AssetManager.h"
#include "EditorTutorial.h"
#include "EditorUtilityBlueprint.h"
#include "EditorUtilityWidget.h"
#include "EditorUtilityWidgetBlueprint.h"
#include "Internationalization/Regex.h"
#include "Misc/FileHelper.h"
#include "Misc/ScopedSlowTask.h"

bool FPjcLibAsset::ProjectContainsRedirectors()
{
	FARFilter Filter;
	Filter.bRecursivePaths = true;
	Filter.PackagePaths.Emplace(PjcConstants::PathRelRoot);
	Filter.ClassNames.Emplace(UObjectRedirector::StaticClass()->GetFName());

	const FAssetRegistryModule& ModuleAssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(PjcConstants::ModuleAssetRegistryName);
	TArray<FAssetData> Redirectors;
	ModuleAssetRegistry.Get().GetAssets(Filter, Redirectors);

	return Redirectors.Num() > 0;
}

bool FPjcLibAsset::AssetIsBlueprint(const FAssetData& InAssetData)
{
	if (!InAssetData.IsValid()) return false;

	const UClass* AssetClass = InAssetData.GetClass();
	if (!AssetClass) return false;

	return AssetClass->IsChildOf(UBlueprint::StaticClass());
}

bool FPjcLibAsset::AssetIsExtReferenced(const FAssetData& InAssetData)
{
	if (!InAssetData.IsValid()) return false;

	const FAssetRegistryModule& ModuleAssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(PjcConstants::ModuleAssetRegistryName);

	TArray<FName> Refs;
	ModuleAssetRegistry.Get().GetReferencers(InAssetData.PackageName, Refs);

	return Refs.ContainsByPredicate([](const FName& Ref)
	{
		return !Ref.ToString().StartsWith(PjcConstants::PathRelRoot.ToString());
	});
}

bool FPjcLibAsset::AssetIsMegascansBase(const FAssetData& InAssetData)
{
	if (!FModuleManager::Get().IsModuleLoaded(PjcConstants::ModuleMegascans)) return false;
	if (!InAssetData.IsValid()) return false;

	return InAssetData.PackagePath.ToString().StartsWith(PjcConstants::PathRelMSPresets.ToString());
}

bool FPjcLibAsset::AssetClassNameInList(const FAssetData& InAssetData, const TSet<FName>& InClassNames)
{
	if (!InAssetData.IsValid() || InClassNames.Num() == 0) return false;

	return InClassNames.Contains(InAssetData.AssetClass) || InClassNames.Contains(GetAssetClassName(InAssetData));
}

bool FPjcLibAsset::AssetRegistryWorking()
{
	const FAssetRegistryModule& ModuleAssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(PjcConstants::ModuleAssetRegistryName);
	ModuleAssetRegistry.Get().SearchAllAssets(true);
	return ModuleAssetRegistry.Get().IsLoadingAssets();
}

bool FPjcLibAsset::IsValidClassName(const FName& InClassName)
{
	const UClass* ClassPtr = FindObject<UClass>(ANY_PACKAGE, *InClassName.ToString());
	return ClassPtr != nullptr;
}

void FPjcLibAsset::FixupRedirectorsInProject(const bool bSlowTaskEnabled)
{
	FScopedSlowTask SlowTask{
		1.0f,
		FText::FromString(TEXT("Fixing redirectors...")),
		GIsEditor && !IsRunningCommandlet() && bSlowTaskEnabled
	};
	SlowTask.MakeDialog(false, false);
	SlowTask.EnterProgressFrame(1.0f);

	FARFilter Filter;
	Filter.bRecursivePaths = true;
	Filter.PackagePaths.Emplace(PjcConstants::PathRelRoot);
	Filter.ClassNames.Emplace(UObjectRedirector::StaticClass()->GetFName());

	TArray<FAssetData> AssetList;
	const FAssetRegistryModule& ModuleAssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(PjcConstants::ModuleAssetRegistryName);
	ModuleAssetRegistry.Get().GetAssets(Filter, AssetList);

	if (AssetList.Num() == 0) return;

	FScopedSlowTask LoadingTask(
		AssetList.Num(),
		FText::FromString(TEXT("Loading redirectors...")),
		GIsEditor && !IsRunningCommandlet() && bSlowTaskEnabled
	);
	LoadingTask.MakeDialog(false, false);

	TArray<UObjectRedirector*> Redirectors;
	Redirectors.Reserve(AssetList.Num());

	for (const auto& Asset : AssetList)
	{
		LoadingTask.EnterProgressFrame(1.0f);

		UObject* AssetObj = Asset.GetAsset();
		if (!AssetObj) continue;

		UObjectRedirector* Redirector = CastChecked<UObjectRedirector>(AssetObj);
		if (!Redirector) continue;

		Redirectors.Emplace(Redirector);
	}

	Redirectors.Shrink();

	const FAssetToolsModule& ModuleAssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>(PjcConstants::ModuleAssetToolsName);
	ModuleAssetTools.Get().FixupReferencers(Redirectors, false);
}

void FPjcLibAsset::AssetRegistryUpdate()
{
	const FAssetRegistryModule& ModuleAssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(PjcConstants::ModuleAssetRegistryName);
	ModuleAssetRegistry.Get().SearchAllAssets(true);
}

void FPjcLibAsset::GetClassNamesPrimary(TSet<FName>& ClassNamesPrimary)
{
	// getting list of primary asset classes that are defined in AssetManager
	const auto& AssetManager = UAssetManager::Get();
	if (!AssetManager.IsValid()) return;

	TSet<FName> ClassNamesPrimaryBase;
	TArray<FPrimaryAssetTypeInfo> AssetTypeInfos;
	AssetManager.Get().GetPrimaryAssetTypeInfoList(AssetTypeInfos);
	ClassNamesPrimaryBase.Reserve(AssetTypeInfos.Num());

	for (const auto& AssetTypeInfo : AssetTypeInfos)
	{
		if (!AssetTypeInfo.AssetBaseClassLoaded) continue;

		ClassNamesPrimaryBase.Emplace(AssetTypeInfo.AssetBaseClassLoaded->GetFName());
	}

	// getting list of primary assets classes that are derived from main primary assets
	ClassNamesPrimary.Empty();
	const FAssetRegistryModule& ModuleAssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(PjcConstants::ModuleAssetRegistryName);
	ModuleAssetRegistry.Get().GetDerivedClassNames(ClassNamesPrimaryBase.Array(), TSet<FName>{}, ClassNamesPrimary);
}

void FPjcLibAsset::GetClassNamesEditor(TSet<FName>& ClassNamesEditor)
{
	const TArray<FName> ClassNamesEditorBase{
		UEditorUtilityWidget::StaticClass()->GetFName(),
		UEditorUtilityBlueprint::StaticClass()->GetFName(),
		UEditorUtilityWidgetBlueprint::StaticClass()->GetFName(),
		UEditorTutorial::StaticClass()->GetFName()
	};

	ClassNamesEditor.Empty();
	const FAssetRegistryModule& ModuleAssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(PjcConstants::ModuleAssetRegistryName);
	ModuleAssetRegistry.Get().GetDerivedClassNames(ClassNamesEditorBase, TSet<FName>{}, ClassNamesEditor);
}

void FPjcLibAsset::GetAssetsAll(TArray<FAssetData>& OutAssets)
{
	const FAssetRegistryModule& ModuleAssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(PjcConstants::ModuleAssetRegistryName);

	OutAssets.Empty();
	ModuleAssetRegistry.Get().GetAssetsByPath(PjcConstants::PathRelRoot, OutAssets, true);
}

void FPjcLibAsset::GetAssetsPrimary(const TArray<FAssetData>& AssetsAll, TArray<FAssetData>& OutAssets)
{
	TSet<FName> ClassNamesPrimary;
	GetClassNamesPrimary(ClassNamesPrimary);

	OutAssets.Empty();
	OutAssets.Reserve(AssetsAll.Num());

	for (const auto& Asset : AssetsAll)
	{
		if (AssetClassNameInList(Asset, ClassNamesPrimary))
		{
			OutAssets.Emplace(Asset);
		}
	}

	OutAssets.Shrink();
}

void FPjcLibAsset::GetAssetsEditor(const TArray<FAssetData>& AssetsAll, TArray<FAssetData>& OutAssets)
{
	TSet<FName> ClassNamesEditor;
	GetClassNamesEditor(ClassNamesEditor);

	OutAssets.Empty();
	OutAssets.Reserve(AssetsAll.Num());

	for (const auto& Asset : AssetsAll)
	{
		if (AssetClassNameInList(Asset, ClassNamesEditor))
		{
			OutAssets.Emplace(Asset);
		}
	}

	OutAssets.Shrink();
}

void FPjcLibAsset::GetAssetsExtReferenced(const TArray<FAssetData>& AssetsAll, TArray<FAssetData>& OutAssets)
{
	OutAssets.Empty();
	OutAssets.Reserve(AssetsAll.Num());

	for (const auto& Asset : AssetsAll)
	{
		if (AssetIsExtReferenced(Asset))
		{
			OutAssets.Emplace(Asset);
		}
	}

	OutAssets.Shrink();
}

void FPjcLibAsset::GetAssetsByPath(const FString& InPath, const bool bRecursive, TArray<FAssetData>& OutAssets)
{
	OutAssets.Empty();

	const FAssetRegistryModule& ModuleAssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(PjcConstants::ModuleAssetRegistryName);
	ModuleAssetRegistry.Get().GetAssetsByPath(FName{*InPath}, OutAssets, bRecursive, true);
}

void FPjcLibAsset::GetAssetsByPackagePaths(const TArray<FName>& InPackagePaths, const bool bRecursive, TArray<FAssetData>& OutAssets)
{
	if (InPackagePaths.Num() == 0) return;

	FARFilter Filter;
	Filter.bRecursivePaths = bRecursive;

	for (const auto& PackagePath : InPackagePaths)
	{
		const FString AssetPath = FPjcLibPath::ToAssetPath(PackagePath.ToString());
		if (AssetPath.IsEmpty()) continue;

		Filter.PackagePaths.Emplace(FName{*AssetPath});
	}

	if (Filter.PackagePaths.Num() == 0) return;

	const FAssetRegistryModule& ModuleAssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(PjcConstants::ModuleAssetRegistryName);

	OutAssets.Empty();
	ModuleAssetRegistry.Get().GetAssets(Filter, OutAssets);
}

void FPjcLibAsset::GetAssetsByObjectPaths(const TArray<FName>& InObjectPaths, TArray<FAssetData>& OutAssets)
{
	if (InObjectPaths.Num() == 0) return;

	FARFilter Filter;

	for (const auto& ObjectPath : InObjectPaths)
	{
		Filter.PackagePaths.Emplace(ObjectPath);
	}

	const FAssetRegistryModule& ModuleAssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(PjcConstants::ModuleAssetRegistryName);

	OutAssets.Empty();
	ModuleAssetRegistry.Get().GetAssets(Filter, OutAssets);
}

// void FPjcLibAsset::GetAssetsExcluded(const FPjcExcludeSettings& InExcludeSettings, TArray<FAssetData>& OutAssets)
// {
// 	const FAssetRegistryModule& ModuleAssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(PjcConstants::ModuleAssetRegistryName);
//
// 	TArray<FAssetData> AssetsAll;
// 	ModuleAssetRegistry.Get().GetAssetsByPath(PjcConstants::PathRelRoot, AssetsAll, true);
//
//
// 	TSet<FAssetData> AssetsExcluded;
//
// 	{
// 		FARFilter Filter;
// 		Filter.bRecursivePaths = true;
//
// 		Filter.PackagePaths.Reserve(InExcludeSettings.ExcludedPaths.Num());
//
// 		for (const auto& ExcludedPath : InExcludeSettings.ExcludedPaths)
// 		{
// 			const FString AssetPath = FPjcLibPath::ToAssetPath(ExcludedPath);
// 			if (AssetPath.IsEmpty()) continue;
//
// 			Filter.PackagePaths.Emplace(AssetPath);
// 		}
//
// 		TArray<FAssetData> Assets;
// 		ModuleAssetRegistry.Get().GetAssets(Filter, Assets);
//
// 		AssetsExcluded.Append(Assets);
// 	}
//
// 	{
// 		for (const auto& Asset : AssetsAll)
// 		{
// 			if (InExcludeSettings.ExcludedClassNames.Contains(Asset.AssetClass) || InExcludeSettings.ExcludedClassNames.Contains(GetAssetClassName(Asset)))
// 			{
// 				AssetsExcluded.Emplace(Asset);
// 			}
// 		}
// 	}
//
// 	{
// 		FARFilter Filter;
//
// 		for (const auto& ExcludedObjectPath : InExcludeSettings.ExcludedAssetObjectPaths)
// 		{
// 			const FName ObjectPath = FPjcLibPath::ToObjectPath(ExcludedObjectPath.ToString());
// 			if (ObjectPath == NAME_None) continue;
//
// 			Filter.ObjectPaths.Emplace(ObjectPath);
// 		}
//
// 		TArray<FAssetData> Assets;
// 		ModuleAssetRegistry.Get().GetAssets(Filter, Assets);
//
// 		AssetsExcluded.Append(Assets);
// 	}
//
// 	OutAssets.Empty();
// 	OutAssets.Append(AssetsExcluded.Array());
// }

void FPjcLibAsset::GetAssetsIndirect(TMap<FAssetData, FPjcAssetIndirectUsageInfo>& AssetsIndirect)
{
	AssetsIndirect.Empty();

	TSet<FString> SourceFiles;
	TSet<FString> ConfigFiles;

	FPjcLibPath::GetFilesInPathByExt(FPjcLibPath::SourceDir(), true, false, PjcConstants::SourceFileExtensions, SourceFiles);
	FPjcLibPath::GetFilesInPathByExt(FPjcLibPath::ConfigDir(), true, false, PjcConstants::ConfigFileExtensions, ConfigFiles);

	TSet<FString> InstalledPlugins;
	FPjcLibPath::GetFoldersInPath(FPjcLibPath::PluginsDir(), false, InstalledPlugins);

	const FString ProjectCleanerPluginPath = FPjcLibPath::PluginsDir() / PjcConstants::ModuleName.ToString();
	TSet<FString> Files;
	for (const auto& InstalledPlugin : InstalledPlugins)
	{
		// ignore ProjectCleaner plugin
		if (InstalledPlugin.Equals(ProjectCleanerPluginPath)) continue;

		FPjcLibPath::GetFilesInPathByExt(InstalledPlugin / TEXT("Source"), true, false, PjcConstants::SourceFileExtensions, Files);
		SourceFiles.Append(Files);

		Files.Reset();

		FPjcLibPath::GetFilesInPathByExt(InstalledPlugin / TEXT("Config"), true, false, PjcConstants::ConfigFileExtensions, Files);
		ConfigFiles.Append(Files);

		Files.Reset();
	}

	TSet<FString> ScanFiles;
	ScanFiles.Append(SourceFiles);
	ScanFiles.Append(ConfigFiles);

	FScopedSlowTask SlowTask{
		static_cast<float>(ScanFiles.Num()),
		FText::FromString(TEXT("Searching Indirectly used assets...")),
		GIsEditor && !IsRunningCommandlet()
	};
	SlowTask.MakeDialog();

	for (const auto& File : ScanFiles)
	{
		SlowTask.EnterProgressFrame(1.0f, FText::FromString(File));

		FString FileContent;
		FFileHelper::LoadFileToString(FileContent, *File);

		if (FileContent.IsEmpty()) continue;

		static FRegexPattern Pattern(TEXT(R"(\/Game([A-Za-z0-9_.\/]+)\b)"));
		FRegexMatcher Matcher(Pattern, FileContent);
		while (Matcher.FindNext())
		{
			FString FoundedAssetObjectPath = Matcher.GetCaptureGroup(0);
			const FName ObjectPath = FPjcLibPath::ToObjectPath(FoundedAssetObjectPath);
			const FAssetData AssetData = GetAssetByObjectPath(ObjectPath);
			if (!AssetData.IsValid()) continue;

			// if founded asset is ok, we loading file lines to determine on what line its used
			TArray<FString> Lines;
			FFileHelper::LoadFileToStringArray(Lines, *File);

			for (int32 i = 0; i < Lines.Num(); ++i)
			{
				if (!Lines.IsValidIndex(i)) continue;
				if (!Lines[i].Contains(FoundedAssetObjectPath)) continue;

				const FString FilePathAbs = FPaths::ConvertRelativePathToFull(File);
				const int32 FileLine = i + 1;


				FPjcAssetIndirectUsageInfo& UsageInfo = AssetsIndirect.FindOrAdd(AssetData);
				UsageInfo.Files.AddUnique(FPjcFileInfo{FileLine, FilePathAbs});
				
				// AssetsIndirectInfos.AddUnique(FPjcAssetIndirectInfo{FileLine, AssetData, FilePathAbs});
				// AssetsIndirect.AddUnique(AssetData);
			}
		}
	}
}

void FPjcLibAsset::GetAssetDeps(const FAssetData& InAssetData, TSet<FAssetData>& OutDeps)
{
	if (!InAssetData.IsValid()) return;

	const FAssetRegistryModule& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(PjcConstants::ModuleAssetRegistryName);

	TArray<FName> Stack;
	TArray<FName> Deps;
	TSet<FName> AllDepsPackageNames;

	Stack.Push(InAssetData.PackageName);

	while (Stack.Num() > 0)
	{
		const FName CurrentPackageName = Stack.Pop();
		Deps.Reset();
		AssetRegistry.Get().GetDependencies(CurrentPackageName, Deps);

		Deps.RemoveAllSwap([&](const FName& Dep)
		{
			return !Dep.ToString().StartsWith(PjcConstants::PathRelRoot.ToString());
		}, false);

		for (const auto& Dep : Deps)
		{
			bool bIsAlreadyInSet = false;
			AllDepsPackageNames.Add(Dep, &bIsAlreadyInSet);
			if (!bIsAlreadyInSet)
			{
				Stack.Add(Dep);
			}
		}
	}

	FARFilter Filter;
	// Filter.bRecursivePaths = true;
	// Filter.PackagePaths.Add(PjcConstants::PathRelRoot);

	for (const auto& Dep : AllDepsPackageNames)
	{
		Filter.PackageNames.Add(Dep);
	}

	TArray<FAssetData> AssetsContainer;
	AssetRegistry.Get().GetAssets(Filter, AssetsContainer);

	OutDeps.Empty();
	OutDeps.Append(AssetsContainer);
}

void FPjcLibAsset::GetAssetsDeps(const TSet<FAssetData>& Assets, TSet<FAssetData>& Dependencies)
{
	Dependencies.Empty();

	const FAssetRegistryModule& ModuleAssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(PjcConstants::ModuleAssetRegistryName);

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
				return !Dep.ToString().StartsWith(*PjcConstants::PathRelRoot.ToString());
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
	Filter.PackagePaths.Add(PjcConstants::PathRelRoot);

	for (const auto& Dep : UsedAssetsDeps)
	{
		Filter.PackageNames.Add(Dep);
	}

	TArray<FAssetData> TempContainer;
	ModuleAssetRegistry.Get().GetAssets(Filter, TempContainer);

	Dependencies.Append(TempContainer);
}

void FPjcLibAsset::GetCachedPaths(TArray<FString>& Paths)
{
	const FAssetRegistryModule& ModuleAssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(PjcConstants::ModuleAssetRegistryName);
	ModuleAssetRegistry.Get().GetAllCachedPaths(Paths);

	Paths.RemoveAllSwap([](const FString& Path)
	{
		return !Path.StartsWith(PjcConstants::PathRelRoot.ToString());
	}, false);
	Paths.Shrink();
}

// void UProjectCleanerLibAsset::GetAssetsDependencies(const TArray<FAssetData>& Assets, TArray<FAssetData>& Dependencies)
// {
// 	Dependencies.Empty();
//
// 	const FAssetRegistryModule& ModuleAssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);
//
// 	TSet<FName> UsedAssetsDeps;
// 	TArray<FName> Stack;
// 	for (const auto& Asset : Assets)
// 	{
// 		UsedAssetsDeps.Add(Asset.PackageName);
// 		Stack.Add(Asset.PackageName);
//
// 		TArray<FName> Deps;
// 		while (Stack.Num() > 0)
// 		{
// 			const auto CurrentPackageName = Stack.Pop(false);
// 			Deps.Reset();
//
// 			ModuleAssetRegistry.Get().GetDependencies(CurrentPackageName, Deps);
//
// 			Deps.RemoveAllSwap([&](const FName& Dep)
// 			{
// 				return !Dep.ToString().StartsWith(*ProjectCleanerConstants::PathRelRoot.ToString());
// 			}, false);
//
// 			for (const auto& Dep : Deps)
// 			{
// 				bool bIsAlreadyInSet = false;
// 				UsedAssetsDeps.Add(Dep, &bIsAlreadyInSet);
// 				if (!bIsAlreadyInSet)
// 				{
// 					Stack.Add(Dep);
// 				}
// 			}
// 		}
// 	}
//
// 	FARFilter Filter;
// 	Filter.bRecursivePaths = true;
// 	Filter.PackagePaths.Add(ProjectCleanerConstants::PathRelRoot);
//
// 	for (const auto& Dep : UsedAssetsDeps)
// 	{
// 		Filter.PackageNames.Add(Dep);
// 	}
//
// 	ModuleAssetRegistry.Get().GetAssets(Filter, Dependencies);
// }


FName FPjcLibAsset::GetAssetClassName(const FAssetData& InAssetData)
{
	if (!InAssetData.IsValid()) return NAME_None;

	if (AssetIsBlueprint(InAssetData))
	{
		const FString GeneratedClassName = InAssetData.TagsAndValues.FindTag(TEXT("GeneratedClass")).GetValue();
		const FString ClassObjectPath = FPackageName::ExportTextPathToObjectPath(*GeneratedClassName);
		return FName{*FPackageName::ObjectPathToObjectName(ClassObjectPath)};
	}

	return InAssetData.AssetClass;
}

FAssetData FPjcLibAsset::GetAssetByObjectPath(const FName& InObjectPath)
{
	const FName ObjectPath = FPjcLibPath::ToObjectPath(InObjectPath.ToString());
	const FAssetRegistryModule& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);

	return AssetRegistry.Get().GetAssetByObjectPath(ObjectPath);
}

int64 FPjcLibAsset::GetAssetSize(const FAssetData& InAssetData)
{
	if (!InAssetData.IsValid()) return 0;

	const FAssetRegistryModule& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);
	const FAssetPackageData* AssetPackageData = AssetRegistry.Get().GetAssetPackageData(InAssetData.PackageName);
	if (!AssetPackageData) return 0;

	return AssetPackageData->DiskSize;
}

int64 FPjcLibAsset::GetAssetsSize(const TArray<FAssetData>& InAssetDatas)
{
	int64 Size = 0;

	const FAssetRegistryModule& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);

	for (const auto& Asset : InAssetDatas)
	{
		if (!Asset.IsValid()) continue;

		const auto AssetPackageData = AssetRegistry.Get().GetAssetPackageData(Asset.PackageName);
		if (!AssetPackageData) continue;

		Size += AssetPackageData->DiskSize;
	}

	return Size;
}
