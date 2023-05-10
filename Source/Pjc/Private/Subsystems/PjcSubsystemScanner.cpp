// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Subsystems/PjcSubsystemScanner.h"
#include "Subsystems/PjcSubsystemHelper.h"
#include "PjcConstants.h"
#include "Pjc.h"
// Engine Headers
#include "FileHelpers.h"
#include "Misc/FileHelper.h"
#include "Misc/ScopedSlowTask.h"
#include "Internationalization/Regex.h"

void UPjcScannerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	MapAssets.Add(EPjcAssetCategory::None);
	MapAssets.Add(EPjcAssetCategory::Any);
	MapAssets.Add(EPjcAssetCategory::Used);
	MapAssets.Add(EPjcAssetCategory::Unused);
	MapAssets.Add(EPjcAssetCategory::Primary);
	MapAssets.Add(EPjcAssetCategory::Indirect);
	MapAssets.Add(EPjcAssetCategory::Circular);
	MapAssets.Add(EPjcAssetCategory::Editor);
	MapAssets.Add(EPjcAssetCategory::Excluded);
	MapAssets.Add(EPjcAssetCategory::ExtReferenced);

	MapFiles.Add(EPjcFileCategory::None);
	MapFiles.Add(EPjcFileCategory::Any);
	MapFiles.Add(EPjcFileCategory::External);
	MapFiles.Add(EPjcFileCategory::Excluded);
	MapFiles.Add(EPjcFileCategory::Corrupted);

	MapFolders.Add(EPjcFolderCategory::None);
	MapFolders.Add(EPjcFolderCategory::Any);
	MapFolders.Add(EPjcFolderCategory::Empty);
}

void UPjcScannerSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

void UPjcScannerSubsystem::ScanProjectAssets(const FPjcAssetExcludeSettings& InExcludeSettings)
{
	const FString ScanPrepErrMsg = GetScanPreparationErrMsg();
	if (!ScanPrepErrMsg.IsEmpty())
	{
		UE_LOG(LogProjectCleaner, Warning, TEXT("%s"), *ScanPrepErrMsg);

		if (DelegateProjectAssetsScanFail.IsBound())
		{
			DelegateProjectAssetsScanFail.Broadcast(ScanPrepErrMsg);
		}

		return;
	}

	bIsIdle = false;
	const double TimeStart = FPlatformTime::Seconds();

	// resetting old cached data
	AssetsIndirectInfos.Reset();
	for (auto& Asset : MapAssets)
	{
		Asset.Value.Reset();
	}

	FScopedSlowTask SlowTaskMain{
		6.0f,
		FText::FromString(TEXT("Scanning project assets...")),
		GIsEditor && !IsRunningCommandlet()
	};
	SlowTaskMain.MakeDialog();

	// searching for indirect assets
	SlowTaskMain.EnterProgressFrame(1.0f);
	FindAssetsIndirect();

	// searching for excluded assets
	SlowTaskMain.EnterProgressFrame(1.0f);
	FindAssetsExcluded(InExcludeSettings);

	// loading primary and editor asset classes
	SlowTaskMain.EnterProgressFrame(1.0f);
	TSet<FName> ClassNamesPrimary;
	TSet<FName> ClassNamesEditor;

	UPjcHelperSubsystem::GetClassNamesPrimary(ClassNamesPrimary);
	UPjcHelperSubsystem::GetClassNamesEditor(ClassNamesEditor);

	// searching for initial used assets and also categorizing other assets
	SlowTaskMain.EnterProgressFrame(1.0f);
	TArray<FAssetData> AssetsAll;
	UPjcHelperSubsystem::GetModuleAssetRegistry().Get().GetAssetsByPath(PjcConstants::PathRoot, AssetsAll, true);

	MapAssets[EPjcAssetCategory::Used].Reserve(AssetsAll.Num());
	MapAssets[EPjcAssetCategory::Unused].Reserve(AssetsAll.Num());

	FScopedSlowTask SlowTask{
		static_cast<float>(AssetsAll.Num()),
		FText::FromString(TEXT(" ")),
		GIsEditor && !IsRunningCommandlet()
	};
	SlowTask.MakeDialog();

	for (const auto& Asset : AssetsAll)
	{
		SlowTask.EnterProgressFrame(1.0f, FText::FromString(Asset.GetFullName()));

		const FName AssetExactClassName = UPjcHelperSubsystem::GetAssetExactClassName(Asset);
		const bool bIsPrimary = ClassNamesPrimary.Contains(Asset.AssetClass) || ClassNamesPrimary.Contains(AssetExactClassName);
		const bool bIsEditor = ClassNamesEditor.Contains(Asset.AssetClass) || ClassNamesEditor.Contains(AssetExactClassName);
		const bool bIsCircular = UPjcHelperSubsystem::AssetIsCircular(Asset);
		const bool bIsExtReferenced = UPjcHelperSubsystem::AssetIsExtReferenced(Asset);
		const bool bIsUsed = bIsPrimary || bIsEditor || bIsExtReferenced;

		MapAssets[EPjcAssetCategory::Any].Emplace(Asset);

		if (bIsPrimary)
		{
			MapAssets[EPjcAssetCategory::Primary].Emplace(Asset);
		}

		if (bIsEditor)
		{
			MapAssets[EPjcAssetCategory::Editor].Emplace(Asset);
		}

		if (bIsCircular)
		{
			MapAssets[EPjcAssetCategory::Circular].Emplace(Asset);
		}

		if (bIsExtReferenced)
		{
			MapAssets[EPjcAssetCategory::ExtReferenced].Emplace(Asset);
		}

		if (bIsUsed)
		{
			MapAssets[EPjcAssetCategory::Used].Emplace(Asset);
		}
	}

	// loading all used assets dependencies recursive
	SlowTaskMain.EnterProgressFrame(1.0f);
	MapAssets[EPjcAssetCategory::Used].Append(MapAssets[EPjcAssetCategory::Indirect]);
	MapAssets[EPjcAssetCategory::Used].Append(MapAssets[EPjcAssetCategory::Excluded]);
	UPjcHelperSubsystem::GetAssetsDependencies(MapAssets[EPjcAssetCategory::Used]);

	// filtering unused assets
	SlowTaskMain.EnterProgressFrame(1.0f);
	MapAssets[EPjcAssetCategory::Unused] = MapAssets[EPjcAssetCategory::Any].Difference(MapAssets[EPjcAssetCategory::Used]);

	bIsIdle = true;

	const double TimeElapsed = FPlatformTime::Seconds() - TimeStart;
	UE_LOG(LogProjectCleaner, Display, TEXT("Project Assets Scanned In %.2f seconds"), TimeElapsed);

	if (DelegateProjectAssetsScanSuccess.IsBound())
	{
		DelegateProjectAssetsScanSuccess.Broadcast();
	}
}

