// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "PjcSubsystem.h"
#include "Pjc.h"
#include "PjcConstants.h"
#include "PjcEditorSettings.h"
#include "Libs/PjcLibPath.h"
#include "Libs/PjcLibAsset.h"
#include "Libs/PjcLibEditor.h"
// Engine Headers
#include "FileHelpers.h"
#include "Misc/ScopedSlowTask.h"

void UPjcSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UPjcSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

void UPjcSubsystem::ProjectScan(const FPjcAssetExcludeSettings& InAssetExcludeSetting, FPjcScanResult& ScanResult) const
{
	ScanResult.bScanSuccess = true;

	if (!CanScanProject(ScanResult.ScanErrMsg))
	{
		ScanResult.bScanSuccess = false;
		return;
	}

	FScopedSlowTask SlowTaskMain{
		2.0f,
		FText::FromString(TEXT("Scanning Project ...")),
		GIsEditor && !IsRunningCommandlet()
	};
	SlowTaskMain.MakeDialog(false, false);

	SlowTaskMain.EnterProgressFrame(1.0f);
	ScanAssets(InAssetExcludeSetting, ScanResult.ScanDataAssets);

	SlowTaskMain.EnterProgressFrame(1.0f);
	ScanFiles(ScanResult.ScanDataFiles);
}

