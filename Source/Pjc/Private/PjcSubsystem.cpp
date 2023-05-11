// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "PjcSubsystem.h"
#include "PjcConstants.h"
// Engine Headers
#include "AssetManagerEditorModule.h"
#include "EditorTutorial.h"
#include "EditorUtilityBlueprint.h"
#include "EditorUtilityWidget.h"
#include "EditorUtilityWidgetBlueprint.h"
#include "ShaderCompiler.h"
#include "Engine/AssetManager.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Internationalization/Regex.h"
#include "Misc/FileHelper.h"
#include "Misc/ScopedSlowTask.h"

void UPjcSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	AssetsCategoryMapping.Add(EPjcAssetCategory::None);
	AssetsCategoryMapping.Add(EPjcAssetCategory::Any);
	AssetsCategoryMapping.Add(EPjcAssetCategory::Used);
	AssetsCategoryMapping.Add(EPjcAssetCategory::Unused);
	AssetsCategoryMapping.Add(EPjcAssetCategory::Primary);
	AssetsCategoryMapping.Add(EPjcAssetCategory::Indirect);
	AssetsCategoryMapping.Add(EPjcAssetCategory::Circular);
	AssetsCategoryMapping.Add(EPjcAssetCategory::Editor);
	AssetsCategoryMapping.Add(EPjcAssetCategory::Excluded);
	AssetsCategoryMapping.Add(EPjcAssetCategory::ExtReferenced);
}

void UPjcSubsystem::Deinitialize()
{
	Super::Deinitialize();

	FilesExternal.Empty();
	FilesCorrupted.Empty();
	FoldersEmpty.Empty();
	AssetsCategoryMapping.Empty();
	AssetsIndirectInfo.Empty();
}

void UPjcSubsystem::GetClassNamesPrimary(TSet<FName>& ClassNames)
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
	ClassNames.Empty();
	GetModuleAssetRegistry().Get().GetDerivedClassNames(ClassNamesPrimaryBase.Array(), TSet<FName>{}, ClassNames);
}

void UPjcSubsystem::GetClassNamesEditor(TSet<FName>& ClassNames)
{
	const TArray<FName> ClassNamesEditorBase{
		UEditorUtilityWidget::StaticClass()->GetFName(),
		UEditorUtilityBlueprint::StaticClass()->GetFName(),
		UEditorUtilityWidgetBlueprint::StaticClass()->GetFName(),
		UEditorTutorial::StaticClass()->GetFName()
	};

	ClassNames.Empty();
	GetModuleAssetRegistry().Get().GetDerivedClassNames(ClassNamesEditorBase, TSet<FName>{}, ClassNames);
}

void UPjcSubsystem::GetSourceAndConfigFiles(TSet<FString>& Files)
{
	const FString DirSrc = FPaths::ConvertRelativePathToFull(FPaths::GameSourceDir());
	const FString DirCfg = FPaths::ConvertRelativePathToFull(FPaths::ProjectConfigDir());
	const FString DirPlg = FPaths::ConvertRelativePathToFull(FPaths::ProjectPluginsDir());

	TSet<FString> SourceFiles;
	TSet<FString> ConfigFiles;

	GetFilesInPathByExt(DirSrc, true, false, PjcConstants::SourceFileExtensions, SourceFiles);
	GetFilesInPathByExt(DirCfg, true, false, PjcConstants::ConfigFileExtensions, ConfigFiles);

	TSet<FString> InstalledPlugins;
	GetFoldersInPath(DirPlg, false, InstalledPlugins);

	const FString ProjectCleanerPluginPath = DirPlg / PjcConstants::ModulePjcName.ToString();
	TSet<FString> PluginFiles;
	for (const auto& InstalledPlugin : InstalledPlugins)
	{
		// ignore ProjectCleaner plugin
		if (InstalledPlugin.Equals(ProjectCleanerPluginPath)) continue;

		GetFilesInPathByExt(InstalledPlugin / TEXT("Source"), true, false, PjcConstants::SourceFileExtensions, PluginFiles);
		SourceFiles.Append(PluginFiles);

		PluginFiles.Reset();

		GetFilesInPathByExt(InstalledPlugin / TEXT("Config"), true, false, PjcConstants::ConfigFileExtensions, PluginFiles);
		ConfigFiles.Append(PluginFiles);

		PluginFiles.Reset();
	}

	Files.Reset();
	Files.Append(SourceFiles);
	Files.Append(ConfigFiles);
}

