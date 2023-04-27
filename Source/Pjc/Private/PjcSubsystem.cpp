// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "PjcSubsystem.h"
#include "PjcConstants.h"
#include "Pjc.h"
#include "Libs/PjcLibPath.h"
#include "Libs/PjcLibAsset.h"
#include "Libs/PjcLibEditor.h"
// Engine Headers
#include "FileHelpers.h"
#include "EditorSettings/PjcEditorAssetExcludeSettings.h"
#include "Misc/ScopedSlowTask.h"

void UPjcSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UPjcSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

void UPjcSubsystem::ScanProject(const FPjcAssetExcludeSettings& InAssetExcludeSettings)
{
	FString ErrMsg;
	if (!CanScanProject(ErrMsg))
	{
		UE_LOG(LogProjectCleaner, Error, TEXT("%s"), *ErrMsg);
		return;
	}

	FScopedSlowTask SlowTask{
		2.0f,
		FText::FromString(TEXT("Scanning project...")),
		GIsEditor && !IsRunningCommandlet()
	};
	SlowTask.MakeDialog(false, false);

	SlowTask.EnterProgressFrame(1.0f);
	ScanProjectAssets(InAssetExcludeSettings);

	SlowTask.EnterProgressFrame(1.0f);
	ScanProjectFilesAndFolders();
}


const TArray<FAssetData>& UPjcSubsystem::GetAssetsAll() const
{
	return AssetsAll;
}

const TArray<FAssetData>& UPjcSubsystem::GetAssetsUsed() const
{
	return AssetsUsed;
}

const TArray<FAssetData>& UPjcSubsystem::GetAssetsUnused() const
{
	return AssetsUnused;
}

const TArray<FAssetData>& UPjcSubsystem::GetAssetsEditor() const
{
	return AssetsEditor;
}

const TArray<FAssetData>& UPjcSubsystem::GetAssetsPrimary() const
{
	return AssetsPrimary;
}

const TArray<FAssetData>& UPjcSubsystem::GetAssetsExcluded() const
{
	return AssetsExcluded;
}

const TArray<FAssetData>& UPjcSubsystem::GetAssetsIndirect() const
{
	return AssetsIndirect;
}

const TArray<FAssetData>& UPjcSubsystem::GetAssetsExtReferenced() const
{
	return AssetsExtReferenced;
}

const TSet<FString>& UPjcSubsystem::GetFilesExternal() const
{
	return FilesExternal;
}

const TSet<FString>& UPjcSubsystem::GetFilesCorrupted() const
{
	return FilesCorrupted;
}

const TSet<FString>& UPjcSubsystem::GetFoldersEmpty() const
{
	return FoldersEmpty;
}

void UPjcSubsystem::ScanProjectAssets()
{
	const UPjcEditorAssetExcludeSettings* EditorAssetExcludeSettings = GetDefault<UPjcEditorAssetExcludeSettings>();
	if (!EditorAssetExcludeSettings) return;

	FPjcAssetExcludeSettings ExcludeSettings;

	ExcludeSettings.ExcludedPackagePaths.Reserve(EditorAssetExcludeSettings->ExcludedPaths.Num());
	ExcludeSettings.ExcludedObjectPaths.Reserve(EditorAssetExcludeSettings->ExcludedAssets.Num());
	ExcludeSettings.ExcludedClassNames.Reserve(EditorAssetExcludeSettings->ExcludedClasses.Num());

	for (const auto& ExcludedPath : EditorAssetExcludeSettings->ExcludedPaths)
	{
		ExcludeSettings.ExcludedPackagePaths.Emplace(FPjcLibPath::ToAssetPath(ExcludedPath.Path));
	}

	for (const auto& ExcludedClass : EditorAssetExcludeSettings->ExcludedClasses)
	{
		if (!ExcludedClass.LoadSynchronous()) continue;

		ExcludeSettings.ExcludedClassNames.Emplace(ExcludedClass.Get()->GetFName());
	}

	for (const auto& ExcludedAsset : EditorAssetExcludeSettings->ExcludedAssets)
	{
		if (!ExcludedAsset.LoadSynchronous()) continue;

		ExcludeSettings.ExcludedObjectPaths.Emplace(ExcludedAsset.ToSoftObjectPath().GetAssetPathName());
	}

	ScanProjectAssets(ExcludeSettings);
}

