// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "ProjectCleanerSubsystem.h"
#include "ProjectCleanerConstants.h"
#include "ProjectCleaner.h"
#include "Settings/ProjectCleanerExcludeSettings.h"
// Engine Headers
#include "AssetToolsModule.h"
#include "AssetViewUtils.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Internationalization/Regex.h"
#include "Misc/FileHelper.h"
#include "Misc/ScopedSlowTask.h"
#include "EditorUtilityBlueprint.h"
#include "EditorUtilityWidget.h"
#include "EditorUtilityWidgetBlueprint.h"
#include "FileHelpers.h"
#include "ObjectTools.h"
#include "Engine/MapBuildDataRegistry.h"
#include "Engine/AssetManager.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Libs/ProjectCleanerLibAsset.h"
#include "Libs/ProjectCleanerLibEditor.h"
#include "Libs/ProjectCleanerLibPath.h"
#include "Widgets/Notifications/SNotificationList.h"

UProjectCleanerSubsystem::UProjectCleanerSubsystem()
	:
	PlatformFile(&FPlatformFileManager::Get().GetPlatformFile()),
	ModuleAssetRegistry(&FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName)),
	ModuleAssetTools(&FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")))
{
}

void UProjectCleanerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UProjectCleanerSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

#if WITH_EDITOR
void UProjectCleanerSubsystem::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	SaveConfig();
}
#endif

void UProjectCleanerSubsystem::ProjectScan()
{
	const UProjectCleanerExcludeSettings* ExcludeSettings = GetDefault<UProjectCleanerExcludeSettings>();
	if (!ExcludeSettings) return;

	FProjectCleanerScanSettings NewScanSettings;

	for (const auto& ExcludedFolder : ExcludeSettings->ExcludedFolders)
	{
		if (ExcludedFolder.Path.IsEmpty()) continue;

		const FString PathAbs = UProjectCleanerLibPath::ConvertToAbs(ExcludedFolder.Path);
		if (PathAbs.IsEmpty() || !FPaths::DirectoryExists(PathAbs)) continue;

		NewScanSettings.ExcludedFolders.AddUnique(ExcludedFolder.Path);
	}

	for (const auto& ExcludedClasses : ExcludeSettings->ExcludedClasses)
	{
		if (!ExcludedClasses.LoadSynchronous()) continue;

		NewScanSettings.ExcludedClasses.AddUnique(ExcludedClasses.Get());
	}

	for (const auto& ExcludedAsset : ExcludeSettings->ExcludedAssets)
	{
		if (!ExcludedAsset.LoadSynchronous()) continue;

		NewScanSettings.ExcludedAssets.AddUnique(ExcludedAsset.Get());
	}

	ProjectScan(NewScanSettings);
}

void UProjectCleanerSubsystem::ProjectScan(const FProjectCleanerScanSettings& InScanSettings)
{
	ScanSettings = InScanSettings;

	ScanDataReset();

	if (UProjectCleanerLibAsset::AssetRegistryWorking())
	{
		ScanData.ScanResult = EProjectCleanerScanResult::AssetRegistryWorking;
		ScanData.ScanResultMsg = ScanResultToString(ScanData.ScanResult);
		return;
	}

	if (UProjectCleanerLibEditor::EditorInPlayMode())
	{
		ScanData.ScanResult = EProjectCleanerScanResult::EditorInPlayMode;
		ScanData.ScanResultMsg = ScanResultToString(ScanData.ScanResult);
		return;
	}

	if (bScanningProject)
	{
		ScanData.ScanResult = EProjectCleanerScanResult::ScanningInProgress;
		ScanData.ScanResultMsg = ScanResultToString(ScanData.ScanResult);
		return;
	}

	if (bCleaningProject)
	{
		ScanData.ScanResult = EProjectCleanerScanResult::CleaningInProgress;
		ScanData.ScanResultMsg = ScanResultToString(ScanData.ScanResult);
		return;
	}

	GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->CloseAllAssetEditors();
	UProjectCleanerLibAsset::FixupRedirectors();

	if (!FEditorFileUtils::SaveDirtyPackages(false, true, true, false, false, false))
	{
		ScanData.ScanResult = EProjectCleanerScanResult::FailedToSaveAssets;
		ScanData.ScanResultMsg = ScanResultToString(ScanData.ScanResult);
		return;
	}

	bScanningProject = true;

	FScopedSlowTask SlowTaskScan{
		9.0f,
		FText::FromString(TEXT("Scanning project...")),
		GIsEditor && !IsRunningCommandlet()
	};
	SlowTaskScan.MakeDialog();

	SlowTaskScan.EnterProgressFrame(1.0f);

	FindAssetsTotal();

	SlowTaskScan.EnterProgressFrame(1.0f);

	FindAssetsPrimary();

	SlowTaskScan.EnterProgressFrame(1.0f);

	FindAssetsIndirect();

	SlowTaskScan.EnterProgressFrame(1.0f);

	FindAssetsExcluded();

	SlowTaskScan.EnterProgressFrame(1.0f);

	FindAssetsUsed();

	SlowTaskScan.EnterProgressFrame(1.0f);

	FindAssetsUnused();

	SlowTaskScan.EnterProgressFrame(1.0f);

	FindFilesCorrupted();

	SlowTaskScan.EnterProgressFrame(1.0f);

	FindFilesNonEngine();

	SlowTaskScan.EnterProgressFrame(1.0f);

	FindFolders();

	bScanningProject = false;
	ScanData.ScanResult = EProjectCleanerScanResult::Success;

	if (DelegateProjectScanned.IsBound())
	{
		DelegateProjectScanned.Broadcast();
	}
}

