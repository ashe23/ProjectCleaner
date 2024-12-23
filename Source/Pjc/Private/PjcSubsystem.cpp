// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "PjcSubsystem.h"
#include "PjcConstants.h"
#include "Pjc.h"
// Engine Headers
#include "AssetManagerEditorModule.h"
#include "AssetViewUtils.h"
#include "EditorTutorial.h"
#include "EditorUtilityBlueprint.h"
#include "EditorUtilityWidget.h"
#include "EditorUtilityWidgetBlueprint.h"
#include "FileHelpers.h"
#include "ObjectTools.h"
#include "ShaderCompiler.h"
#include "Engine/AssetManager.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Internationalization/Regex.h"
#include "Misc/FileHelper.h"
#include "Misc/ScopedSlowTask.h"

void UPjcSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	bFirstScan = true;
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

void UPjcSubsystem::GetAssetsAll(TArray<FAssetData>& Assets)
{
	if (GetModuleAssetRegistry().Get().IsLoadingAssets()) return;

	Assets.Reset();

	GetModuleAssetRegistry().Get().GetAssetsByPath(PjcConstants::PathRoot, Assets, true);

	// filtering assets that are in '/Game/__ExternalActors__' and '/Game/__ExternalObjects__' folders
	FARFilter Filter;
	Filter.bRecursivePaths = true;
	Filter.PackagePaths.Add(FName{*GetPathExternalActors()});
	Filter.PackagePaths.Add(FName{*GetPathExternalObjects()});
	GetModuleAssetRegistry().Get().UseFilterToExcludeAssets(Assets, Filter);
}

void UPjcSubsystem::GetAssetsUsed(TArray<FAssetData>& Assets, const bool bShowSlowTask)
{
	if (GetModuleAssetRegistry().Get().IsLoadingAssets()) return;

	TSet<FTopLevelAssetPath> ClassNamesPrimary;
	TSet<FTopLevelAssetPath> ClassNamesEditor;
	TArray<FAssetData> AssetsAll;
	TArray<FAssetData> AssetsIndirect;
	TArray<FAssetData> AssetsExcluded;
	TArray<FAssetData> AssetsMegascans;
	TArray<FPjcAssetIndirectInfo> AssetsIndirectInfos;

	GetAssetsAll(AssetsAll);
	GetAssetsIndirect(AssetsIndirect, AssetsIndirectInfos, bShowSlowTask);
	GetAssetsExcluded(AssetsExcluded, bShowSlowTask);
	GetClassNamesPrimary(ClassNamesPrimary);
	GetClassNamesEditor(ClassNamesEditor);

	if (FModuleManager::Get().IsModuleLoaded(PjcConstants::ModuleMegascans))
	{
		GetModuleAssetRegistry().Get().GetAssetsByPath(PjcConstants::PathMSPresets, AssetsMegascans, true);
	}

	TSet<FAssetData> AssetsUsed;
	AssetsUsed.Append(AssetsIndirect);
	AssetsUsed.Append(AssetsExcluded);
	AssetsUsed.Append(AssetsMegascans);

	FScopedSlowTask SlowTask{
		static_cast<float>(AssetsAll.Num()),
		FText::FromString(TEXT("Searching used assets...")),
		bShowSlowTask && GIsEditor && !IsRunningCommandlet()
	};
	SlowTask.MakeDialog(false, false);

	for (const auto& Asset : AssetsAll)
	{
		SlowTask.EnterProgressFrame(1.0f, FText::FromString(Asset.GetFullName()));

		const FTopLevelAssetPath AssetExactClassName = GetAssetExactClassName(Asset);
		const bool bIsPrimary = ClassNamesPrimary.Contains(Asset.AssetClassPath) || ClassNamesPrimary.Contains(
			AssetExactClassName);
		const bool bIsEditor = ClassNamesEditor.Contains(Asset.AssetClassPath) || ClassNamesEditor.Contains(
			AssetExactClassName);
		const bool bIsExtReferenced = AssetIsExtReferenced(Asset);
		const bool bIsUsed = bIsPrimary || bIsEditor || bIsExtReferenced;

		if (bIsUsed)
		{
			AssetsUsed.Emplace(Asset);
		}
	}

	GetAssetsDependencies(AssetsUsed);

	Assets.Reset();
	Assets = AssetsUsed.Array();
}

void UPjcSubsystem::GetAssetsUnused(TArray<FAssetData>& Assets, const bool bShowSlowTask)
{
	if (GetModuleAssetRegistry().Get().IsLoadingAssets()) return;

	TArray<FAssetData> AssetsAll;
	TArray<FAssetData> AssetsUsed;

	GetAssetsAll(AssetsAll);
	GetAssetsUsed(AssetsUsed, bShowSlowTask);

	const TSet<FAssetData> AssetsUsedSet{AssetsUsed};
	Assets.Reset(AssetsAll.Num());

	FScopedSlowTask SlowTask{
		static_cast<float>(AssetsAll.Num()),
		FText::FromString(TEXT("Searching unused assets...")),
		bShowSlowTask && GIsEditor && !IsRunningCommandlet()
	};
	SlowTask.MakeDialog(false, false);

	for (const auto& Asset : AssetsAll)
	{
		SlowTask.EnterProgressFrame(1.0f, FText::FromString(Asset.GetFullName()));

		if (!AssetsUsedSet.Contains(Asset))
		{
			Assets.Emplace(Asset);
		}
	}
	Assets.Shrink();
}

void UPjcSubsystem::GetAssetsPrimary(TArray<FAssetData>& Assets, const bool bShowSlowTask)
{
	if (GetModuleAssetRegistry().Get().IsLoadingAssets()) return;

	TArray<FAssetData> AssetsAll;
	GetAssetsAll(AssetsAll);

	TSet<FTopLevelAssetPath> ClassNamesPrimary;
	GetClassNamesPrimary(ClassNamesPrimary);

	Assets.Reset(AssetsAll.Num());

	FScopedSlowTask SlowTask{
		static_cast<float>(AssetsAll.Num()),
		FText::FromString(TEXT("Searching primary assets...")),
		bShowSlowTask && GIsEditor && !IsRunningCommandlet()
	};
	SlowTask.MakeDialog(false, false);

	for (const auto& Asset : AssetsAll)
	{
		SlowTask.EnterProgressFrame(1.0f, FText::FromString(Asset.GetFullName()));

		if (ClassNamesPrimary.Contains(Asset.AssetClassPath) || ClassNamesPrimary.Contains(
			GetAssetExactClassName(Asset)))
		{
			Assets.Emplace(Asset);
		}
	}

	Assets.Shrink();
}