void UPjcSubsystem::GetAssetsDependencies(TSet<FAssetData>& Assets)
{
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

			GetModuleAssetRegistry().Get().GetDependencies(CurrentPackageName, Deps);

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

	for (const auto& Dep : UsedAssetsDeps)
	{
		Filter.PackageNames.Add(Dep);
	}

	TArray<FAssetData> TempContainer;
	GetModuleAssetRegistry().Get().GetAssets(Filter, TempContainer);

	Assets.Append(TempContainer);
}

void UPjcSubsystem::ShowNotification(const FString& Msg, const SNotificationItem::ECompletionState State, const float Duration)
{
	FNotificationInfo Info{FText::FromString(Msg)};
	Info.Text = FText::FromString(Msg);
	Info.ExpireDuration = Duration;

	const auto NotificationPtr = FSlateNotificationManager::Get().AddNotification(Info);
	if (!NotificationPtr.IsValid()) return;

	NotificationPtr.Get()->SetCompletionState(State);
}

void UPjcSubsystem::ShowNotificationWithOutputLog(const FString& Msg, const SNotificationItem::ECompletionState State, const float Duration)
{
	FNotificationInfo Info{FText::FromString(Msg)};
	Info.Text = FText::FromString(Msg);
	Info.ExpireDuration = Duration;
	Info.Hyperlink = FSimpleDelegate::CreateLambda([]()
	{
		FGlobalTabmanager::Get()->TryInvokeTab(FName{TEXT("OutputLog")});
	});
	Info.HyperlinkText = FText::FromString(TEXT("Show OutputLog..."));

	const auto NotificationPtr = FSlateNotificationManager::Get().AddNotification(Info);
	if (!NotificationPtr.IsValid()) return;

	NotificationPtr.Get()->SetCompletionState(State);
}

void UPjcSubsystem::ShaderCompilationEnable()
{
	if (!GShaderCompilingManager) return;

	GShaderCompilingManager->SkipShaderCompilation(false);
}

void UPjcSubsystem::ShaderCompilationDisable()
{
	if (!GShaderCompilingManager) return;

	GShaderCompilingManager->SkipShaderCompilation(true);
}

void UPjcSubsystem::OpenPathInFileExplorer(const FString& InPath)
{
	if (InPath.IsEmpty()) return;
	if (!FPaths::DirectoryExists(InPath)) return;

	FPlatformProcess::ExploreFolder(*InPath);
}

void UPjcSubsystem::OpenAssetEditor(const FAssetData& InAsset)
{
	if (!InAsset.IsValid()) return;
	if (!GEditor) return;

	TArray<FName> AssetNames;
	AssetNames.Add(InAsset.ToSoftObjectPath().GetAssetPathName());

	GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorsForAssets(AssetNames);
}

void UPjcSubsystem::OpenSizeMapViewer(const TArray<FAssetData>& InAssets)
{
	if (InAssets.Num() == 0) return;

	TArray<FName> PackageNames;
	PackageNames.Reserve(InAssets.Num());

	for (const auto& Asset : InAssets)
	{
		PackageNames.Emplace(Asset.PackageName);
	}

	IAssetManagerEditorModule::Get().OpenSizeMapUI(PackageNames);
}

void UPjcSubsystem::OpenReferenceViewer(const TArray<FAssetData>& InAssets)
{
	if (InAssets.Num() == 0) return;

	TArray<FName> PackageNames;
	PackageNames.Reserve(InAssets.Num());

	for (const auto& Asset : InAssets)
	{
		PackageNames.Emplace(Asset.PackageName);
	}

	IAssetManagerEditorModule::Get().OpenReferenceViewerUI(PackageNames);
}