void UProjectCleanerSubsystem::ProjectClean(const bool bRemoveEmptyFolders)
{
	if (ScanData.ScanResult != EProjectCleanerScanResult::Success) return;
	if (ScanData.AssetsUnused.Num() == 0) return;

	bCleaningProject = true;

	int32 AssetsDeletedNum = 0;
	const int32 AssetsTotalNum = ScanData.AssetsUnused.Num();

	TArray<FAssetData> Bucket;
	TArray<UObject*> LoadedAssets;
	LoadedAssets.Reserve(ProjectCleanerConstants::DeletionBucketSize);
	Bucket.Reserve(ProjectCleanerConstants::DeletionBucketSize);

	FScopedSlowTask SlowTask(
		AssetsTotalNum,
		FText::FromString(TEXT("Removing unused assets..."))
	);
	SlowTask.MakeDialog();

	while (ScanData.AssetsUnused.Num() > 0)
	{
		BucketFill(Bucket, ProjectCleanerConstants::DeletionBucketSize);

		if (Bucket.Num() == 0)
		{
			break;
		}

		if (!BucketPrepare(Bucket, LoadedAssets))
		{
			UE_LOG(LogProjectCleaner, Error, TEXT("Failed to load some assets. Aborting."))
			break;
		}

		AssetsDeletedNum += BucketDelete(LoadedAssets);
		SlowTask.EnterProgressFrame(
			Bucket.Num(),
			FText::FromString(FString::Printf(TEXT("Deleted %d of %d assets"), AssetsDeletedNum, AssetsTotalNum))
		);

		Bucket.Reset();
		LoadedAssets.Reset();
	}

	// Cleaning empty packages
	const TSet<FName> EmptyPackages = ModuleAssetRegistry->Get().GetCachedEmptyPackages();
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

	bCleaningProject = false;

	if (bRemoveEmptyFolders)
	{
		ProjectCleanEmptyFolders();
	}

	ProjectScan();

	TArray<FString> FocusFolders;
	FocusFolders.Add(ProjectCleanerConstants::PathRelRoot.ToString());

	const FContentBrowserModule& CBModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	CBModule.Get().SyncBrowserToFolders(FocusFolders);
}

void UProjectCleanerSubsystem::ProjectCleanEmptyFolders()
{
	if (ScanData.ScanResult != EProjectCleanerScanResult::Success) return;
	if (ScanData.FoldersEmpty.Num() == 0) return;

	bCleaningProject = true;

	FScopedSlowTask SlowTask{
		static_cast<float>(ScanData.FoldersEmpty.Num()),
		FText::FromString(TEXT("Removing empty folders...")),
		GIsEditor && !IsRunningCommandlet()
	};
	SlowTask.MakeDialog();

	int32 DeletedFolderNum = 0;

	ScanData.FoldersEmpty.Sort([](const FString& FolderA, const FString& FolderB)
	{
		return FolderA.Len() > FolderB.Len();
	});

	for (const auto& EmptyFolder : ScanData.FoldersEmpty)
	{
		SlowTask.EnterProgressFrame(1.0f, FText::FromString(FString::Printf(TEXT("Removing %s folder"), *EmptyFolder)));

		if (!IFileManager::Get().DeleteDirectory(*EmptyFolder, true, false))
		{
			UE_LOG(LogProjectCleaner, Error, TEXT("Failed to remove %s folder"), *EmptyFolder);
		}
		else
		{
			++DeletedFolderNum;
		}
	}

	const bool bSuccess = DeletedFolderNum == ScanData.FoldersEmpty.Num();
	const FString Title = FString::Printf(TEXT("Deleted %d of %d folders"), DeletedFolderNum, ScanData.FoldersEmpty.Num());
	FNotificationInfo Info{FText::FromString(Title)};
	Info.ExpireDuration = bSuccess ? 5.0f : 10.0f;
	if (!bSuccess)
	{
		Info.Hyperlink = FSimpleDelegate::CreateLambda([]()
		{
			FGlobalTabmanager::Get()->TryInvokeTab(FName{TEXT("OutputLog")});
		});
		Info.HyperlinkText = FText::FromString(TEXT("Show OutputLog..."));
	}
	const auto NotificationPtr = FSlateNotificationManager::Get().AddNotification(Info);
	if (NotificationPtr.IsValid())
	{
		NotificationPtr.Get()->SetCompletionState(bSuccess ? SNotificationItem::CS_Success : SNotificationItem::CS_Fail);
	}

	bCleaningProject = false;

	ProjectScan();
}

