// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "PjcSubsystem.h"
#include "PjcConstants.h"
#include "Libs/PjcLibAsset.h"
#include "Libs/PjcLibEditor.h"
#include "Libs/PjcLibPath.h"
// Engine Headers
#include "FileHelpers.h"
#include "ObjectTools.h"
#include "Pjc.h"
#include "PjcEditorSettings.h"
#include "Misc/ScopedSlowTask.h"

void UPjcSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UPjcSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

void UPjcSubsystem::ProjectScan()
{
	const UPjcEditorSettings* EditorSettings = GetDefault<UPjcEditorSettings>();
	if (!EditorSettings) return;

	FPjcExcludeSettings ExcludeSettings;

	ExcludeSettings.ExcludedPaths.Reserve(EditorSettings->ExcludedPaths.Num());
	ExcludeSettings.ExcludedClassNames.Reserve(EditorSettings->ExcludedClasses.Num());
	ExcludeSettings.ExcludedAssetObjectPaths.Reserve(EditorSettings->ExcludedAssets.Num());

	for (const auto& ExcludedPath : EditorSettings->ExcludedPaths)
	{
		const FString AssetPath = FPjcLibPath::ToAssetPath(ExcludedPath.Path);
		if (AssetPath.IsEmpty()) continue;

		ExcludeSettings.ExcludedPaths.Emplace(AssetPath);
	}

	for (const auto& ExcludedClass : EditorSettings->ExcludedClasses)
	{
		if (!ExcludedClass.LoadSynchronous()) continue;

		ExcludeSettings.ExcludedClassNames.Emplace(ExcludedClass.Get()->GetFName());
	}

	for (const auto& ExcludedAsset : EditorSettings->ExcludedAssets)
	{
		if (!ExcludedAsset.LoadSynchronous()) continue;

		ExcludeSettings.ExcludedAssetObjectPaths.Emplace(ExcludedAsset.ToSoftObjectPath().GetAssetPathName());
	}

	ProjectScanBySettings(ExcludeSettings, LastScanResult);

	if (!LastScanResult.bScanSuccess)
	{
		UE_LOG(LogProjectCleaner, Error, TEXT("%s"), *LastScanResult.ScanErrMsg);
		FPjcLibEditor::ShowNotificationWithOutputLog(LastScanResult.ScanErrMsg, SNotificationItem::ECompletionState::CS_Fail, 5.0f);
	}

	if (DelegateOnProjectScan.IsBound())
	{
		DelegateOnProjectScan.Broadcast(LastScanResult);
	}
}

void UPjcSubsystem::ProjectClean()
{
	// const UPjcEditorSettings* EditorSettings = GetDefault<UPjcEditorSettings>();
	// if (!EditorSettings) return;

	FPjcLibEditor::ShaderCompilationDisable();

	ObjectTools::DeleteAssets(LastScanResult.ScanData.AssetsUnused);

	FPjcLibEditor::ShaderCompilationEnable();
}

