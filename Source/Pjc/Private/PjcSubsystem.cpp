// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "PjcSubsystem.h"
#include "PjcConstants.h"
#include "PjcSettings.h"
#include "Pjc.h"
// Engine Headers
#include "AssetToolsModule.h"
#include "IAssetRegistry.h"
#include "IAssetTools.h"
#include "FileHelpers.h"
#include "Engine/AssetManager.h"
#include "Engine/AssetManagerTypes.h"
#include "Internationalization/Regex.h"
#include "Misc/FileHelper.h"
#include "Misc/ScopedSlowTask.h"
#include "EditorTutorial.h"
#include "EditorUtilityBlueprint.h"
#include "EditorUtilityWidget.h"
#include "EditorUtilityWidgetBlueprint.h"
#include "ObjectTools.h"
#include "ShaderCompiler.h"
#include "Engine/MapBuildDataRegistry.h"
#include "Framework/Notifications/NotificationManager.h"

void UPjcSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	ModuleAssetRegistry = &FModuleManager::LoadModuleChecked<FAssetRegistryModule>(PjcConstants::ModuleAssetRegistryName).Get();
	ModuleAssetTools = &FModuleManager::LoadModuleChecked<FAssetToolsModule>(PjcConstants::ModuleAssetToolsName);

	EngineFileExtensions.Emplace(TEXT("uasset"));
	EngineFileExtensions.Emplace(TEXT("umap"));
	EngineFileExtensions.Emplace(TEXT("collection"));
}

void UPjcSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

#if WITH_EDITOR
void UPjcSubsystem::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	SaveConfig();
}
#endif

// bool UPjcSubsystem::EditorIsInPlayMode()
// {
// 	return GEditor && GEditor->PlayWorld || GIsPlayInEditorWorld;
// }
//
// bool UPjcSubsystem::AssetRegistryIsWorking()
// {
// 	return FModuleManager::LoadModuleChecked<FAssetRegistryModule>(PjcConstants::ModuleAssetRegistryName).Get().IsLoadingAssets();
// }

bool UPjcSubsystem::AssetIsBlueprint(const FAssetData& InAssetData)
{
	if (!InAssetData.IsValid()) return false;

	const UClass* AssetClass = InAssetData.GetClass();
	if (!AssetClass) return false;

	return AssetClass->IsChildOf(UBlueprint::StaticClass());
}

// bool UPjcSubsystem::AssetIsExcluded(const FPjcExcludeSettings& ExcludeSettings, const FAssetData& InAssetData)
// {
// 	if (!InAssetData.IsValid()) return false;
//
// 	const FName AssetClassName = GetAssetClassName(InAssetData);
//
// 	if (ExcludeSettings.ExcludedClassNames.Contains(InAssetData.AssetClass) || ExcludeSettings.ExcludedClassNames.Contains(AssetClassName))
// 	{
// 		return true;
// 	}
//
// 	if (ExcludeSettings.ExcludedObjectPaths.Contains(InAssetData.ObjectPath))
// 	{
// 		return true;
// 	}
//
// 	for (const auto& ExcludedPath : ExcludeSettings.ExcludedPaths)
// 	{
// 		if (InAssetData.PackagePath.ToString().StartsWith(ExcludedPath.ToString()))
// 		{
// 			return true;
// 		}
// 	}
//
// 	return false;
// }
//
// bool UPjcSubsystem::AssetHasExtRefs(const FAssetData& InAssetData)
// {
// 	if (!InAssetData.IsValid()) return false;
//
// 	const FAssetRegistryModule& ModuleAssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(PjcConstants::ModuleAssetRegistryName);
//
// 	TArray<FName> Refs;
// 	ModuleAssetRegistry.Get().GetReferencers(InAssetData.PackageName, Refs);
//
// 	return Refs.ContainsByPredicate([](const FName& Ref)
// 	{
// 		return !Ref.ToString().StartsWith(PjcConstants::PathRelRoot.ToString());
// 	});
// }
//
bool UPjcSubsystem::PathIsEmpty(const FString& InPath)
{
	const FString PathRel = PathConvertToRel(InPath);
	const FString PathAbs = PathConvertToAbs(InPath);

	if (PathRel.IsEmpty() || PathAbs.IsEmpty()) return false;

	const FAssetRegistryModule& ModuleAssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(PjcConstants::ModuleAssetRegistryName);

	if (ModuleAssetRegistry.Get().HasAssets(FName{*PathRel}, true))
	{
		return false;
	}

	TArray<FString> Files;
	IFileManager::Get().FindFilesRecursive(Files, *PathAbs, TEXT("*"), true, false);

	return Files.Num() == 0;
}

//
// bool UPjcSubsystem::PathIsExcluded(const FString& InPath)
// {
// 	const UPjcSettings* Settings = GetDefault<UPjcSettings>();
// 	if (!Settings) return false;
// 	if (Settings->ExcludedPaths.Num() == 0) return false;
//
// 	for (const auto& ExcludedPath : Settings->ExcludedPaths)
// 	{
// 		const FString PathRel = PathConvertToRel(ExcludedPath.Path);
// 		if (PathRel.IsEmpty()) continue;
//
// 		if (PathConvertToRel(InPath).StartsWith(PathRel, ESearchCase::CaseSensitive))
// 		{
// 			return true;
// 		}
// 	}
//
// 	return false;
// }

bool UPjcSubsystem::PathIsEngineGenerated(const FString& InPath)
{
	TSet<FString> PathsEngineGenerated;
	PathsEngineGenerated.Emplace(FPaths::ConvertRelativePathToFull(FPaths::GameDevelopersDir() / FPaths::GameUserDeveloperFolderName()));
	PathsEngineGenerated.Emplace(FPaths::ConvertRelativePathToFull(FPaths::GameUserDeveloperDir() / TEXT("Collections")));
	PathsEngineGenerated.Emplace(FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() / TEXT("Developers")));
	PathsEngineGenerated.Emplace(FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() / TEXT("Collections")));

	return PathsEngineGenerated.Contains(PathNormalize(InPath));
}


FString UPjcSubsystem::PathNormalize(const FString& InPath)
{
	if (InPath.IsEmpty()) return {};

	FString Path = FPaths::ConvertRelativePathToFull(InPath).TrimStartAndEnd();
	FPaths::RemoveDuplicateSlashes(Path);

	if (FPaths::GetExtension(Path).IsEmpty())
	{
		FPaths::NormalizeDirectoryName(Path);
	}
	else
	{
		FPaths::NormalizeFilename(Path);
	}

	return Path;
}

FString UPjcSubsystem::PathConvertToAbs(const FString& InPath)
{
	const FString PathNormalized = PathNormalize(InPath);
	const FString PathContentDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("Content"));

	if (PathNormalized.StartsWith(PathContentDir)) return PathNormalized;
	if (PathNormalized.StartsWith(PjcConstants::PathRelRoot.ToString()))
	{
		FString Path = PathNormalized;
		Path.RemoveFromStart(PjcConstants::PathRelRoot.ToString());

		return PathContentDir + Path;
	}

	return {};
}