void UPjcScannerSubsystem::ScanProjectAssets()
{
	const UPjcEditorAssetExcludeSettings* EditorAssetExcludeSettings = GetDefault<UPjcEditorAssetExcludeSettings>();
	if (!EditorAssetExcludeSettings) return;

	FPjcAssetExcludeSettings ExcludeSettings;
	ExcludeSettings.ExcludedFolders.Reserve(EditorAssetExcludeSettings->ExcludedFolders.Num());
	ExcludeSettings.ExcludedClasses.Reserve(EditorAssetExcludeSettings->ExcludedClasses.Num());
	ExcludeSettings.ExcludedAssets.Reserve(EditorAssetExcludeSettings->ExcludedAssets.Num());

	for (const auto& ExcludedFolder : EditorAssetExcludeSettings->ExcludedFolders)
	{
		ExcludeSettings.ExcludedFolders.Emplace(ExcludedFolder.Path);
	}

	for (const auto& ExcludedClass : EditorAssetExcludeSettings->ExcludedClasses)
	{
		if (!ExcludedClass.LoadSynchronous()) continue;

		ExcludeSettings.ExcludedClasses.Emplace(ExcludedClass.Get()->GetName());
	}

	for (const auto& ExcludedAsset : EditorAssetExcludeSettings->ExcludedAssets)
	{
		if (!ExcludedAsset.LoadSynchronous()) continue;

		ExcludeSettings.ExcludedAssets.Emplace(ExcludedAsset.ToSoftObjectPath().GetAssetPathString());
	}

	ScanProjectAssets(ExcludeSettings);
}