void UPjcSubsystem::GetAssetsIndirect(TArray<FAssetData>& Assets, TArray<FPjcAssetIndirectInfo>& AssetsIndirectInfos,
                                      const bool bShowSlowTask)
{
	if (GetModuleAssetRegistry().Get().IsLoadingAssets()) return;

	Assets.Reset();
	AssetsIndirectInfos.Reset();

	TSet<FString> ScanFiles;
	GetSourceAndConfigFiles(ScanFiles);

	FScopedSlowTask SlowTask{
		static_cast<float>(ScanFiles.Num()),
		FText::FromString(TEXT("Searching Indirectly used assets...")),
		bShowSlowTask && GIsEditor && !IsRunningCommandlet()
	};
	SlowTask.MakeDialog(false, false);

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
			if (ObjectPath.IsEmpty()) continue;

			const FAssetData AssetData = GetModuleAssetRegistry().Get().GetAssetByObjectPath(
				FSoftObjectPath{ObjectPath});
			if (!AssetData.IsValid() || FolderIsExternal(AssetData.PackagePath.ToString())) continue;

			// if founded asset is ok, we loading file lines to determine on what line its used
			TArray<FString> Lines;
			FFileHelper::LoadFileToStringArray(Lines, *File);

			for (int32 i = 0; i < Lines.Num(); ++i)
			{
				if (!Lines.IsValidIndex(i)) continue;
				if (!Lines[i].Contains(FoundedAssetObjectPath)) continue;

				const FString FilePathAbs = FPaths::ConvertRelativePathToFull(File);
				const int32 FileLine = i + 1;

				AssetsIndirectInfos.AddUnique(FPjcAssetIndirectInfo{AssetData, FilePathAbs, FileLine});
				Assets.AddUnique(AssetData);
			}
		}
	}
}

void UPjcSubsystem::GetAssetsCircular(TArray<FAssetData>& Assets, const bool bShowSlowTask)
{
	if (GetModuleAssetRegistry().Get().IsLoadingAssets()) return;

	TArray<FAssetData> AssetsAll;
	GetAssetsAll(AssetsAll);

	Assets.Reset(AssetsAll.Num());

	FScopedSlowTask SlowTask{
		static_cast<float>(AssetsAll.Num()),
		FText::FromString(TEXT("Searching circular assets...")),
		bShowSlowTask && GIsEditor && !IsRunningCommandlet()
	};
	SlowTask.MakeDialog(false, false);

	for (const auto& Asset : AssetsAll)
	{
		SlowTask.EnterProgressFrame(1.0f, FText::FromString(Asset.GetFullName()));

		if (AssetIsCircular(Asset))
		{
			Assets.Emplace(Asset);
		}
	}

	Assets.Shrink();
}

void UPjcSubsystem::GetAssetsEditor(TArray<FAssetData>& Assets, const bool bShowSlowTask)
{
	if (GetModuleAssetRegistry().Get().IsLoadingAssets()) return;

	TArray<FAssetData> AssetsAll;
	GetAssetsAll(AssetsAll);

	TSet<FTopLevelAssetPath> ClassNamesEditor;
	GetClassNamesEditor(ClassNamesEditor);

	Assets.Reset(AssetsAll.Num());

	FScopedSlowTask SlowTask{
		static_cast<float>(AssetsAll.Num()),
		FText::FromString(TEXT("Searching editor assets...")),
		bShowSlowTask && GIsEditor && !IsRunningCommandlet()
	};
	SlowTask.MakeDialog(false, false);

	for (const auto& Asset : AssetsAll)
	{
		SlowTask.EnterProgressFrame(1.0f, FText::FromString(Asset.GetFullName()));

		if (ClassNamesEditor.Contains(Asset.AssetClassPath) || ClassNamesEditor.Contains(GetAssetExactClassName(Asset)))
		{
			Assets.Emplace(Asset);
		}
	}

	Assets.Shrink();
}

void UPjcSubsystem::GetAssetsExcluded(TArray<FAssetData>& Assets, const bool bShowSlowTask)
{
	if (GetModuleAssetRegistry().Get().IsLoadingAssets()) return;

	Assets.Reset();

	const UPjcAssetExcludeSettings* AssetExcludeSettings = GetDefault<UPjcAssetExcludeSettings>();
	if (!AssetExcludeSettings) return;

	FScopedSlowTask SlowTask{
		4.0f,
		FText::FromString(TEXT("Searching excluded assets...")),
		bShowSlowTask && GIsEditor && !IsRunningCommandlet()
	};
	SlowTask.MakeDialog(false, false);
	SlowTask.EnterProgressFrame(1.0f);

	TArray<FAssetData> AssetsAll;
	GetAssetsAll(AssetsAll);

	TSet<FAssetData> AssetsExcluded;
	AssetsExcluded.Reserve(AssetsAll.Num());

	SlowTask.EnterProgressFrame(1.0f);
	// assets excluded by class names
	{
		TSet<FTopLevelAssetPath> ClassNamesExcluded;
		ClassNamesExcluded.Reserve(AssetExcludeSettings->ExcludedClasses.Num());

		for (const auto& ExcludedClass : AssetExcludeSettings->ExcludedClasses)
		{
			if (!ExcludedClass.LoadSynchronous() || ExcludedClass.IsNull()) continue;

			ClassNamesExcluded.Emplace(ExcludedClass.Get()->GetClassPathName());
		}

		for (const auto& Asset : AssetsAll)
		{
			if (ClassNamesExcluded.Contains(Asset.AssetClassPath) || ClassNamesExcluded.Contains(
				GetAssetExactClassName(Asset)))
			{
				AssetsExcluded.Emplace(Asset);
			}
		}
	}

	SlowTask.EnterProgressFrame(1.0f);
	// assets excluded by package paths
	{
		FARFilter Filter;

		Filter.bRecursivePaths = true;
		Filter.PackagePaths.Reserve(AssetExcludeSettings->ExcludedFolders.Num());

		for (const auto& ExcludedFolder : AssetExcludeSettings->ExcludedFolders)
		{
			if (!ExcludedFolder.Path.StartsWith(PjcConstants::PathRoot.ToString())) continue;

			Filter.PackagePaths.Emplace(ExcludedFolder.Path);
		}

		if (Filter.PackagePaths.Num() > 0)
		{
			TArray<FAssetData> AssetsExcludedByPackagePaths;
			GetModuleAssetRegistry().Get().GetAssets(Filter, AssetsExcludedByPackagePaths);

			AssetsExcluded.Append(AssetsExcludedByPackagePaths);
		}
	}

	SlowTask.EnterProgressFrame(1.0f);
	// assets excluded by object paths
	{
		FARFilter Filter;

		Filter.SoftObjectPaths.Reserve(AssetExcludeSettings->ExcludedAssets.Num());

		for (const auto& ExcludedAsset : AssetExcludeSettings->ExcludedAssets)
		{
			if (!ExcludedAsset.LoadSynchronous()) continue;

			Filter.SoftObjectPaths.Emplace(ExcludedAsset.ToSoftObjectPath());
		}

		if (Filter.SoftObjectPaths.Num() > 0)
		{
			TArray<FAssetData> AssetsExcludedByObjectPaths;
			GetModuleAssetRegistry().Get().GetAssets(Filter, AssetsExcludedByObjectPaths);

			AssetsExcluded.Append(AssetsExcludedByObjectPaths);
		}
	}

	Assets = AssetsExcluded.Array();
}