FString UPjcSubsystem::PathConvertToRel(const FString& InPath)
{
	const FString PathNormalized = PathNormalize(InPath);
	const FString PathContentDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("Content"));

	if (PathNormalized.StartsWith(PjcConstants::PathRelRoot.ToString())) return PathNormalized;
	if (PathNormalized.StartsWith(PathContentDir))
	{
		FString Path = PathNormalized;
		Path.RemoveFromStart(PathContentDir);

		return PjcConstants::PathRelRoot.ToString() + Path;
	}

	return {};
}

FName UPjcSubsystem::PathConvertToObjectPath(const FString& InPath)
{
	return FName{*FPackageName::ExportTextPathToObjectPath(InPath)};
}

FName UPjcSubsystem::GetAssetClassName(const FAssetData& InAssetData)
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

void UPjcSubsystem::GetFoldersInPath(const FString& SearchPath, const bool bSearchRecursive, TSet<FString>& OutFolders)
{
	OutFolders.Empty();

	class FFindFoldersVisitor : public IPlatformFile::FDirectoryVisitor
	{
	public:
		TSet<FString>& Folders;

		explicit FFindFoldersVisitor(TSet<FString>& InFolders) : Folders(InFolders) { }

		virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory) override
		{
			if (bIsDirectory)
			{
				Folders.Emplace(FPaths::ConvertRelativePathToFull(FilenameOrDirectory));
			}

			return true;
		}
	};

	FFindFoldersVisitor FindFoldersVisitor{OutFolders};
	if (bSearchRecursive)
	{
		FPlatformFileManager::Get().GetPlatformFile().IterateDirectoryRecursively(*SearchPath, FindFoldersVisitor);
	}
	else
	{
		FPlatformFileManager::Get().GetPlatformFile().IterateDirectory(*SearchPath, FindFoldersVisitor);
	}
}

void UPjcSubsystem::GetFilesInPath(const FString& SearchPath, const bool bSearchRecursive, TSet<FString>& OutFiles)
{
	OutFiles.Empty();

	class FFindFilesVisitor : public IPlatformFile::FDirectoryVisitor
	{
	public:
		TSet<FString>& Files;

		explicit FFindFilesVisitor(TSet<FString>& InFiles) : Files(InFiles) { }

		virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory) override
		{
			if (!bIsDirectory)
			{
				Files.Emplace(FPaths::ConvertRelativePathToFull(FilenameOrDirectory));
			}

			return true;
		}
	};

	FFindFilesVisitor FindFilesVisitor{OutFiles};

	if (bSearchRecursive)
	{
		FPlatformFileManager::Get().GetPlatformFile().IterateDirectoryRecursively(*SearchPath, FindFilesVisitor);
	}
	else
	{
		FPlatformFileManager::Get().GetPlatformFile().IterateDirectory(*SearchPath, FindFilesVisitor);
	}
}

void UPjcSubsystem::GetFilesInPathByExt(const FString& SearchPath, const bool bSearchRecursive, const bool bSearchInvert, const TSet<FString>& Extensions, TSet<FString>& OutFiles)
{
	OutFiles.Empty();

	class FFindFilesVisitor : public IPlatformFile::FDirectoryVisitor
	{
	public:
		const bool bSearchInvert;
		TSet<FString>& Files;
		const TSet<FString>& Extensions;

		explicit FFindFilesVisitor(const bool bInSearchInvert, TSet<FString>& InFiles, const TSet<FString>& InExtensions)
			: bSearchInvert(bInSearchInvert),
			  Files(InFiles),
			  Extensions(InExtensions) { }

		virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory) override
		{
			if (!bIsDirectory)
			{
				if (Extensions.Num() == 0)
				{
					Files.Emplace(FPaths::ConvertRelativePathToFull(FilenameOrDirectory));
					return true;
				}

				const FString Ext = FPaths::GetExtension(FilenameOrDirectory, false).ToLower();
				const bool bExistsInSearchList = Extensions.Contains(Ext);

				if (
					bExistsInSearchList && !bSearchInvert ||
					!bExistsInSearchList && bSearchInvert
				)
				{
					Files.Emplace(FPaths::ConvertRelativePathToFull(FilenameOrDirectory));
				}
			}

			return true;
		}
	};

	TSet<FString> ExtensionsNormalized;
	ExtensionsNormalized.Reserve(Extensions.Num());

	for (const auto& Ext : Extensions)
	{
		const FString ExtNormalized = Ext.Replace(TEXT("."), TEXT("")).ToLower();
		ExtensionsNormalized.Emplace(ExtNormalized);
	}

	FFindFilesVisitor FindFilesVisitor{bSearchInvert, OutFiles, ExtensionsNormalized};
	if (bSearchRecursive)
	{
		FPlatformFileManager::Get().GetPlatformFile().IterateDirectoryRecursively(*SearchPath, FindFilesVisitor);
	}
	else
	{
		FPlatformFileManager::Get().GetPlatformFile().IterateDirectory(*SearchPath, FindFilesVisitor);
	}
}

void UPjcSubsystem::GetClassNamesPrimary(TSet<FName>& OutClassNames)
{
	// getting list of primary asset classes that are defined in AssetManager
	TSet<FName> ClassNamesPrimary;
	const auto& AssetManager = UAssetManager::Get();
	if (!AssetManager.IsValid()) return;

	TArray<FPrimaryAssetTypeInfo> AssetTypeInfos;
	AssetManager.Get().GetPrimaryAssetTypeInfoList(AssetTypeInfos);
	ClassNamesPrimary.Reserve(AssetTypeInfos.Num());

	for (const auto& AssetTypeInfo : AssetTypeInfos)
	{
		if (!AssetTypeInfo.AssetBaseClassLoaded) continue;

		ClassNamesPrimary.Emplace(AssetTypeInfo.AssetBaseClassLoaded->GetFName());
	}

	OutClassNames.Empty();
	// getting list of primary assets classes that are derived from main primary assets
	const FAssetRegistryModule& ModuleAssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(PjcConstants::ModuleAssetRegistryName);
	ModuleAssetRegistry.Get().GetDerivedClassNames(ClassNamesPrimary.Array(), TSet<FName>{}, OutClassNames);
}

void UPjcSubsystem::GetClassNamesEditor(TSet<FName>& OutClassNames)
{
	const TArray<FName> ClassNamesEditor{
		UEditorUtilityWidget::StaticClass()->GetFName(),
		UEditorUtilityBlueprint::StaticClass()->GetFName(),
		UEditorUtilityWidgetBlueprint::StaticClass()->GetFName(),
		UEditorTutorial::StaticClass()->GetFName()
	};

	OutClassNames.Empty();
	const FAssetRegistryModule& ModuleAssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(PjcConstants::ModuleAssetRegistryName);
	ModuleAssetRegistry.Get().GetDerivedClassNames(ClassNamesEditor, TSet<FName>{}, OutClassNames);
}

void UPjcSubsystem::GetClassNamesUsed(TSet<FName>& OutClassNames)
{
	TSet<FName> ClassNamesPrimary;
	TSet<FName> ClassNamesEditor;

	GetClassNamesPrimary(ClassNamesPrimary);
	GetClassNamesEditor(ClassNamesEditor);

	OutClassNames.Append(ClassNamesPrimary);
	OutClassNames.Append(ClassNamesEditor);

	// extra classes we considering used
	OutClassNames.Emplace(UMapBuildDataRegistry::StaticClass()->GetFName());
}