void UPjcSubsystem::ProjectScanBySettings(const FPjcExcludeSettings& InExcludeSettings, FPjcScanResult& OutScanResult) const
{
	FScopedSlowTask SlowTask{
		5.0f,
		FText::FromString(TEXT("Scanning Project...")),
		GIsEditor && !IsRunningCommandlet()
	};
	SlowTask.MakeDialog(false, false);
	SlowTask.EnterProgressFrame(1.0f);
	
	// Clear previous scan data
	OutScanResult.Clear();

	// Check for ongoing scanning or cleaning operations
	if (bScanningInProgress)
	{
		OutScanResult.bScanSuccess = false;
		OutScanResult.ScanErrMsg = TEXT("Scanning is in progress. Please wait until it has finished and then try again.");
		return;
	}

	if (bCleaningInProgress)
	{
		OutScanResult.bScanSuccess = false;
		OutScanResult.ScanErrMsg = TEXT("Cleaning is in progress. Please wait until it has finished and then try again.");
		return;
	}

	// Check if AssetRegistry is still discovering assets
	if (FPjcLibAsset::AssetRegistryWorking())
	{
		OutScanResult.bScanSuccess = false;
		OutScanResult.ScanErrMsg = TEXT("Scanning of the project has failed. AssetRegistry is still discovering assets. Please try again after it has finished.");
		return;
	}

	// Check if the editor is in play mode
	if (FPjcLibEditor::EditorInPlayMode())
	{
		OutScanResult.bScanSuccess = false;
		OutScanResult.ScanErrMsg = TEXT("Scanning of the project has failed. The editor is in play mode. Please exit play mode and try again.");
		return;
	}

	// Close all asset editors and fix redirectors if not running a commandlet
	if (!IsRunningCommandlet())
	{
		if (!GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->CloseAllAssetEditors())
		{
			OutScanResult.bScanSuccess = false;
			OutScanResult.ScanErrMsg = TEXT("Scanning of the project has failed because not all asset editors are closed.");
			return;
		}

		FPjcLibAsset::FixupRedirectorsInProject(true);
	}

	// Check if project still contains redirectors
	if (FPjcLibAsset::ProjectContainsRedirectors())
	{
		OutScanResult.bScanSuccess = false;
		OutScanResult.ScanErrMsg = TEXT("Failed to scan project, because not all redirectors are fixed.");
		return;
	}

	// Saving all unsaved assets in project, before scanning
	if (!FEditorFileUtils::SaveDirtyPackages(true, true, true, false, false, false))
	{
		OutScanResult.bScanSuccess = false;
		OutScanResult.ScanErrMsg = TEXT("Scanning of the project has failed because not all assets have been saved.");
		return;
	}

	const double ScanStartTime = FPlatformTime::Seconds();

	SlowTask.EnterProgressFrame(1.0f);

	ScanAssets(InExcludeSettings, OutScanResult);

	SlowTask.EnterProgressFrame(1.0f);

	ScanFiles(OutScanResult);

	SlowTask.EnterProgressFrame(1.0f);

	ScanFolders(OutScanResult);

	SlowTask.EnterProgressFrame(1.0f);

	ScanStatsUpdate(OutScanResult);

	const double ScanTime = FPlatformTime::Seconds() - ScanStartTime;

	UE_LOG(LogProjectCleaner, Display, TEXT("Project Scanned in %.2f seconds"), ScanTime);
}

FPjcDelegateOnProjectScan& UPjcSubsystem::OnProjectScan()
{
	return DelegateOnProjectScan;
}

const FPjcScanResult& UPjcSubsystem::GetLastScanResult() const
{
	return LastScanResult;
}

#if WITH_EDITOR
void UPjcSubsystem::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	SaveConfig();
}
#endif

