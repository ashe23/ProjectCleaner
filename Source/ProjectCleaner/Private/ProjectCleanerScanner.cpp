// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "ProjectCleanerScanner.h"
#include "ProjectCleaner.h"
#include "ProjectCleanerConstants.h"
#include "Libs/ProjectCleanerLibPath.h"
// Engine Headers
#include "AssetToolsModule.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "EditorUtilityBlueprint.h"
#include "EditorUtilityWidget.h"
#include "EditorUtilityWidgetBlueprint.h"
#include "FileHelpers.h"
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

void FProjectCleanerScanner::Scan(const FProjectCleanerScanSettings& ScanSettings)
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


	RunPreScanActions();

	// 1. searching for forbidden stuff
	FindForbiddenAssets();
	FindForbiddenFolders();

	

	RunPostScanActions();
}

void FProjectCleanerScanner::GetScanResult(FProjectCleanerScanResult& ScanResult)
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
}

void FProjectCleanerScanner::RunPostScanActions()
{
	ScanState = EProjectCleanerScanState::Idle;
	// todo:ashe23 add on scan finish delegate maybe?
}