void UPjcSubsystem::GetAssetDeps(const FAssetData& InAssetData, TArray<FAssetData>& OutAssets)
{
	OutAssets.Empty();

	if (!InAssetData.IsValid()) return;

	const FAssetRegistryModule& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(PjcConstants::ModuleAssetRegistryName);

	TArray<FName> Deps;
	AssetRegistry.Get().GetDependencies(InAssetData.PackageName, Deps);

	Deps.RemoveAllSwap([&](const FName& Dep)
	{
		return !Dep.ToString().StartsWith(*PjcConstants::PathRelRoot.ToString());
	}, false);

	if (Deps.Num() == 0) return;

	FARFilter Filter;

	for (const auto& Dep : Deps)
	{
		Filter.PackageNames.Add(Dep);
	}

	TArray<FAssetData> Assets;
	AssetRegistry.Get().GetAssets(Filter, Assets);
}

void UPjcSubsystem::GetAssetsByPackagePaths(const TArray<FName>& InPackagePaths, const bool bRecursive, TArray<FAssetData>& OutAssets)
{
	OutAssets.Empty();

	if (InPackagePaths.Num() == 0) return;

	FARFilter Filter;
	Filter.bRecursivePaths = bRecursive;

	for (const auto& PackagePath : InPackagePaths)
	{
		Filter.PackagePaths.Emplace(PackagePath);
	}

	const FAssetRegistryModule& ModuleAssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(PjcConstants::ModuleAssetRegistryName);
	ModuleAssetRegistry.Get().GetAssets(Filter, OutAssets);
}

void UPjcSubsystem::GetAssetsByClassNames(const TArray<FName>& InClassNames, TArray<FAssetData>& OutAssets)
{
	OutAssets.Empty();

	if (InClassNames.Num() == 0) return;

	const FAssetRegistryModule& ModuleAssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(PjcConstants::ModuleAssetRegistryName);

	TArray<FAssetData> AssetsAll;
	ModuleAssetRegistry.Get().GetAssetsByPath(PjcConstants::PathRelRoot, AssetsAll, true);

	const TSet<FName> SearchClassNames{InClassNames};

	OutAssets.Reset(AssetsAll.Num());

	for (const auto& Asset : AssetsAll)
	{
		const FName AssetClassName = GetAssetClassName(Asset);
		if (SearchClassNames.Contains(Asset.AssetClass) || SearchClassNames.Contains(AssetClassName))
		{
			OutAssets.Emplace(Asset);
		}
	}

	OutAssets.Shrink();
}

void UPjcSubsystem::GetAssetsByObjectPaths(const TArray<FName>& InObjectPaths, TArray<FAssetData>& OutAssets)
{
	OutAssets.Empty();

	if (InObjectPaths.Num() == 0) return;

	FARFilter Filter;

	for (const auto& ObjectPath : InObjectPaths)
	{
		Filter.ObjectPaths.Emplace(PathConvertToObjectPath(ObjectPath.ToString()));
	}

	const FAssetRegistryModule& ModuleAssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(PjcConstants::ModuleAssetRegistryName);
	ModuleAssetRegistry.Get().GetAssets(Filter, OutAssets);
}

// void UPjcSubsystem::GetAssetsIndirect(TArray<FPjcAssetIndirectUsageInfo>& AssetsIndirect)
// {
// 	AssetsIndirect.Empty();
//
// 	TSet<FString> SourceFiles;
// 	TSet<FString> ConfigFiles;
//
// 	const TSet<FString> SourceFileExtensions{TEXT(".cpp"), TEXT(".h"), TEXT(".cs")};
// 	const TSet<FString> ConfigFileExtensions{TEXT(".ini")};
//
// 	const FString SourceDir = FPaths::ConvertRelativePathToFull(FPaths::GameSourceDir());
// 	const FString ConfigsDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectConfigDir());
// 	const FString PluginsDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectPluginsDir());
//
// 	GetFilesInPathByExt(SourceDir, true, false, SourceFileExtensions, SourceFiles);
// 	GetFilesInPathByExt(ConfigsDir, true, false, ConfigFileExtensions, ConfigFiles);
//
// 	TSet<FString> InstalledPlugins;
// 	GetFoldersInPath(PluginsDir, false, InstalledPlugins);
//
// 	TSet<FString> Files;
// 	for (const auto& InstalledPlugin : InstalledPlugins)
// 	{
// 		GetFilesInPathByExt(InstalledPlugin / TEXT("Source"), true, false, SourceFileExtensions, Files);
// 		SourceFiles.Append(Files);
//
// 		Files.Reset();
//
// 		GetFilesInPathByExt(InstalledPlugin / TEXT("Config"), true, false, ConfigFileExtensions, Files);
// 		ConfigFiles.Append(Files);
//
// 		Files.Reset();
// 	}
//
// 	TSet<FString> FilesAll;
// 	FilesAll.Append(SourceFiles);
// 	FilesAll.Append(ConfigFiles);
//
// 	FScopedSlowTask SlowTask{
// 		static_cast<float>(FilesAll.Num()),
// 		FText::FromString(TEXT("Searching Indirectly used assets...")),
// 		GIsEditor && !IsRunningCommandlet()
// 	};
// 	SlowTask.MakeDialog();
//
// 	for (const auto& File : FilesAll)
// 	{
// 		SlowTask.EnterProgressFrame(1.0f);
//
// 		FString FileContent;
// 		FFileHelper::LoadFileToString(FileContent, *File);
//
// 		if (FileContent.IsEmpty()) continue;
//
// 		static FRegexPattern Pattern(TEXT(R"(\/Game([A-Za-z0-9_.\/]+)\b)"));
// 		FRegexMatcher Matcher(Pattern, FileContent);
// 		while (Matcher.FindNext())
// 		{
// 			FString FoundedAssetObjectPath = Matcher.GetCaptureGroup(0);
// 			const FName ObjectPath = PathConvertToObjectPath(FoundedAssetObjectPath);
// 			const FAssetData AssetData = GetAssetByObjectPath(ObjectPath);
// 			if (!AssetData.IsValid()) continue;
//
// 			// if founded asset is ok, we loading file lines to determine on what line its used
// 			TArray<FString> Lines;
// 			FFileHelper::LoadFileToStringArray(Lines, *File);
//
// 			for (int32 i = 0; i < Lines.Num(); ++i)
// 			{
// 				if (!Lines.IsValidIndex(i)) continue;
// 				if (!Lines[i].Contains(FoundedAssetObjectPath)) continue;
//
// 				FPjcAssetIndirectUsageInfo Info;
// 				Info.AssetData = AssetData;
// 				// file_path => file_line
// 				Info.UsageInfo.FindOrAdd(FPaths::ConvertRelativePathToFull(File), i + 1);
//
// 				AssetsIndirect.AddUnique(Info);
// 			}
// 		}
// 	}
// }