void UPjcSubsystem::ScanAssets(const FPjcExcludeSettings& InExcludeSettings, FPjcScanResult& OutScanResult) const
{
	const double ScanStartTime = FPlatformTime::Seconds();

	TSet<FAssetData> AssetsAll;
	TSet<FAssetData> AssetsExcluded;
	TSet<FAssetData> AssetsUsed;
	TSet<FName> ClassNamesPrimary;
	TSet<FName> ClassNamesEditor;

	FPjcLibAsset::GetAssetsByPath(PjcConstants::PathRelRoot.ToString(), true, OutScanResult.ScanData.AssetsAll);
	FPjcLibAsset::GetAssetsIndirect(OutScanResult.ScanData.AssetsIndirect, OutScanResult.ScanData.AssetsIndirectInfos);
	FPjcLibAsset::GetAssetsExcluded(InExcludeSettings, OutScanResult.ScanData.AssetsExcluded);
	FPjcLibAsset::GetClassNamesPrimary(ClassNamesPrimary);
	FPjcLibAsset::GetClassNamesEditor(ClassNamesEditor);

	AssetsAll.Append(OutScanResult.ScanData.AssetsAll);
	AssetsExcluded.Append(OutScanResult.ScanData.AssetsExcluded);
	AssetsUsed.Append(OutScanResult.ScanData.AssetsIndirect);
	AssetsUsed.Append(OutScanResult.ScanData.AssetsExcluded);

	FScopedSlowTask SlowTask{
		static_cast<float>(OutScanResult.ScanData.AssetsAll.Num()),
		FText::FromString(TEXT("Scanning Assets...")),
		GIsEditor && !IsRunningCommandlet()
	};
	SlowTask.MakeDialog(false, false);

	for (const auto& Asset : OutScanResult.ScanData.AssetsAll)
	{
		SlowTask.EnterProgressFrame(1.0f, FText::FromString(Asset.ToSoftObjectPath().GetAssetPathString()));

		const bool bIsPrimary = FPjcLibAsset::AssetClassNameInList(Asset, ClassNamesPrimary);
		const bool bIsEditor = FPjcLibAsset::AssetClassNameInList(Asset, ClassNamesEditor);
		const bool bIsExtReferenced = FPjcLibAsset::AssetIsExtReferenced(Asset);
		const bool bIsUsed = bIsPrimary || bIsEditor || bIsExtReferenced || FPjcLibAsset::AssetIsMegascansBase(Asset);

		if (bIsPrimary)
		{
			OutScanResult.ScanData.AssetsPrimary.Emplace(Asset);
		}

		if (bIsEditor)
		{
			OutScanResult.ScanData.AssetsEditor.Emplace(Asset);
		}

		if (bIsExtReferenced)
		{
			OutScanResult.ScanData.AssetsExtReferenced.Emplace(Asset);
		}

		if (bIsUsed)
		{
			AssetsUsed.Emplace(Asset);
		}
	}

	TSet<FAssetData> AssetsUsedDependencies;
	FPjcLibAsset::GetAssetsDeps(AssetsUsed, AssetsUsedDependencies);

	const TSet<FAssetData> AssetsUnused = AssetsAll.Difference(AssetsUsedDependencies);

	OutScanResult.ScanData.AssetsUsed.Append(AssetsUsedDependencies.Array());
	OutScanResult.ScanData.AssetsUnused.Append(AssetsUnused.Array());

	const double ScanTime = FPlatformTime::Seconds() - ScanStartTime;

	UE_LOG(LogProjectCleaner, Display, TEXT("Assets scanned in %.2f seconds"), ScanTime);
}

void UPjcSubsystem::ScanFiles(FPjcScanResult& OutScanResult) const
{
	const double ScanStartTime = FPlatformTime::Seconds();

	TSet<FString> Files;
	FPjcLibPath::GetFilesInPath(FPjcLibPath::ContentDir(), true, Files);

	OutScanResult.ScanData.FilesExternal.Reserve(Files.Num());
	OutScanResult.ScanData.AssetsCorrupted.Reserve(Files.Num());

	FScopedSlowTask SlowTask{
		static_cast<float>(Files.Num()),
		FText::FromString(TEXT("Scanning Project Files...")),
		GIsEditor && !IsRunningCommandlet()
	};
	SlowTask.MakeDialog(false, false);

	for (const auto& File : Files)
	{
		SlowTask.EnterProgressFrame(1.0f, FText::FromString(File));

		const FString FilePathAbs = FPjcLibPath::ToAbsolute(File);
		const FString FileExtension = FPjcLibPath::GetFileExtension(FilePathAbs, false).ToLower();

		if (PjcConstants::EngineFileExtensions.Contains(FileExtension))
		{
			const FAssetData AssetData = FPjcLibAsset::GetAssetByObjectPath(FPjcLibPath::ToObjectPath(FilePathAbs));
			if (!AssetData.IsValid())
			{
				OutScanResult.ScanData.AssetsCorrupted.Emplace(FilePathAbs);
			}
		}
		else
		{
			OutScanResult.ScanData.FilesExternal.Emplace(FilePathAbs);
		}
	}

	const double ScanTime = FPlatformTime::Seconds() - ScanStartTime;

	UE_LOG(LogProjectCleaner, Display, TEXT("Files scanned in %.2f seconds"), ScanTime);
}