void UPjcScannerSubsystem::ScanProjectPaths(const FPjcFileExcludeSettings& InExcludeSettings)
{
	const double TimeStart = FPlatformTime::Seconds();

	for (auto& File : MapFiles)
	{
		File.Value.Reset();
	}

	for (auto& Folder : MapFolders)
	{
		Folder.Value.Reset();
	}

	TSet<FString> ExcludedFolders;
	TSet<FString> ExcludedFiles;
	TSet<FString> ExcludedExtensions;

	ExcludedFolders.Reserve(InExcludeSettings.ExcludedFolders.Num());
	ExcludedFiles.Reserve(InExcludeSettings.ExcludedFiles.Num());
	ExcludedExtensions.Reserve(InExcludeSettings.ExcludedExtensions.Num());

	for (const auto& ExcludedFolder : InExcludeSettings.ExcludedFolders)
	{
		const FString PathAbsolute = UPjcHelperSubsystem::PathConvertToAbsolute(ExcludedFolder);
		if (PathAbsolute.IsEmpty() || !FPaths::DirectoryExists(PathAbsolute)) continue;

		ExcludedFolders.Emplace(PathAbsolute);
	}

	for (const auto& ExcludedFile : InExcludeSettings.ExcludedFiles)
	{
		const FString PathAbsolute = UPjcHelperSubsystem::PathConvertToAbsolute(ExcludedFile);
		if (PathAbsolute.IsEmpty() || !FPaths::FileExists(PathAbsolute)) continue;

		ExcludedFiles.Emplace(PathAbsolute);
	}

	for (const auto& ExcludedExtension : InExcludeSettings.ExcludedExtensions)
	{
		if (ExcludedExtension.IsEmpty()) continue;
		const FString ExtWithoutDot = ExcludedExtension.Replace(TEXT("."), TEXT("")).ToLower();

		ExcludedExtensions.Emplace(ExtWithoutDot);
	}

	struct FContentFolderVisitor : IPlatformFile::FDirectoryVisitor
	{
		const TSet<FString>& ExcludedFolders;
		const TSet<FString>& ExcludedFiles;
		const TSet<FString>& ExcludedExtensions;
		TMap<EPjcFileCategory, TSet<FString>>& Files;
		TMap<EPjcFolderCategory, TSet<FString>>& Folders;

		explicit FContentFolderVisitor(
			const TSet<FString>& InExcludedFolders,
			const TSet<FString>& InExcludedFiles,
			const TSet<FString>& InExcludedExtensions,
			TMap<EPjcFileCategory, TSet<FString>>& InFiles,
			TMap<EPjcFolderCategory, TSet<FString>>& InFolders
		) :
			ExcludedFolders(InExcludedFolders),
			ExcludedFiles(InExcludedFiles),
			ExcludedExtensions(InExcludedExtensions),
			Files(InFiles),
			Folders(InFolders) {}

		virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory) override
		{
			const FString PathAbs = FPaths::ConvertRelativePathToFull(FilenameOrDirectory);

			if (bIsDirectory)
			{
				Folders[EPjcFolderCategory::Any].Emplace(PathAbs);

				const FString PathRel = UPjcHelperSubsystem::PathConvertToRelative(PathAbs);

				if (UPjcHelperSubsystem::PathIsEmpty(PathAbs) && !UPjcHelperSubsystem::PathIsEngineGenerated(PathAbs))
				{
					Folders[EPjcFolderCategory::Empty].Emplace(PathAbs);
				}

				return true;
			}

			Files[EPjcFileCategory::Any].Emplace(PathAbs);

			const FString FileExtension = FPaths::GetExtension(PathAbs, false).ToLower();

			if (PjcConstants::EngineFileExtensions.Contains(FileExtension))
			{
				const FString ObjectPath = UPjcHelperSubsystem::PathConvertToObjectPath(PathAbs);
				const FAssetData AssetData = UPjcHelperSubsystem::GetModuleAssetRegistry().Get().GetAssetByObjectPath(FName{*ObjectPath});

				if (!AssetData.IsValid())
				{
					Files[EPjcFileCategory::Corrupted].Emplace(PathAbs);
				}
			}
			else
			{
				Files[EPjcFileCategory::External].Emplace(PathAbs);

				if (ExcludedExtensions.Contains(FileExtension) || ExcludedFiles.Contains(PathAbs))
				{
					Files[EPjcFileCategory::Excluded].Emplace(PathAbs);
				}

				for (const auto& ExcludedFolder : ExcludedFolders)
				{
					if (FPaths::IsUnderDirectory(FPaths::GetPath(PathAbs), ExcludedFolder))
					{
						Files[EPjcFileCategory::Excluded].Emplace(PathAbs);
						break;
					}
				}
			}

			return true;
		}
	};

	FContentFolderVisitor Visitor{ExcludedFolders, ExcludedFiles, ExcludedExtensions, MapFiles, MapFolders};
	FPlatformFileManager::Get().GetPlatformFile().IterateDirectoryRecursively(*FPaths::ProjectContentDir(), Visitor);

	const double TimeElapsed = FPlatformTime::Seconds() - TimeStart;
	UE_LOG(LogProjectCleaner, Display, TEXT("Project Files Scanned In %.2f seconds"), TimeElapsed);

	if (DelegateProjectFilesScanSuccess.IsBound())
	{
		DelegateProjectFilesScanSuccess.Broadcast();
	}
}

