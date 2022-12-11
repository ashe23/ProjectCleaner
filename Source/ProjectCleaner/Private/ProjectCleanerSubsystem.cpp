// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "ProjectCleanerSubsystem.h"
#include "ProjectCleaner.h"
#include "ProjectCleanerConstants.h"
// Engine Headers
#include "FileHelpers.h"
#include "AssetToolsModule.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/ScopedSlowTask.h"

UProjectCleanerSubsystem::UProjectCleanerSubsystem() :
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

	ContainersEmpty();
}

#if WITH_EDITOR
void UProjectCleanerSubsystem::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	SaveConfig();
}
#endif

void UProjectCleanerSubsystem::ScanProject()
{
	if (ModuleAssetRegistry->Get().IsLoadingAssets())
	{
		ModuleAssetRegistry->Get().WaitForCompletion();
	}

	FixupRedirectors();

	FEditorFileUtils::SaveDirtyPackages(
		true,
		true,
		true,
		false,
		false,
		false
	);

	UE_LOG(LogProjectCleaner, Warning, TEXT("Scanning Project"));
}

void UProjectCleanerSubsystem::ToggleConfigCleanEmptyFolder()
{
	bAutoCleanEmptyFolders = !bAutoCleanEmptyFolders;

	PostEditChange();
}

void UProjectCleanerSubsystem::ToggleConfigScanDevFolder()
{
	bScanDevFolder = !bScanDevFolder;

	PostEditChange();
}

void UProjectCleanerSubsystem::ContainersEmpty()
{
	AssetsAll.Empty();
	AssetsIndirect.Empty();
	AssetsExcluded.Empty();
	AssetsUsed.Empty();
	AssetsUnused.Empty();

	FilesNonEngine.Empty();
	FilesCorrupted.Empty();

	FoldersAll.Empty();
	FoldersEmpty.Empty();
}

void UProjectCleanerSubsystem::ContainersReset()
{
	AssetsAll.Reset();
	AssetsIndirect.Reset();
	AssetsExcluded.Reset();
	AssetsUsed.Reset();
	AssetsUnused.Reset();

	FilesNonEngine.Reset();
	FilesCorrupted.Reset();

	FoldersAll.Reset();
	FoldersEmpty.Reset();
}

void UProjectCleanerSubsystem::ContainersShrink()
{
	AssetsAll.Shrink();
	AssetsIndirect.Shrink();
	AssetsExcluded.Shrink();
	AssetsUsed.Shrink();
	AssetsUnused.Shrink();

	FilesNonEngine.Shrink();
	FilesCorrupted.Shrink();

	FoldersAll.Shrink();
	FoldersEmpty.Shrink();
}

void UProjectCleanerSubsystem::FixupRedirectors() const
{
	FScopedSlowTask FixRedirectorsTask{
		1.0f,
		FText::FromString(ProjectCleanerConstants::MsgFixingRedirectors)
	};
	FixRedirectorsTask.MakeDialog();

	FARFilter Filter;
	Filter.bRecursivePaths = true;
	Filter.PackagePaths.Emplace(ProjectCleanerConstants::PathRelRoot);
	Filter.ClassNames.Emplace(UObjectRedirector::StaticClass()->GetFName());

	// Getting all redirectors in given path
	TArray<FAssetData> AssetList;
	ModuleAssetRegistry->Get().GetAssets(Filter, AssetList);

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

	// Fix up all founded redirectors
	ModuleAssetTools->Get().FixupReferencers(Redirectors);

	FixRedirectorsTask.EnterProgressFrame(1.0f);
}
