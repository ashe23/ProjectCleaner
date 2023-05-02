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