void UPjcSubsystem::OpenAssetAuditViewer(const TArray<FAssetData>& InAssets)
{
	if (InAssets.Num() == 0) return;

	TArray<FName> PackageNames;
	PackageNames.Reserve(InAssets.Num());

	for (const auto& Asset : InAssets)
	{
		PackageNames.Emplace(Asset.PackageName);
	}

	IAssetManagerEditorModule::Get().OpenAssetAuditUI(PackageNames);
}

void UPjcSubsystem::TryOpenFile(const FString& InPath)
{
	if (InPath.IsEmpty()) return;
	if (!FPaths::FileExists(InPath)) return;

	FPlatformProcess::LaunchFileInDefaultExternalApplication(*InPath);
}

void UPjcSubsystem::FixupRedirectorsInProject()
{
	FScopedSlowTask SlowTask{
		1.0f,
		FText::FromString(TEXT("Fixing redirectors...")),
		GIsEditor && !IsRunningCommandlet()
	};
	SlowTask.MakeDialog(false, false);
	SlowTask.EnterProgressFrame(1.0f);

	FARFilter Filter;
	Filter.bRecursivePaths = true;
	Filter.PackagePaths.Emplace(PjcConstants::PathRoot);
	Filter.ClassNames.Emplace(UObjectRedirector::StaticClass()->GetFName());

	TArray<FAssetData> AssetList;
	GetModuleAssetRegistry().Get().GetAssets(Filter, AssetList);

	if (AssetList.Num() == 0) return;

	FScopedSlowTask LoadingTask(
		AssetList.Num(),
		FText::FromString(TEXT("Loading redirectors...")),
		GIsEditor && !IsRunningCommandlet()
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

	GetModuleAssetTools().Get().FixupReferencers(Redirectors, false);
}

void UPjcSubsystem::GetFilesInPath(const FString& InSearchPath, const bool bSearchRecursive, TSet<FString>& OutFiles)
{
	OutFiles.Empty();

	struct FFindFilesVisitor : IPlatformFile::FDirectoryVisitor
	{
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
		FPlatformFileManager::Get().GetPlatformFile().IterateDirectoryRecursively(*InSearchPath, FindFilesVisitor);
	}
	else
	{
		FPlatformFileManager::Get().GetPlatformFile().IterateDirectory(*InSearchPath, FindFilesVisitor);
	}
}

void UPjcSubsystem::GetFilesInPathByExt(const FString& InSearchPath, const bool bSearchRecursive, const bool bExtSearchInvert, const TSet<FString>& InExtensions, TSet<FString>& OutFiles)
{
	OutFiles.Empty();

	struct FFindFilesVisitor : IPlatformFile::FDirectoryVisitor
	{
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
				const FString FullPath = FPaths::ConvertRelativePathToFull(FilenameOrDirectory);

				if (Extensions.Num() == 0)
				{
					Files.Emplace(FullPath);
					return true;
				}

				const FString Ext = FPaths::GetExtension(FullPath, false);
				const bool bExistsInSearchList = Extensions.Contains(Ext);

				if (
					bExistsInSearchList && !bSearchInvert ||
					!bExistsInSearchList && bSearchInvert
				)
				{
					Files.Emplace(FullPath);
				}
			}

			return true;
		}
	};

	TSet<FString> ExtensionsNormalized;
	ExtensionsNormalized.Reserve(InExtensions.Num());

	for (const auto& Ext : InExtensions)
	{
		const FString ExtNormalized = Ext.Replace(TEXT("."), TEXT(""));
		ExtensionsNormalized.Emplace(ExtNormalized);
	}

	FFindFilesVisitor FindFilesVisitor{bExtSearchInvert, OutFiles, ExtensionsNormalized};
	if (bSearchRecursive)
	{
		FPlatformFileManager::Get().GetPlatformFile().IterateDirectoryRecursively(*InSearchPath, FindFilesVisitor);
	}
	else
	{
		FPlatformFileManager::Get().GetPlatformFile().IterateDirectory(*InSearchPath, FindFilesVisitor);
	}
}