void UPjcSubsystem::FilterAssetsByClassNames(const TSet<FName>& InClassNames, const TArray<FAssetData>& InAssets, const bool bInvertedFilter, TArray<FAssetData>& OutAssets)
{
	OutAssets.Reset(InAssets.Num());

	for (const auto& Asset : InAssets)
	{
		const FName AssetClassName = GetAssetClassName(Asset);
		const bool bIsInSet = InClassNames.Contains(AssetClassName);

		if (bIsInSet && !bInvertedFilter || !bIsInSet && bInvertedFilter)
		{
			OutAssets.Emplace(Asset);
		}
	}

	OutAssets.Shrink();
}

void UPjcSubsystem::FilterAssetsByPackagePaths(const TArray<FName>& InPackagePaths, const TArray<FAssetData>& InAssets, const bool bInvertedFilter, TArray<FAssetData>& OutAssets)
{
	OutAssets.Reset(InAssets.Num());

	for (const auto& Asset : InAssets)
	{
		const bool bIsInSet = InPackagePaths.ContainsByPredicate([&Asset](const FName& Path)
		{
			return Path.ToString().StartsWith(Asset.PackagePath.ToString(), ESearchCase::CaseSensitive);
		});

		if (bIsInSet && !bInvertedFilter || !bIsInSet && bInvertedFilter)
		{
			OutAssets.Emplace(Asset);
		}
	}

	OutAssets.Shrink();
}


int64 UPjcSubsystem::GetFileSize(const FString& InFilePath)
{
	if (!FPaths::FileExists(InFilePath)) return 0;

	return FPlatformFileManager::Get().GetPlatformFile().FileSize(*InFilePath);
}

int64 UPjcSubsystem::GetFilesSize(const TArray<FString>& InFilePaths)
{
	int64 Size = 0;

	for (const auto& FilePath : InFilePaths)
	{
		if (!FPaths::FileExists(FilePath)) continue;

		Size += FPlatformFileManager::Get().GetPlatformFile().FileSize(*FilePath);
	}

	return Size;
}

int64 UPjcSubsystem::GetAssetSize(const FAssetData& InAssetData)
{
	if (!InAssetData.IsValid()) return 0;

	const FAssetRegistryModule& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);
	const FAssetPackageData* AssetPackageData = AssetRegistry.Get().GetAssetPackageData(InAssetData.PackageName);
	if (!AssetPackageData) return 0;

	return AssetPackageData->DiskSize;
}

int64 UPjcSubsystem::GetAssetsSize(const TArray<FAssetData>& InAssetsDatas)
{
	int64 Size = 0;

	const FAssetRegistryModule& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);

	for (const auto& Asset : InAssetsDatas)
	{
		if (!Asset.IsValid()) continue;

		const auto AssetPackageData = AssetRegistry.Get().GetAssetPackageData(Asset.PackageName);
		if (!AssetPackageData) continue;

		Size += AssetPackageData->DiskSize;
	}

	return Size;
}

int32 UPjcSubsystem::DeleteFolders(const TArray<FString>& InFolderPaths, const bool bRecursive)
{
	int32 FoldersDeletedNum = 0;

	TSet<FString> SubFolders;

	for (const auto& Folder : InFolderPaths)
	{
		SubFolders.Reset();

		if (bRecursive)
		{
			GetFoldersInPath(Folder, false, SubFolders);
		}

		if (!IFileManager::Get().DeleteDirectory(*Folder, true, bRecursive)) continue;

		++FoldersDeletedNum;

		if (bRecursive)
		{
			FoldersDeletedNum += SubFolders.Num();
		}
	}

	return FoldersDeletedNum;
}

int32 UPjcSubsystem::DeleteFiles(const TArray<FString>& InFilePaths)
{
	int32 FilesDeletedNum = 0;

	for (const auto& File : InFilePaths)
	{
		if (!IFileManager::Get().Delete(*File, true)) continue;

		++FilesDeletedNum;
	}

	return FilesDeletedNum;
}

FAssetData UPjcSubsystem::GetAssetByObjectPath(const FName& InObjectPath)
{
	const FName ObjectPath = PathConvertToObjectPath(InObjectPath.ToString());
	const FAssetRegistryModule& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);

	return AssetRegistry.Get().GetAssetByObjectPath(ObjectPath);
}

