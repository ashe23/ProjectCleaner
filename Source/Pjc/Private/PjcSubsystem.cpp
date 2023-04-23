// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "PjcSubsystem.h"
#include "Libs/PjcLibAsset.h"
#include "Libs/PjcLibEditor.h"
#include "PjcEditorSettings.h"
// Engine Headers
#include "FileHelpers.h"
#include "Pjc.h"
#include "PjcConstants.h"
#include "Libs/PjcLibPath.h"
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
	ScanDataAssets.AssetsPrimary.Shrink();
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
	// todo:ashe23
}

void UPjcSubsystem::Test()
{
	FPjcScanDataAssets DataAssets;
	ScanAssets(FPjcLibEditor::GetEditorAssetExcludeSettings(), DataAssets);
}

FPjcDelegateOnScanAssets& UPjcSubsystem::OnScanAssets()
{
	return DelegateOnScanAssets;
}

// void UPjcSubsystem::ProjectScan()
// {
// 	const UPjcEditorSettings* EditorSettings = GetDefault<UPjcEditorSettings>();
// 	if (!EditorSettings) return;
//
// 	FPjcExcludeSettings ExcludeSettings;
//
// 	// ExcludeSettings.ExcludedPaths.Reserve(EditorSettings->ExcludedPaths.Num());
// 	// ExcludeSettings.ExcludedClassNames.Reserve(EditorSettings->ExcludedClasses.Num());
// 	// ExcludeSettings.ExcludedAssetObjectPaths.Reserve(EditorSettings->ExcludedAssets.Num());
// 	//
// 	// for (const auto& ExcludedPath : EditorSettings->ExcludedPaths)
// 	// {
// 	// 	const FString AssetPath = FPjcLibPath::ToAssetPath(ExcludedPath.Path);
// 	// 	if (AssetPath.IsEmpty()) continue;
// 	//
// 	// 	ExcludeSettings.ExcludedPaths.Emplace(AssetPath);
// 	// }
// 	//
// 	// for (const auto& ExcludedClass : EditorSettings->ExcludedClasses)
// 	// {
// 	// 	if (!ExcludedClass.LoadSynchronous()) continue;
// 	//
// 	// 	ExcludeSettings.ExcludedClassNames.Emplace(ExcludedClass.Get()->GetFName());
// 	// }
// 	//
// 	// for (const auto& ExcludedAsset : EditorSettings->ExcludedAssets)
// 	// {
// 	// 	if (!ExcludedAsset.LoadSynchronous()) continue;
// 	//
// 	// 	ExcludeSettings.ExcludedAssetObjectPaths.Emplace(ExcludedAsset.ToSoftObjectPath().GetAssetPathName());
// 	// }
//
// 	ProjectScanBySettings(ExcludeSettings, LastScanResult);
//
// 	if (!LastScanResult.bScanSuccess)
// 	{
// 		UE_LOG(LogProjectCleaner, Error, TEXT("%s"), *LastScanResult.ScanErrMsg);
// 		FPjcLibEditor::ShowNotificationWithOutputLog(LastScanResult.ScanErrMsg, SNotificationItem::ECompletionState::CS_Fail, 5.0f);
// 	}
//
// 	if (DelegateOnProjectScan.IsBound())
// 	{
// 		DelegateOnProjectScan.Broadcast(LastScanResult);
// 	}
// }
//
// void UPjcSubsystem::ProjectClean()
// {
// 	// const UPjcEditorSettings* EditorSettings = GetDefault<UPjcEditorSettings>();
// 	// if (!EditorSettings) return;
//
// 	FPjcLibEditor::ShaderCompilationDisable();
//
// 	ObjectTools::DeleteAssets(LastScanResult.ScanData.AssetsUnused);
//
// 	FPjcLibEditor::ShaderCompilationEnable();
// }
//
// void UPjcSubsystem::ProjectScanBySettings(const FPjcExcludeSettings& InExcludeSettings, FPjcScanResult& OutScanResult) const
// {
// 	FScopedSlowTask SlowTask{
// 		5.0f,
// 		FText::FromString(TEXT("Scanning Project...")),
// 		GIsEditor && !IsRunningCommandlet()
// 	};
// 	SlowTask.MakeDialog(false, false);
// 	SlowTask.EnterProgressFrame(1.0f);
//
// 	// Clear previous scan data
// 	OutScanResult.Clear();
//
// 	// Check for ongoing scanning or cleaning operations
// 	if (bScanningInProgress)
// 	{
// 		OutScanResult.bScanSuccess = false;
// 		OutScanResult.ScanErrMsg = TEXT("Scanning is in progress. Please wait until it has finished and then try again.");
// 		return;
// 	}
//
// 	if (bCleaningInProgress)
// 	{
// 		OutScanResult.bScanSuccess = false;
// 		OutScanResult.ScanErrMsg = TEXT("Cleaning is in progress. Please wait until it has finished and then try again.");
// 		return;
// 	}
//
// 	// Check if AssetRegistry is still discovering assets
// 	if (FPjcLibAsset::AssetRegistryWorking())
// 	{
// 		OutScanResult.bScanSuccess = false;
// 		OutScanResult.ScanErrMsg = TEXT("Scanning of the project has failed. AssetRegistry is still discovering assets. Please try again after it has finished.");
// 		return;
// 	}
//
// 	// Check if the editor is in play mode
// 	if (FPjcLibEditor::EditorInPlayMode())
// 	{
// 		OutScanResult.bScanSuccess = false;
// 		OutScanResult.ScanErrMsg = TEXT("Scanning of the project has failed. The editor is in play mode. Please exit play mode and try again.");
// 		return;
// 	}
//
// 	// Close all asset editors and fix redirectors if not running a commandlet
// 	if (!IsRunningCommandlet())
// 	{
// 		if (!GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->CloseAllAssetEditors())
// 		{
// 			OutScanResult.bScanSuccess = false;
// 			OutScanResult.ScanErrMsg = TEXT("Scanning of the project has failed because not all asset editors are closed.");
// 			return;
// 		}
//
// 		FPjcLibAsset::FixupRedirectorsInProject(true);
// 	}
//
// 	// Check if project still contains redirectors
// 	if (FPjcLibAsset::ProjectContainsRedirectors())
// 	{
// 		OutScanResult.bScanSuccess = false;
// 		OutScanResult.ScanErrMsg = TEXT("Failed to scan project, because not all redirectors are fixed.");
// 		return;
// 	}
//
// 	// Saving all unsaved assets in project, before scanning
// 	if (!FEditorFileUtils::SaveDirtyPackages(true, true, true, false, false, false))
// 	{
// 		OutScanResult.bScanSuccess = false;
// 		OutScanResult.ScanErrMsg = TEXT("Scanning of the project has failed because not all assets have been saved.");
// 		return;
// 	}
//
// 	const double ScanStartTime = FPlatformTime::Seconds();
//
// 	SlowTask.EnterProgressFrame(1.0f);
//
// 	ScanAssets(InExcludeSettings, OutScanResult);
//
// 	SlowTask.EnterProgressFrame(1.0f);
//
// 	ScanFiles(OutScanResult);
//
// 	SlowTask.EnterProgressFrame(1.0f);
//
// 	ScanFolders(OutScanResult);
//
// 	SlowTask.EnterProgressFrame(1.0f);
//
// 	ScanStatsUpdate(OutScanResult);
//
// 	const double ScanTime = FPlatformTime::Seconds() - ScanStartTime;
//
// 	UE_LOG(LogProjectCleaner, Display, TEXT("Project Scanned in %.2f seconds"), ScanTime);
// }
//
// void UPjcSubsystem::ScanProjectFiles(const UPjcFileScanSettings& InScanSettings, const bool bShowSlowTask, TSet<FString>& OutFilesExternal, TSet<FString>& OutFilesCorrupted) const
// {
// 	OutFilesExternal.Empty();
// 	OutFilesCorrupted.Empty();
//
// 	const bool bSlowTaskEnabled = bShowSlowTask && GEditor && !IsRunningCommandlet();
//
// 	FScopedSlowTask SlowTaskMain{
// 		1.0f,
// 		FText::FromString(TEXT("Scanning Content Directory Files...")),
// 		bSlowTaskEnabled
// 	};
// 	SlowTaskMain.MakeDialog(false, false);
// 	SlowTaskMain.EnterProgressFrame(1.0f);
//
// 	// first gathering all files inside projects 'Content' folder
// 	TSet<FString> FilesAll;
// 	FPjcLibPath::GetFilesInPath(FPjcLibPath::ContentDir(), true, FilesAll);
//
// 	// reserving some space
// 	OutFilesExternal.Reserve(FilesAll.Num());
// 	OutFilesCorrupted.Reserve(FilesAll.Num());
//
// 	FScopedSlowTask SlowTask{
// 		static_cast<float>(FilesAll.Num()),
// 		FText::FromString(TEXT("")),
// 		bSlowTaskEnabled
// 	};
//
// 	// traversing files and filtering only corrupted and external files
// 	for (const auto& File : FilesAll)
// 	{
// 		SlowTask.EnterProgressFrame(1.0f, FText::FromString(File));
//
// 		const FString FileExtension = FPjcLibPath::GetFileExtension(File, false).ToLower();
// 		
// 		// if (InScanSettings.ExcludedFiles.Contains(File)) continue;
// 		// if (InScanSettings.ExcludedFileExtensions.Contains(FileExtension)) continue;
//
// 		if (FPjcLibPath::IsExternalFile(File))
// 		{
// 			OutFilesExternal.Emplace(File);
// 		}
//
// 		if (FPjcLibPath::IsCorruptedAssetFile(File))
// 		{
// 			OutFilesCorrupted.Emplace(File);
// 		}
// 	}
//
// 	OutFilesExternal.Shrink();
// 	OutFilesCorrupted.Shrink();
// }
//
// FPjcDelegateOnProjectScan& UPjcSubsystem::OnProjectScan()
// {
// 	return DelegateOnProjectScan;
// }
//
// const FPjcScanResult& UPjcSubsystem::GetLastScanResult() const
// {
// 	return LastScanResult;
// }

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