const FProjectCleanerScanData& UProjectCleanerSubsystem::GetScanData() const
{
	return ScanData;
}

bool UProjectCleanerSubsystem::CanScanProject() const
{
	return !UProjectCleanerLibAsset::AssetRegistryWorking() && !UProjectCleanerLibEditor::EditorInPlayMode() && !ScanningInProgress() && !CleaningInProgress();
}

bool UProjectCleanerSubsystem::AssetIsExcluded(const FAssetData& AssetData) const
{
	return AssetExcludedByPath(AssetData) || AssetExcludedByClass(AssetData) || AssetExcludedByObject(AssetData);
}

bool UProjectCleanerSubsystem::ScanningInProgress() const
{
	return bScanningProject;
}

bool UProjectCleanerSubsystem::CleaningInProgress() const
{
	return bCleaningProject;
}

FProjectCleanerDelegateProjectScanned& UProjectCleanerSubsystem::OnProjectScanned()
{
	return DelegateProjectScanned;
}

bool UProjectCleanerSubsystem::FolderIsEngineGenerated(const FString& FolderPathAbs) const
{
	TSet<FString> Folders;
	Folders.Reserve(4);
	Folders.Add(FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() / TEXT("Collections")));
	Folders.Add(FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() / TEXT("Developers")));
	Folders.Add(FPaths::ConvertRelativePathToFull(FPaths::GameDevelopersDir() / FPaths::GameUserDeveloperFolderName()));
	Folders.Add(FPaths::ConvertRelativePathToFull(FPaths::GameUserDeveloperDir() / TEXT("Collections")));

	return Folders.Contains(FolderPathAbs);
}

bool UProjectCleanerSubsystem::CanShowFolder(const FString& FolderPathAbs) const
{
	const FString PathAbs = UProjectCleanerLibPath::ConvertToAbs(FolderPathAbs);
	if (PathAbs.IsEmpty()) return false;

	if (FolderIsEngineGenerated(FolderPathAbs))
	{
		TArray<FAssetData> Assets;
		ModuleAssetRegistry->Get().GetAssetsByPath(FName{UProjectCleanerLibPath::ConvertToRel(PathAbs)}, Assets, true);

		return Assets.Num() > 0;
	}

	return true;
}

FString UProjectCleanerSubsystem::ScanResultToString(const EProjectCleanerScanResult ScanResult)
{
	switch (ScanResult)
	{
		case EProjectCleanerScanResult::None: return TEXT("");
		case EProjectCleanerScanResult::Success: return TEXT("");
		case EProjectCleanerScanResult::AssetRegistryWorking: return TEXT("Cant scan project because AssetRegistry still working");
		case EProjectCleanerScanResult::EditorInPlayMode: return TEXT("Cant scan project because Editor is in Play mode");
		case EProjectCleanerScanResult::ScanningInProgress: return TEXT("Scanning in progress");
		case EProjectCleanerScanResult::CleaningInProgress: return TEXT("Cleaning in progress");
		case EProjectCleanerScanResult::FailedToSaveAssets: return TEXT("Cant scan project because failed to save some assets");
		default: return TEXT("");
	}
}

void UProjectCleanerSubsystem::FindAssetsTotal()
{
	ModuleAssetRegistry->Get().GetAssetsByPath(ProjectCleanerConstants::PathRelRoot, ScanData.AssetsAll, true);
	// todo:ashe23 for ue5 make sure we exclude __External*__ folders
}