// void UPjcSubsystem::ProjectScanByExcludeSettings(const FPjcExcludeSettings& ExcludeSettings, FPjcScanData& ScanData)
// {
// 	ScanData.Empty();
//
// 	const FAssetRegistryModule& ModuleAssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(PjcConstants::ModuleAssetRegistryName);
//
// 	if (ModuleAssetRegistry.Get().IsLoadingAssets())
// 	{
// 		ScanData.ResultMsg = TEXT("Failed to scan project, because AssetRegistry still working");
// 		return;
// 	}
//
// 	if (GEditor && GEditor->PlayWorld || GIsPlayInEditorWorld)
// 	{
// 		ScanData.ResultMsg = TEXT("Failed to scan project, because editor is in play mode");
// 		return;
// 	}
//
// 	if (!IsRunningCommandlet())
// 	{
// 		if (!GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->CloseAllAssetEditors())
// 		{
// 			ScanData.ResultMsg = TEXT("Failed to scan project, because not all asset editors are closed");
// 			return;
// 		}
//
// 		FixupRedirectors();
// 	}
//
// 	if (ProjectContainsRedirectors())
// 	{
// 		ScanData.ResultMsg = TEXT("Failed to scan project, because not all redirectors are fixed.");
// 		return;
// 	}
//
// 	if (!FEditorFileUtils::SaveDirtyPackages(true, true, true, false, false, false))
// 	{
// 		ScanData.ResultMsg = TEXT("Failed to scan project, because not all assets are saved");
// 		return;
// 	}
//
// 	const double ScanStartTime = FPlatformTime::Seconds();
//
// 	FScopedSlowTask ScanSlowTask{
// 		4.0f,
// 		FText::FromString(TEXT("Scanning Project...")),
// 		GIsEditor && !IsRunningCommandlet()
// 	};
// 	ScanSlowTask.MakeDialog();
//
// 	ScanSlowTask.EnterProgressFrame(1.0f);
// 	TArray<FPjcAssetIndirectUsageInfo> AssetsIndirect;
// 	GetAssetsIndirect(AssetsIndirect);
//
// 	struct FContentFolderVisitor : IPlatformFile::FDirectoryVisitor
// 	{
// 	private:
// 		TSet<FString> EngineFileExtensions;
// 		TSet<FName> ClassNamesUsed;
// 		const bool bMegascansPluginActive;
// 		const FPjcExcludeSettings& ExcludeSettings;
//
// 	public:
// 		TSet<FString> FilesNonEngine;
// 		TSet<FString> FilesCorrupted;
// 		TSet<FString> FoldersEmpty;
// 		TSet<FAssetData> AssetsAll;
// 		TSet<FAssetData> AssetsExcluded;
// 		TSet<FAssetData> AssetsUsed;
//
// 		explicit FContentFolderVisitor(const FPjcExcludeSettings& InExcludeSettings) :
// 			bMegascansPluginActive(FModuleManager::Get().IsModuleLoaded("MegascansPlugin")),
// 			ExcludeSettings(InExcludeSettings)
// 		{
// 			EngineFileExtensions.Emplace(TEXT("umap"));
// 			EngineFileExtensions.Emplace(TEXT("uasset"));
// 			EngineFileExtensions.Emplace(TEXT("collection"));
//
// 			GetClassNamesUsed(ClassNamesUsed);
// 		}
//
// 		virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory) override
// 		{
// 			const FString PathAbs = PathConvertToAbs(FilenameOrDirectory);
// 			const FString PathRel = PathConvertToRel(FilenameOrDirectory);
//
// 			if (bIsDirectory)
// 			{
// 				if (PathIsEmpty(PathRel) && !PathIsEngineGenerated(PathAbs) && !PathIsExcluded(PathRel))
// 				{
// 					FoldersEmpty.Emplace(PathAbs);
// 				}
//
// 				return true;
// 			}
//
// 			const FString FileExt = FPaths::GetExtension(PathAbs, false).ToLower();
//
// 			if (EngineFileExtensions.Contains(FileExt))
// 			{
// 				const FString FilePath = PathConvertToRel(FPaths::GetPath(PathAbs));
// 				const FString FileName = FPaths::GetBaseFilename(PathAbs);
// 				const FString ObjectPath = FString::Printf(TEXT("%s/%s.%s"), *FilePath, *FileName, *FileName);
// 				const FAssetData AssetData = GetAssetByObjectPath(FName{*ObjectPath});
//
// 				if (AssetData.IsValid())
// 				{
// 					AssetsAll.Emplace(AssetData);
//
// 					const bool bAssetIsExcluded = AssetIsExcluded(ExcludeSettings, AssetData);
//
// 					if (
// 						ClassNamesUsed.Contains(AssetData.AssetClass) ||
// 						ClassNamesUsed.Contains(GetAssetClassName(AssetData)) ||
// 						(bMegascansPluginActive && AssetData.PackagePath.ToString().StartsWith(PjcConstants::PathRelMSPresets.ToString())) ||
// 						AssetHasExtRefs(AssetData) ||
// 						bAssetIsExcluded
// 					)
// 					{
// 						AssetsUsed.Emplace(AssetData);
//
// 						if (bAssetIsExcluded)
// 						{
// 							AssetsExcluded.Emplace(AssetData);
// 						}
// 					}
// 				}
// 				else
// 				{
// 					FilesCorrupted.Emplace(PathAbs);
// 				}
// 			}
// 			else
// 			{
// 				FilesNonEngine.Emplace(PathAbs);
// 			}
//
// 			return true;
// 		}
// 	};
//
// 	ScanSlowTask.EnterProgressFrame(1.0f);
// 	FContentFolderVisitor Visitor{ExcludeSettings};
// 	FPlatformFileManager::Get().GetPlatformFile().IterateDirectoryRecursively(*FPaths::ProjectContentDir(), Visitor);
//
// 	for (const auto& Info : AssetsIndirect)
// 	{
// 		Visitor.AssetsUsed.Emplace(Info.AssetData);
// 	}
//
// 	ScanSlowTask.EnterProgressFrame(1.0f);
// 	TSet<FAssetData> AssetsUsedDeps;
// 	GetAssetsDeps(Visitor.AssetsUsed, AssetsUsedDeps);
//
// 	ScanData.AssetsAll.Reset(Visitor.AssetsAll.Num());
// 	ScanData.AssetsExcluded.Reset(Visitor.AssetsExcluded.Num());
// 	ScanData.FilesNonEngine.Reset(Visitor.FilesNonEngine.Num());
// 	ScanData.FilesCorrupted.Reset(Visitor.FilesCorrupted.Num());
// 	ScanData.FoldersEmpty.Reset(Visitor.FoldersEmpty.Num());
//
// 	ScanData.AssetsAll = Visitor.AssetsAll.Array();
// 	ScanData.AssetsExcluded = Visitor.AssetsExcluded.Array();
// 	ScanData.FilesNonEngine = Visitor.FilesNonEngine.Array();
// 	ScanData.FilesCorrupted = Visitor.FilesCorrupted.Array();
// 	ScanData.FoldersEmpty = Visitor.FoldersEmpty.Array();
//
// 	ScanData.AssetsUsed.Reset(ScanData.AssetsAll.Num());
// 	ScanData.AssetsUnused.Reset(ScanData.AssetsAll.Num());
//
// 	ScanSlowTask.EnterProgressFrame(1.0f);
//
// 	FScopedSlowTask SlowTask{
// 		static_cast<float>(ScanData.AssetsAll.Num()),
// 		FText::FromString(TEXT("Searching for unused assets...")),
// 		GIsEditor && !IsRunningCommandlet()
// 	};
// 	SlowTask.MakeDialog();
//
// 	for (const auto& Asset : ScanData.AssetsAll)
// 	{
// 		SlowTask.EnterProgressFrame(1.0f, FText::FromName(Asset.ObjectPath));
//
// 		if (AssetsUsedDeps.Contains(Asset) || Visitor.AssetsUsed.Contains(Asset))
// 		{
// 			ScanData.AssetsUsed.Emplace(Asset);
// 		}
// 		else
// 		{
// 			ScanData.AssetsUnused.Emplace(Asset);
// 		}
// 	}
//
// 	ScanData.AssetsUsed.Shrink();
// 	ScanData.AssetsUnused.Shrink();
//
// 	ScanData.SizeAssetsAll = GetAssetsSize(ScanData.AssetsAll);
// 	ScanData.SizeAssetsUsed = GetAssetsSize(ScanData.AssetsUsed);
// 	ScanData.SizeAssetsUnused = GetAssetsSize(ScanData.AssetsUnused);
// 	ScanData.SizeAssetsExcluded = GetAssetsSize(ScanData.AssetsExcluded);
// 	ScanData.SizeFilesNonEngine = GetFilesSize(ScanData.FilesNonEngine);
// 	ScanData.SizeFilesCorrupted = GetFilesSize(ScanData.FilesCorrupted);
//
// 	const double ScanTime = FPlatformTime::Seconds() - ScanStartTime;
//
// 	UE_LOG(LogProjectCleaner, Display, TEXT("Project Scanned in %.2f seconds"), ScanTime);
// }

// void UPjcSubsystem::ProjectScan(FPjcScanData& ScanData)
// {
// 	const UPjcSettings* Settings = GetDefault<UPjcSettings>();
// 	if (!Settings) return;
//
// 	FPjcExcludeSettings ExcludeSettings;
//
// 	ExcludeSettings.ExcludedPaths.Reserve(Settings->ExcludedPaths.Num());
// 	ExcludeSettings.ExcludedClassNames.Reserve(Settings->ExcludedClasses.Num());
// 	ExcludeSettings.ExcludedObjectPaths.Reserve(Settings->ExcludedAssets.Num());
//
// 	for (const FDirectoryPath& ExcludedPath : Settings->ExcludedPaths)
// 	{
// 		const FString PathRel = PathConvertToRel(ExcludedPath.Path);
// 		if (PathRel.IsEmpty()) continue;
//
// 		ExcludeSettings.ExcludedPaths.Emplace(FName{*PathRel});
// 	}
//
// 	for (const auto& ExcludedClass : Settings->ExcludedClasses)
// 	{
// 		if (!ExcludedClass.LoadSynchronous()) continue;
//
// 		ExcludeSettings.ExcludedClassNames.Emplace(ExcludedClass.Get()->GetFName());
// 	}
//
// 	for (const auto& ExcludedObjectPath : Settings->ExcludedAssets)
// 	{
// 		if (!ExcludedObjectPath.LoadSynchronous()) continue;
//
// 		ExcludeSettings.ExcludedObjectPaths.Emplace(ExcludedObjectPath.ToSoftObjectPath().GetAssetPathName());
// 	}
//
// 	ProjectScanByExcludeSettings(ExcludeSettings, ScanData);
// }

