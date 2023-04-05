// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "PjcSubsystem.h"
#include "Pjc.h"
#include "PjcConstants.h"
#include "PjcExcludeSettings.h"
#include "Libs/PjcLibPath.h"
#include "Libs/PjcLibAsset.h"
#include "Libs/PjcLibEditor.h"
// Engine Headers
#include "FileHelpers.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/MapBuildDataRegistry.h"
#include "Misc/ScopedSlowTask.h"

void UPjcSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
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

void UPjcSubsystem::ProjectScan()
{
	const UPjcExcludeSettings* ExcludeSettings = GetDefault<UPjcExcludeSettings>();
	if (!ExcludeSettings) return;

	FScopedSlowTask SlowTask{
		2.0f,
		FText::FromString(TEXT("Scanning project...")),
		GIsEditor && !IsRunningCommandlet()
	};
	SlowTask.MakeDialog(false, false);
	SlowTask.EnterProgressFrame(1.0f);

	FPjcScanSettings ScanSettings;

	ScanSettings.ExcludedPaths.Reserve(ExcludeSettings->ExcludedPaths.Num());
	ScanSettings.ExcludedClassNames.Reserve(ExcludeSettings->ExcludedClasses.Num());
	ScanSettings.ExcludedObjectPaths.Reserve(ExcludeSettings->ExcludedAssets.Num());

	for (const auto& ExcludedPath : ExcludeSettings->ExcludedPaths)
	{
		const FString AssetPath = FPjcLibPath::ToAssetPath(ExcludedPath.Path);
		if (AssetPath.IsEmpty()) continue;

		ScanSettings.ExcludedPaths.Emplace(AssetPath);
	}

	for (const auto& ExcludedClass : ExcludeSettings->ExcludedClasses)
	{
		if (!ExcludedClass.LoadSynchronous()) continue;

		ScanSettings.ExcludedClassNames.Emplace(ExcludedClass.Get()->GetFName());
	}

	for (const auto& ExcludedAsset : ExcludeSettings->ExcludedAssets)
	{
		if (!ExcludedAsset.LoadSynchronous()) continue;

		ScanSettings.ExcludedObjectPaths.Emplace(ExcludedAsset.ToSoftObjectPath().GetAssetPathName());
	}

	SlowTask.EnterProgressFrame(1.0f);
	ProjectScanBySettings(ScanSettings, LastScanResult);

	if (DelegateOnProjectScan.IsBound())
	{
		DelegateOnProjectScan.Broadcast(LastScanResult);
	}
}

const FPjcScanResult& UPjcSubsystem::GetLastScanResult()
{
	return LastScanResult;
}