void UProjectCleanerSubsystem::FindAssetsPrimary()
{
	FScopedSlowTask SlowTask{
		3.0f,
		FText::FromString(TEXT("Scanning for primary assets...")),
		GIsEditor && !IsRunningCommandlet()
	};
	SlowTask.MakeDialog();

	TArray<FName> PrimaryAssetClasses;
	// getting list of primary asset classes that are defined in AssetManager
	const auto& AssetManager = UAssetManager::Get();
	if (!AssetManager.IsValid()) return;

	SlowTask.EnterProgressFrame(1.0f, FText::FromString(TEXT("Searching assets in AssetManager...")));

	TArray<FPrimaryAssetTypeInfo> AssetTypeInfos;
	AssetManager.Get().GetPrimaryAssetTypeInfoList(AssetTypeInfos);
	PrimaryAssetClasses.Reserve(AssetTypeInfos.Num());

	for (const auto& AssetTypeInfo : AssetTypeInfos)
	{
		if (!AssetTypeInfo.AssetBaseClassLoaded) continue;

		PrimaryAssetClasses.AddUnique(AssetTypeInfo.AssetBaseClassLoaded->GetFName());
	}

	SlowTask.EnterProgressFrame(1.0f, FText::FromString(TEXT("Searching assets that derived from primary assets...")));

	// getting list of primary assets classes that are derived from main primary assets
	TSet<FName> DerivedFromPrimaryAssets;
	{
		const TSet<FName> ExcludedClassNames;
		ModuleAssetRegistry->Get().GetDerivedClassNames(PrimaryAssetClasses, ExcludedClassNames, DerivedFromPrimaryAssets);
	}

	for (const auto& DerivedClassName : DerivedFromPrimaryAssets)
	{
		PrimaryAssetClasses.AddUnique(DerivedClassName);
	}

	FARFilter Filter;
	Filter.bRecursiveClasses = true;
	Filter.bRecursivePaths = true;
	Filter.PackagePaths.Add(ProjectCleanerConstants::PathRelRoot);

	for (const auto& ClassName : PrimaryAssetClasses)
	{
		Filter.ClassNames.Add(ClassName);
	}
	ModuleAssetRegistry->Get().GetAssets(Filter, ScanData.AssetsPrimary);

	SlowTask.EnterProgressFrame(1.0f, FText::FromString(TEXT("Searching blueprint primary assets...")));

	// getting primary blueprint assets
	FARFilter FilterBlueprint;
	FilterBlueprint.bRecursivePaths = true;
	FilterBlueprint.bRecursiveClasses = true;
	FilterBlueprint.PackagePaths.Add(ProjectCleanerConstants::PathRelRoot);
	FilterBlueprint.ClassNames.Add(UBlueprint::StaticClass()->GetFName());

	TArray<FAssetData> BlueprintAssets;
	ModuleAssetRegistry->Get().GetAssets(FilterBlueprint, BlueprintAssets);

	for (const auto& BlueprintAsset : BlueprintAssets)
	{
		const FName BlueprintClass = FName{*UProjectCleanerLibAsset::GetAssetClassName(BlueprintAsset)};
		if (PrimaryAssetClasses.Contains(BlueprintClass))
		{
			ScanData.AssetsPrimary.AddUnique(BlueprintAsset);
		}
	}
}