void UPjcSubsystem::GetFoldersInPath(const FString& InSearchPath, const bool bSearchRecursive, TSet<FString>& OutFolders)
{
	OutFolders.Empty();

	struct FFindFoldersVisitor : IPlatformFile::FDirectoryVisitor
	{
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
		FPlatformFileManager::Get().GetPlatformFile().IterateDirectoryRecursively(*InSearchPath, FindFoldersVisitor);
	}
	else
	{
		FPlatformFileManager::Get().GetPlatformFile().IterateDirectory(*InSearchPath, FindFoldersVisitor);
	}
}

void UPjcSubsystem::FindAssetsIndirect()
{
	TSet<FString> ScanFiles;
	GetSourceAndConfigFiles(ScanFiles);

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
			const FString ObjectPath = PathConvertToObjectPath(FoundedAssetObjectPath);
			const FAssetData AssetData = GetModuleAssetRegistry().Get().GetAssetByObjectPath(FName{*ObjectPath});
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

				TArray<FPjcFileInfo>& Infos = AssetsIndirectInfo.FindOrAdd(AssetData);
				Infos.AddUnique(FPjcFileInfo{FileLine, FilePathAbs});
			}
		}
	}

	TArray<FAssetData> AssetsIndirect;
	AssetsIndirectInfo.GetKeys(AssetsIndirect);

	AssetsCategoryMapping[EPjcAssetCategory::Indirect].Reset();
	AssetsCategoryMapping[EPjcAssetCategory::Indirect].Append(AssetsIndirect);
}

void UPjcSubsystem::FindAssetsExcluded()
{
	// todo:ashe23
}

bool UPjcSubsystem::AssetIsBlueprint(const FAssetData& InAsset)
{
	if (!InAsset.IsValid()) return false;

	const UClass* AssetClass = InAsset.GetClass();
	if (!AssetClass) return false;

	return AssetClass->IsChildOf(UBlueprint::StaticClass());
}

bool UPjcSubsystem::AssetIsExtReferenced(const FAssetData& InAsset)
{
	if (!InAsset.IsValid()) return false;

	TArray<FName> Refs;
	GetModuleAssetRegistry().Get().GetReferencers(InAsset.PackageName, Refs);

	return Refs.ContainsByPredicate([](const FName& Ref)
	{
		return !Ref.ToString().StartsWith(PjcConstants::PathRoot.ToString());
	});
}

bool UPjcSubsystem::AssetIsCircular(const FAssetData& InAsset)
{
	if (!InAsset.IsValid()) return false;

	TArray<FName> Refs;
	TArray<FName> Deps;

	GetModuleAssetRegistry().Get().GetReferencers(InAsset.PackageName, Refs);
	GetModuleAssetRegistry().Get().GetDependencies(InAsset.PackageName, Deps);

	for (const auto& Ref : Refs)
	{
		if (Deps.Contains(Ref))
		{
			return true;
		}
	}

	return false;
}

bool UPjcSubsystem::EditorIsInPlayMode()
{
	return GEditor && GEditor->PlayWorld || GIsPlayInEditorWorld;
}

bool UPjcSubsystem::ProjectContainsRedirectors()
{
	FARFilter Filter;
	Filter.bRecursivePaths = true;
	Filter.PackagePaths.Emplace(PjcConstants::PathRoot);
	Filter.ClassNames.Emplace(UObjectRedirector::StaticClass()->GetFName());

	TArray<FAssetData> Redirectors;
	GetModuleAssetRegistry().Get().GetAssets(Filter, Redirectors);

	return Redirectors.Num() > 0;
}