void UPjcSubsystem::ScanFolders(FPjcScanResult& OutScanResult) const
{
	const double ScanStartTime = FPlatformTime::Seconds();

	TSet<FString> Folders;
	FPjcLibPath::GetFoldersInPath(FPjcLibPath::ContentDir(), true, Folders);

	OutScanResult.ScanData.FoldersEmpty.Reserve(Folders.Num());

	FScopedSlowTask SlowTask{
		static_cast<float>(Folders.Num()),
		FText::FromString(TEXT("Scanning Project Folders...")),
		GIsEditor && !IsRunningCommandlet()
	};
	SlowTask.MakeDialog(false, false);

	for (const auto& Folder : Folders)
	{
		SlowTask.EnterProgressFrame(1.0f, FText::FromString(Folder));

		if (FPjcLibPath::IsPathEmpty(Folder) && !FPjcLibPath::IsPathEngineGenerated(Folder))
		{
			OutScanResult.ScanData.FoldersEmpty.Emplace(Folder);
		}
	}

	const double ScanTime = FPlatformTime::Seconds() - ScanStartTime;

	UE_LOG(LogProjectCleaner, Display, TEXT("Folders scanned in %.2f seconds"), ScanTime);
}

void UPjcSubsystem::ScanStatsUpdate(FPjcScanResult& InScanResult) const
{
	InScanResult.ScanData.Shrink();

	InScanResult.ScanStats.NumAssetsTotal = InScanResult.ScanData.AssetsAll.Num();
	InScanResult.ScanStats.NumAssetsUnused = InScanResult.ScanData.AssetsUnused.Num();
	InScanResult.ScanStats.NumAssetsUsed = InScanResult.ScanData.AssetsUsed.Num();
	InScanResult.ScanStats.NumAssetsPrimary = InScanResult.ScanData.AssetsPrimary.Num();
	InScanResult.ScanStats.NumAssetsIndirect = InScanResult.ScanData.AssetsIndirect.Num();
	InScanResult.ScanStats.NumAssetsEditor = InScanResult.ScanData.AssetsEditor.Num();
	InScanResult.ScanStats.NumAssetsExcluded = InScanResult.ScanData.AssetsExcluded.Num();
	InScanResult.ScanStats.NumAssetsExtReferenced = InScanResult.ScanData.AssetsExtReferenced.Num();
	InScanResult.ScanStats.NumAssetsCorrupted = InScanResult.ScanData.AssetsCorrupted.Num();
	InScanResult.ScanStats.NumFilesExternal = InScanResult.ScanData.FilesExternal.Num();
	InScanResult.ScanStats.NumFoldersEmpty = InScanResult.ScanData.FoldersEmpty.Num();

	InScanResult.ScanStats.SizeAssetsTotal = FPjcLibAsset::GetAssetsSize(InScanResult.ScanData.AssetsAll);
	InScanResult.ScanStats.SizeAssetsUnused = FPjcLibAsset::GetAssetsSize(InScanResult.ScanData.AssetsUnused);
	InScanResult.ScanStats.SizeAssetsUsed = FPjcLibAsset::GetAssetsSize(InScanResult.ScanData.AssetsUsed);
	InScanResult.ScanStats.SizeAssetsPrimary = FPjcLibAsset::GetAssetsSize(InScanResult.ScanData.AssetsPrimary);
	InScanResult.ScanStats.SizeAssetsIndirect = FPjcLibAsset::GetAssetsSize(InScanResult.ScanData.AssetsIndirect);
	InScanResult.ScanStats.SizeAssetsEditor = FPjcLibAsset::GetAssetsSize(InScanResult.ScanData.AssetsEditor);
	InScanResult.ScanStats.SizeAssetsExcluded = FPjcLibAsset::GetAssetsSize(InScanResult.ScanData.AssetsExcluded);
	InScanResult.ScanStats.SizeAssetsExtReferenced = FPjcLibAsset::GetAssetsSize(InScanResult.ScanData.AssetsExtReferenced);
	InScanResult.ScanStats.SizeAssetsCorrupted = FPjcLibPath::GetFilesSize(InScanResult.ScanData.AssetsCorrupted);
	InScanResult.ScanStats.SizeFilesExternal = FPjcLibPath::GetFilesSize(InScanResult.ScanData.FilesExternal);
}