void UProjectCleanerSubsystem::FindAssetsIndirect()
{
	FScopedSlowTask SlowTask{
		2.0f,
		FText::FromString(TEXT("Scanning for indirect assets...")),
		GIsEditor && !IsRunningCommandlet()
	};
	SlowTask.MakeDialog();
	SlowTask.EnterProgressFrame(1.0f, FText::FromString(TEXT("Preparing scan directories...")));

	const FString SourceDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() + TEXT("Source/"));
	const FString ConfigDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() + TEXT("Config/"));
	const FString PluginsDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() + TEXT("Plugins/"));

	TSet<FString> Files;
	Files.Reserve(200); // reserving some space

	// 1) finding all source files in main project "Source" directory (<yourproject>/Source/*)
	TArray<FString> FilesToScan;
	PlatformFile->FindFilesRecursively(FilesToScan, *SourceDir, TEXT(".cs"));
	PlatformFile->FindFilesRecursively(FilesToScan, *SourceDir, TEXT(".cpp"));
	PlatformFile->FindFilesRecursively(FilesToScan, *SourceDir, TEXT(".h"));
	PlatformFile->FindFilesRecursively(FilesToScan, *ConfigDir, TEXT(".ini"));
	Files.Append(FilesToScan);

	// 2) we should find all source files in plugins folder (<yourproject>/Plugins/*)
	TArray<FString> ProjectPluginsFiles;
	// finding all installed plugins in "Plugins" directory
	struct FDirectoryVisitor : IPlatformFile::FDirectoryVisitor
	{
		virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory) override
		{
			if (bIsDirectory)
			{
				InstalledPlugins.Add(FilenameOrDirectory);
			}

			return true;
		}

		TArray<FString> InstalledPlugins;
	};

	FDirectoryVisitor Visitor;
	PlatformFile->IterateDirectory(*PluginsDir, Visitor);

	// 3) for every installed plugin we scanning only "Source" and "Config" folders
	for (const auto& Dir : Visitor.InstalledPlugins)
	{
		const FString PluginSourcePathDir = Dir + "/Source";
		const FString PluginConfigPathDir = Dir + "/Config";

		PlatformFile->FindFilesRecursively(ProjectPluginsFiles, *PluginSourcePathDir, TEXT(".cs"));
		PlatformFile->FindFilesRecursively(ProjectPluginsFiles, *PluginSourcePathDir, TEXT(".cpp"));
		PlatformFile->FindFilesRecursively(ProjectPluginsFiles, *PluginSourcePathDir, TEXT(".h"));
		PlatformFile->FindFilesRecursively(ProjectPluginsFiles, *PluginConfigPathDir, TEXT(".ini"));
	}

	Files.Append(ProjectPluginsFiles);
	Files.Shrink();

	FScopedSlowTask SlowTaskFiles{
		static_cast<float>(Files.Num()),
		FText::FromString(TEXT("Scanning files for indirect asset usages...")),
		GIsEditor && !IsRunningCommandlet()
	};
	SlowTaskFiles.MakeDialog();

	for (const auto& File : Files)
	{
		SlowTaskFiles.EnterProgressFrame(1.0f, FText::FromString(FString::Printf(TEXT("Scanning %s"), *File)));

		if (!PlatformFile->FileExists(*File)) continue;

		FString FileContent;
		FFileHelper::LoadFileToString(FileContent, *File);

		if (FileContent.IsEmpty()) continue;

		static FRegexPattern Pattern(TEXT(R"(\/Game([A-Za-z0-9_.\/]+)\b)"));
		FRegexMatcher Matcher(Pattern, FileContent);
		while (Matcher.FindNext())
		{
			FString FoundedAssetObjectPath = Matcher.GetCaptureGroup(0);

			// if ObjectPath ends with "_C" , then its probably blueprint, so we trim that
			if (FoundedAssetObjectPath.EndsWith("_C"))
			{
				FString TrimmedObjectPath = FoundedAssetObjectPath;
				TrimmedObjectPath.RemoveFromEnd("_C");

				FoundedAssetObjectPath = TrimmedObjectPath;
			}

			const FAssetData* AssetData = ScanData.AssetsAll.FindByPredicate([&](const FAssetData& Elem)
			{
				return
					Elem.ObjectPath.ToString() == (FoundedAssetObjectPath) ||
					Elem.PackageName.ToString() == (FoundedAssetObjectPath);
			});

			if (!AssetData) continue;

			// if founded asset is ok, we loading file by lines to determine on what line its used
			TArray<FString> Lines;
			FFileHelper::LoadFileToStringArray(Lines, *File);
			for (int32 i = 0; i < Lines.Num(); ++i)
			{
				if (!Lines.IsValidIndex(i)) continue;
				if (!Lines[i].Contains(FoundedAssetObjectPath)) continue;

				FProjectCleanerIndirectAssetInfo IndirectAsset;
				IndirectAsset.AssetData = *AssetData;
				IndirectAsset.FilePath = FPaths::ConvertRelativePathToFull(File);
				IndirectAsset.LineNum = i + 1;
				ScanData.AssetsIndirectInfo.AddUnique(IndirectAsset);
				ScanData.AssetsIndirect.AddUnique(*AssetData);
			}
		}
	}

	SlowTask.EnterProgressFrame(1.0f);
}

void UProjectCleanerSubsystem::FindAssetsExcluded()
{
	FScopedSlowTask SlowTask{
		static_cast<float>(ScanData.AssetsAll.Num()),
		FText::FromString(TEXT("Scanning for excluded assets based on specified settings...")),
		GIsEditor && !IsRunningCommandlet()
	};
	SlowTask.MakeDialog();

	ScanData.AssetsExcluded.Reserve(ScanData.AssetsAll.Num());

	for (const auto& Asset : ScanData.AssetsAll)
	{
		SlowTask.EnterProgressFrame(1.0f, FText::FromString(FString::Printf(TEXT("%s"), *Asset.AssetName.ToString())));

		if (AssetIsExcluded(Asset))
		{
			ScanData.AssetsExcluded.AddUnique(Asset);
		}
	}

	ScanData.AssetsExcluded.Shrink();
}