void UPjcSubsystem::ProjectScan()
{
	const UPjcSettings* Settings = GetDefault<UPjcSettings>();
	if (!Settings) return;

	FString ErrMsg;
	if (!CanScanProject(ErrMsg))
	{
		if (DelegateOnProjectScan.IsBound())
		{
			DelegateOnProjectScan.Broadcast(EPjcScanResult::Fail, ErrMsg);
		}

		return;
	}

	AssetsAll.Empty();
	AssetsUsed.Empty();
	AssetsExcluded.Empty();
	AssetsPrimary.Empty();
	AssetsEditor.Empty();
	AssetsExtReferenced.Empty();
	AssetsUnused.Empty();
	AssetsIndirect.Empty();
	FilesNonEngine.Empty();
	FilesCorrupted.Empty();
	FoldersEmpty.Empty();

	ScannerState = EPjcScannerState::Scanning;
	const double ScanStartTime = FPlatformTime::Seconds();

	FScopedSlowTask SlowTask{
		4.0f,
		FText::FromString(TEXT("Scanning Project...")),
		GIsEditor && !IsRunningCommandlet()
	};
	SlowTask.MakeDialog(false, false);

	SlowTask.EnterProgressFrame(1.0f);
	FindAssetsIndirect();

	SlowTask.EnterProgressFrame(1.0f);
	ModuleAssetRegistry->GetAssetsByPath(PjcConstants::PathRelRoot, AssetsAll, true);

	struct FContentFolderVisitor : IPlatformFile::FDirectoryVisitor
	{
	private:
		UPjcSubsystem* SubsystemPtr = nullptr;
		const UPjcSettings* SettingsPtr = nullptr;
		const bool bMegascansPluginActive = false;
		TSet<FName> ClassNamesPrimary;
		TSet<FName> ClassNamesEditor;
		TSet<FName> ClassNamesUsedExtra;

	public:
		explicit FContentFolderVisitor(UPjcSubsystem* InSubsystem) :
			SubsystemPtr(InSubsystem),
			SettingsPtr(GetDefault<UPjcSettings>()),
			bMegascansPluginActive(FModuleManager::Get().IsModuleLoaded("MegascansPlugin"))
		{
			GetClassNamesPrimary(ClassNamesPrimary);
			GetClassNamesEditor(ClassNamesEditor);

			ClassNamesUsedExtra.Emplace(UMapBuildDataRegistry::StaticClass()->GetFName());
		}

		virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory) override
		{
			const FString PathAbs = PathConvertToAbs(FilenameOrDirectory);
			const FString PathRel = PathConvertToRel(FilenameOrDirectory);

			if (PathAbs.IsEmpty() || PathRel.IsEmpty()) return true;

			if (bIsDirectory)
			{
				if (PathIsEmpty(PathAbs) && !PathIsEngineGenerated(PathAbs))
				{
					SubsystemPtr->FoldersEmpty.Emplace(PathAbs);
				}

				return true;
			}

			const FString FileExt = FPaths::GetExtension(PathAbs, false).ToLower();

			if (SubsystemPtr->EngineFileExtensions.Contains(FileExt))
			{
				const FString FilePath = PathConvertToRel(FPaths::GetPath(PathAbs));
				const FString FileName = FPaths::GetBaseFilename(PathAbs);
				const FString ObjectPath = FString::Printf(TEXT("%s/%s.%s"), *FilePath, *FileName, *FileName);
				const FAssetData AssetData = GetAssetByObjectPath(FName{*ObjectPath});

				if (AssetData.IsValid())
				{
					const FName AssetClassName = GetAssetClassName(AssetData);

					const bool bIsPrimary = AssetIsPrimary(AssetData, AssetClassName);
					const bool bIsEditor = AssetIsEditor(AssetData, AssetClassName);
					const bool bIsExcluded = AssetIsExcluded(AssetData);
					const bool bIsMegascansBaseAsset = AssetIsMegascansBase(AssetData);
					const bool bIsExtReferenced = AssetIsExtReferenced(AssetData);
					const bool bIsIndirect = SubsystemPtr->AssetsIndirect.Contains(AssetData);
					const bool bIsUsed = bIsPrimary || bIsEditor || bIsExcluded || bIsMegascansBaseAsset || bIsExtReferenced || bIsIndirect || ClassNamesUsedExtra.Contains(AssetData.AssetClass) ||
						ClassNamesUsedExtra.Contains(AssetClassName);

					if (bIsPrimary)
					{
						SubsystemPtr->AssetsPrimary.Emplace(AssetData);
					}

					if (bIsEditor)
					{
						SubsystemPtr->AssetsEditor.Emplace(AssetData);
					}

					if (bIsExcluded)
					{
						SubsystemPtr->AssetsExcluded.Emplace(AssetData);
					}

					if (bIsExtReferenced)
					{
						SubsystemPtr->AssetsExtReferenced.Emplace(AssetData);
					}

					if (bIsUsed)
					{
						SubsystemPtr->AssetsUsed.Emplace(AssetData);
					}
				}
				else
				{
					SubsystemPtr->FilesCorrupted.Emplace(PathAbs);
				}
			}
			else
			{
				SubsystemPtr->FilesNonEngine.Emplace(PathAbs);
			}

			return true;
		}

		bool AssetIsPrimary(const FAssetData& InAssetData, const FName& AssetClassName) const
		{
			return ClassNamesPrimary.Contains(InAssetData.AssetClass) || ClassNamesPrimary.Contains(AssetClassName);
		}

		bool AssetIsEditor(const FAssetData& InAssetData, const FName& AssetClassName) const
		{
			return ClassNamesEditor.Contains(InAssetData.AssetClass) || ClassNamesEditor.Contains(AssetClassName);
		}

		bool AssetIsExtReferenced(const FAssetData& InAssetData) const
		{
			TArray<FName> Refs;
			SubsystemPtr->ModuleAssetRegistry->GetReferencers(InAssetData.PackageName, Refs);

			return Refs.ContainsByPredicate([](const FName& Ref)
			{
				return !Ref.ToString().StartsWith(PjcConstants::PathRelRoot.ToString());
			});
		}

		bool AssetIsMegascansBase(const FAssetData& InAssetData) const
		{
			return bMegascansPluginActive && InAssetData.PackagePath.ToString().StartsWith(PjcConstants::PathRelMSPresets.ToString());
		}

		bool AssetIsExcluded(const FAssetData& InAssetData) const
		{
			if (!SettingsPtr) return false;

			for (const auto& ExcludedPath : SettingsPtr->ExcludedPaths)
			{
				const FString PathRel = PathConvertToRel(ExcludedPath.Path);
				if (PathRel.IsEmpty()) continue;

				if (InAssetData.PackagePath.ToString().StartsWith(PathRel, ESearchCase::CaseSensitive))
				{
					return true;
				}
			}

			for (const auto& ExcludedClass : SettingsPtr->ExcludedClasses)
			{
				if (!ExcludedClass.LoadSynchronous()) continue;

				const FName ExcludedClassName = ExcludedClass.Get()->GetFName();

				if (InAssetData.AssetClass.IsEqual(ExcludedClassName) || GetAssetClassName(InAssetData).IsEqual(ExcludedClassName))
				{
					return true;
				}
			}

			for (const auto& ExcludedAsset : SettingsPtr->ExcludedAssets)
			{
				if (!ExcludedAsset.LoadSynchronous()) continue;

				if (InAssetData.ObjectPath.IsEqual(ExcludedAsset.ToSoftObjectPath().GetAssetPathName()))
				{
					return true;
				}
			}

			return false;
		}
	};

	SlowTask.EnterProgressFrame(1.0f);
	FContentFolderVisitor Visitor{this};
	FPlatformFileManager::Get().GetPlatformFile().IterateDirectoryRecursively(*FPaths::ProjectContentDir(), Visitor);

	SlowTask.EnterProgressFrame(1.0f);
	TSet<FAssetData> AssetsUsedDeps;
	GetAssetsDeps(AssetsUsed, AssetsUsedDeps);

	AssetsUsed.Append(AssetsUsedDeps);

	for (const auto& Asset : AssetsAll)
	{
		if (AssetsUsed.Contains(Asset)) continue;

		AssetsUnused.Add(Asset);
	}

	ScannerState = EPjcScannerState::Idle;
	const double ScanTime = FPlatformTime::Seconds() - ScanStartTime;

	UE_LOG(LogProjectCleaner, Display, TEXT("Project Scanned in %.2f seconds"), ScanTime);

	if (DelegateOnProjectScan.IsBound())
	{
		DelegateOnProjectScan.Broadcast(EPjcScanResult::Success, TEXT(""));
	}
}