void UPjcSubsystem::ProjectScanBySettings(const FPjcScanSettings& InScanSettings, FPjcScanResult& OutScanResult) const
{
	// Clear previous scan data
	OutScanResult.Clear();

	// Initialize ScanResult with default success state
	OutScanResult.bSuccess = true;
	OutScanResult.ErrMsg.Empty();

	// Check for ongoing scanning or cleaning operations
	if (bScanningInProgress)
	{
		OutScanResult.bSuccess = false;
		OutScanResult.ErrMsg = TEXT("Scanning is in progress. Please wait until it has finished and then try again.");
		return;
	}

	if (bCleaningInProgress)
	{
		OutScanResult.bSuccess = false;
		OutScanResult.ErrMsg = TEXT("Cleaning is in progress. Please wait until it has finished and then try again.");
		return;
	}

	// Check if AssetRegistry is still discovering assets
	if (FPjcLibAsset::AssetRegistryWorking())
	{
		OutScanResult.bSuccess = false;
		OutScanResult.ErrMsg = TEXT("Scanning of the project has failed. AssetRegistry is still discovering assets. Please try again after it has finished.");
		return;
	}

	// Check if the editor is in play mode
	if (FPjcLibEditor::EditorInPlayMode())
	{
		OutScanResult.bSuccess = false;
		OutScanResult.ErrMsg = TEXT("Scanning of the project has failed. The editor is in play mode. Please exit play mode and try again.");
		return;
	}

	// Updating asset registry
	FPjcLibAsset::AssetRegistryUpdate();

	// Close all asset editors and fix redirectors if not running a commandlet
	if (!IsRunningCommandlet())
	{
		if (!GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->CloseAllAssetEditors())
		{
			OutScanResult.bSuccess = false;
			OutScanResult.ErrMsg = TEXT("Scanning of the project has failed because not all asset editors are closed.");
			return;
		}

		FPjcLibAsset::FixupRedirectorsInProject(true);
	}

	// Check if project still contains redirectors
	if (FPjcLibAsset::ProjectContainsRedirectors())
	{
		OutScanResult.bSuccess = false;
		OutScanResult.ErrMsg = TEXT("Failed to scan project, because not all redirectors are fixed.");
		return;
	}

	// Saving all unsaved assets in project, before scanning
	if (!FEditorFileUtils::SaveDirtyPackages(true, true, true, false, false, false))
	{
		OutScanResult.bSuccess = false;
		OutScanResult.ErrMsg = TEXT("Scanning of the project has failed because not all assets have been saved.");
		return;
	}

	struct FContentFolderVisitor : IPlatformFile::FDirectoryVisitor
	{
		FCriticalSection Mutex;
		const FPjcScanSettings& ScanSettings;
		FPjcScanResult& ScanResult;

		explicit FContentFolderVisitor(const FPjcScanSettings& InScanSettings, FPjcScanResult& InScanResult) : ScanSettings(InScanSettings), ScanResult(InScanResult)
		{
			ValidatedExcludedPaths.Empty(ScanSettings.ExcludedPaths.Num());
			ValidatedExcludedClassNames.Empty(ScanSettings.ExcludedClassNames.Num());
			ValidatedExcludedObjectPaths.Empty(ScanSettings.ExcludedObjectPaths.Num());

			for (const auto& ExcludedPath : ScanSettings.ExcludedPaths)
			{
				const FString AssetPath = FPjcLibPath::ToAssetPath(ExcludedPath);
				if (AssetPath.IsEmpty()) continue;

				ValidatedExcludedPaths.Emplace(AssetPath);
			}

			for (const auto& ExcludedClassName : ScanSettings.ExcludedClassNames)
			{
				if (!FPjcLibAsset::IsValidClassName(ExcludedClassName)) continue;

				ValidatedExcludedClassNames.Emplace(ExcludedClassName);
			}

			for (const auto& ExcludedObjectPath : ScanSettings.ExcludedObjectPaths)
			{
				const FName ObjectPath = FPjcLibPath::ToObjectPath(ExcludedObjectPath.ToString());
				if (ObjectPath == NAME_None) continue;

				ValidatedExcludedObjectPaths.Emplace(ObjectPath);
			}

			FPjcLibAsset::GetClassNamesPrimary(ClassNamesPrimary);
			FPjcLibAsset::GetClassNamesEditor(ClassNamesEditor);
			FPjcLibAsset::GetAssetsIndirect(ScanResult.AssetsIndirectInfos);

			ScanResult.AssetsIndirectInfos.GetKeys(ScanResult.AssetsIndirect);
		}

		virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory) override
		{
			FScopeLock ScopeLock(&Mutex);

			const FString PathAbsolute = FPjcLibPath::ToAbsolute(FilenameOrDirectory);

			if (bIsDirectory)
			{
				ScanResult.FoldersTotal.Emplace(PathAbsolute);

				const bool bPathIsEngineGenerated = FPjcLibPath::IsPathEngineGenerated(PathAbsolute);
				const bool bPathIsExcluded = PathIsExcluded(PathAbsolute);

				if (FPjcLibPath::IsPathEmpty(PathAbsolute) && !bPathIsEngineGenerated && !bPathIsExcluded)
				{
					ScanResult.FoldersEmpty.Emplace(PathAbsolute);
				}

				return true;
			}

			ScanResult.FilesTotal.Emplace(PathAbsolute);

			const FString FileExtension = FPjcLibPath::GetFileExtension(PathAbsolute, false);

			if (PjcConstants::EngineFileExtensions.Contains(FileExtension))
			{
				// now we must check if asset registered in AssetRegistry
				const FName ObjectPath = FPjcLibPath::ToObjectPath(PathAbsolute);
				const FAssetData AssetData = FPjcLibAsset::GetAssetByObjectPath(ObjectPath);

				if (AssetData.IsValid())
				{
					ScanResult.FilesAsset.Emplace(PathAbsolute);
					ScanResult.AssetsAll.Emplace(AssetData);

					const FName AssetClassName = FPjcLibAsset::GetAssetClassName(AssetData);

					const bool bAssetIsPrimary = ClassNamesPrimary.Contains(AssetData.AssetClass) || ClassNamesPrimary.Contains(AssetClassName);
					const bool bAssetIsEditor = ClassNamesEditor.Contains(AssetData.AssetClass) || ClassNamesEditor.Contains(AssetClassName);
					const bool bAssetIsExtReferenced = FPjcLibAsset::AssetIsExtReferenced(AssetData);
					const bool bAssetIsIndirect = ScanResult.AssetsIndirect.Contains(AssetData);
					const bool bAssetIsExcluded = AssetIsExcluded(AssetData);
					const bool bAssetIsUsed = bAssetIsPrimary || bAssetIsEditor || bAssetIsExtReferenced || bAssetIsIndirect || bAssetIsExcluded;

					if (bAssetIsPrimary)
					{
						ScanResult.AssetsPrimary.Emplace(AssetData);
					}

					if (bAssetIsEditor)
					{
						ScanResult.AssetsEditor.Emplace(AssetData);
					}

					if (bAssetIsExtReferenced)
					{
						ScanResult.AssetsExtReferenced.Emplace(AssetData);
					}

					if (bAssetIsExcluded)
					{
						ScanResult.AssetsExcluded.Emplace(AssetData);
					}

					if (bAssetIsUsed)
					{
						TSet<FAssetData> Deps;
						FPjcLibAsset::GetAssetDeps(AssetData, Deps);

						ScanResult.AssetsUsed.Emplace(AssetData);
						ScanResult.AssetsUsed.Append(Deps);
					}
				}
				else
				{
					ScanResult.FilesCorruptedAsset.Emplace(PathAbsolute);
				}
			}
			else
			{
				ScanResult.FilesNonAsset.Emplace(PathAbsolute);
			}

			return true;
		}

	private:
		bool AssetIsExcluded(const FAssetData& InAssetData) const
		{
			if (!InAssetData.IsValid()) return false;

			if (ValidatedExcludedObjectPaths.Contains(InAssetData.ObjectPath)) return true;

			for (const auto ExcludedPath : ValidatedExcludedPaths)
			{
				if (InAssetData.PackagePath.ToString().StartsWith(ExcludedPath))
				{
					return true;
				}
			}

			const FName AssetClassName = FPjcLibAsset::GetAssetClassName(InAssetData);

			return ValidatedExcludedClassNames.Contains(InAssetData.AssetClass) || ValidatedExcludedClassNames.Contains(AssetClassName);
		}

		bool PathIsExcluded(const FString& InPath) const
		{
			for (const auto& ExcludedPath : ValidatedExcludedPaths)
			{
				const FString PathAbs = FPjcLibPath::ToAbsolute(ExcludedPath);
				if (InPath.StartsWith(PathAbs))
				{
					return true;
				}
			}

			return false;
		}

		TSet<FString> ValidatedExcludedPaths;
		TSet<FName> ValidatedExcludedClassNames;
		TSet<FName> ValidatedExcludedObjectPaths;
		TSet<FName> ClassNamesPrimary;
		TSet<FName> ClassNamesEditor;
	};

	const double ScanStartTime = FPlatformTime::Seconds();

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	FContentFolderVisitor Visitor{InScanSettings, OutScanResult};
	PlatformFile.IterateDirectoryRecursively(*FPjcLibPath::ContentDir(), Visitor);

	OutScanResult.AssetsUnused = OutScanResult.AssetsAll.Difference(OutScanResult.AssetsUsed);

	const double ScanTime = FPlatformTime::Seconds() - ScanStartTime;

	UE_LOG(LogProjectCleaner, Display, TEXT("Project Scanned in %.2f seconds"), ScanTime);
}