void UProjectCleanerSubsystem::FindAssetsUsed()
{
	FScopedSlowTask SlowTask{
		3.0f,
		FText::FromString(TEXT("Searching for used assets..")),
		GIsEditor && !IsRunningCommandlet()
	};
	SlowTask.MakeDialog();

	SlowTask.EnterProgressFrame(1.0f, FText::FromString(TEXT("Searching assets with external referencers..")));

	TArray<FAssetData> AssetsWithExternalRefs;
	AssetsWithExternalRefs.Reserve(ScanData.AssetsAll.Num());

	TArray<FName> Refs;
	for (const auto& Asset : ScanData.AssetsAll)
	{
		ModuleAssetRegistry->Get().GetReferencers(Asset.PackageName, Refs);

		const bool HasExternalRefs = Refs.ContainsByPredicate([](const FName& Ref)
		{
			return !Ref.ToString().StartsWith(ProjectCleanerConstants::PathRelRoot.ToString());
		});

		if (HasExternalRefs)
		{
			AssetsWithExternalRefs.AddUnique(Asset);
		}

		Refs.Reset();
	}

	AssetsWithExternalRefs.Shrink();

	SlowTask.EnterProgressFrame(1.0f, FText::FromString(TEXT("Searching editor or plugin specific assets...")));

	TArray<FAssetData> AssetsEditor;
	TArray<FAssetData> AssetsMegascans;

	FARFilter FilterEditorAssets;
	FilterEditorAssets.bRecursivePaths = true;
	FilterEditorAssets.bRecursiveClasses = true;
	FilterEditorAssets.PackagePaths.Add(ProjectCleanerConstants::PathRelRoot);
	FilterEditorAssets.ClassNames.Add(UEditorUtilityWidget::StaticClass()->GetFName());
	FilterEditorAssets.ClassNames.Add(UEditorUtilityBlueprint::StaticClass()->GetFName());
	FilterEditorAssets.ClassNames.Add(UEditorUtilityWidgetBlueprint::StaticClass()->GetFName());
	FilterEditorAssets.ClassNames.Add(UMapBuildDataRegistry::StaticClass()->GetFName());
	ModuleAssetRegistry->Get().GetAssets(FilterEditorAssets, AssetsEditor);

	if (FModuleManager::Get().IsModuleLoaded(TEXT("MegascansPlugin")))
	{
		ModuleAssetRegistry->Get().GetAssetsByPath(ProjectCleanerConstants::PathRelMSPresets, AssetsMegascans, true);
	}

	SlowTask.EnterProgressFrame(1.0f, FText::FromString(TEXT("Searching for used assets dependencies...")));

	ScanData.AssetsUsed.Reserve(
		ScanData.AssetsPrimary.Num() + ScanData.AssetsIndirect.Num() + AssetsWithExternalRefs.Num() + AssetsEditor.Num() + AssetsMegascans.Num() + ScanData.AssetsExcluded.Num()
	);

	for (const auto& Asset : ScanData.AssetsPrimary)
	{
		ScanData.AssetsUsed.AddUnique(Asset);
	}

	for (const auto& Asset : ScanData.AssetsIndirect)
	{
		ScanData.AssetsUsed.AddUnique(Asset);
	}

	for (const auto& Asset : AssetsWithExternalRefs)
	{
		ScanData.AssetsUsed.AddUnique(Asset);
	}

	for (const auto& Asset : AssetsEditor)
	{
		ScanData.AssetsUsed.AddUnique(Asset);
	}

	for (const auto& Asset : AssetsMegascans)
	{
		ScanData.AssetsUsed.AddUnique(Asset);
	}

	for (const auto& Asset : ScanData.AssetsExcluded)
	{
		ScanData.AssetsUsed.AddUnique(Asset);
	}

	TArray<FAssetData> AssetsDependencies;
	UProjectCleanerLibAsset::GetAssetsDependencies(ScanData.AssetsUsed, AssetsDependencies);

	ScanData.AssetsUsed.Reserve(ScanData.AssetsUsed.Num() + AssetsDependencies.Num());

	for (const auto& Asset : AssetsDependencies)
	{
		ScanData.AssetsUsed.AddUnique(Asset);
	}

	ScanData.AssetsUsed.Shrink();
}

void UProjectCleanerSubsystem::FindAssetsUnused()
{
	FScopedSlowTask SlowTask{
		static_cast<float>(ScanData.AssetsAll.Num()),
		FText::FromString(TEXT("Searching for unused assets...")),
		GIsEditor && !IsRunningCommandlet()
	};
	SlowTask.MakeDialog();

	ScanData.AssetsUnused.Reserve(ScanData.AssetsAll.Num());

	for (const auto& Asset : ScanData.AssetsAll)
	{
		SlowTask.EnterProgressFrame(1.0f, FText::FromString(FString::Printf(TEXT("%s"), *Asset.AssetName.ToString())));

		if (!ScanData.AssetsUsed.Contains(Asset))
		{
			ScanData.AssetsUnused.AddUnique(Asset);
		}
	}

	ScanData.AssetsUnused.Shrink();
}

void UProjectCleanerSubsystem::FindFilesCorrupted()
{
	TArray<FString> Files;
	IFileManager::Get().FindFilesRecursive(Files, *FPaths::ProjectContentDir(), TEXT("*.*"), true, false);

	FScopedSlowTask SlowTask{
		static_cast<float>(Files.Num()),
		FText::FromString(TEXT("Searching for corrupted files...")),
		GIsEditor && !IsRunningCommandlet()
	};
	SlowTask.MakeDialog();

	ScanData.FilesCorrupted.Reserve(Files.Num());

	for (const auto& File : Files)
	{
		SlowTask.EnterProgressFrame(1.0f, FText::FromString(FString::Printf(TEXT("%s"), *File)));

		const FString FilePathAbs = FPaths::ConvertRelativePathToFull(File);
		if (!FileHasEngineExtension(FilePathAbs)) continue;
		if (!FileIsCorrupted(FilePathAbs)) continue;

		ScanData.FilesCorrupted.AddUnique(FilePathAbs);
	}

	ScanData.FilesCorrupted.Shrink();
}

