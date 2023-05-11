// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "PjcSubsystem.h"
#include "PjcConstants.h"
// Engine Headers
#include "AssetManagerEditorModule.h"
#include "EditorTutorial.h"
#include "EditorUtilityBlueprint.h"
#include "EditorUtilityWidget.h"
#include "EditorUtilityWidgetBlueprint.h"
#include "FileHelpers.h"
#include "Pjc.h"
#include "ShaderCompiler.h"
#include "Engine/AssetManager.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Internationalization/Regex.h"
#include "Misc/FileHelper.h"
#include "Misc/ScopedSlowTask.h"

void UPjcSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UPjcSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

void UPjcSubsystem::ScanProjectAssets(TMap<EPjcAssetCategory, TSet<FAssetData>>& AssetsCategoryMapping) const
{
	AssetsCategoryMapping.Empty();
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

	if (GetModuleAssetRegistry().Get().IsLoadingAssets()) return;
	if (GEditor && !GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->CloseAllAssetEditors()) return;
	if (!FEditorFileUtils::SaveDirtyPackages(true, true, true, false, false, false)) return;

	FixupRedirectorsInProject();

	if (ProjectContainsRedirectors()) return;

	const double ScanStartTime = FPlatformTime::Seconds();

	FScopedSlowTask SlowTaskMain{
		6.0f,
		FText::FromString(TEXT("Scanning project assets...")),
		GIsEditor && !IsRunningCommandlet()
	};
	SlowTaskMain.MakeDialog(false, false);
	SlowTaskMain.EnterProgressFrame(1.0f);

	for (auto& Mapping : AssetsCategoryMapping)
	{
		Mapping.Value.Reset();
	}

	SlowTaskMain.EnterProgressFrame(1.0f);
	TSet<FName> ClassNamesPrimary;
	TSet<FName> ClassNamesEditor;
	TSet<FName> ClassNamesExcluded;
	TArray<FAssetData> AssetsAll;
	TMap<FAssetData, TArray<FPjcFileInfo>> AssetsIndirectInfo;

	GetClassNamesPrimary(ClassNamesPrimary);
	GetClassNamesEditor(ClassNamesEditor);
	GetClassNamesExcluded(ClassNamesExcluded);
	GetModuleAssetRegistry().Get().GetAssetsByPath(PjcConstants::PathRoot, AssetsAll, true);

	SlowTaskMain.EnterProgressFrame(1.0f);
	FindAssetsIndirect(AssetsIndirectInfo);
	FindAssetsExcludedByPaths(AssetsCategoryMapping);

	SlowTaskMain.EnterProgressFrame(1.0f);

	FScopedSlowTask SlowTask{
		static_cast<float>(AssetsAll.Num()),
		FText::FromString(TEXT("Scanning project assets...")),
		GIsEditor && !IsRunningCommandlet()
	};
	SlowTask.MakeDialog(false, false);

	for (const auto& Asset : AssetsAll)
	{
		SlowTask.EnterProgressFrame(1.0f, FText::FromString(Asset.GetFullName()));

		const FName AssetExactClassName = GetAssetExactClassName(Asset);
		const bool bIsPrimary = ClassNamesPrimary.Contains(Asset.AssetClass) || ClassNamesPrimary.Contains(AssetExactClassName);
		const bool bIsEditor = ClassNamesEditor.Contains(Asset.AssetClass) || ClassNamesEditor.Contains(AssetExactClassName);
		const bool bIsExcluded = ClassNamesExcluded.Contains(Asset.AssetClass) || ClassNamesExcluded.Contains(AssetExactClassName);
		const bool bIsCircular = AssetIsCircular(Asset);
		const bool bIsExtReferenced = AssetIsExtReferenced(Asset);
		const bool bIsUsed = bIsPrimary || bIsEditor || bIsExtReferenced;

		AssetsCategoryMapping[EPjcAssetCategory::Any].Emplace(Asset);

		if (bIsPrimary)
		{
			AssetsCategoryMapping[EPjcAssetCategory::Primary].Emplace(Asset);
		}

		if (bIsEditor)
		{
			AssetsCategoryMapping[EPjcAssetCategory::Editor].Emplace(Asset);
		}

		if (bIsExcluded)
		{
			AssetsCategoryMapping[EPjcAssetCategory::Excluded].Emplace(Asset);
		}

		if (bIsCircular)
		{
			AssetsCategoryMapping[EPjcAssetCategory::Circular].Emplace(Asset);
		}

		if (bIsExtReferenced)
		{
			AssetsCategoryMapping[EPjcAssetCategory::ExtReferenced].Emplace(Asset);
		}

		if (bIsUsed)
		{
			AssetsCategoryMapping[EPjcAssetCategory::Used].Emplace(Asset);
		}
	}

	// loading all used assets dependencies recursive
	SlowTaskMain.EnterProgressFrame(1.0f);
	TArray<FAssetData> AssetsIndirect;
	AssetsIndirectInfo.GetKeys(AssetsIndirect);
	AssetsCategoryMapping[EPjcAssetCategory::Indirect].Append(AssetsIndirect);
	AssetsCategoryMapping[EPjcAssetCategory::Used].Append(AssetsCategoryMapping[EPjcAssetCategory::Excluded]);
	AssetsCategoryMapping[EPjcAssetCategory::Used].Append(AssetsCategoryMapping[EPjcAssetCategory::Indirect]);
	GetAssetsDependencies(AssetsCategoryMapping[EPjcAssetCategory::Used]);

	// filtering unused assets
	SlowTaskMain.EnterProgressFrame(1.0f);
	AssetsCategoryMapping[EPjcAssetCategory::Unused] = AssetsCategoryMapping[EPjcAssetCategory::Any].Difference(AssetsCategoryMapping[EPjcAssetCategory::Used]);

	const double ScanTime = FPlatformTime::Seconds() - ScanStartTime;
	UE_LOG(LogProjectCleaner, Display, TEXT("Project assets scanned in %.2f seconds."), ScanTime);
}