void UPjcScannerSubsystem::ScanProjectPaths()
{
	const UPjcEditorFileExcludeSettings* EditorFileExcludeSettings = GetDefault<UPjcEditorFileExcludeSettings>();
	if (!EditorFileExcludeSettings) return;

	FPjcFileExcludeSettings FileExcludeSettings;

	FileExcludeSettings.ExcludedFolders.Reserve(EditorFileExcludeSettings->ExcludedFolders.Num());
	FileExcludeSettings.ExcludedFiles.Reserve(EditorFileExcludeSettings->ExcludedFiles.Num());
	FileExcludeSettings.ExcludedExtensions.Reserve(EditorFileExcludeSettings->ExcludedExtensions.Num());

	for (const auto& ExcludedFolder : EditorFileExcludeSettings->ExcludedFolders)
	{
		FileExcludeSettings.ExcludedFolders.Emplace(ExcludedFolder.Path);
	}

	for (const auto& ExcludedFile : EditorFileExcludeSettings->ExcludedFiles)
	{
		FileExcludeSettings.ExcludedFiles.Emplace(ExcludedFile.FilePath);
	}

	for (const auto& ExcludedExtension : EditorFileExcludeSettings->ExcludedExtensions)
	{
		FileExcludeSettings.ExcludedExtensions.Emplace(ExcludedExtension);
	}

	ScanProjectPaths(FileExcludeSettings);
}

const TSet<FAssetData>& UPjcScannerSubsystem::GetAssetsByCategory(const EPjcAssetCategory AssetCategory)
{
	check(MapAssets.Contains(AssetCategory));
	return MapAssets[AssetCategory];
}

const TSet<FString>& UPjcScannerSubsystem::GetFilesByCategory(const EPjcFileCategory FileCategory)
{
	check(MapFiles.Contains(FileCategory));
	return MapFiles[FileCategory];
}

const TSet<FString>& UPjcScannerSubsystem::GetFoldersByCategory(const EPjcFolderCategory FolderCategory)
{
	check(MapFolders.Contains(FolderCategory));
	return MapFolders[FolderCategory];
}

void UPjcScannerSubsystem::GetAssetIndirectInfo(const FAssetData& InAssetData, TArray<FPjcFileInfo>& Infos)
{
	if (!InAssetData.IsValid()) return;

	Infos.Reset();

	if (AssetsIndirectInfos.Contains(InAssetData))
	{
		Infos.Append(AssetsIndirectInfos[InAssetData]);
	}
}

FPjcDelegateProjectAssetsScanFail& UPjcScannerSubsystem::OnProjectAssetsScanFail()
{
	return DelegateProjectAssetsScanFail;
}

FPjcDelegateProjectFilesScanSuccess& UPjcScannerSubsystem::OnProjectFilesScanSuccess()
{
	return DelegateProjectFilesScanSuccess;
}

FPjcDelegateProjectAssetsScanSuccess& UPjcScannerSubsystem::OnProjectAssetsScanSuccess()
{
	return DelegateProjectAssetsScanSuccess;
}