void UProjectCleanerSubsystem::FindFilesNonEngine()
{
	TArray<FString> Files;
	IFileManager::Get().FindFilesRecursive(Files, *FPaths::ProjectContentDir(), TEXT("*.*"), true, false);

	FScopedSlowTask SlowTask{
		static_cast<float>(Files.Num()),
		FText::FromString(TEXT("Searching for non engine files...")),
		GIsEditor && !IsRunningCommandlet()
	};
	SlowTask.MakeDialog();

	ScanData.FilesNonEngine.Reserve(Files.Num());

	for (const auto& File : Files)
	{
		SlowTask.EnterProgressFrame(1.0f, FText::FromString(FString::Printf(TEXT("%s"), *File)));

		const FString FilePathAbs = FPaths::ConvertRelativePathToFull(File);
		if (FileHasEngineExtension(FilePathAbs)) continue;

		ScanData.FilesNonEngine.AddUnique(FilePathAbs);
	}

	ScanData.FilesNonEngine.Shrink();
}

void UProjectCleanerSubsystem::FindFolders()
{
	TArray<FString> Folders;
	IFileManager::Get().FindFilesRecursive(Folders, *FPaths::ProjectContentDir(), TEXT("*.*"), false, true);
	
	TArray<FAssetData> Assets;
	for (const auto& Folder : Folders)
	{
		const FString FolderPathAbs = FPaths::ConvertRelativePathToFull(Folder);
	
		ScanData.FoldersAll.AddUnique(FolderPathAbs);
	
		if (UProjectCleanerLibPath::FolderIsEmpty(FolderPathAbs) && !UProjectCleanerLibPath::FolderIsExcluded(FolderPathAbs))
		{
			ScanData.FoldersEmpty.AddUnique(FolderPathAbs);
		}
	}
}

void UProjectCleanerSubsystem::ScanDataReset()
{
	ScanData.ScanResult = EProjectCleanerScanResult::None;
	ScanData.ScanResultMsg.Reset();
	ScanData.AssetsAll.Reset();
	ScanData.AssetsUsed.Reset();
	ScanData.AssetsPrimary.Reset();
	ScanData.AssetsIndirect.Reset();
	ScanData.AssetsIndirectInfo.Reset();
	ScanData.AssetsExcluded.Reset();
	ScanData.AssetsUnused.Reset();
	ScanData.FoldersAll.Reset();
	ScanData.FoldersEmpty.Reset();
	ScanData.FilesCorrupted.Reset();
	ScanData.FilesNonEngine.Reset();
}

bool UProjectCleanerSubsystem::AssetExcludedByPath(const FAssetData& AssetData) const
{
	for (const auto& ExcludedFolder : ScanSettings.ExcludedFolders)
	{
		const FString FolderPathAbs = UProjectCleanerLibPath::ConvertToAbs(ExcludedFolder);
		const FString AssetPathAbs = UProjectCleanerLibPath::ConvertToAbs(AssetData.PackagePath.ToString());

		if (FolderPathAbs.IsEmpty() || AssetPathAbs.IsEmpty()) continue;
		if (FPaths::IsUnderDirectory(AssetPathAbs, FolderPathAbs))
		{
			return true;
		}
	}

	return false;
}

bool UProjectCleanerSubsystem::AssetExcludedByClass(const FAssetData& AssetData) const
{
	const FString AssetClassName = UProjectCleanerLibAsset::GetAssetClassName(AssetData);

	for (const auto& ExcludedClass : ScanSettings.ExcludedClasses)
	{
		if (!ExcludedClass) continue;

		const FString ExcludedClassName = ExcludedClass->GetName();
		if (AssetClassName.Equals(ExcludedClassName))
		{
			return true;
		}
	}

	return false;
}

bool UProjectCleanerSubsystem::AssetExcludedByObject(const FAssetData& AssetData) const
{
	for (const auto& ExcludedAsset : ScanSettings.ExcludedAssets)
	{
		if (!ExcludedAsset.LoadSynchronous()) continue;

		if (ExcludedAsset.ToSoftObjectPath() == AssetData.ToSoftObjectPath())
		{
			return true;
		}
	}

	return false;
}

bool UProjectCleanerSubsystem::FileHasEngineExtension(const FString& InFilePath)
{
	const FString Extension = FPaths::GetExtension(InFilePath).ToLower();

	TSet<FString> EngineExtensions;
	EngineExtensions.Reserve(3);
	EngineExtensions.Add(TEXT("uasset"));
	EngineExtensions.Add(TEXT("umap"));
	EngineExtensions.Add(TEXT("collection"));

	return EngineExtensions.Contains(Extension);
}