void UPjcSubsystem::GetAssetsExtReferenced(TArray<FAssetData>& Assets, const bool bShowSlowTask)
{
	if (GetModuleAssetRegistry().Get().IsLoadingAssets()) return;

	TArray<FAssetData> AssetsAll;
	GetAssetsAll(AssetsAll);

	Assets.Reset(Assets.Num());

	FScopedSlowTask SlowTask{
		static_cast<float>(AssetsAll.Num()),
		FText::FromString(TEXT("Searching assets with external referencers...")),
		bShowSlowTask && GIsEditor && !IsRunningCommandlet()
	};
	SlowTask.MakeDialog(false, false);

	for (const auto& Asset : AssetsAll)
	{
		SlowTask.EnterProgressFrame(1.0f, FText::FromString(Asset.GetFullName()));

		if (AssetIsExtReferenced(Asset))
		{
			Assets.Emplace(Asset);
		}
	}

	Assets.Shrink();
}

void UPjcSubsystem::GetClassNamesPrimary(TSet<FTopLevelAssetPath>& ClassNames)
{
	// getting list of primary asset classes that are defined in AssetManager
	const auto& AssetManager = UAssetManager::Get();
	if (!AssetManager.IsInitialized()) return;

	TSet<FTopLevelAssetPath> ClassNamesPrimaryBase;
	TArray<FPrimaryAssetTypeInfo> AssetTypeInfos;
	AssetManager.Get().GetPrimaryAssetTypeInfoList(AssetTypeInfos);
	ClassNamesPrimaryBase.Reserve(AssetTypeInfos.Num());

	for (const auto& AssetTypeInfo : AssetTypeInfos)
	{
		if (!AssetTypeInfo.AssetBaseClassLoaded) continue;

		ClassNamesPrimaryBase.Emplace(FTopLevelAssetPath{AssetTypeInfo.AssetBaseClassLoaded.Get()});
	}

	// getting list of primary assets classes that are derived from main primary assets
	ClassNames.Empty();
	GetModuleAssetRegistry().Get().GetDerivedClassNames(ClassNamesPrimaryBase.Array(), TSet<FTopLevelAssetPath>{},
	                                                    ClassNames);
}

void UPjcSubsystem::GetClassNamesEditor(TSet<FTopLevelAssetPath>& ClassNames)
{
	const TArray ClassNamesEditorBase{
		UEditorUtilityWidget::StaticClass()->GetClassPathName(),
		UEditorUtilityBlueprint::StaticClass()->GetClassPathName(),
		UEditorUtilityWidgetBlueprint::StaticClass()->GetClassPathName(),
		UEditorTutorial::StaticClass()->GetClassPathName()
	};

	ClassNames.Empty();
	GetModuleAssetRegistry().Get().GetDerivedClassNames(ClassNamesEditorBase, TSet<FTopLevelAssetPath>{}, ClassNames);
}

void UPjcSubsystem::GetClassNamesExcluded(TSet<FTopLevelAssetPath>& ClassNames)
{
	const UPjcAssetExcludeSettings* AssetExcludeSettings = GetDefault<UPjcAssetExcludeSettings>();
	if (!AssetExcludeSettings) return;

	ClassNames.Empty(AssetExcludeSettings->ExcludedClasses.Num());
	for (const auto& ExcludedClass : AssetExcludeSettings->ExcludedClasses)
	{
		if (!ExcludedClass.LoadSynchronous() || ExcludedClass.IsNull()) continue;

		ClassNames.Emplace(ExcludedClass.Get()->GetClassPathName());
	}
}

