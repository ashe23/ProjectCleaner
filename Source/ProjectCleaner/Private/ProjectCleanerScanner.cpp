// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "ProjectCleanerScanner.h"
#include "ProjectCleaner.h"
#include "ProjectCleanerConstants.h"
#include "Libs/ProjectCleanerLibPath.h"
#include "Libs/ProjectCleanerLibAsset.h"
// Engine Headers
#include "AssetToolsModule.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "EditorUtilityBlueprint.h"
#include "EditorUtilityWidget.h"
#include "EditorUtilityWidgetBlueprint.h"
#include "FileHelpers.h"
#include "Engine/AssetManager.h"
#include "Engine/MapBuildDataRegistry.h"
#include "Misc/ScopedSlowTask.h"

FProjectCleanerScanner::FProjectCleanerScanner(const EProjectCleanerScanMethod InScanMethod)
	: ScanMethod(InScanMethod),
	  ScanState(EProjectCleanerScanState::Idle),
	  ModuleAssetRegistry(FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName)),
	  ModuleAssetTools(FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"))),
	  PlatformFile(FPlatformFileManager::Get().GetPlatformFile())
{
}

void FProjectCleanerScanner::Scan(const FProjectCleanerScanSettings& InScanSettings)
{
	if (ModuleAssetRegistry.Get().IsLoadingAssets())
	{
		UE_LOG(LogProjectCleaner, Warning, TEXT("Cant start scanning project, because AssetRegistry still working."));
		return;
	}

	if (GEditor && GEditor->PlayWorld || GIsPlayInEditorWorld)
	{
		UE_LOG(LogProjectCleaner, Warning, TEXT("Cant start scanning project, because editor is in play mode."));
		return;
	}

	// we only scan Content (/Game) folder
	const FString ScanPathRel = UProjectCleanerLibPath::Convert(InScanSettings.ScanPath, EProjectCleanerPathType::Relative);
	if (ScanPathRel.IsEmpty() || !ScanPathRel.StartsWith(ProjectCleanerConstants::PathRelRoot.ToString()))
	{
		UE_LOG(LogProjectCleaner, Error, TEXT("Invalid Scan Path %s. Only Content folder allowed to scan"), *InScanSettings.ScanPath);
		return;
	}

	RunPreScanActions();

	// 1. searching for forbidden stuff
	FindForbiddenAssets();
	FindForbiddenFolders();

	// 2. searching for used assets
	FindAssetsUsed();
	ModuleAssetRegistry.Get().GetAssetsByPath(ProjectCleanerConstants::PathRelRoot, ScanResult.AssetsAll, true);


	RunPostScanActions();
}

void FProjectCleanerScanner::GetScanResult(FProjectCleanerScanResult& InScanResult)
{
}

void FProjectCleanerScanner::FindForbiddenFolders()
{
	FoldersForbidden.Reset();

	FoldersForbidden.Add(UProjectCleanerLibPath::FolderCollections(EProjectCleanerPathType::Absolute));
	FoldersForbidden.Add(UProjectCleanerLibPath::FolderDeveloperCollections(EProjectCleanerPathType::Absolute));
	// todo:ashe23 for ue5 add __ExternalObject__ and __ExternalActors__ folders

	if (FModuleManager::Get().IsModuleLoaded(ProjectCleanerConstants::PluginNameMegascans))
	{
		FoldersForbidden.Add(UProjectCleanerLibPath::FolderMsPresets(EProjectCleanerPathType::Absolute));
	}
}

void FProjectCleanerScanner::FindForbiddenAssets()
{
	AssetsForbidden.Reset();

	FARFilter Filter;
	Filter.bRecursivePaths = true;
	Filter.bRecursiveClasses = true;
	Filter.PackagePaths.Add(ProjectCleanerConstants::PathRelRoot);
	Filter.ClassNames.Add(UEditorUtilityWidget::StaticClass()->GetFName());
	Filter.ClassNames.Add(UEditorUtilityBlueprint::StaticClass()->GetFName());
	Filter.ClassNames.Add(UEditorUtilityWidgetBlueprint::StaticClass()->GetFName());
	Filter.ClassNames.Add(UMapBuildDataRegistry::StaticClass()->GetFName());

	ModuleAssetRegistry.Get().GetAssets(Filter, AssetsForbidden);
}

void FProjectCleanerScanner::FindAssetsUsed()
{
	// 1. searching for primary assets

	TArray<FAssetData> AssetsPrimary;
	TArray<FName> PrimaryAssetClasses;

	// 1.1 getting list of primary asset classes that are defined in AssetManager
	const auto& AssetManager = UAssetManager::Get();
	if (!AssetManager.IsValid()) return;

	TArray<FPrimaryAssetTypeInfo> AssetTypeInfos;
	AssetManager.Get().GetPrimaryAssetTypeInfoList(AssetTypeInfos);
	PrimaryAssetClasses.Reserve(AssetTypeInfos.Num());

	for (const auto& AssetTypeInfo : AssetTypeInfos)
	{
		if (!AssetTypeInfo.AssetBaseClassLoaded) continue;

		PrimaryAssetClasses.AddUnique(AssetTypeInfo.AssetBaseClassLoaded->GetFName());
	}

	// 1.2 getting list of primary assets classes that are derived from main primary assets
	TSet<FName> DerivedFromPrimaryAssets;
	{
		const TSet<FName> ExcludedClassNames;
		ModuleAssetRegistry.Get().GetDerivedClassNames(PrimaryAssetClasses, ExcludedClassNames, DerivedFromPrimaryAssets);
	}

	for (const auto& DerivedClassName : DerivedFromPrimaryAssets)
	{
		PrimaryAssetClasses.AddUnique(DerivedClassName);
	}

	FARFilter Filter;
	Filter.bRecursiveClasses = true;
	Filter.bRecursivePaths = true;
	Filter.PackagePaths.Add(FName{ProjectCleanerConstants::PathRelRoot});

	for (const auto& ClassName : PrimaryAssetClasses)
	{
		Filter.ClassNames.Add(ClassName);
	}
	ModuleAssetRegistry.Get().GetAssets(Filter, AssetsPrimary);

	FARFilter FilterBlueprint;
	FilterBlueprint.bRecursivePaths = true;
	FilterBlueprint.bRecursiveClasses = true;
	FilterBlueprint.PackagePaths.Add(ProjectCleanerConstants::PathRelRoot);
	FilterBlueprint.ClassNames.Add(UBlueprint::StaticClass()->GetFName());

	TArray<FAssetData> BlueprintAssets;
	ModuleAssetRegistry.Get().GetAssets(FilterBlueprint, BlueprintAssets);

	for (const auto& BlueprintAsset : BlueprintAssets)
	{
		const FName BlueprintClass = FName{*UProjectCleanerLibAsset::GetAssetClassName(BlueprintAsset)};
		if (PrimaryAssetClasses.Contains(BlueprintClass))
		{
			AssetsPrimary.AddUnique(BlueprintAsset);
		}
	}
}

void FProjectCleanerScanner::RunPreScanActions()
{
	// before we start scanning we should fixup redirectors
	FScopedSlowTask FixRedirectorsTask{
		1.0f,
		FText::FromString(ProjectCleanerConstants::MsgFixingRedirectors)
	};
	FixRedirectorsTask.MakeDialog();

	FARFilter Filter;
	Filter.bRecursivePaths = true;
	Filter.PackagePaths.Emplace(ProjectCleanerConstants::PathRelRoot);
	Filter.ClassNames.Emplace(UObjectRedirector::StaticClass()->GetFName());

	TArray<FAssetData> AssetList;
	ModuleAssetRegistry.Get().GetAssets(Filter, AssetList);

	if (AssetList.Num() == 0) return;

	FScopedSlowTask FixRedirectorsLoadingTask(
		AssetList.Num(),
		FText::FromString(ProjectCleanerConstants::MsgLoadingRedirectors)
	);
	FixRedirectorsLoadingTask.MakeDialog();

	TArray<UObjectRedirector*> Redirectors;
	Redirectors.Reserve(AssetList.Num());

	for (const auto& Asset : AssetList)
	{
		FixRedirectorsLoadingTask.EnterProgressFrame();

		UObject* AssetObj = Asset.GetAsset();
		if (!AssetObj) continue;

		UObjectRedirector* Redirector = CastChecked<UObjectRedirector>(AssetObj);
		if (!Redirector) continue;

		Redirectors.Add(Redirector);
	}

	Redirectors.Shrink();

	ModuleAssetTools.Get().FixupReferencers(Redirectors);

	FixRedirectorsTask.EnterProgressFrame(1.0f);

	// and save all assets
	FEditorFileUtils::SaveDirtyPackages(false, true, true, false, false, false);

	ScanState = EProjectCleanerScanState::Scanning;

	ScanResult.Reset();
}

void FProjectCleanerScanner::RunPostScanActions()
{
	ScanState = EProjectCleanerScanState::Idle;
	// todo:ashe23 add on scan finish delegate maybe?
}