void UPjcSubsystem::GetAssetsByCategory(const EPjcAssetCategory AssetCategory, TSet<FAssetData>& Assets)
{
	TMap<EPjcAssetCategory, TSet<FAssetData>> AssetsCategoryMapping;

	ScanProjectAssets(AssetsCategoryMapping);

	Assets.Reset();
	Assets.Append(AssetsCategoryMapping[AssetCategory]);
}

void UPjcSubsystem::GetAssetIndirectInfo(const FAssetData& Asset, TArray<FPjcFileInfo>& Infos)
{
	Infos.Reset();

	TMap<FAssetData, TArray<FPjcFileInfo>> AssetsIndirectInfo;
	FindAssetsIndirect(AssetsIndirectInfo);

	if (AssetsIndirectInfo.Contains(Asset))
	{
		Infos = *AssetsIndirectInfo.Find(Asset);
	}
}

void UPjcSubsystem::GetFilesExternal(TSet<FString>& FilesExternal)
{
	GetFilesInPathByExt(FPaths::ProjectContentDir(), true, true, PjcConstants::EngineFileExtensions, FilesExternal);
}

void UPjcSubsystem::GetFilesCorrupted(TSet<FString>& FilesCorrupted)
{
	FilesCorrupted.Reset();

	TSet<FString> Files;
	GetFilesInPathByExt(FPaths::ProjectContentDir(), true, false, PjcConstants::EngineFileExtensions, Files);

	for (const auto& File : Files)
	{
		const FString ObjectPath = PathConvertToObjectPath(File);
		if (ObjectPath.IsEmpty()) continue;

		const FAssetData AssetData = GetModuleAssetRegistry().Get().GetAssetByObjectPath(FName{*ObjectPath});
		if (!AssetData.IsValid())
		{
			FilesCorrupted.Emplace(File);
		}
	}
}

void UPjcSubsystem::GetFoldersEmpty(TSet<FString>& FoldersEmpty)
{
	FoldersEmpty.Reset();

	TArray<FString> PathsAll;
	GetModuleAssetRegistry().Get().GetAllCachedPaths(PathsAll);

	for (const auto& Path : PathsAll)
	{
		if (PathIsEmpty(Path) && !PathIsExcluded(Path) && !PathIsEngineGenerated(Path))
		{
			FoldersEmpty.Emplace(PathConvertToAbsolute(Path));
		}
	}
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

void UPjcSubsystem::GetClassNamesExcluded(TSet<FName>& ClassNames)
{
	const UPjcAssetExcludeSettings* AssetExcludeSettings = GetDefault<UPjcAssetExcludeSettings>();
	if (!AssetExcludeSettings) return;

	ClassNames.Empty(AssetExcludeSettings->ExcludedClasses.Num());
	for (const auto& ExcludedClass : AssetExcludeSettings->ExcludedClasses)
	{
		if (!ExcludedClass.LoadSynchronous() || ExcludedClass.IsNull()) continue;

		ClassNames.Emplace(ExcludedClass.Get()->GetFName());
	}
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

void UPjcSubsystem::FindAssetsIndirect(TMap<FAssetData, TArray<FPjcFileInfo>>& AssetsIndirectInfo)
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
}

void UPjcSubsystem::FindAssetsExcludedByPaths(TMap<EPjcAssetCategory, TSet<FAssetData>>& AssetsCategoryMapping)
{
	AssetsCategoryMapping[EPjcAssetCategory::Excluded].Reset();

	const UPjcAssetExcludeSettings* AssetExcludeSettings = GetDefault<UPjcAssetExcludeSettings>();
	if (!AssetExcludeSettings) return;

	{
		FARFilter Filter;
		Filter.bRecursivePaths = true;
		Filter.PackagePaths.Reserve(AssetExcludeSettings->ExcludedFolders.Num());

		for (const auto& ExcludedFolder : AssetExcludeSettings->ExcludedFolders)
		{
			if (!ExcludedFolder.Path.StartsWith(PjcConstants::PathRoot.ToString())) continue;

			Filter.PackagePaths.Emplace(FName{*ExcludedFolder.Path});
		}

		if (Filter.PackagePaths.Num() > 0)
		{
			TArray<FAssetData> Assets;
			GetModuleAssetRegistry().Get().GetAssets(Filter, Assets);

			AssetsCategoryMapping[EPjcAssetCategory::Excluded].Append(Assets);
		}
	}

	{
		FARFilter Filter;
		Filter.ObjectPaths.Reserve(AssetExcludeSettings->ExcludedAssets.Num());

		for (const auto& ExcludedAsset : AssetExcludeSettings->ExcludedAssets)
		{
			if (!ExcludedAsset.LoadSynchronous() || ExcludedAsset.IsNull()) continue;

			Filter.ObjectPaths.Emplace(ExcludedAsset.ToSoftObjectPath().GetAssetPathName());
		}

		if (Filter.ObjectPaths.Num() > 0)
		{
			TArray<FAssetData> Assets;
			GetModuleAssetRegistry().Get().GetAssets(Filter, Assets);

			AssetsCategoryMapping[EPjcAssetCategory::Excluded].Append(Assets);
		}
	}
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