bool UProjectCleanerSubsystem::FileIsCorrupted(const FString& InFilePathAbs) const
{
	if (!FPaths::FileExists(InFilePathAbs)) return false;

	const FString RelativePath = UProjectCleanerLibPath::ConvertToRel(InFilePathAbs);

	// here we got absolute path "C:/MyProject/Content/material.uasset"
	// we must first convert that path to In Engine Internal Path like "/Game/material.uasset"
	// const FString RelativePath = Convert(InFilePathAbs, EProjectCleanerPathType::Relative);
	if (RelativePath.IsEmpty()) return false;

	// Converting file path to object path (This is for searching in AssetRegistry)
	// example "/Game/Name.uasset" => "/Game/Name.Name"
	FString ObjectPath = RelativePath;
	ObjectPath.RemoveFromEnd(FPaths::GetExtension(RelativePath, true));
	ObjectPath.Append(TEXT(".") + FPaths::GetBaseFilename(RelativePath));

	const FAssetData AssetData = ModuleAssetRegistry->Get().GetAssetByObjectPath(FName{*ObjectPath});

	// if its does not exist in asset registry, then something wrong with asset
	return !AssetData.IsValid();
}

void UProjectCleanerSubsystem::BucketFill(TArray<FAssetData>& Bucket, const int32 BucketSize)
{
	// Searching Root assets
	int32 Index = 0;
	TArray<FName> Refs;
	while (Bucket.Num() < BucketSize && ScanData.AssetsUnused.IsValidIndex(Index))
	{
		const FAssetData CurrentAsset = ScanData.AssetsUnused[Index];
		ModuleAssetRegistry->Get().GetReferencers(CurrentAsset.PackageName, Refs);
		Refs.RemoveAllSwap([&](const FName& Ref)
		{
			return !Ref.ToString().StartsWith(ProjectCleanerConstants::PathRelRoot.ToString()) || Ref.IsEqual(CurrentAsset.PackageName);
		}, false);
		Refs.Shrink();

		if (Refs.Num() == 0)
		{
			Bucket.AddUnique(CurrentAsset);
			ScanData.AssetsUnused.RemoveAt(Index);
		}

		Refs.Reset();

		++Index;
	}

	if (Bucket.Num() > 0)
	{
		return;
	}

	// if root assets not found, we deleting assets single by finding its referencers
	if (ScanData.AssetsUnused.Num() == 0)
	{
		return;
	}

	TArray<FAssetData> Stack;
	Stack.Add(ScanData.AssetsUnused[0]);

	while (Stack.Num() > 0)
	{
		const FAssetData Current = Stack.Pop(false);
		Bucket.AddUnique(Current);
		ScanData.AssetsUnused.Remove(Current);

		ModuleAssetRegistry->Get().GetReferencers(Current.PackageName, Refs);

		Refs.RemoveAllSwap([&](const FName& Ref)
		{
			return !Ref.ToString().StartsWith(ProjectCleanerConstants::PathRelRoot.ToString()) || Ref.IsEqual(Current.PackageName);
		}, false);
		Refs.Shrink();

		for (const auto& Ref : Refs)
		{
			const FString ObjectPath = Ref.ToString() + TEXT(".") + FPaths::GetBaseFilename(*Ref.ToString());
			const FAssetData AssetData = ModuleAssetRegistry->Get().GetAssetByObjectPath(FName{*ObjectPath});
			if (AssetData.IsValid())
			{
				if (!Bucket.Contains(AssetData))
				{
					Stack.Add(AssetData);
				}

				Bucket.AddUnique(AssetData);
				ScanData.AssetsUnused.Remove(AssetData);
			}
		}

		Refs.Reset();
	}
}

bool UProjectCleanerSubsystem::BucketPrepare(const TArray<FAssetData>& Bucket, TArray<UObject*>& LoadedAssets) const
{
	TArray<FString> ObjectPaths;
	ObjectPaths.Reserve(Bucket.Num());

	for (const auto& Asset : Bucket)
	{
		ObjectPaths.Add(Asset.ObjectPath.ToString());
	}

	return AssetViewUtils::LoadAssetsIfNeeded(ObjectPaths, LoadedAssets, false, true);
}

int32 UProjectCleanerSubsystem::BucketDelete(const TArray<UObject*>& LoadedAssets) const
{
	int32 DeletedAssetsNum = ObjectTools::DeleteObjects(LoadedAssets, false);

	if (DeletedAssetsNum == 0)
	{
		DeletedAssetsNum = ObjectTools::ForceDeleteObjects(LoadedAssets, false);
	}

	return DeletedAssetsNum;
}