void UPjcSubsystem::ProjectClean() const
{
	GShaderCompilingManager->SkipShaderCompilation(true);
	ObjectTools::DeleteAssets(AssetsUnused.Array());
	GShaderCompilingManager->SkipShaderCompilation(false);
}

void UPjcSubsystem::Test(const FName& Path)
{
	const FAssetData AssetData = ModuleAssetRegistry->GetAssetByObjectPath(Path);
	return;
}

FPjcDelegateOnProjectScan& UPjcSubsystem::OnProjectScan()
{
	return DelegateOnProjectScan;
}

bool UPjcSubsystem::CanScanProject(FString& ErrMsg) const
{
	ErrMsg.Empty();

	if (ScannerState == EPjcScannerState::Scanning)
	{
		ErrMsg = TEXT("Scanning is in progress. Please wait until it has finished and then try again.");
		return false;
	}

	if (ScannerState == EPjcScannerState::Cleaning)
	{
		ErrMsg = TEXT("Cleaning is in progress. Please wait until it has finished and then try again.");
		return false;
	}

	if (ModuleAssetRegistry->IsLoadingAssets())
	{
		ErrMsg = TEXT("Scanning of the project has failed. AssetRegistry is still discovering assets. Please try again after it has finished.");
		return false;
	}

	if (GEditor && GEditor->PlayWorld || GIsPlayInEditorWorld)
	{
		ErrMsg = TEXT("Scanning of the project has failed. AssetRegistry is still discovering assets. Please try again after it has finished.");
		return false;
	}

	if (!IsRunningCommandlet())
	{
		if (!GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->CloseAllAssetEditors())
		{
			ErrMsg = TEXT("Scanning of the project has failed because not all asset editors are closed.");
			return false;
		}

		FixupRedirectors();
	}

	if (ProjectContainsRedirectors())
	{
		ErrMsg = TEXT("Failed to scan project, because not all redirectors are fixed.");
		return false;
	}

	if (!FEditorFileUtils::SaveDirtyPackages(true, true, true, false, false, false))
	{
		ErrMsg = TEXT("Scanning of the project has failed because not all assets have been saved.");
		return false;
	}

	return true;
}

bool UPjcSubsystem::ProjectContainsRedirectors() const
{
	FARFilter Filter;
	Filter.bRecursivePaths = true;
	Filter.PackagePaths.Emplace(PjcConstants::PathRelRoot);
	Filter.ClassNames.Emplace(UObjectRedirector::StaticClass()->GetFName());

	TArray<FAssetData> Redirectors;
	ModuleAssetRegistry->GetAssets(Filter, Redirectors);

	return Redirectors.Num() > 0;
}