void UPjcSubsystem::ScanAssets(const FPjcAssetExcludeSettings& InAssetExcludeSetting, FPjcScanDataAssets& ScanDataAssets) const
{
	const double ScanStartTime = FPlatformTime::Seconds();

	FScopedSlowTask SlowTaskMain{
		5.0f,
		FText::FromString(TEXT("Scanning Project Assets...")),
		GIsEditor && !IsRunningCommandlet()
	};
	SlowTaskMain.MakeDialog(false, false);

	SlowTaskMain.EnterProgressFrame(1.0f);
	// first getting all assets in project
	FPjcLibAsset::GetAssetsByPath(PjcConstants::PathRelRoot.ToString(), true, ScanDataAssets.AssetsAll);

	SlowTaskMain.EnterProgressFrame(1.0f);
	// reserving some space for data
	const int32 NumAssetsTotal = ScanDataAssets.AssetsAll.Num();

	ScanDataAssets.AssetsUnused.Reset(NumAssetsTotal);
	ScanDataAssets.AssetsUsed.Reset(NumAssetsTotal);
	ScanDataAssets.AssetsEditor.Reset(NumAssetsTotal);
	ScanDataAssets.AssetsExcluded.Reset(NumAssetsTotal);
	ScanDataAssets.AssetsPrimary.Reset(NumAssetsTotal);
	ScanDataAssets.AssetsExtReferenced.Reset(NumAssetsTotal);

	SlowTaskMain.EnterProgressFrame(1.0f);
	// getting initial data
	TSet<FName> ClassNamesPrimary;
	TSet<FName> ClassNamesEditor;
	TSet<FName> ClassNamesExcluded{InAssetExcludeSetting.ExcludedClassNames};
	TSet<FAssetData> AssetsExcluded;
	TSet<FAssetData> AssetsUsedInitial;

	FPjcLibAsset::GetClassNamesPrimary(ClassNamesPrimary);
	FPjcLibAsset::GetClassNamesEditor(ClassNamesEditor);
	FPjcLibAsset::GetAssetsIndirect(ScanDataAssets.AssetsIndirect);

	AssetsUsedInitial.Reserve(NumAssetsTotal);
	AssetsExcluded.Reserve(NumAssetsTotal);

	// loading initial used assets
	{
		TArray<FAssetData> AssetsExcludedByPackagePaths;
		TArray<FAssetData> AssetsExcludedByObjectPaths;

		FPjcLibAsset::GetAssetsByPackagePaths(InAssetExcludeSetting.ExcludedPackagePaths, true, AssetsExcludedByPackagePaths);
		FPjcLibAsset::GetAssetsByObjectPaths(InAssetExcludeSetting.ExcludedObjectPaths, AssetsExcludedByObjectPaths);

		TArray<FAssetData> AssetsIndirect;
		ScanDataAssets.AssetsIndirect.GetKeys(AssetsIndirect);

		AssetsExcluded.Append(AssetsExcludedByPackagePaths);
		AssetsExcluded.Append(AssetsExcludedByObjectPaths);

		AssetsUsedInitial.Append(AssetsExcludedByPackagePaths);
		AssetsUsedInitial.Append(AssetsExcludedByObjectPaths);
		AssetsUsedInitial.Append(AssetsIndirect);
	}

	SlowTaskMain.EnterProgressFrame(1.0f);

	FScopedSlowTask SlowTask{
		static_cast<float>(NumAssetsTotal),
		FText::FromString(TEXT("Scanning Project Assets...")),
		GIsEditor && !IsRunningCommandlet()
	};
	SlowTask.MakeDialog(false, false);

	for (const auto& Asset : ScanDataAssets.AssetsAll)
	{
		SlowTask.EnterProgressFrame(1.0f, FText::FromName(Asset.ToSoftObjectPath().GetAssetPathName()));

		const bool bIsPrimary = FPjcLibAsset::AssetClassNameInList(Asset, ClassNamesPrimary);
		const bool bIsEditor = FPjcLibAsset::AssetClassNameInList(Asset, ClassNamesEditor);
		const bool bIsExcluded = FPjcLibAsset::AssetClassNameInList(Asset, ClassNamesExcluded);
		const bool bIsExtReferenced = FPjcLibAsset::AssetIsExtReferenced(Asset);
		const bool bIsMegascansBase = FPjcLibAsset::AssetIsMegascansBase(Asset);
		const bool bIsUsed = bIsPrimary || bIsEditor || bIsExcluded || bIsExtReferenced || bIsMegascansBase;

		if (bIsPrimary)
		{
			ScanDataAssets.AssetsPrimary.Emplace(Asset);
		}

		if (bIsEditor)
		{
			ScanDataAssets.AssetsEditor.Emplace(Asset);
		}

		if (bIsExcluded)
		{
			AssetsExcluded.Emplace(Asset);
		}

		if (bIsExtReferenced)
		{
			ScanDataAssets.AssetsExtReferenced.Emplace(Asset);
		}

		if (bIsUsed)
		{
			AssetsUsedInitial.Emplace(Asset);
		}
	}

	SlowTaskMain.EnterProgressFrame(1.0f);
	// now getting all used assets dependencies recursive
	TSet<FAssetData> AssetsUsed;
	FPjcLibAsset::GetAssetsDeps(AssetsUsedInitial, AssetsUsed);

	SlowTask.EnterProgressFrame(1.0f);
	// finally filtering all unused assets
	const TSet<FAssetData> AssetsAll{ScanDataAssets.AssetsAll};
	const TSet<FAssetData> AssetsUnused = AssetsAll.Difference(AssetsUsed);

	ScanDataAssets.AssetsUsed.Append(AssetsUsed.Array());
	ScanDataAssets.AssetsUnused.Append(AssetsUnused.Array());
	ScanDataAssets.AssetsExcluded.Append(AssetsExcluded.Array());

	// shrinking data containers
	ScanDataAssets.AssetsAll.Shrink();
	ScanDataAssets.AssetsUnused.Shrink();
	ScanDataAssets.AssetsUsed.Shrink();
	ScanDataAssets.AssetsPrimary.Shrink();
	ScanDataAssets.AssetsEditor.Shrink();
	ScanDataAssets.AssetsExtReferenced.Shrink();
	ScanDataAssets.AssetsExcluded.Shrink();

	const double ScanTime = FPlatformTime::Seconds() - ScanStartTime;

	UE_LOG(LogProjectCleaner, Display, TEXT("Project assets scanned in %f seconds"), ScanTime);

	if (DelegateOnScanAssets.IsBound())
	{
		DelegateOnScanAssets.Broadcast(ScanDataAssets);
	}
}

