// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Libs/PjcLibAsset.h"
#include "PjcConstants.h"
#include "Libs/PjcLibPath.h"
// Engine Headers
#include "EditorTutorial.h"
#include "EditorUtilityBlueprint.h"
#include "EditorUtilityWidget.h"
#include "EditorUtilityWidgetBlueprint.h"
#include "Engine/AssetManager.h"
#include "Internationalization/Regex.h"
#include "Misc/FileHelper.h"
#include "Misc/ScopedSlowTask.h"

FAssetRegistryModule& FPjcLibAsset::GetAssetRegistry()
{
	return FModuleManager::LoadModuleChecked<FAssetRegistryModule>(PjcConstants::ModuleAssetRegistry);
}

void FPjcLibAsset::GetAssetsInPath(const FString& InPath, const bool bRecursive, TArray<FAssetData>& OutAssets)
{
	const FString ContentPath = FPjcLibPath::ToContentPath(InPath);
	if (ContentPath.IsEmpty()) return;

	const FAssetRegistryModule& AssetRegistry = GetAssetRegistry();

	OutAssets.Reset();
	AssetRegistry.Get().GetAssetsByPath(FName{*InPath}, OutAssets, bRecursive);
}

void FPjcLibAsset::GetAssetsInPaths(const TArray<FString>& InPaths, const bool bRecursive, TArray<FAssetData>& OutAssets)
{
	const FAssetRegistryModule& AssetRegistry = GetAssetRegistry();

	FARFilter Filter;
	Filter.bRecursivePaths = bRecursive;

	Filter.PackagePaths.Reserve(InPaths.Num());

	for (const auto& Path : InPaths)
	{
		const FString PathContent = FPjcLibPath::ToContentPath(Path);
		if (PathContent.IsEmpty()) continue;

		Filter.PackagePaths.Emplace(FName{*PathContent});
	}

	if (Filter.PackagePaths.Num() == 0) return;

	OutAssets.Reset();
	AssetRegistry.Get().GetAssets(Filter, OutAssets);
}

void FPjcLibAsset::GetAssetsByObjectPaths(const TArray<FString>& InObjectPaths, TArray<FAssetData>& OutAssets)
{
	const FAssetRegistryModule& AssetRegistry = GetAssetRegistry();

	FARFilter Filter;

	Filter.ObjectPaths.Reserve(InObjectPaths.Num());

	for (const auto& Path : InObjectPaths)
	{
		const FString ObjectPath = FPjcLibPath::ToObjectPath(Path);
		if (ObjectPath.IsEmpty()) continue;

		Filter.ObjectPaths.Emplace(FName{*ObjectPath});
	}

	if (Filter.ObjectPaths.Num() == 0) return;

	OutAssets.Reset();
	AssetRegistry.Get().GetAssets(Filter, OutAssets);
}

void FPjcLibAsset::GetAssetsIndirect(TArray<FAssetData>& OutAssets)
{
	TMap<FAssetData, FPjcAssetIndirectUsageInfo> Infos;
	GetAssetsIndirect(Infos);

	Infos.GetKeys(OutAssets);
}

void FPjcLibAsset::GetAssetsIndirect(TMap<FAssetData, FPjcAssetIndirectUsageInfo>& AssetsIndirectInfos)
{
	AssetsIndirectInfos.Empty();

	const FString DirSrc = FPaths::ConvertRelativePathToFull(FPaths::GameSourceDir());
	const FString DirCfg = FPaths::ConvertRelativePathToFull(FPaths::ProjectConfigDir());
	const FString DirPlg = FPaths::ConvertRelativePathToFull(FPaths::ProjectPluginsDir());

	TSet<FString> SourceFiles;
	TSet<FString> ConfigFiles;

	FPjcLibPath::GetFilesInPathByExt(DirSrc, true, false, PjcConstants::SourceFileExtensions, SourceFiles);
	FPjcLibPath::GetFilesInPathByExt(DirCfg, true, false, PjcConstants::ConfigFileExtensions, ConfigFiles);

	TSet<FString> InstalledPlugins;
	FPjcLibPath::GetFoldersInPath(DirPlg, false, InstalledPlugins);

	const FString ProjectCleanerPluginPath = DirPlg / PjcConstants::ModulePjcName.ToString();
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

	const FAssetRegistryModule& AssetRegistry = GetAssetRegistry();

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
			const FString ObjectPath = FPjcLibPath::ToObjectPath(FoundedAssetObjectPath);
			const FAssetData AssetData = AssetRegistry.Get().GetAssetByObjectPath(FName{*ObjectPath});
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

				FPjcAssetIndirectUsageInfo& UsageInfo = AssetsIndirectInfos.FindOrAdd(AssetData);
				UsageInfo.FileInfos.AddUnique(FPjcFileInfo{FileLine, FilePathAbs});
			}
		}
	}
}