void UPjcSubsystem::ProjectScan2(const FPjcScanSettings& InScanSettings)
{
	const double ScanStartTime = FPlatformTime::Seconds();

	TArray<FString> FilesTotal;
	IFileManager::Get().FindFilesRecursive(FilesTotal, *FPjcLibPath::ContentDir(), TEXT("*"), true, false);

	// initial data
	TSet<FName> ClassNamesPrimary;
	TSet<FName> ClassNamesEditor;
	TSet<FName> ClassNamesExcluded{InScanSettings.ExcludedClassNames};

	FPjcLibAsset::GetClassNamesPrimary(ClassNamesPrimary);
	FPjcLibAsset::GetClassNamesEditor(ClassNamesEditor);

	TMap<FAssetData, TArray<FPjcAssetUsageInfo>> AssetsIndirectInfos;
	FPjcLibAsset::GetAssetsIndirect(AssetsIndirectInfos);

	TSet<FAssetData> AssetsExcluded;
	TSet<FAssetData> AssetsIndirect;
	FPjcLibAsset::GetAssetsExcluded(InScanSettings, AssetsExcluded);
	AssetsIndirectInfos.GetKeys(AssetsIndirect);

	// data
	TSet<FAssetData> AssetsTotal;
	TSet<FAssetData> AssetsPrimary;
	TSet<FAssetData> AssetsEditor;
	TSet<FAssetData> AssetsExtReferenced;
	TSet<FAssetData> AssetsUsed;
	
	TSet<FString> FilesNonAsset;
	TSet<FString> FilesCorrupted;

	AssetsTotal.Reserve(FilesTotal.Num());
	AssetsPrimary.Reserve(FilesTotal.Num());
	AssetsEditor.Reserve(FilesTotal.Num());
	AssetsExtReferenced.Reserve(FilesTotal.Num());
	AssetsUsed.Reserve(FilesTotal.Num());
	
	FilesNonAsset.Reserve(FilesTotal.Num());
	FilesCorrupted.Reserve(FilesTotal.Num());

	FScopedSlowTask SlowTask{
		static_cast<float>(FilesTotal.Num()),
		FText::FromString(TEXT("Scanning content directory files...")),
		GIsEditor && !IsRunningCommandlet()
	};
	SlowTask.MakeDialog(false, false);

	for (const FString& File : FilesTotal)
	{
		SlowTask.EnterProgressFrame(1.0f, FText::FromString(FString::Printf(TEXT("%s"), *File)));

		const FString FilePathAbs = FPaths::ConvertRelativePathToFull(File);
		const FString FileExtension = FPaths::GetExtension(FilePathAbs, false);

		if (PjcConstants::EngineFileExtensions.Contains(FileExtension))
		{
			const FName ObjectPath = FPjcLibPath::ToObjectPath(FilePathAbs);
			const FAssetData AssetData = FPjcLibAsset::GetAssetByObjectPath(ObjectPath);

			if (AssetData.IsValid())
			{
				AssetsTotal.Emplace(AssetData);

				const bool bIsPrimaryAsset = FPjcLibAsset::AssetClassNameInList(AssetData, ClassNamesPrimary);
				const bool bIsEditorAsset = FPjcLibAsset::AssetClassNameInList(AssetData, ClassNamesEditor);
				const bool bIsIndirectAsset = AssetsIndirect.Contains(AssetData);
				const bool bIsExtReferencedAsset = FPjcLibAsset::AssetIsExtReferenced(AssetData);
				const bool bIsExcludedAsset =  FPjcLibAsset::AssetClassNameInList(AssetData, ClassNamesExcluded) || AssetsExcluded.Contains(AssetData);
				const bool bIsMegascansAsset = FPjcLibAsset::AssetIsMegascansBase(AssetData);
				const bool bIsUsedAsset = bIsPrimaryAsset || bIsEditorAsset || bIsIndirectAsset || bIsExtReferencedAsset || bIsExcludedAsset || bIsMegascansAsset;

				if (bIsPrimaryAsset)
				{
					AssetsPrimary.Emplace(AssetData);
				}

				if (bIsEditorAsset)
				{
					AssetsEditor.Emplace(AssetData);
				}

				if (bIsExtReferencedAsset)
				{
					AssetsExtReferenced.Emplace(AssetData);
				}

				if (bIsExcludedAsset)
				{
					AssetsExcluded.Emplace(AssetData);
				}

				if (bIsUsedAsset)
				{
					TSet<FAssetData> Deps;
					FPjcLibAsset::GetAssetDeps(AssetData, Deps);

					AssetsUsed.Emplace(AssetData);
					AssetsUsed.Append(Deps);
				}
			}
			else
			{
				FilesCorrupted.Emplace(FilePathAbs);
			}
		}
		else
		{
			FilesNonAsset.Emplace(FilePathAbs);
		}
	}

	AssetsTotal.Shrink();
	AssetsPrimary.Shrink();
	AssetsEditor.Shrink();
	AssetsExtReferenced.Shrink();
	AssetsUsed.Shrink();
	FilesNonAsset.Shrink();
	FilesCorrupted.Shrink();

	const TSet<FAssetData> AssetsUnused = AssetsTotal.Difference(AssetsUsed);

	const double ScanTime = FPlatformTime::Seconds() - ScanStartTime;
	UE_LOG(LogProjectCleaner, Display, TEXT("Project Scanned in %f seconds"), ScanTime);

	return;
}

FPjcDelegateOnProjectScan& UPjcSubsystem::OnProjectScan()
{
	return DelegateOnProjectScan;
}
