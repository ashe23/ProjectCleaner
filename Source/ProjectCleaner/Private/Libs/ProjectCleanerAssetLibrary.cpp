// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Libs/ProjectCleanerAssetLibrary.h"
#include "ProjectCleanerConstants.h"
// Engine Headers
#include "AssetRegistry/AssetRegistryModule.h"
#include "IContentBrowserSingleton.h"
#include "AssetToolsModule.h"
#include "ContentBrowserModule.h"
#include "FileHelpers.h"
#include "Misc/ScopedSlowTask.h"

bool UProjectCleanerAssetLibrary::AssetRegistryIsLoadingAssets()
{
	return FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName).Get().IsLoadingAssets();
}

void UProjectCleanerAssetLibrary::AssetRegistryUpdate(const bool bSyncScan)
{
	const auto& AssetRegistry = FModuleManager::GetModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);

	TArray<FString> ScanFolders;
	ScanFolders.Add(ProjectCleanerConstants::PathRelativeRoot);

	AssetRegistry.Get().ScanPathsSynchronous(ScanFolders, true);
	AssetRegistry.Get().SearchAllAssets(bSyncScan);
}

void UProjectCleanerAssetLibrary::AssetsSaveAll(const bool bShowDialogWindow)
{
	FEditorFileUtils::SaveDirtyPackages(
		bShowDialogWindow,
		true,
		true,
		false,
		false,
		false
	);
}

void UProjectCleanerAssetLibrary::AssetsGetAll(TArray<FAssetData>& Assets)
{
	const auto& AssetRegistry = FModuleManager::GetModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName).Get();

	Assets.Reset();
	Assets.Reserve(AssetRegistry.GetAllocatedSize());

	AssetRegistry.GetAssetsByPath(FName{*ProjectCleanerConstants::PathRelativeRoot}, Assets, true);
}

int64 UProjectCleanerAssetLibrary::AssetsGetTotalSize(const TArray<FAssetData>& Assets)
{
	const auto& AssetRegistry = FModuleManager::GetModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName).Get();
	int64 Size = 0;
	for (const auto& Asset : Assets)
	{
		const auto AssetPackageData = AssetRegistry.GetAssetPackageData(Asset.PackageName);
		if (!AssetPackageData) continue;
		Size += AssetPackageData->DiskSize;
	}

	return Size;
}

void UProjectCleanerAssetLibrary::FixupRedirectors(const FString& Path)
{
	if (Path.IsEmpty()) return;

	const auto& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName).Get();
	const auto& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get();

	FScopedSlowTask FixRedirectorsTask{
		1.0f,
		FText::FromString(ProjectCleanerConstants::MsgFixingRedirectors)
	};
	FixRedirectorsTask.MakeDialog();

	FARFilter Filter;
	Filter.bRecursivePaths = true;
	Filter.PackagePaths.Emplace(Path);
	Filter.ClassNames.Emplace(UObjectRedirector::StaticClass()->GetFName());

	// Getting all redirectors in project
	TArray<FAssetData> AssetList;
	AssetRegistry.GetAssets(Filter, AssetList);

	if (AssetList.Num() > 0)
	{
		FScopedSlowTask FixRedirectorsLoadingTask(
			AssetList.Num(),
			FText::FromString(ProjectCleanerConstants::MsgLoadingAssets)
		);
		FixRedirectorsLoadingTask.MakeDialog();

		TArray<UObjectRedirector*> Redirectors;
		Redirectors.Reserve(AssetList.Num());

		for (const auto& Asset : AssetList)
		{
			FixRedirectorsLoadingTask.EnterProgressFrame();

			UObject* AssetObj = Asset.GetAsset();
			if (!AssetObj) continue;

			const auto Redirector = CastChecked<UObjectRedirector>(AssetObj);
			if (!Redirector) continue;

			Redirectors.Add(Redirector);
		}

		Redirectors.Shrink();

		// Fix up all founded redirectors
		AssetTools.FixupReferencers(Redirectors);
	}

	FixRedirectorsTask.EnterProgressFrame(1.0f);
}

void UProjectCleanerAssetLibrary::ContentBrowserFocusOnFolder(const FString& FolderPath)
{
	if (FolderPath.IsEmpty()) return;

	TArray<FString> FocusFolders;
	FocusFolders.Add(FolderPath);

	const FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	ContentBrowserModule.Get().SyncBrowserToFolders(FocusFolders);
}

bool UProjectCleanerAssetLibrary::IsEngineExtension(const FString& Extension)
{
	return Extension.ToLower().Equals(TEXT("uasset")) || Extension.ToLower().Equals(TEXT("umap"));
}

bool UProjectCleanerAssetLibrary::IsUnderMegascansFolder(const FString& AssetPackagePath)
{
	return AssetPackagePath.StartsWith(ProjectCleanerConstants::PathMegascansPlugin);
}