void UPjcSubsystem::GetFiles(const FString& InSearchPath, const bool bSearchRecursive, TArray<FString>& OutFiles)
{
	OutFiles.Empty();

	struct FFindFilesVisitor : IPlatformFile::FDirectoryVisitor
	{
		TArray<FString>& Files;

		explicit FFindFilesVisitor(TArray<FString>& InFiles) : Files(InFiles)
		{
		}

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

void UPjcSubsystem::GetFilesByExt(const FString& InSearchPath, const bool bSearchRecursive, const bool bExtSearchInvert,
                                  const TSet<FString>& InExtensions, TArray<FString>& OutFiles)
{
	OutFiles.Empty();

	struct FFindFilesVisitor : IPlatformFile::FDirectoryVisitor
	{
		const bool bSearchInvert;
		TArray<FString>& Files;
		const TSet<FString>& Extensions;

		explicit FFindFilesVisitor(const bool bInSearchInvert, TArray<FString>& InFiles,
		                           const TSet<FString>& InExtensions)
			: bSearchInvert(bInSearchInvert),
			  Files(InFiles),
			  Extensions(InExtensions)
		{
		}

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
					(
						bExistsInSearchList && !bSearchInvert
					)
					||
					(
						!bExistsInSearchList && bSearchInvert
					)
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

void UPjcSubsystem::GetFilesExternalAll(TArray<FString>& Files)
{
	GetFilesByExt(
		FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir()),
		true,
		true,
		PjcConstants::EngineFileExtensions,
		Files
	);
}

void UPjcSubsystem::GetFilesExternalFiltered(TArray<FString>& Files, const bool bShowSlowTask)
{
	const UPjcFileExcludeSettings* FileExcludeSettings = GetDefault<UPjcFileExcludeSettings>();
	if (!FileExcludeSettings) return;

	TArray<FString> FilesExternalAll;
	GetFilesExternalAll(FilesExternalAll);

	Files.Reset(FilesExternalAll.Num());

	FScopedSlowTask SlowTask(
		static_cast<float>(FilesExternalAll.Num()),
		FText::FromString(TEXT("Searching for external files...")),
		bShowSlowTask && GIsEditor && !IsRunningCommandlet()
	);
	SlowTask.MakeDialog(false, false);

	for (const auto& File : FilesExternalAll)
	{
		SlowTask.EnterProgressFrame(1.0f, FText::FromString(File));

		const FString FileExt = FPaths::GetExtension(File, false).ToLower();
		const bool bExcludedByExt = FileExcludeSettings->ExcludedExtensions.Contains(FileExt);
		const bool bExcludedByFile = FileExcludeSettings->ExcludedFiles.ContainsByPredicate(
			[&](const FFilePath& InFilePath)
			{
				if (!InFilePath.FilePath.StartsWith(TEXT("Content"))) return false;

				const FString Path = FPaths::ProjectDir() / InFilePath.FilePath;
				const FString PathAbs = PathConvertToAbsolute(Path);
				if (PathAbs.IsEmpty()) return false;

				return File.Equals(PathAbs);
			});

		if (bExcludedByExt || bExcludedByFile) continue;

		Files.Emplace(File);
	}

	Files.Shrink();
}

void UPjcSubsystem::GetFilesExternalExcluded(TArray<FString>& Files, const bool bShowSlowTask)
{
	const UPjcFileExcludeSettings* FileExcludeSettings = GetDefault<UPjcFileExcludeSettings>();
	if (!FileExcludeSettings) return;

	TArray<FString> FilesExternalAll;
	GetFilesExternalAll(FilesExternalAll);

	Files.Reset(FilesExternalAll.Num());

	FScopedSlowTask SlowTask(
		static_cast<float>(FilesExternalAll.Num()),
		FText::FromString(TEXT("Searching for external files...")),
		bShowSlowTask && GIsEditor && !IsRunningCommandlet()
	);
	SlowTask.MakeDialog(false, false);

	for (const auto& File : FilesExternalAll)
	{
		SlowTask.EnterProgressFrame(1.0f, FText::FromString(File));

		const FString FileExt = FPaths::GetExtension(File, false).ToLower();
		const bool bExcludedByExt = FileExcludeSettings->ExcludedExtensions.Contains(FileExt);
		const bool bExcludedByFile = FileExcludeSettings->ExcludedFiles.ContainsByPredicate(
			[&](const FFilePath& InFilePath)
			{
				if (!InFilePath.FilePath.StartsWith(TEXT("Content"))) return false;

				const FString Path = FPaths::ProjectDir() / InFilePath.FilePath;
				const FString PathAbs = PathConvertToAbsolute(Path);
				if (PathAbs.IsEmpty()) return false;

				return File.Equals(PathAbs);
			});

		if (bExcludedByExt || bExcludedByFile)
		{
			Files.Emplace(File);
		}
	}

	Files.Shrink();
}

void UPjcSubsystem::GetFilesCorrupted(TArray<FString>& Files, const bool bShowSlowTask)
{
	TArray<FString> FileAssets;

	GetFilesByExt(
		FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir()),
		true,
		false,
		PjcConstants::EngineFileExtensions,
		FileAssets
	);

	Files.Reset(FileAssets.Num());

	FScopedSlowTask SlowTask(
		static_cast<float>(FileAssets.Num()),
		FText::FromString(TEXT("Searching for corrupted asset files...")),
		bShowSlowTask && GIsEditor && !IsRunningCommandlet()
	);
	SlowTask.MakeDialog(false, false);

	for (const auto& File : FileAssets)
	{
		SlowTask.EnterProgressFrame(1.0f, FText::FromString(File));

		if (FolderIsExternal(PathConvertToRelative(FPaths::GetPath(File)))) continue;

		const FString Path = PathConvertToObjectPath(File);
		if (Path.IsEmpty()) continue;
		if (GetModuleAssetRegistry().Get().GetAssetByObjectPath(FSoftObjectPath{Path}).IsValid()) continue;

		Files.Emplace(File);
	}

	Files.Shrink();
}

void UPjcSubsystem::GetFolders(const FString& InSearchPath, const bool bSearchRecursive, TArray<FString>& OutFolders)
{
	OutFolders.Empty();

	struct FFindFoldersVisitor : IPlatformFile::FDirectoryVisitor
	{
		TArray<FString>& Folders;

		explicit FFindFoldersVisitor(TArray<FString>& InFolders) : Folders(InFolders)
		{
		}

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

void UPjcSubsystem::GetFoldersEmpty(TArray<FString>& Folders)
{
	TArray<FString> FoldersAll;
	GetFolders(FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir()), true, FoldersAll);

	Folders.Reset(FoldersAll.Num());

	for (const auto& Folder : FoldersAll)
	{
		if (FolderIsEmpty(Folder) && !FolderIsEngineGenerated(Folder) && !FolderIsExcluded(Folder))
		{
			Folders.Emplace(Folder);
		}
	}

	Folders.Shrink();
}

void UPjcSubsystem::DeleteAssetsUnused(const bool bShowSlowTask, const bool bShowEditorNotification)
{
	if (GetModuleAssetRegistry().Get().IsLoadingAssets())
	{
		UE_LOG(LogProjectCleaner, Warning,
		       TEXT("Failed to delete unused assets, because AssetRegistry still discovering assets."));
		UE_LOG(LogProjectCleaner, Warning, TEXT("Please wait until it finished adn then try again."));
		return;
	}

	if (GEditor && !GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->CloseAllAssetEditors())
	{
		UE_LOG(LogProjectCleaner, Warning,
		       TEXT("Failed to delete unused assets because some editor windows are still open."));
		UE_LOG(LogProjectCleaner, Warning,
		       TEXT(
			       "Please try again, check the OutputLog for reasons why certain windows haven't closed, or try to manually close all editor windows."
		       ));
		return;
	}

	TArray<FAssetData> Redirectors;
	GetProjectRedirectors(Redirectors);
	FixProjectRedirectors(Redirectors);

	if (ProjectHasRedirectors())
	{
		UE_LOG(LogProjectCleaner, Warning,
		       TEXT("Failed to delete unused assets because project contains redirectors that failed to fix."));
		UE_LOG(LogProjectCleaner, Warning, TEXT("Please fix redirectors first manually and then try again."));
		return;
	}

	if (!FEditorFileUtils::SaveDirtyPackages(true, true, true, false, false, false))
	{
		UE_LOG(LogProjectCleaner, Warning,
		       TEXT("Failed to delete unused assets because project contains unsaved assets."));
		UE_LOG(LogProjectCleaner, Warning,
		       TEXT("Please save those assets and then try again. Check OutputLog for more information"));
		return;
	}

	TArray<FAssetData> AssetsUnused;
	GetAssetsUnused(AssetsUnused);

	if (AssetsUnused.Num() == 0)
	{
		UE_LOG(LogProjectCleaner, Warning,
		       TEXT("The project currently contains no unused assets, thus there are no items to delete."));
		return;
	}

	constexpr int32 BucketSize = PjcConstants::BucketSize;
	const int32 NumAssetsTotal = AssetsUnused.Num();
	int32 NumAssetsDeleted = 0;

	TArray<FAssetData> Bucket;
	TArray<UObject*> LoadedAssets;
	LoadedAssets.Reserve(BucketSize);
	Bucket.Reserve(BucketSize);

	// Assets must be loaded first when deleting, which can cause a lot of unnecessary shader compilation work.
	// Therefore, we disable shader compilation during this stage for faster deletion and then enable it afterwards.
	ShaderCompilationDisable();

	FScopedSlowTask SlowTask(
		AssetsUnused.Num(),
		FText::FromString(TEXT("Deleting unused assets...")),
		bShowSlowTask && GIsEditor && !IsRunningCommandlet()
	);
	SlowTask.MakeDialog(false, false);

	bool bErrors = false;

	while (AssetsUnused.Num() > 0)
	{
		BucketFill(AssetsUnused, Bucket, BucketSize);

		if (Bucket.Num() == 0)
		{
			break;
		}

		if (!BucketPrepare(Bucket, LoadedAssets))
		{
			bErrors = true;
			UE_LOG(LogProjectCleaner, Error, TEXT("Failed to load some assets. Aborting."));
			break;
		}

		NumAssetsDeleted += BucketDelete(LoadedAssets);
		const FString ProgressMsg = FString::Printf(TEXT("Deleted %d of %d assets"), NumAssetsDeleted, NumAssetsTotal);
		SlowTask.EnterProgressFrame(Bucket.Num(), FText::FromString(ProgressMsg));

		Bucket.Reset();
		LoadedAssets.Reset();
	}

	// const TSet<FName> EmptyPackages = GetModuleAssetRegistry().Get().GetCachedEmptyPackages();
	const TSet<FName> EmptyPackages = GetModuleAssetRegistry().Get().GetCachedEmptyPackagesCopy();
	TArray<UPackage*> AssetPackages;
	for (const auto& EmptyPackage : EmptyPackages)
	{
		UPackage* Package = FindPackage(nullptr, *EmptyPackage.ToString());
		if (Package && Package->IsValidLowLevel())
		{
			AssetPackages.Add(Package);
		}
	}

	if (AssetPackages.Num() > 0)
	{
		ObjectTools::CleanupAfterSuccessfulDelete(AssetPackages);
	}

	ShaderCompilationEnable();

	const FString Msg = FString::Printf(TEXT("Deleted %d of %d assets"), NumAssetsDeleted, NumAssetsTotal);
	UE_LOG(LogProjectCleaner, Display, TEXT("%s"), *Msg);

	if (bErrors)
	{
		UE_LOG(LogProjectCleaner, Error,
		       TEXT("There very some errors while deleting assets. Please check OutputLog for more information."));
	}

	if (bShowEditorNotification && GEditor)
	{
		if (NumAssetsDeleted == NumAssetsTotal)
		{
			ShowNotification(Msg, SNotificationItem::CS_Success, 3.0f);
		}
		else
		{
			ShowNotificationWithOutputLog(Msg, SNotificationItem::CS_Fail, 5.0f);
		}
	}
}

void UPjcSubsystem::DeleteFoldersEmpty(const bool bShowSlowTask, const bool bShowEditorNotification)
{
	TArray<FString> FoldersEmpty;
	GetFoldersEmpty(FoldersEmpty);

	if (FoldersEmpty.Num() == 0)
	{
		UE_LOG(LogProjectCleaner, Warning,
		       TEXT("The project currently contains no empty folders, thus there are no items to delete."));
		return;
	}

	FScopedSlowTask SlowTaskMain(
		1.0f,
		FText::FromString(TEXT("Deleting empty folders...")),
		bShowSlowTask && GIsEditor && !IsRunningCommandlet()
	);
	SlowTaskMain.MakeDialog(false, false);
	SlowTaskMain.EnterProgressFrame(1.0f);

	FScopedSlowTask SlowTask(
		FoldersEmpty.Num(),
		FText::FromString(TEXT(" ")),
		bShowSlowTask && GIsEditor && !IsRunningCommandlet()
	);
	SlowTask.MakeDialog(false, false);

	bool bErrors = false;

	const int32 NumFoldersTotal = FoldersEmpty.Num();
	int32 NumFoldersDeleted = 0;

	for (const auto& Folder : FoldersEmpty)
	{
		SlowTask.EnterProgressFrame(1.0f, FText::FromString(Folder));

		if (!IFileManager::Get().DeleteDirectory(*Folder, true, true))
		{
			bErrors = true;
			UE_LOG(LogProjectCleaner, Error, TEXT("Failed to delete folder: %s"), *Folder);
			continue;
		}

		++NumFoldersDeleted;

		// if folder deleted successfully, we must remove it from cached AssetRegistry paths also.
		GetModuleAssetRegistry().Get().RemovePath(PathConvertToRelative(Folder));
	}

	const FString Msg = FString::Printf(TEXT("Deleted %d of %d empty folders"), NumFoldersDeleted, NumFoldersTotal);
	UE_LOG(LogProjectCleaner, Display, TEXT("%s"), *Msg);

	if (bErrors)
	{
		UE_LOG(LogProjectCleaner, Error,
		       TEXT("Failed to delete some folders. Please check OutputLog for more information"));
	}

	if (bShowEditorNotification && GEditor)
	{
		if (NumFoldersDeleted == NumFoldersTotal)
		{
			ShowNotification(Msg, SNotificationItem::CS_Success, 3.0f);
		}
		else
		{
			ShowNotificationWithOutputLog(Msg, SNotificationItem::CS_Fail, 5.0f);
		}
	}
}

void UPjcSubsystem::DeleteFilesExternal(const bool bShowSlowTask, const bool bShowEditorNotification)
{
	TArray<FString> FilesExternalFiltered;
	GetFilesExternalFiltered(FilesExternalFiltered);

	if (FilesExternalFiltered.Num() == 0)
	{
		UE_LOG(LogProjectCleaner, Warning,
		       TEXT("The project currently contains no external files, thus there are no items to delete."));
		return;
	}

	FScopedSlowTask SlowTask(
		FilesExternalFiltered.Num(),
		FText::FromString(TEXT("Deleting external files ...")),
		bShowSlowTask && GIsEditor && !IsRunningCommandlet()
	);
	SlowTask.MakeDialog(false, false);

	bool bErrors = false;

	const int32 NumFilesTotal = FilesExternalFiltered.Num();
	int32 NumFilesDeleted = 0;

	for (const auto& File : FilesExternalFiltered)
	{
		SlowTask.EnterProgressFrame(1.0f, FText::FromString(File));

		if (!IFileManager::Get().Delete(*File, true))
		{
			bErrors = true;
			UE_LOG(LogProjectCleaner, Error, TEXT("Failed to delete file: %s"), *File);
		}

		++NumFilesDeleted;
	}

	const FString Msg = FString::Printf(TEXT("Deleted %d of %d external files"), NumFilesDeleted, NumFilesTotal);
	UE_LOG(LogProjectCleaner, Display, TEXT("%s"), *Msg);

	if (bErrors)
	{
		UE_LOG(LogProjectCleaner, Error,
		       TEXT("Failed to delete some files. Please check OutputLog for more information"));
	}

	if (bShowEditorNotification && GEditor)
	{
		if (NumFilesDeleted == NumFilesTotal)
		{
			ShowNotification(Msg, SNotificationItem::CS_Success, 3.0f);
		}
		else
		{
			ShowNotificationWithOutputLog(Msg, SNotificationItem::CS_Fail, 5.0f);
		}
	}
}

void UPjcSubsystem::DeleteFilesCorrupted(const bool bShowSlowTask, const bool bShowEditorNotification)
{
	TArray<FString> FilesCorrupted;
	GetFilesCorrupted(FilesCorrupted);

	if (FilesCorrupted.Num() == 0)
	{
		UE_LOG(LogProjectCleaner, Warning,
		       TEXT("The project currently contains no corrupted files, thus there are no items to delete."));
		return;
	}

	FScopedSlowTask SlowTask(
		FilesCorrupted.Num(),
		FText::FromString(TEXT("Deleting corrupted asset files ...")),
		bShowSlowTask && GIsEditor && !IsRunningCommandlet()
	);
	SlowTask.MakeDialog(false, false);

	bool bErrors = false;

	const int32 NumFilesTotal = FilesCorrupted.Num();
	int32 NumFilesDeleted = 0;

	for (const auto& File : FilesCorrupted)
	{
		SlowTask.EnterProgressFrame(1.0f, FText::FromString(File));

		if (!IFileManager::Get().Delete(*File, true))
		{
			bErrors = true;
			UE_LOG(LogProjectCleaner, Error, TEXT("Failed to delete file: %s"), *File);
		}

		++NumFilesDeleted;
	}

	const FString Msg = FString::Printf(TEXT("Deleted %d of %d corrupted files"), NumFilesDeleted, NumFilesTotal);
	UE_LOG(LogProjectCleaner, Display, TEXT("%s"), *Msg);

	if (bErrors)
	{
		UE_LOG(LogProjectCleaner, Error,
		       TEXT("Failed to delete some files. Please check OutputLog for more information"));
	}

	if (bShowEditorNotification && GEditor)
	{
		if (NumFilesDeleted == NumFilesTotal)
		{
			ShowNotification(Msg, SNotificationItem::CS_Success, 3.0f);
		}
		else
		{
			ShowNotificationWithOutputLog(Msg, SNotificationItem::CS_Fail, 5.0f);
		}
	}
}

void UPjcSubsystem::GetProjectRedirectors(TArray<FAssetData>& Redirectors)
{
	FARFilter Filter;
	Filter.bRecursivePaths = true;
	Filter.PackagePaths.Emplace(PjcConstants::PathRoot);
	Filter.ClassPaths.Emplace(UObjectRedirector::StaticClass()->GetClassPathName());

	Redirectors.Reset();
	GetModuleAssetRegistry().Get().GetAssets(Filter, Redirectors);
}

bool UPjcSubsystem::ProjectHasRedirectors()
{
	TArray<FAssetData> Redirectors;
	GetProjectRedirectors(Redirectors);

	return Redirectors.Num() > 0;
}

void UPjcSubsystem::FixProjectRedirectors(const TArray<FAssetData>& Redirectors, const bool bShowSlowTask)
{
	if (Redirectors.Num() == 0) return;

	FScopedSlowTask SlowTask{
		static_cast<float>(Redirectors.Num()),
		FText::FromString(TEXT("Fixing redirectors...")),
		bShowSlowTask && GIsEditor && !IsRunningCommandlet()
	};
	SlowTask.MakeDialog(false, false);

	TArray<UObjectRedirector*> RedirectorObjects;
	RedirectorObjects.Reserve(Redirectors.Num());

	for (const auto& Redirector : Redirectors)
	{
		SlowTask.EnterProgressFrame(1.0f, FText::FromString(Redirector.GetFullName()));

		UObjectRedirector* RedirectorObject = CastChecked<UObjectRedirector>(Redirector.GetAsset());
		if (!RedirectorObject) continue;

		RedirectorObjects.Emplace(RedirectorObject);
	}

	GetModuleAssetTools().Get().FixupReferencers(RedirectorObjects, false);
}

bool UPjcSubsystem::EditorIsInPlayMode()
{
	return ( GEditor && GEditor->PlayWorld ) || GIsPlayInEditorWorld;
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
		return !Ref.ToString().StartsWith(PjcConstants::PathRoot.ToString()) || FolderIsExternal(Ref.ToString());
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

	FString ObjectPath = FPackageName::ExportTextPathToObjectPath(InPath);
	ObjectPath.RemoveFromEnd(TEXT("_C")); // we should remove _C prefix if its blueprint asset

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

int64 UPjcSubsystem::GetAssetSize(const FAssetData& InAsset)
{
	if (!InAsset.IsValid()) return 0;

	const auto& AssetPackageData = GetModuleAssetRegistry().Get().GetAssetPackageDataCopy(InAsset.PackageName);
	if (!AssetPackageData.IsSet()) return 0;

	return AssetPackageData->DiskSize;
}

int64 UPjcSubsystem::GetAssetsTotalSize(const TArray<FAssetData>& InAssets)
{
	int64 Size = 0;

	for (const auto& Asset : InAssets)
	{
		if (!Asset.IsValid()) continue;

		const auto& AssetPackageData = GetModuleAssetRegistry().Get().GetAssetPackageDataCopy(Asset.PackageName);
		if (!AssetPackageData.IsSet()) continue;

		Size += AssetPackageData->DiskSize;
	}

	return Size;
}

int64 UPjcSubsystem::GetFileSize(const FString& InFile)
{
	if (InFile.IsEmpty() || !FPaths::FileExists(InFile)) return 0;

	return IFileManager::Get().FileSize(*InFile);
}

int64 UPjcSubsystem::GetFilesTotalSize(const TArray<FString>& Files)
{
	int64 Size = 0;

	for (const auto& File : Files)
	{
		if (File.IsEmpty() || !FPaths::FileExists(File)) continue;

		Size += IFileManager::Get().FileSize(*File);
	}

	return Size;
}

FTopLevelAssetPath UPjcSubsystem::GetAssetExactClassName(const FAssetData& InAsset)
{
	if (!InAsset.IsValid()) return {};

	if (AssetIsBlueprint(InAsset))
	{
		const FString GeneratedClassName = InAsset.TagsAndValues.FindTag(TEXT("GeneratedClass")).GetValue();
		const FString ClassObjectPath = FPackageName::ExportTextPathToObjectPath(*GeneratedClassName);
		const FName ClassName = FName{*FPackageName::ObjectPathToObjectName(ClassObjectPath)};

		FTopLevelAssetPath ClassPathName;
		if (ClassName != NAME_None)
		{
			const FString ShortClassName = ClassName.ToString();
			ClassPathName = UClass::TryConvertShortTypeNameToPathName<UStruct>(
				*ShortClassName, ELogVerbosity::Warning, TEXT("AssetRegistry using deprecated function"));
			// UE_CLOG(ClassPathName.IsNull(), LogClass, Error, TEXT("Failed to convert short class name %s to class path name."), *ShortClassName);
		}

		return ClassPathName;
	}

	return InAsset.AssetClassPath;
}

bool UPjcSubsystem::FolderIsEmpty(const FString& InPath)
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

bool UPjcSubsystem::FolderIsExcluded(const FString& InPath)
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

bool UPjcSubsystem::FolderIsEngineGenerated(const FString& InPath)
{
	const FString PathDevelopers = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() / TEXT("Developers"));
	const FString PathCollections = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir()) /
		TEXT("Collections");
	const FString PathCurrentDeveloper = PathDevelopers / FPaths::GameUserDeveloperFolderName();
	const FString PathCurrentDeveloperCollections = PathCurrentDeveloper / TEXT("Collections");

	TSet<FString> EngineGeneratedPaths;
	EngineGeneratedPaths.Emplace(PathDevelopers);
	EngineGeneratedPaths.Emplace(PathCollections);
	EngineGeneratedPaths.Emplace(PathCurrentDeveloper);
	EngineGeneratedPaths.Emplace(PathCurrentDeveloperCollections);

	return EngineGeneratedPaths.Contains(InPath);
}

bool UPjcSubsystem::FolderIsExternal(const FString& InPath)
{
	return InPath.StartsWith(GetPathExternalActors()) || InPath.StartsWith(GetPathExternalObjects());
}

void UPjcSubsystem::GetSourceAndConfigFiles(TSet<FString>& Files)
{
	const FString DirSrc = FPaths::ConvertRelativePathToFull(FPaths::GameSourceDir());
	const FString DirCfg = FPaths::ConvertRelativePathToFull(FPaths::ProjectConfigDir());
	const FString DirPlg = FPaths::ConvertRelativePathToFull(FPaths::ProjectPluginsDir());

	TArray<FString> SourceFiles;
	TArray<FString> ConfigFiles;

	GetFilesByExt(DirSrc, true, false, PjcConstants::SourceFileExtensions, SourceFiles);
	GetFilesByExt(DirCfg, true, false, PjcConstants::ConfigFileExtensions, ConfigFiles);

	TArray<FString> InstalledPlugins;
	GetFolders(DirPlg, false, InstalledPlugins);

	const FString ProjectCleanerPluginPath = DirPlg / PjcConstants::ModulePjcName.ToString();
	TArray<FString> PluginFiles;
	for (const auto& InstalledPlugin : InstalledPlugins)
	{
		// ignore ProjectCleaner plugin
		if (InstalledPlugin.Equals(ProjectCleanerPluginPath)) continue;

		GetFilesByExt(InstalledPlugin / TEXT("Source"), true, false, PjcConstants::SourceFileExtensions, PluginFiles);
		SourceFiles.Append(PluginFiles);

		PluginFiles.Reset();

		GetFilesByExt(InstalledPlugin / TEXT("Config"), true, false, PjcConstants::ConfigFileExtensions, PluginFiles);
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
			const auto CurrentPackageName = Stack.Pop(EAllowShrinking::No);
			Deps.Reset();

			GetModuleAssetRegistry().Get().GetDependencies(CurrentPackageName, Deps);

			Deps.RemoveAllSwap([&](const FName& Dep)
			{
				return !Dep.ToString().StartsWith(*PjcConstants::PathRoot.ToString());
			}, EAllowShrinking::No);

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

void UPjcSubsystem::ShowNotification(const FString& Msg, const SNotificationItem::ECompletionState State,
                                     const float Duration)
{
	FNotificationInfo Info{FText::FromString(Msg)};
	Info.Text = FText::FromString(TEXT("ProjectCleaner"));
	Info.SubText = FText::FromString(Msg);
	Info.ExpireDuration = Duration;

	const auto NotificationPtr = FSlateNotificationManager::Get().AddNotification(Info);
	if (!NotificationPtr.IsValid()) return;

	NotificationPtr.Get()->SetCompletionState(State);
}

void UPjcSubsystem::ShowNotificationWithOutputLog(const FString& Msg, const SNotificationItem::ECompletionState State,
                                                  const float Duration)
{
	FNotificationInfo Info{FText::FromString(Msg)};
	Info.Text = FText::FromString(TEXT("ProjectCleaner"));
	Info.SubText = FText::FromString(Msg);
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

	FPlatformProcess::ExploreFolder(*InPath);
}

void UPjcSubsystem::OpenAssetEditor(const FAssetData& InAsset)
{
	if (!InAsset.IsValid()) return;
	if (!GEditor) return;

	TArray<FSoftObjectPath> AssetNames;
	AssetNames.Add(InAsset.ToSoftObjectPath());

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

FString UPjcSubsystem::GetPathExternalActors()
{
	return FString::Printf(TEXT("/Game/%s"), FPackagePath::GetExternalActorsFolderName());
}

FString UPjcSubsystem::GetPathExternalObjects()
{
	return FString::Printf(TEXT("/Game/%s"), FPackagePath::GetExternalObjectsFolderName());
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

void UPjcSubsystem::BucketFill(TArray<FAssetData>& AssetsUnused, TArray<FAssetData>& Bucket, const int32 BucketSize)
{
	// Searching Root assets (assets without referencers)
	int32 Index = 0;
	TArray<FName> Refs;
	while (Bucket.Num() < BucketSize && AssetsUnused.IsValidIndex(Index))
	{
		const FAssetData CurrentAsset = AssetsUnused[Index];
		GetModuleAssetRegistry().Get().GetReferencers(CurrentAsset.PackageName, Refs);
		Refs.RemoveAllSwap([&](const FName& Ref)
		{
			return !Ref.ToString().StartsWith(PjcConstants::PathRoot.ToString()) || Ref.IsEqual(
				CurrentAsset.PackageName);
		}, EAllowShrinking::No);
		Refs.Shrink();

		if (Refs.Num() == 0)
		{
			Bucket.AddUnique(CurrentAsset);
			AssetsUnused.RemoveAt(Index);
		}

		Refs.Reset();

		++Index;
	}

	if (Bucket.Num() > 0)
	{
		return;
	}

	// if root assets not found, we deleting assets single by finding its referencers
	if (AssetsUnused.Num() == 0)
	{
		return;
	}

	TArray<FAssetData> Stack;
	Stack.Add(AssetsUnused[0]);

	while (Stack.Num() > 0)
	{
		const FAssetData Current = Stack.Pop(EAllowShrinking::No);
		Bucket.AddUnique(Current);
		AssetsUnused.Remove(Current);

		GetModuleAssetRegistry().Get().GetReferencers(Current.PackageName, Refs);

		// Refs.RemoveAllSwap([&](const FName& Ref)
		// {
		// 	return !Ref.ToString().StartsWith(PjcConstants::PathRoot.ToString()) || Ref.IsEqual(Current.PackageName);
		// }, false);
		Refs.RemoveAllSwap([&](const FName& Ref)
		{
			return !Ref.ToString().StartsWith(PjcConstants::PathRoot.ToString()) || Ref.IsEqual(Current.PackageName);
		}, EAllowShrinking::No);
		Refs.Shrink();

		for (const auto& Ref : Refs)
		{
			const FString ObjectPath = Ref.ToString() + TEXT(".") + FPaths::GetBaseFilename(*Ref.ToString());
			const FAssetData AssetData = GetModuleAssetRegistry().Get().GetAssetByObjectPath(
				FSoftObjectPath{ObjectPath});
			if (AssetData.IsValid())
			{
				if (!Bucket.Contains(AssetData))
				{
					Stack.Add(AssetData);
				}

				Bucket.AddUnique(AssetData);
				AssetsUnused.Remove(AssetData);
			}
		}

		Refs.Reset();
	}
}

bool UPjcSubsystem::BucketPrepare(const TArray<FAssetData>& Bucket, TArray<UObject*>& LoadedAssets)
{
	TArray<FString> ObjectPaths;
	ObjectPaths.Reserve(Bucket.Num());

	for (const auto& Asset : Bucket)
	{
		ObjectPaths.Add(Asset.GetSoftObjectPath().ToString());
	}

	AssetViewUtils::FLoadAssetsSettings Settings{
		.bAlwaysPromptBeforeLoading = false,
		.bFollowRedirectors = true,
		.bLoadWorldPartitionMaps = false,
		.bLoadAllExternalObjects = false,
	};

	return LoadAssetsIfNeeded(ObjectPaths, LoadedAssets, Settings) == AssetViewUtils::ELoadAssetsResult::Success;
}

int32 UPjcSubsystem::BucketDelete(const TArray<UObject*>& LoadedAssets)
{
	int32 DeletedAssetsNum = ObjectTools::DeleteObjects(LoadedAssets, false);

	if (DeletedAssetsNum == 0)
	{
		DeletedAssetsNum = ObjectTools::ForceDeleteObjects(LoadedAssets, false);
	}

	return DeletedAssetsNum;
}