bool UPjcSubsystem::PathIsEmpty(const FString& InPath)
{
	if (InPath.IsEmpty()) return false;

	const FName PathRel = FName{*PathConvertToRelative(InPath)};
	if (GetModuleAssetRegistry().Get().HasAssets(PathRel, true))
	{
		return false;
	}

	const FString PathAbs = PathConvertToAbsolute(InPath);
	if (PathAbs.IsEmpty()) return false;

	TArray<FString> Files;
	IFileManager::Get().FindFilesRecursive(Files, *PathAbs, TEXT("*"), true, false);

	return Files.Num() == 0;
}

bool UPjcSubsystem::PathIsExcluded(const FString& InPath)
{
	const UPjcAssetExcludeSettings* EditorAssetExcludeSettings = GetDefault<UPjcAssetExcludeSettings>();
	if (!EditorAssetExcludeSettings) return false;

	const FString PathRel = PathConvertToRelative(InPath);
	if (PathRel.IsEmpty()) return false;

	for (const auto& ExcludedPath : EditorAssetExcludeSettings->ExcludedFolders)
	{
		const FString ExcludedPathRel = PathConvertToRelative(ExcludedPath.Path);
		if (ExcludedPathRel.IsEmpty()) continue;

		if (PathRel.StartsWith(ExcludedPathRel))
		{
			return true;
		}
	}

	return false;
}

bool UPjcSubsystem::PathIsEngineGenerated(const FString& InPath)
{
	const FString PathDevelopers = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir());
	const FString PathCollections = PathDevelopers / TEXT("Collections");
	const FString PathCurrentDeveloper = PathDevelopers / FPaths::GameUserDeveloperFolderName();
	const FString PathCurrentDeveloperCollections = PathCurrentDeveloper / TEXT("Collections");

	TSet<FString> EngineGeneratedPaths;
	EngineGeneratedPaths.Emplace(PathDevelopers);
	EngineGeneratedPaths.Emplace(PathCollections);
	EngineGeneratedPaths.Emplace(PathCurrentDeveloper);
	EngineGeneratedPaths.Emplace(PathCurrentDeveloperCollections);

	return EngineGeneratedPaths.Contains(InPath);
}

int64 UPjcSubsystem::GetAssetSize(const FAssetData& InAsset)
{
	if (!InAsset.IsValid()) return 0;

	const FAssetPackageData* AssetPackageData = GetModuleAssetRegistry().Get().GetAssetPackageData(InAsset.PackageName);
	if (!AssetPackageData) return 0;

	return AssetPackageData->DiskSize;
}

int64 UPjcSubsystem::GetAssetsTotalSize(const TSet<FAssetData>& InAssets)
{
	int64 Size = 0;

	for (const auto& Asset : InAssets)
	{
		if (!Asset.IsValid()) continue;

		const auto AssetPackageData = GetModuleAssetRegistry().Get().GetAssetPackageData(Asset.PackageName);
		if (!AssetPackageData) continue;

		Size += AssetPackageData->DiskSize;
	}

	return Size;
}

FName UPjcSubsystem::GetAssetExactClassName(const FAssetData& InAsset)
{
	if (!InAsset.IsValid()) return NAME_None;

	if (AssetIsBlueprint(InAsset))
	{
		const FString GeneratedClassName = InAsset.TagsAndValues.FindTag(TEXT("GeneratedClass")).GetValue();
		const FString ClassObjectPath = FPackageName::ExportTextPathToObjectPath(*GeneratedClassName);
		return FName{*FPackageName::ObjectPathToObjectName(ClassObjectPath)};
	}

	return InAsset.AssetClass;
}

