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

bool FPjcLibAsset::AssetRegistryWorking()
{
	const FAssetRegistryModule& ModuleAssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(PjcConstants::ModuleAssetRegistryName);
	return ModuleAssetRegistry.Get().IsLoadingAssets();
}

void FPjcLibAsset::FixupRedirectorsInProject(const bool bSlowTaskEnabled)
{
	FScopedSlowTask SlowTask{
		1.0f,
		FText::FromString(TEXT("Fixing redirectors...")),
		GIsEditor && !IsRunningCommandlet() && bSlowTaskEnabled
	};
	SlowTask.MakeDialog();

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
	LoadingTask.MakeDialog();

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

	SlowTask.EnterProgressFrame(1.0f);
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

void FPjcLibAsset::GetAssetsIndirect(TMap<FAssetData, TArray<FPjcAssetUsageInfo>>& AssetsIndirect)
{
	AssetsIndirect.Empty();

	TSet<FString> SourceFiles;
	TSet<FString> ConfigFiles;

	// const TSet<FString> SourceFileExtensions{TEXT(".cpp"), TEXT(".h"), TEXT(".cs")};
	// const TSet<FString> ConfigFileExtensions{TEXT(".ini")};
	//
	// const FString SourceDir = FPaths::ConvertRelativePathToFull(FPaths::GameSourceDir());
	// const FString ConfigsDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectConfigDir());
	// const FString PluginsDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectPluginsDir());
	//
	// GetFilesInPathByExt(SourceDir, true, false, SourceFileExtensions, SourceFiles);
	// GetFilesInPathByExt(ConfigsDir, true, false, ConfigFileExtensions, ConfigFiles);
	//
	// TSet<FString> InstalledPlugins;
	// GetFoldersInPath(PluginsDir, false, InstalledPlugins);
	//
	// TSet<FString> Files;
	// for (const auto& InstalledPlugin : InstalledPlugins)
	// {
	// 	GetFilesInPathByExt(InstalledPlugin / TEXT("Source"), true, false, SourceFileExtensions, Files);
	// 	SourceFiles.Append(Files);
	//
	// 	Files.Reset();
	//
	// 	GetFilesInPathByExt(InstalledPlugin / TEXT("Config"), true, false, ConfigFileExtensions, Files);
	// 	ConfigFiles.Append(Files);
	//
	// 	Files.Reset();
	// }

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
				const FPjcAssetUsageInfo UsageInfo{FileLine, FilePathAbs};

				if (const auto FileUsageInfo = AssetsIndirect.Find(AssetData))
				{
					FileUsageInfo->AddUnique(UsageInfo);
				}
				else
				{
					auto Value = AssetsIndirect.Emplace(AssetData);
					Value.Emplace(UsageInfo);
				}
			}
		}
	}
}

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
