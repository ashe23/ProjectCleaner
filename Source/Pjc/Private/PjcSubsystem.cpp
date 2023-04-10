// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "PjcSubsystem.h"
#include "PjcConstants.h"
#include "Libs/PjcLibAsset.h"
#include "Libs/PjcLibEditor.h"
#include "Libs/PjcLibPath.h"
// Engine Headers
#include "FileHelpers.h"
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

	if (DelegateOnProjectScan.IsBound())
	{
		DelegateOnProjectScan.Broadcast(LastScanResult);
	}
}

void UPjcSubsystem::ProjectScanBySettings(const FPjcExcludeSettings& InExcludeSettings, FPjcScanResult& OutScanResult) const
{
	// Clear previous scan data
	OutScanResult.Clear();

	// Initialize ScanResult with default success state
	OutScanResult.bScanSuccess = true;
	OutScanResult.ScanErrMsg.Empty();

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

	FScopedSlowTask SlowTask{
		4.0f,
		FText::FromString(TEXT("Scanning Project...")),
		GIsEditor && !IsRunningCommandlet()
	};
	SlowTask.MakeDialog(false, false);

	SlowTask.EnterProgressFrame(1.0f);

	FPjcLibAsset::GetAssetsAll(OutScanResult.ScanData.AssetsAll);
	FPjcLibAsset::GetAssetsIndirect(OutScanResult.ScanData.AssetsIndirect, OutScanResult.ScanData.AssetsIndirectInfos);
	FPjcLibAsset::GetAssetsExcluded(InExcludeSettings, OutScanResult.ScanData.AssetsExcluded);

	TSet<FName> ClassNamesPrimary;
	TSet<FName> ClassNamesEditor;

	FPjcLibAsset::GetClassNamesPrimary(ClassNamesPrimary);
	FPjcLibAsset::GetClassNamesEditor(ClassNamesEditor);

	OutScanResult.ScanData.AssetsUsed.Reserve(OutScanResult.ScanData.AssetsAll.Num());
	OutScanResult.ScanData.AssetsUnused.Reserve(OutScanResult.ScanData.AssetsAll.Num());

	TSet<FAssetData> AssetsUsed;
	const TSet<FAssetData> AssetsExcluded{OutScanResult.ScanData.AssetsExcluded};


	SlowTask.EnterProgressFrame(1.0f);

	FScopedSlowTask SubTask{
		static_cast<float>(OutScanResult.ScanData.AssetsAll.Num()),
		FText::FromString(TEXT("Scanning Assets...")),
		GIsEditor && !IsRunningCommandlet()
	};
	SubTask.MakeDialog(false, false);

	for (const auto& Asset : OutScanResult.ScanData.AssetsAll)
	{
		SubTask.EnterProgressFrame(1.0f, FText::FromString(Asset.ToSoftObjectPath().GetAssetPathString()));

		const bool bIsPrimary = FPjcLibAsset::AssetClassNameInList(Asset, ClassNamesPrimary);
		const bool bIsIndirect = OutScanResult.ScanData.AssetsIndirect.Contains(Asset);
		const bool bIsEditor = FPjcLibAsset::AssetClassNameInList(Asset, ClassNamesEditor);
		const bool bIsExcluded = AssetsExcluded.Contains(Asset);
		const bool bIsExtReferenced = FPjcLibAsset::AssetIsExtReferenced(Asset);
		const bool bIsUsed = bIsPrimary || bIsIndirect || bIsEditor || bIsExcluded || bIsExtReferenced || FPjcLibAsset::AssetIsMegascansBase(Asset);

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

	TSet<FAssetData> AssetDeps;
	FPjcLibAsset::GetAssetsDeps(AssetsUsed, AssetDeps);

	for (const auto& Asset : OutScanResult.ScanData.AssetsAll)
	{
		if (!AssetDeps.Contains(Asset))
		{
			OutScanResult.ScanData.AssetsUnused.Emplace(Asset);
		}
	}

	OutScanResult.ScanData.AssetsUsed.Append(AssetDeps.Array());

	SlowTask.EnterProgressFrame(1.0f);

	TArray<FString> Paths;
	FPjcLibAsset::GetCachedPaths(Paths);

	OutScanResult.ScanData.FoldersEmpty.Reserve(Paths.Num());

	for (const auto& Path : Paths)
	{
		if (FPjcLibPath::IsPathEmpty(Path) && !FPjcLibPath::IsPathEngineGenerated(Path))
		{
			OutScanResult.ScanData.FoldersEmpty.Emplace(FPjcLibPath::ToAbsolute(Path));
		}
	}

	TArray<FString> Files;
	IFileManager::Get().FindFilesRecursive(Files, *FPjcLibPath::ContentDir(), TEXT("*"), true, false);
	
	OutScanResult.ScanData.FilesExternal.Reserve(Files.Num());
	OutScanResult.ScanData.AssetsCorrupted.Reserve(Files.Num());
	
	for (const auto& File : Files)
	{
		const FString FilePathAbs = FPjcLibPath::ToAbsolute(File);
		if (FilePathAbs.IsEmpty()) continue;
	
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

	SlowTask.EnterProgressFrame(1.0f);
	OutScanResult.ScanData.Shrink();

	OutScanResult.ScanStats.NumAssetsTotal = OutScanResult.ScanData.AssetsAll.Num();
	OutScanResult.ScanStats.NumAssetsUnused = OutScanResult.ScanData.AssetsUnused.Num();
	OutScanResult.ScanStats.NumAssetsUsed = OutScanResult.ScanData.AssetsUsed.Num();
	OutScanResult.ScanStats.NumAssetsPrimary = OutScanResult.ScanData.AssetsPrimary.Num();
	OutScanResult.ScanStats.NumAssetsIndirect = OutScanResult.ScanData.AssetsIndirect.Num();
	OutScanResult.ScanStats.NumAssetsEditor = OutScanResult.ScanData.AssetsEditor.Num();
	OutScanResult.ScanStats.NumAssetsExcluded = OutScanResult.ScanData.AssetsExcluded.Num();
	OutScanResult.ScanStats.NumAssetsExtReferenced = OutScanResult.ScanData.AssetsExtReferenced.Num();
	OutScanResult.ScanStats.NumAssetsCorrupted = OutScanResult.ScanData.AssetsCorrupted.Num();
	OutScanResult.ScanStats.NumFilesExternal = OutScanResult.ScanData.FilesExternal.Num();
	OutScanResult.ScanStats.NumFoldersEmpty = OutScanResult.ScanData.FoldersEmpty.Num();

	OutScanResult.ScanStats.SizeAssetsTotal = FPjcLibAsset::GetAssetsSize(OutScanResult.ScanData.AssetsAll);
	OutScanResult.ScanStats.SizeAssetsUnused = FPjcLibAsset::GetAssetsSize(OutScanResult.ScanData.AssetsUnused);
	OutScanResult.ScanStats.SizeAssetsUsed = FPjcLibAsset::GetAssetsSize(OutScanResult.ScanData.AssetsUsed);
	OutScanResult.ScanStats.SizeAssetsPrimary = FPjcLibAsset::GetAssetsSize(OutScanResult.ScanData.AssetsPrimary);
	OutScanResult.ScanStats.SizeAssetsIndirect = FPjcLibAsset::GetAssetsSize(OutScanResult.ScanData.AssetsIndirect);
	OutScanResult.ScanStats.SizeAssetsEditor = FPjcLibAsset::GetAssetsSize(OutScanResult.ScanData.AssetsEditor);
	OutScanResult.ScanStats.SizeAssetsExcluded = FPjcLibAsset::GetAssetsSize(OutScanResult.ScanData.AssetsExcluded);
	OutScanResult.ScanStats.SizeAssetsExtReferenced = FPjcLibAsset::GetAssetsSize(OutScanResult.ScanData.AssetsExtReferenced);
	OutScanResult.ScanStats.SizeAssetsCorrupted = FPjcLibPath::GetFilesSize(OutScanResult.ScanData.AssetsCorrupted);
	OutScanResult.ScanStats.SizeFilesExternal = FPjcLibPath::GetFilesSize(OutScanResult.ScanData.FilesExternal);

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