void UPjcSubsystem::ScanProjectAssets(const FPjcAssetExcludeSettings& InAssetExcludeSettings)
{
	FString ErrMsg;
	if (!CanScanProject(ErrMsg))
	{
		UE_LOG(LogProjectCleaner, Error, TEXT("%s"), *ErrMsg);
		return;
	}

	const double StartTime = FPlatformTime::Seconds();

	FScopedSlowTask SlowTask{
		3.0f,
		FText::FromString(TEXT("Scanning project assets...")),
		GIsEditor && !IsRunningCommandlet()
	};
	SlowTask.MakeDialog(false, false);
	SlowTask.EnterProgressFrame(1.0f);

	bScanningInProgress = true;

	AssetsAll.Reset();
	AssetsUsed.Reset();
	AssetsUnused.Reset();
	AssetsEditor.Reset();
	AssetsPrimary.Reset();
	AssetsExcluded.Reset();
	AssetsIndirect.Reset();
	AssetsExtReferenced.Reset();
	AssetsIndirectInfo.Reset();

	TSet<FName> ClassNamesPrimary;
	TSet<FName> ClassNamesEditor;
	TSet<FName> ClassNamesExcluded{InAssetExcludeSettings.ExcludedClassNames};

	TSet<FAssetData> AssetsUsedInitial;
	TSet<FAssetData> AssetsUsedTotal;
	TSet<FAssetData> AssetsExcludedTotal;
	TArray<FAssetData> AssetsExcludedByPaths;
	TArray<FAssetData> AssetsExcludedByObjectPaths;
	TArray<FAssetData> AssetsMegascansReserved;

	FPjcLibAsset::GetAssetsByPath(PjcConstants::PathRelRoot, true, AssetsAll);
	FPjcLibAsset::GetAssetsByPath(PjcConstants::PathRelMSPresets, true, AssetsMegascansReserved);
	FPjcLibAsset::GetClassNamesPrimary(ClassNamesPrimary);
	FPjcLibAsset::GetClassNamesEditor(ClassNamesEditor);
	FPjcLibAsset::GetAssetsByPackagePaths(InAssetExcludeSettings.ExcludedPackagePaths, true, AssetsExcludedByPaths);
	FPjcLibAsset::GetAssetsByObjectPaths(InAssetExcludeSettings.ExcludedObjectPaths, AssetsExcludedByObjectPaths);
	FPjcLibAsset::GetAssetsIndirect(AssetsIndirectInfo);
	AssetsIndirectInfo.GetKeys(AssetsIndirect);

	AssetsExcludedTotal.Append(AssetsExcludedByPaths);
	AssetsExcludedTotal.Append(AssetsExcludedByObjectPaths);

	AssetsUsedInitial.Append(AssetsIndirect);
	AssetsUsedInitial.Append(AssetsMegascansReserved);

	SlowTask.EnterProgressFrame(1.0f);

	for (const auto& Asset : AssetsAll)
	{
		const bool bIsPrimary = FPjcLibAsset::AssetClassNameInList(Asset, ClassNamesPrimary);
		const bool bIsEditor = FPjcLibAsset::AssetClassNameInList(Asset, ClassNamesEditor);
		const bool bIsExcluded = FPjcLibAsset::AssetClassNameInList(Asset, ClassNamesExcluded);
		const bool bIsExtReferenced = FPjcLibAsset::AssetIsExtReferenced(Asset);
		const bool bIsUsed = bIsPrimary || bIsEditor || bIsExcluded || bIsExtReferenced;

		if (bIsPrimary)
		{
			AssetsPrimary.Emplace(Asset);
		}

		if (bIsEditor)
		{
			AssetsEditor.Emplace(Asset);
		}

		if (bIsExcluded)
		{
			AssetsExcludedTotal.Emplace(Asset);
		}

		if (bIsExtReferenced)
		{
			AssetsExtReferenced.Emplace(Asset);
		}

		if (bIsUsed)
		{
			AssetsUsedInitial.Emplace(Asset);
		}
	}

	SlowTask.EnterProgressFrame(1.0f);
	AssetsUsedInitial.Append(AssetsExcludedTotal);
	FPjcLibAsset::GetAssetsDeps(AssetsUsedInitial, AssetsUsedTotal);

	const TSet<FAssetData> AssetsAllSet{AssetsAll};
	const TSet<FAssetData> AssetsUnusedSet = AssetsAllSet.Difference(AssetsUsedTotal);

	AssetsExcluded = AssetsExcludedTotal.Array();
	AssetsUsed = AssetsUsedTotal.Array();
	AssetsUnused = AssetsUnusedSet.Array();

	bScanningInProgress = false;

	const double ElapsedTime = FPlatformTime::Seconds() - StartTime;
	UE_LOG(LogProjectCleaner, Display, TEXT("Project assets scanned in %f seconds"), ElapsedTime);

	if (DelegateOnScanAssets.IsBound())
	{
		DelegateOnScanAssets.Broadcast();
	}
}

void UPjcSubsystem::ScanProjectFilesAndFolders()
{
	const double StartTime = FPlatformTime::Seconds();

	TArray<FString> PathsAll;
	IFileManager::Get().FindFilesRecursive(PathsAll, *FPjcLibPath::ContentDir(), TEXT("*"), true, true);

	FilesExternal.Reset();
	FilesCorrupted.Reset();
	FoldersEmpty.Reset();

	FScopedSlowTask SlowTask{
		static_cast<float>(PathsAll.Num()),
		FText::FromString(TEXT("Scanning project files...")),
		GIsEditor && !IsRunningCommandlet()
	};
	SlowTask.MakeDialog(false, false);

	for (const auto& Path : PathsAll)
	{
		SlowTask.EnterProgressFrame(1.0f, FText::FromString(Path));

		const FString PathAbs = FPjcLibPath::ToAbsolute(Path);

		if (FPjcLibPath::IsDir(PathAbs))
		{
			if (FPjcLibPath::IsPathEmpty(PathAbs) && !FPjcLibPath::IsPathEngineGenerated(PathAbs))
			{
				FoldersEmpty.Emplace(PathAbs);
			}
		}
		else
		{
			if (FPjcLibPath::IsExternalFile(PathAbs))
			{
				FilesExternal.Emplace(PathAbs);
			}

			if (FPjcLibPath::IsCorruptedAssetFile(PathAbs))
			{
				FilesCorrupted.Emplace(PathAbs);
			}
		}
	}

	const double ElapsedTime = FPlatformTime::Seconds() - StartTime;
	UE_LOG(LogProjectCleaner, Display, TEXT("Project files and folders scanned in %f seconds"), ElapsedTime);

	if (DelegateOnScanFiles.IsBound())
	{
		DelegateOnScanFiles.Broadcast();
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
		if (GEditor && !GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->CloseAllAssetEditors())
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