void FPjcLibAsset::GetAssetsExcludedByPaths(TSet<FAssetData>& OutAssets)
{
	const UPjcEditorSettings* PjcEditorSettings = GetDefault<UPjcEditorSettings>();
	if (!PjcEditorSettings) return;

	TArray<FString> ExcludedPaths;

	ExcludedPaths.Reserve(PjcEditorSettings->ExcludedFolders.Num());

	for (const auto& ExcludedFolder : PjcEditorSettings->ExcludedFolders)
	{
		ExcludedPaths.Emplace(ExcludedFolder.Path);
	}

	TArray<FAssetData> AssetsExcludedByPaths;
	TArray<FAssetData> AssetsExcludedByObjectPaths;

	GetAssetsInPaths(ExcludedPaths, true, AssetsExcludedByPaths);
	GetAssetsByObjectPaths(PjcEditorSettings->ExcludedAssets, AssetsExcludedByObjectPaths);

	OutAssets.Reset();
	OutAssets.Append(AssetsExcludedByPaths);
	OutAssets.Append(AssetsExcludedByObjectPaths);
}

void FPjcLibAsset::GetAssetsDeps(const TSet<FAssetData>& Assets, TSet<FAssetData>& Dependencies)
{
	Dependencies.Empty();

	const FAssetRegistryModule& AssetRegistry = GetAssetRegistry();

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

			AssetRegistry.Get().GetDependencies(CurrentPackageName, Deps);

			Deps.RemoveAllSwap([&](const FName& Dep)
			{
				return !Dep.ToString().StartsWith(*PjcConstants::PathRoot.ToString());
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
	Filter.PackagePaths.Add(PjcConstants::PathRoot);

	for (const auto& Dep : UsedAssetsDeps)
	{
		Filter.PackageNames.Add(Dep);
	}

	TArray<FAssetData> TempContainer;
	AssetRegistry.Get().GetAssets(Filter, TempContainer);

	Dependencies.Append(TempContainer);
}

void FPjcLibAsset::LoadAssetsDependencies(TSet<FAssetData>& InAssets)
{
	const FAssetRegistryModule& AssetRegistry = GetAssetRegistry();

	for (const auto& Asset : InAssets)
	{
		// todo:ashe23
		// AssetRegistry.Get().GetDependencies()
	}
}

void FPjcLibAsset::FilterAssetsByPath(const TArray<FAssetData>& InAssets, const FString& InPath, TArray<FAssetData>& OutAssets)
{
	OutAssets.Reset();
	OutAssets.Reserve(InAssets.Num());

	for (const auto& Asset : InAssets)
	{
		if (Asset.PackagePath.ToString().StartsWith(InPath))
		{
			OutAssets.Emplace(Asset);
		}
	}
}

int64 FPjcLibAsset::GetAssetsTotalSize(const TArray<FAssetData>& InAssets)
{
	int64 Size = 0;

	const FAssetRegistryModule& AssetRegistry = GetAssetRegistry();

	for (const auto& Asset : InAssets)
	{
		if (!Asset.IsValid()) continue;

		const auto AssetPackageData = AssetRegistry.Get().GetAssetPackageData(Asset.PackageName);
		if (!AssetPackageData) continue;

		Size += AssetPackageData->DiskSize;
	}

	return Size;
}

void FPjcLibAsset::GetClassNamesPrimary(TSet<FName>& OutClassNames)
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
	const FAssetRegistryModule& AssetRegistry = GetAssetRegistry();

	OutClassNames.Empty();
	AssetRegistry.Get().GetDerivedClassNames(ClassNamesPrimaryBase.Array(), TSet<FName>{}, OutClassNames);
}

void FPjcLibAsset::GetClassNamesEditor(TSet<FName>& OutClassNames)
{
	const TArray<FName> ClassNamesEditorBase{
		UEditorUtilityWidget::StaticClass()->GetFName(),
		UEditorUtilityBlueprint::StaticClass()->GetFName(),
		UEditorUtilityWidgetBlueprint::StaticClass()->GetFName(),
		UEditorTutorial::StaticClass()->GetFName()
	};

	const FAssetRegistryModule& ModuleAssetRegistry = GetAssetRegistry();

	OutClassNames.Empty();
	ModuleAssetRegistry.Get().GetDerivedClassNames(ClassNamesEditorBase, TSet<FName>{}, OutClassNames);
}

void FPjcLibAsset::GetClassNamesExcluded(TSet<FName>& OutClassNames)
{
	const UPjcEditorSettings* PjcEditorSettings = GetDefault<UPjcEditorSettings>();
	if (!PjcEditorSettings) return;

	OutClassNames.Reset();
	OutClassNames.Reserve(PjcEditorSettings->ExcludedClasses.Num());

	for (const auto& ExcludedClass : PjcEditorSettings->ExcludedClasses)
	{
		if (!ExcludedClass.LoadSynchronous()) continue;

		OutClassNames.Emplace(ExcludedClass.Get()->GetFName());
	}
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

	const FAssetRegistryModule& AssetRegistry = GetAssetRegistry();

	TArray<FName> Refs;
	AssetRegistry.Get().GetReferencers(InAssetData.PackageName, Refs);

	return Refs.ContainsByPredicate([](const FName& Ref)
	{
		return !Ref.ToString().StartsWith(PjcConstants::PathRoot.ToString());
	});
}

FName FPjcLibAsset::GetAssetExactClassName(const FAssetData& InAssetData)
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