// void UPjcSubsystem::ScanAssets(const FPjcExcludeSettings& InExcludeSettings, FPjcScanResult& OutScanResult) const
// {
// 	const double ScanStartTime = FPlatformTime::Seconds();
//
// 	TSet<FAssetData> AssetsAll;
// 	TSet<FAssetData> AssetsExcluded;
// 	TSet<FAssetData> AssetsUsed;
// 	TSet<FName> ClassNamesPrimary;
// 	TSet<FName> ClassNamesEditor;
//
// 	FPjcLibAsset::GetAssetsByPath(PjcConstants::PathRelRoot.ToString(), true, OutScanResult.ScanData.AssetsAll);
// 	FPjcLibAsset::GetAssetsIndirect(OutScanResult.ScanData.AssetsIndirect, OutScanResult.ScanData.AssetsIndirectInfos);
// 	FPjcLibAsset::GetAssetsExcluded(InExcludeSettings, OutScanResult.ScanData.AssetsExcluded);
// 	FPjcLibAsset::GetClassNamesPrimary(ClassNamesPrimary);
// 	FPjcLibAsset::GetClassNamesEditor(ClassNamesEditor);
//
// 	AssetsAll.Append(OutScanResult.ScanData.AssetsAll);
// 	AssetsExcluded.Append(OutScanResult.ScanData.AssetsExcluded);
// 	AssetsUsed.Append(OutScanResult.ScanData.AssetsIndirect);
// 	AssetsUsed.Append(OutScanResult.ScanData.AssetsExcluded);
//
// 	FScopedSlowTask SlowTask{
// 		static_cast<float>(OutScanResult.ScanData.AssetsAll.Num()),
// 		FText::FromString(TEXT("Scanning Assets...")),
// 		GIsEditor && !IsRunningCommandlet()
// 	};
// 	SlowTask.MakeDialog(false, false);
//
// 	for (const auto& Asset : OutScanResult.ScanData.AssetsAll)
// 	{
// 		SlowTask.EnterProgressFrame(1.0f, FText::FromString(Asset.ToSoftObjectPath().GetAssetPathString()));
//
// 		const bool bIsPrimary = FPjcLibAsset::AssetClassNameInList(Asset, ClassNamesPrimary);
// 		const bool bIsEditor = FPjcLibAsset::AssetClassNameInList(Asset, ClassNamesEditor);
// 		const bool bIsExtReferenced = FPjcLibAsset::AssetIsExtReferenced(Asset);
// 		const bool bIsUsed = bIsPrimary || bIsEditor || bIsExtReferenced || FPjcLibAsset::AssetIsMegascansBase(Asset);
//
// 		if (bIsPrimary)
// 		{
// 			OutScanResult.ScanData.AssetsPrimary.Emplace(Asset);
// 		}
//
// 		if (bIsEditor)
// 		{
// 			OutScanResult.ScanData.AssetsEditor.Emplace(Asset);
// 		}
//
// 		if (bIsExtReferenced)
// 		{
// 			OutScanResult.ScanData.AssetsExtReferenced.Emplace(Asset);
// 		}
//
// 		if (bIsUsed)
// 		{
// 			AssetsUsed.Emplace(Asset);
// 		}
// 	}
//
// 	TSet<FAssetData> AssetsUsedDependencies;
// 	FPjcLibAsset::GetAssetsDeps(AssetsUsed, AssetsUsedDependencies);
//
// 	const TSet<FAssetData> AssetsUnused = AssetsAll.Difference(AssetsUsedDependencies);
//
// 	OutScanResult.ScanData.AssetsUsed.Append(AssetsUsedDependencies.Array());
// 	OutScanResult.ScanData.AssetsUnused.Append(AssetsUnused.Array());
//
// 	const double ScanTime = FPlatformTime::Seconds() - ScanStartTime;
//
// 	UE_LOG(LogProjectCleaner, Display, TEXT("Assets scanned in %.2f seconds"), ScanTime);
// }
//
// void UPjcSubsystem::ScanFiles(FPjcScanResult& OutScanResult) const
// {
// 	const double ScanStartTime = FPlatformTime::Seconds();
//
// 	TSet<FString> Files;
// 	FPjcLibPath::GetFilesInPath(FPjcLibPath::ContentDir(), true, Files);
//
// 	OutScanResult.ScanData.FilesExternal.Reserve(Files.Num());
// 	OutScanResult.ScanData.AssetsCorrupted.Reserve(Files.Num());
//
// 	FScopedSlowTask SlowTask{
// 		static_cast<float>(Files.Num()),
// 		FText::FromString(TEXT("Scanning Project Files...")),
// 		GIsEditor && !IsRunningCommandlet()
// 	};
// 	SlowTask.MakeDialog(false, false);
//
// 	for (const auto& File : Files)
// 	{
// 		SlowTask.EnterProgressFrame(1.0f, FText::FromString(File));
//
// 		const FString FilePathAbs = FPjcLibPath::ToAbsolute(File);
// 		const FString FileExtension = FPjcLibPath::GetFileExtension(FilePathAbs, false).ToLower();
//
// 		if (PjcConstants::EngineFileExtensions.Contains(FileExtension))
// 		{
// 			const FAssetData AssetData = FPjcLibAsset::GetAssetByObjectPath(FPjcLibPath::ToObjectPath(FilePathAbs));
// 			if (!AssetData.IsValid())
// 			{
// 				OutScanResult.ScanData.AssetsCorrupted.Emplace(FilePathAbs);
// 			}
// 		}
// 		else
// 		{
// 			OutScanResult.ScanData.FilesExternal.Emplace(FilePathAbs);
// 		}
// 	}
//
// 	const double ScanTime = FPlatformTime::Seconds() - ScanStartTime;
//
// 	UE_LOG(LogProjectCleaner, Display, TEXT("Files scanned in %.2f seconds"), ScanTime);
// }
//
// void UPjcSubsystem::ScanFolders(FPjcScanResult& OutScanResult) const
// {
// 	const double ScanStartTime = FPlatformTime::Seconds();
//
// 	TSet<FString> Folders;
// 	FPjcLibPath::GetFoldersInPath(FPjcLibPath::ContentDir(), true, Folders);
//
// 	OutScanResult.ScanData.FoldersEmpty.Reserve(Folders.Num());
//
// 	FScopedSlowTask SlowTask{
// 		static_cast<float>(Folders.Num()),
// 		FText::FromString(TEXT("Scanning Project Folders...")),
// 		GIsEditor && !IsRunningCommandlet()
// 	};
// 	SlowTask.MakeDialog(false, false);
//
// 	for (const auto& Folder : Folders)
// 	{
// 		SlowTask.EnterProgressFrame(1.0f, FText::FromString(Folder));
//
// 		if (FPjcLibPath::IsPathEmpty(Folder) && !FPjcLibPath::IsPathEngineGenerated(Folder))
// 		{
// 			OutScanResult.ScanData.FoldersEmpty.Emplace(Folder);
// 		}
// 	}
//
// 	const double ScanTime = FPlatformTime::Seconds() - ScanStartTime;
//
// 	UE_LOG(LogProjectCleaner, Display, TEXT("Folders scanned in %.2f seconds"), ScanTime);
// }
//
// void UPjcSubsystem::ScanStatsUpdate(FPjcScanResult& InScanResult) const
// {
// 	InScanResult.ScanData.Shrink();
//
// 	InScanResult.ScanStats.NumAssetsTotal = InScanResult.ScanData.AssetsAll.Num();
// 	InScanResult.ScanStats.NumAssetsUnused = InScanResult.ScanData.AssetsUnused.Num();
// 	InScanResult.ScanStats.NumAssetsUsed = InScanResult.ScanData.AssetsUsed.Num();
// 	InScanResult.ScanStats.NumAssetsPrimary = InScanResult.ScanData.AssetsPrimary.Num();
// 	InScanResult.ScanStats.NumAssetsIndirect = InScanResult.ScanData.AssetsIndirect.Num();
// 	InScanResult.ScanStats.NumAssetsEditor = InScanResult.ScanData.AssetsEditor.Num();
// 	InScanResult.ScanStats.NumAssetsExcluded = InScanResult.ScanData.AssetsExcluded.Num();
// 	InScanResult.ScanStats.NumAssetsExtReferenced = InScanResult.ScanData.AssetsExtReferenced.Num();
// 	InScanResult.ScanStats.NumAssetsCorrupted = InScanResult.ScanData.AssetsCorrupted.Num();
// 	InScanResult.ScanStats.NumFilesExternal = InScanResult.ScanData.FilesExternal.Num();
// 	InScanResult.ScanStats.NumFoldersEmpty = InScanResult.ScanData.FoldersEmpty.Num();
//
// 	InScanResult.ScanStats.SizeAssetsTotal = FPjcLibAsset::GetAssetsSize(InScanResult.ScanData.AssetsAll);
// 	InScanResult.ScanStats.SizeAssetsUnused = FPjcLibAsset::GetAssetsSize(InScanResult.ScanData.AssetsUnused);
// 	InScanResult.ScanStats.SizeAssetsUsed = FPjcLibAsset::GetAssetsSize(InScanResult.ScanData.AssetsUsed);
// 	InScanResult.ScanStats.SizeAssetsPrimary = FPjcLibAsset::GetAssetsSize(InScanResult.ScanData.AssetsPrimary);
// 	InScanResult.ScanStats.SizeAssetsIndirect = FPjcLibAsset::GetAssetsSize(InScanResult.ScanData.AssetsIndirect);
// 	InScanResult.ScanStats.SizeAssetsEditor = FPjcLibAsset::GetAssetsSize(InScanResult.ScanData.AssetsEditor);
// 	InScanResult.ScanStats.SizeAssetsExcluded = FPjcLibAsset::GetAssetsSize(InScanResult.ScanData.AssetsExcluded);
// 	InScanResult.ScanStats.SizeAssetsExtReferenced = FPjcLibAsset::GetAssetsSize(InScanResult.ScanData.AssetsExtReferenced);
// 	InScanResult.ScanStats.SizeAssetsCorrupted = FPjcLibPath::GetFilesSize(InScanResult.ScanData.AssetsCorrupted);
// 	InScanResult.ScanStats.SizeFilesExternal = FPjcLibPath::GetFilesSize(InScanResult.ScanData.FilesExternal);
// }