void UPjcSubsystem::FixupRedirectors() const
{
	FScopedSlowTask SlowTask{
		1.0f,
		FText::FromString(TEXT("Fixing redirectors...")),
		GIsEditor && !IsRunningCommandlet()
	};
	SlowTask.MakeDialog();

	FARFilter Filter;
	Filter.bRecursivePaths = true;
	Filter.PackagePaths.Emplace(PjcConstants::PathRelRoot);
	Filter.ClassNames.Emplace(UObjectRedirector::StaticClass()->GetFName());

	TArray<FAssetData> AssetList;
	ModuleAssetRegistry->GetAssets(Filter, AssetList);

	if (AssetList.Num() == 0) return;

	FScopedSlowTask LoadingTask(
		AssetList.Num(),
		FText::FromString(TEXT("Loading redirectors...")),
		GIsEditor && !IsRunningCommandlet()
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

	ModuleAssetTools->Get().FixupReferencers(Redirectors, false);

	SlowTask.EnterProgressFrame(1.0f);
}

void UPjcSubsystem::FindAssetsIndirect()
{
	AssetsIndirect.Empty();

	TSet<FString> SourceFiles;
	TSet<FString> ConfigFiles;

	const TSet<FString> SourceFileExtensions{TEXT(".cpp"), TEXT(".h"), TEXT(".cs")};
	const TSet<FString> ConfigFileExtensions{TEXT(".ini")};

	const FString SourceDir = FPaths::ConvertRelativePathToFull(FPaths::GameSourceDir());
	const FString ConfigsDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectConfigDir());
	const FString PluginsDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectPluginsDir());

	GetFilesInPathByExt(SourceDir, true, false, SourceFileExtensions, SourceFiles);
	GetFilesInPathByExt(ConfigsDir, true, false, ConfigFileExtensions, ConfigFiles);

	TSet<FString> InstalledPlugins;
	GetFoldersInPath(PluginsDir, false, InstalledPlugins);

	TSet<FString> Files;
	for (const auto& InstalledPlugin : InstalledPlugins)
	{
		GetFilesInPathByExt(InstalledPlugin / TEXT("Source"), true, false, SourceFileExtensions, Files);
		SourceFiles.Append(Files);

		Files.Reset();

		GetFilesInPathByExt(InstalledPlugin / TEXT("Config"), true, false, ConfigFileExtensions, Files);
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
			const FName ObjectPath = PathConvertToObjectPath(FoundedAssetObjectPath);
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

void UPjcSubsystem::FindAssetsPrimary()
{
	TSet<FName> ClassNamesPrimary;
	GetClassNamesPrimary(ClassNamesPrimary);

	for (const auto& Asset : AssetsAll)
	{
		if (ClassNamesPrimary.Contains(Asset.AssetClass) || ClassNamesPrimary.Contains(GetAssetClassName(Asset)))
		{
			AssetsPrimary.Emplace(Asset);
		}
	}
}

int32 UPjcSubsystem::GetNumAssetsAll() const
{
	return AssetsAll.Num();
}

int32 UPjcSubsystem::GetNumAssetsUsed() const
{
	return AssetsUsed.Num();
}

int32 UPjcSubsystem::GetNumAssetsExcluded() const
{
	return AssetsExcluded.Num();
}

int32 UPjcSubsystem::GetNumAssetsPrimary() const
{
	return AssetsPrimary.Num();
}

int32 UPjcSubsystem::GetNumAssetsIndirect() const
{
	return AssetsIndirect.Num();
}

int32 UPjcSubsystem::GetNumAssetsEditor() const
{
	return AssetsEditor.Num();
}

int32 UPjcSubsystem::GetNumAssetsExtReferenced() const
{
	return AssetsExtReferenced.Num();
}

int32 UPjcSubsystem::GetNumAssetsUnused() const
{
	return AssetsUnused.Num();
}

int32 UPjcSubsystem::GetNumFilesNonEngine() const
{
	return FilesNonEngine.Num();
}

int32 UPjcSubsystem::GetNumFilesNonCorrupted() const
{
	return FilesCorrupted.Num();
}

int32 UPjcSubsystem::GetNumFolderEmpty() const
{
	return FoldersEmpty.Num();
}

int64 UPjcSubsystem::GetSizeAssetsAll() const
{
	int64 Size = 0;

	for (const auto& Asset : AssetsAll)
	{
		Size += GetAssetSize(Asset);
	}

	return Size;
}

int64 UPjcSubsystem::GetSizeAssetsUsed() const
{
	int64 Size = 0;

	for (const auto& Asset : AssetsUsed)
	{
		Size += GetAssetSize(Asset);
	}

	return Size;
}

int64 UPjcSubsystem::GetSizeAssetsExcluded() const
{
	int64 Size = 0;

	for (const auto& Asset : AssetsExcluded)
	{
		Size += GetAssetSize(Asset);
	}

	return Size;
}

int64 UPjcSubsystem::GetSizeAssetsPrimary() const
{
	int64 Size = 0;

	for (const auto& Asset : AssetsPrimary)
	{
		Size += GetAssetSize(Asset);
	}

	return Size;
}

int64 UPjcSubsystem::GetSizeAssetsIndirect() const
{
	int64 Size = 0;

	TArray<FAssetData> Assets;
	AssetsIndirect.GetKeys(Assets);

	for (const auto& Asset : Assets)
	{
		Size += GetAssetSize(Asset);
	}

	return Size;
}

int64 UPjcSubsystem::GetSizeAssetsEditor() const
{
	int64 Size = 0;

	for (const auto& Asset : AssetsEditor)
	{
		Size += GetAssetSize(Asset);
	}

	return Size;
}

int64 UPjcSubsystem::GetSizeAssetsExtReferenced() const
{
	int64 Size = 0;

	for (const auto& Asset : AssetsExtReferenced)
	{
		Size += GetAssetSize(Asset);
	}

	return Size;
}

int64 UPjcSubsystem::GetSizeAssetsUnused() const
{
	int64 Size = 0;

	for (const auto& Asset : AssetsUnused)
	{
		Size += GetAssetSize(Asset);
	}

	return Size;
}

int64 UPjcSubsystem::GetSizeFilesNonEngine() const
{
	int64 Size = 0;

	for (const auto& File : FilesNonEngine)
	{
		Size += GetFileSize(File);
	}

	return Size;
}

int64 UPjcSubsystem::GetSizeFilesCorrupted() const
{
	int64 Size = 0;

	for (const auto& File : FilesCorrupted)
	{
		Size += GetFileSize(File);
	}

	return Size;
}

EPjcScannerState UPjcSubsystem::GetScannerState() const
{
	return ScannerState;
}

void UPjcSubsystem::GetAssetsDeps(const TSet<FAssetData>& InAssets, TSet<FAssetData>& OutAssetsDeps)
{
	const FAssetRegistryModule& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(PjcConstants::ModuleAssetRegistryName);

	TArray<FName> Stack;
	TArray<FName> Deps;
	TSet<FName> DepsPackageNames;

	for (const auto& Asset : InAssets)
	{
		Stack.Add(Asset.PackageName);

		while (Stack.Num() > 0)
		{
			const auto CurrentPackageName = Stack.Pop();
			Deps.Reset();
			AssetRegistry.Get().GetDependencies(CurrentPackageName, Deps);

			Deps.RemoveAllSwap([&](const FName& Dep)
			{
				return !Dep.ToString().StartsWith(*PjcConstants::PathRelRoot.ToString());
			}, false);

			for (const auto& Dep : Deps)
			{
				bool bIsAlreadyInSet = false;
				DepsPackageNames.Add(Dep, &bIsAlreadyInSet);
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

	for (const auto& Dep : DepsPackageNames)
	{
		Filter.PackageNames.Add(Dep);
	}

	TArray<FAssetData> AssetsContainer;
	AssetRegistry.Get().GetAssets(Filter, AssetsContainer);

	OutAssetsDeps.Empty();
	OutAssetsDeps.Append(AssetsContainer);
}

//
// void UPjcSubsystem::ShowModal(const FString& Msg, const EPjcModalStatus ModalStatus, const float Duration)
// {
// 	FNotificationInfo Info{FText::FromString(Msg)};
// 	Info.Text = FText::FromString(Msg);
// 	Info.ExpireDuration = Duration;
//
// 	const auto NotificationPtr = FSlateNotificationManager::Get().AddNotification(Info);
// 	if (!NotificationPtr.IsValid()) return;
//
// 	NotificationPtr.Get()->SetCompletionState(GetCompletionStateFromModalStatus(ModalStatus));
// }
//
// void UPjcSubsystem::ShowModalWithOutputLog(const FString& Msg, const EPjcModalStatus ModalStatus, const float Duration)
// {
// 	FNotificationInfo Info{FText::FromString(Msg)};
// 	Info.Text = FText::FromString(Msg);
// 	Info.ExpireDuration = Duration;
// 	Info.Hyperlink = FSimpleDelegate::CreateLambda([]()
// 	{
// 		FGlobalTabmanager::Get()->TryInvokeTab(FName{TEXT("OutputLog")});
// 	});
// 	Info.HyperlinkText = FText::FromString(TEXT("Show OutputLog..."));
//
// 	const auto NotificationPtr = FSlateNotificationManager::Get().AddNotification(Info);
// 	if (!NotificationPtr.IsValid()) return;
//
// 	NotificationPtr.Get()->SetCompletionState(GetCompletionStateFromModalStatus(ModalStatus));
// }