FString UPjcScannerSubsystem::GetScanPreparationErrMsg() const
{
	if (!bIsIdle)
	{
		return TEXT("Scanner Currently Working. Please wait until it has finished and then try again.");
	}

	if (UPjcHelperSubsystem::GetModuleAssetRegistry().Get().IsLoadingAssets())
	{
		return TEXT("Scanning of the project has failed. AssetRegistry is still discovering assets. Please try again after it has finished.");
	}

	if (UPjcHelperSubsystem::EditorIsInPlayMode())
	{
		return TEXT("Scanning of the project has failed. The editor is in play mode. Please exit play mode and try again.");
	}

	if (!IsRunningCommandlet())
	{
		if (GEditor && !GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->CloseAllAssetEditors())
		{
			return TEXT("Scanning of the project has failed because not all asset editors are closed.");
		}
	}

	FixupRedirectorsInProject();

	if (UPjcHelperSubsystem::ProjectContainsRedirectors())
	{
		return TEXT("Failed to scan project, because not all redirectors are fixed.");
	}

	if (!FEditorFileUtils::SaveDirtyPackages(true, true, true, false, false, false))
	{
		return TEXT("Scanning of the project has failed because not all assets have been saved.");
	}

	return {};
}

void UPjcScannerSubsystem::FindAssetsIndirect()
{
	const FString DirSrc = FPaths::ConvertRelativePathToFull(FPaths::GameSourceDir());
	const FString DirCfg = FPaths::ConvertRelativePathToFull(FPaths::ProjectConfigDir());
	const FString DirPlg = FPaths::ConvertRelativePathToFull(FPaths::ProjectPluginsDir());

	TSet<FString> SourceFiles;
	TSet<FString> ConfigFiles;

	UPjcHelperSubsystem::GetFilesInPathByExt(DirSrc, true, false, PjcConstants::SourceFileExtensions, SourceFiles);
	UPjcHelperSubsystem::GetFilesInPathByExt(DirCfg, true, false, PjcConstants::ConfigFileExtensions, ConfigFiles);

	TSet<FString> InstalledPlugins;
	UPjcHelperSubsystem::GetFoldersInPath(DirPlg, false, InstalledPlugins);

	const FString ProjectCleanerPluginPath = DirPlg / PjcConstants::ModulePjcName.ToString();
	TSet<FString> Files;
	for (const auto& InstalledPlugin : InstalledPlugins)
	{
		// ignore ProjectCleaner plugin
		if (InstalledPlugin.Equals(ProjectCleanerPluginPath)) continue;

		UPjcHelperSubsystem::GetFilesInPathByExt(InstalledPlugin / TEXT("Source"), true, false, PjcConstants::SourceFileExtensions, Files);
		SourceFiles.Append(Files);

		Files.Reset();

		UPjcHelperSubsystem::GetFilesInPathByExt(InstalledPlugin / TEXT("Config"), true, false, PjcConstants::ConfigFileExtensions, Files);
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
			const FString ObjectPath = UPjcHelperSubsystem::PathConvertToObjectPath(FoundedAssetObjectPath);
			const FAssetData AssetData = UPjcHelperSubsystem::GetModuleAssetRegistry().Get().GetAssetByObjectPath(FName{*ObjectPath});
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

				TArray<FPjcFileInfo>& Infos = AssetsIndirectInfos.FindOrAdd(AssetData);
				Infos.AddUnique(FPjcFileInfo{FileLine, FilePathAbs});
			}
		}
	}

	TArray<FAssetData> AssetsIndirect;
	AssetsIndirectInfos.GetKeys(AssetsIndirect);
	MapAssets[EPjcAssetCategory::Indirect].Append(AssetsIndirect);
}

void UPjcScannerSubsystem::FindAssetsExcluded(const FPjcAssetExcludeSettings& InExcludeSettings)
{
	FPjcAssetSearchFilter Filter;
	Filter.bRecursivePaths = true;
	Filter.PackagePaths.Append(InExcludeSettings.ExcludedFolders);
	Filter.ClassNames.Append(InExcludeSettings.ExcludedClasses);
	Filter.ObjectPaths.Append(InExcludeSettings.ExcludedAssets);

	TArray<FAssetData> AssetsExcluded;
	UPjcHelperSubsystem::GetAssetsByFilter(Filter, AssetsExcluded);

	MapAssets[EPjcAssetCategory::Excluded].Append(AssetsExcluded);
}

void UPjcScannerSubsystem::FixupRedirectorsInProject() const
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
	UPjcHelperSubsystem::GetModuleAssetRegistry().Get().GetAssets(Filter, AssetList);

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

	UPjcHelperSubsystem::GetModuleAssetTools().Get().FixupReferencers(Redirectors, false);
}