void UPjcSubsystem::ScanFiles(FPjcScanDataFiles& ScanDataFiles) const
{
	const float ScanStartTime = FPlatformTime::Seconds();

	FScopedSlowTask SlowTaskMain{
		1.0f,
		FText::FromString(TEXT("Scanning Project Files...")),
		GIsEditor && !IsRunningCommandlet()
	};
	SlowTaskMain.MakeDialog(false, false);

	SlowTaskMain.EnterProgressFrame(1.0f);
	TArray<FString> Paths;
	FPjcLibAsset::GetCachedPaths(Paths);

	ScanDataFiles.FilesExternal.Reset(Paths.Num());
	ScanDataFiles.FilesCorrupted.Reset(Paths.Num());
	ScanDataFiles.FoldersEmpty.Reset(Paths.Num());

	FScopedSlowTask SlowTask{
		static_cast<float>(Paths.Num()),
		FText{},
		GIsEditor && !IsRunningCommandlet()
	};
	SlowTask.MakeDialog(false, false);

	TSet<FString> Files;
	for (const auto& Path : Paths)
	{
		SlowTask.EnterProgressFrame(1.0f);

		const FString PathAbs = FPjcLibPath::ToAbsolute(Path);
		if (PathAbs.IsEmpty()) continue;

		if (FPjcLibPath::IsPathEmpty(PathAbs) && !FPjcLibPath::IsPathEngineGenerated(PathAbs))
		{
			ScanDataFiles.FoldersEmpty.Emplace(PathAbs);
		}

		FPjcLibPath::GetFilesInPath(PathAbs, true, Files);

		for (const auto& File : Files)
		{
			if (FPjcLibPath::IsExternalFile(File))
			{
				ScanDataFiles.FilesExternal.Emplace(File);
			}

			if (FPjcLibPath::IsCorruptedAssetFile(File))
			{
				ScanDataFiles.FilesCorrupted.Emplace(File);
			}
		}
	}

	ScanDataFiles.FilesExternal.Shrink();
	ScanDataFiles.FilesCorrupted.Shrink();
	ScanDataFiles.FoldersEmpty.Shrink();

	const double ScanTime = FPlatformTime::Seconds() - ScanStartTime;

	UE_LOG(LogProjectCleaner, Display, TEXT("Project files scanned in %f seconds"), ScanTime);

	if (DelegateOnScanFiles.IsBound())
	{
		DelegateOnScanFiles.Broadcast(ScanDataFiles);
	}
}

FPjcDelegateOnScanAssets& UPjcSubsystem::OnScanAssets()
{
	return DelegateOnScanAssets;
}

FPjcDelegateOnScanFiles& UPjcSubsystem::OnScanFiles()
{
	return DelegateOnScanFiles;
}

#if WITH_EDITOR
void UPjcSubsystem::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	SaveConfig();
}
#endif

bool UPjcSubsystem::CanScanProject(FString& ErrMsg) const
{
	// Check for ongoing scanning or cleaning operations
	if (bScanningInProgress)
	{
		ErrMsg = TEXT("Scanning is in progress. Please wait until it has finished and then try again.");
		return false;
	}

	if (bCleaningInProgress)
	{
		ErrMsg = TEXT("Cleaning is in progress. Please wait until it has finished and then try again.");
		return false;
	}

	// Check if AssetRegistry is still discovering assets
	if (FPjcLibAsset::AssetRegistryWorking())
	{
		ErrMsg = TEXT("Scanning of the project has failed. AssetRegistry is still discovering assets. Please try again after it has finished.");
		return false;
	}

	// Check if the editor is in play mode
	if (FPjcLibEditor::EditorInPlayMode())
	{
		ErrMsg = TEXT("Scanning of the project has failed. The editor is in play mode. Please exit play mode and try again.");
		return false;
	}

	// Close all asset editors and fix redirectors if not running a commandlet
	if (!IsRunningCommandlet())
	{
		if (!GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->CloseAllAssetEditors())
		{
			ErrMsg = TEXT("Scanning of the project has failed because not all asset editors are closed.");
			return false;
		}

		FPjcLibAsset::FixupRedirectorsInProject(true);
	}

	// Check if project still contains redirectors
	if (FPjcLibAsset::ProjectContainsRedirectors())
	{
		ErrMsg = TEXT("Failed to scan project, because not all redirectors are fixed.");
		return false;
	}

	// Saving all unsaved assets in project, before scanning
	if (!FEditorFileUtils::SaveDirtyPackages(true, true, true, false, false, false))
	{
		ErrMsg = TEXT("Scanning of the project has failed because not all assets have been saved.");
		return false;
	}

	return true;
}