FString UPjcSubsystem::PathNormalize(const FString& InPath)
{
	if (InPath.IsEmpty()) return {};

	// Ensure the path dont starts with a slash or a disk drive letter
	if (!(InPath.StartsWith(TEXT("/")) || InPath.StartsWith(TEXT("\\")) || (InPath.Len() > 2 && InPath[1] == ':')))
	{
		return {};
	}

	FString Path = FPaths::ConvertRelativePathToFull(InPath).TrimStartAndEnd();
	FPaths::RemoveDuplicateSlashes(Path);

	// Collapse any ".." or "." references in the path
	FPaths::CollapseRelativeDirectories(Path);

	if (FPaths::GetExtension(Path).IsEmpty())
	{
		FPaths::NormalizeDirectoryName(Path);
	}
	else
	{
		FPaths::NormalizeFilename(Path);
	}

	// Ensure the path does not end with a trailing slash
	if (Path.EndsWith(TEXT("/")) || Path.EndsWith(TEXT("\\")))
	{
		Path = Path.LeftChop(1);
	}

	return Path;
}

FString UPjcSubsystem::PathConvertToAbsolute(const FString& InPath)
{
	const FString PathNormalized = PathNormalize(InPath);
	const FString PathProjectContent = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir()).LeftChop(1);

	if (PathNormalized.IsEmpty()) return {};
	if (PathNormalized.StartsWith(PathProjectContent)) return PathNormalized;
	if (PathNormalized.StartsWith(PjcConstants::PathRoot.ToString()))
	{
		FString Path = PathNormalized;
		Path.RemoveFromStart(PjcConstants::PathRoot.ToString());

		return Path.IsEmpty() ? PathProjectContent : PathProjectContent / Path;
	}

	return {};
}

FString UPjcSubsystem::PathConvertToRelative(const FString& InPath)
{
	const FString PathNormalized = PathNormalize(InPath);
	const FString PathProjectContent = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir()).LeftChop(1);

	if (PathNormalized.IsEmpty()) return {};
	if (PathNormalized.StartsWith(PjcConstants::PathRoot.ToString())) return PathNormalized;
	if (PathNormalized.StartsWith(PathProjectContent))
	{
		FString Path = PathNormalized;
		Path.RemoveFromStart(PathProjectContent);

		return Path.IsEmpty() ? PjcConstants::PathRoot.ToString() : PjcConstants::PathRoot.ToString() / Path;
	}

	return {};
}

FString UPjcSubsystem::PathConvertToObjectPath(const FString& InPath)
{
	if (FPaths::FileExists(InPath))
	{
		const FString FileName = FPaths::GetBaseFilename(InPath);
		const FString AssetPath = PathConvertToRelative(FPaths::GetPath(InPath));

		return FString::Printf(TEXT("%s/%s.%s"), *AssetPath, *FileName, *FileName);
	}

	const FString ObjectPath = FPackageName::ExportTextPathToObjectPath(InPath);

	if (!ObjectPath.StartsWith(PjcConstants::PathRoot.ToString())) return {};

	TArray<FString> Parts;
	ObjectPath.ParseIntoArray(Parts, TEXT("/"), true);

	if (Parts.Num() > 0)
	{
		FString Left;
		FString Right;
		Parts.Last().Split(TEXT("."), &Left, &Right);

		if (!Left.IsEmpty() && !Right.IsEmpty() && Left.Equals(*Right))
		{
			return ObjectPath;
		}
	}

	return {};
}

FAssetRegistryModule& UPjcSubsystem::GetModuleAssetRegistry()
{
	return FModuleManager::LoadModuleChecked<FAssetRegistryModule>(PjcConstants::ModuleAssetRegistry);
}

FAssetToolsModule& UPjcSubsystem::GetModuleAssetTools()
{
	return FModuleManager::LoadModuleChecked<FAssetToolsModule>(PjcConstants::ModuleAssetTools);
}

FContentBrowserModule& UPjcSubsystem::GetModuleContentBrowser()
{
	return FModuleManager::LoadModuleChecked<FContentBrowserModule>(PjcConstants::ModuleContentBrowser);
}

FPropertyEditorModule& UPjcSubsystem::GetModulePropertyEditor()
{
	return FModuleManager::LoadModuleChecked<FPropertyEditorModule>(PjcConstants::ModulePropertyEditor);
}
