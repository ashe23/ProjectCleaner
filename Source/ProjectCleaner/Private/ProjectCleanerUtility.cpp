// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#include "ProjectCleanerUtility.h"
#include "ProjectCleanerHelper.h"
#include "UI/ProjectCleanerSourceCodeAssetsUI.h"
#include "UI/ProjectCleanerExcludeOptionsUI.h"
#include "Graph/AssetRelationalMap.h"
// Engine Headers
#include "ObjectTools.h"
#include "FileHelpers.h"
#include "AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "Engine/AssetManager.h"
#include "UObject/ObjectRedirector.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformFilemanager.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Misc/ScopedSlowTask.h"
#include "Engine/AssetManagerSettings.h"
#include "Engine/MapBuildDataRegistry.h"

void ProjectCleanerUtility::GetAllAssets(const FAssetRegistryModule* AssetRegistry, TArray<FAssetData>& Assets)
{
	if (!AssetRegistry) return;
	AssetRegistry->Get().GetAssetsByPath(FName{ "/Game" }, Assets, true);
}

void ProjectCleanerUtility::GetInvalidProjectFiles(const FAssetRegistryModule* AssetRegistry, const TSet<FString>& ProjectFilesFromDisk, TSet<FString>& CorruptedFiles, TSet<FString>& NonUAssetFiles)
{
	if (!AssetRegistry) return;

	CorruptedFiles.Reserve(ProjectFilesFromDisk.Num());
	NonUAssetFiles.Reserve(ProjectFilesFromDisk.Num());
	
	for (const auto& ProjectFile : ProjectFilesFromDisk)
	{
		if (IsEngineExtension(FPaths::GetExtension(ProjectFile, false)))
		{
			// here we got absolute path "C:/MyProject/Content/material.uasset"
			// we must first convert that path to In Engine Internal Path like "/Game/material.uasset"
			const FString InternalFilePath = ProjectCleanerHelper::ConvertAbsolutePathToInternal(ProjectFile);
			// Converting file path to objectpath (This is for searching in AssetRegistry)
			// example "/Game/Name.uasset" => "/Game/Name.Name"
			auto ObjectPath = InternalFilePath;
			ObjectPath.RemoveFromEnd(FPaths::GetExtension(InternalFilePath, true));
			ObjectPath.Append(TEXT(".") + FPaths::GetBaseFilename(InternalFilePath));

			// Trying to find that file in AssetRegistry
			const auto AssetData = AssetRegistry->Get().GetAssetByObjectPath(FName{ *ObjectPath });
			// Adding to CorruptedFiles list, if we cant find it in AssetRegistry
			if (AssetData.IsValid()) continue;
			CorruptedFiles.Add(ProjectFile);
		}
		else
		{
			NonUAssetFiles.Add(ProjectFile);
		}
	}

	CorruptedFiles.Compact();
	NonUAssetFiles.Compact();
}

void ProjectCleanerUtility::GetAllPrimaryAssetClasses(const UAssetManager& AssetManager, TSet<FName>& PrimaryAssetClasses)
{
	if (!AssetManager.IsValid()) return;

	PrimaryAssetClasses.Reserve(10);
	
	const UAssetManagerSettings& Settings = AssetManager.GetSettings();
	TArray<FPrimaryAssetId> Ids;
	for (const auto& Type : Settings.PrimaryAssetTypesToScan)
	{
		AssetManager.Get().GetPrimaryAssetIdList(Type.PrimaryAssetType, Ids);
		for(const auto& Id : Ids)
		{
			FAssetData Data;
			AssetManager.Get().GetPrimaryAssetData(Id, Data);
			if(!Data.IsValid()) continue;
			PrimaryAssetClasses.Add(Data.AssetClass);
		}
		Ids.Reset();
	}
}

void ProjectCleanerUtility::RemoveMegascansPluginAssetsIfActive(TArray<FAssetData>& UnusedAssets)
{
	const bool IsMegascansLoaded = FModuleManager::Get().IsModuleLoaded("MegascansPlugin");
	if (!IsMegascansLoaded) return;
	
	UnusedAssets.RemoveAll([&](const FAssetData& Elem)
	{
		return FPaths::IsUnderDirectory(Elem.PackagePath.ToString(), TEXT("/Game/MSPresets"));
	});
}

void ProjectCleanerUtility::RemoveAssetsUsedIndirectly(
	TArray<FAssetData>& UnusedAssets,
	AssetRelationalMap& RelationalMap,
	TArray<FSourceCodeFile> SourceCodeFiles,
	TArray<TWeakObjectPtr<USourceCodeAsset>>& SourceCodeAssets)
{
	// 1) parsing files and checking if assets used there
	TSet<FName> FoundedAssets;
	FoundedAssets.Reserve(UnusedAssets.Num());
	
	for (const auto& UnusedAsset : UnusedAssets)
	{
		const auto File = GetFileWhereAssetUsed(UnusedAsset, SourceCodeFiles);
		if(!File) continue;

		FoundedAssets.Add(UnusedAsset.PackageName);
		
		auto Obj = NewObject<USourceCodeAsset>();
		Obj->AssetName = UnusedAsset.AssetName.ToString();
		Obj->AssetPath = UnusedAsset.PackageName.ToString();
		Obj->SourceCodePath = File->AbsoluteFilePath;
		SourceCodeAssets.Add(Obj);
	}

	TSet<FName> FilteredAssets;
	FilteredAssets.Reserve(UnusedAssets.Num());
	
	// 2) for founded assets find all linked assets
	for (const auto& FoundedAsset : FoundedAssets)
	{
		const auto AssetNode = RelationalMap.FindByPackageName(FoundedAsset);
		if (!AssetNode) continue;

		FilteredAssets.Add(FoundedAsset);
		for (const auto& LinkedAsset : AssetNode->LinkedAssetsData)
		{
			FilteredAssets.Add(LinkedAsset->PackageName);
		}
	}
	
	// 3) remove founded assets from unused assets list
	UnusedAssets.RemoveAll([&](const FAssetData& Elem)
	{
		return FilteredAssets.Contains(Elem.PackageName);
	});
}

void ProjectCleanerUtility::RemoveAssetsWithExternalReferences(TArray<FAssetData>& UnusedAssets, AssetRelationalMap& RelationalMap)
{
	TSet<FName> FilteredAssets;
	FilteredAssets.Reserve(UnusedAssets.Num());
	
	for (const auto& Node : RelationalMap.GetNodes())
	{
		if (!Node.HasExternalReferencers()) continue;

		UE_LOG(LogTemp, Warning, TEXT("External Asset Reference Detected: %s"), *Node.AssetData.PackageName.ToString());

		FilteredAssets.Add(Node.AssetData.PackageName);
		FilteredAssets.Append(Node.LinkedAssets);
	}

	UnusedAssets.RemoveAll([&](const FAssetData& Elem)
	{
		return FilteredAssets.Contains(Elem.PackageName);
	});
}

void ProjectCleanerUtility::RemoveAssetsExcludedByUser(
	const FAssetRegistryModule* AssetRegistry,
	TArray<FAssetData>& UnusedAssets,
	TSet<FAssetData>& ExcludedAssets,
	TArray<FAssetData>& LinkedAssets,
	TArray<FAssetData>& UserExcludedAssets,
	AssetRelationalMap& RelationalMap,
	const UExcludeOptions* ExcludeOptions)
{
	if (!AssetRegistry) return;
	if (!ExcludeOptions) return;

	TSet<FAssetData> FilteredAssets;
	FilteredAssets.Reserve(UnusedAssets.Num());
	
	for (const auto FilterPath : ExcludeOptions->Paths)
	{
		TArray<FAssetData> IterationAssets;
		IterationAssets.Reserve(UnusedAssets.Num());
		AssetRegistry->Get().GetAssetsByPath(FName{ *FilterPath.Path }, IterationAssets, true);

		//we should add only unused assets
		IterationAssets.RemoveAll([&](const FAssetData& Elem) {
			return !UnusedAssets.Contains(Elem);
		});
		FilteredAssets.Append(IterationAssets);
		IterationAssets.Reset();
	}

	for (const auto& Asset : UserExcludedAssets)
	{
		FilteredAssets.Add(Asset);
	}

	TArray<FAssetData> AssetsExcludedByClass;
	AssetsExcludedByClass.Reserve(UnusedAssets.Num());
	RelationalMap.FindAssetsByClass(ExcludeOptions->Classes, AssetsExcludedByClass);

	for (const auto& Asset : AssetsExcludedByClass)
	{
		FilteredAssets.Add(Asset);
	}

	for (const auto& FilteredAsset : FilteredAssets)
	{
		ExcludedAssets.Add(FilteredAsset);
		const auto Node = RelationalMap.FindByPackageName(FilteredAsset.PackageName);
		if (!Node) continue;
		for (const auto& LinkedAsset : Node->LinkedAssetsData)
		{
			LinkedAssets.Add(*LinkedAsset);
		}
	}

	LinkedAssets.RemoveAll([&](const FAssetData& Elem) {
		return ExcludedAssets.Contains(Elem);
	});

	UnusedAssets.RemoveAll([&](const FAssetData& Elem) {
		return ExcludedAssets.Contains(Elem) || LinkedAssets.Contains(Elem);
	});
}

void ProjectCleanerUtility::FixupRedirectors()
{
	FScopedSlowTask SlowTask{1.0f, FText::FromString("Fixing up Redirectors...")};
	SlowTask.MakeDialog();

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::GetModuleChecked<FAssetRegistryModule>("AssetRegistry");

	const FName RootPath = TEXT("/Game");

	FARFilter Filter;
	Filter.bRecursivePaths = true;
	Filter.PackagePaths.Emplace(RootPath);
	// Filter.ClassNames.Emplace(TEXT("ObjectRedirector"));
	Filter.ClassNames.Emplace(UObjectRedirector::StaticClass()->GetFName());

	// Query for a list of assets
	TArray<FAssetData> AssetList;
	AssetRegistryModule.Get().GetAssets(Filter, AssetList);

	if (AssetList.Num() > 0)
	{
		TArray<UObject*> Objects;
		// loading asset if needed
		for (const auto& Asset : AssetList)
		{
			Objects.Add(Asset.GetAsset());
		}

		// converting them to redirectors
		TArray<UObjectRedirector*> Redirectors;
		for (auto Object : Objects)
		{
			const auto Redirector = CastChecked<UObjectRedirector>(Object);
			Redirectors.Add(Redirector);
		}

		// Fix up all founded redirectors
		FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
		AssetToolsModule.Get().FixupReferencers(Redirectors);
	}

	SlowTask.EnterProgressFrame(1.0f);
}

int32 ProjectCleanerUtility::DeleteAssets(TArray<FAssetData>& Assets)
{
	// first try to delete normally
	int32 DeletedAssets = ObjectTools::DeleteAssets(Assets, false);

	// if normally not working try to force delete
	if (DeletedAssets == 0)
	{
		TArray<UObject*> AssetObjects;
		AssetObjects.Reserve(Assets.Num());

		for (const auto& Asset : Assets)
		{
			AssetObjects.Add(Asset.GetAsset());
		}

		DeletedAssets = ObjectTools::ForceDeleteObjects(AssetObjects, false);
	}

	return DeletedAssets;
}

void ProjectCleanerUtility::SaveAllAssets()
{
	FEditorFileUtils::SaveDirtyPackages(
		true,
		true,
		true,
		false,
		false,
		false
	);
}

int64 ProjectCleanerUtility::GetTotalSize(const TArray<FAssetData>& AssetContainer)
{
	int64 Size = 0;
	for (const auto& Asset : AssetContainer)
	{
		FAssetRegistryModule& AssetRegistry = FModuleManager::GetModuleChecked<FAssetRegistryModule>("AssetRegistry");
		const auto AssetPackageData = AssetRegistry.Get().GetAssetPackageData(Asset.PackageName);
		if (!AssetPackageData) continue;
		Size += AssetPackageData->DiskSize;
	}

	return Size;
}

bool ProjectCleanerUtility::IsEngineExtension(const FString& Extension)
{
	return Extension.Equals("uasset") || Extension.Equals("umap");
}

const FSourceCodeFile* ProjectCleanerUtility::GetFileWhereAssetUsed(const FAssetData& Asset, const TArray<FSourceCodeFile>& SourceCodeFiles)
{
	for (const auto& File : SourceCodeFiles)
	{
		//	todo:ashe23 BUG with similar names

		// Wrapping in quotes AssetName => "AssetName"
		FString QuotedAssetName = Asset.AssetName.ToString();
		QuotedAssetName.InsertAt(0, TEXT("\""));
		QuotedAssetName.Append(TEXT("\""));

		if (
			File.Content.Contains(Asset.PackageName.ToString()) ||
			File.Content.Contains(QuotedAssetName)
			)
		{
			return &File;
		}
	}

	return nullptr;
}

void ProjectCleanerUtility::RemoveUsedAssets(TArray<FAssetData>& Assets, const TSet<FName>& PrimaryAssetClasses)
{
	FAssetRegistryModule& AssetRegistry = FModuleManager::GetModuleChecked<FAssetRegistryModule>("AssetRegistry");
	
	FARFilter Filter;
	Filter.ClassNames.Add(UWorld::StaticClass()->GetFName());

	for (const auto& AssetClass : PrimaryAssetClasses)
	{
		Filter.ClassNames.Add(AssetClass);
	}
	Filter.PackagePaths.Add("/Game");
	Filter.bRecursivePaths = true;

	TSet<FName> UsedAssets;
	UsedAssets.Reserve(Assets.Num());
	TArray<FName> PackageNamesToProcess;
	{
		TArray<FAssetData> FoundAssets;
		AssetRegistry.Get().GetAssets(Filter, FoundAssets);
		for (const FAssetData& AssetData : FoundAssets)
		{
			PackageNamesToProcess.Add(AssetData.PackageName);
			UsedAssets.Add(AssetData.PackageName);
		}
	}

	TArray<FAssetIdentifier> AssetDependencies;
	while (PackageNamesToProcess.Num() > 0)
	{
		const FName PackageName = PackageNamesToProcess.Pop(false);
		AssetDependencies.Reset();
		AssetRegistry.Get().GetDependencies(FAssetIdentifier(PackageName), AssetDependencies);
		for (const FAssetIdentifier& Dependency : AssetDependencies)
		{
			bool bIsAlreadyInSet = false;
			UsedAssets.Add(Dependency.PackageName, &bIsAlreadyInSet);
			if (!bIsAlreadyInSet && Dependency.IsValid())
			{
				PackageNamesToProcess.Add(Dependency.PackageName);
			}
		}
	}

	Assets.RemoveAll([&](const FAssetData& Elem)
	{
		return UsedAssets.Contains(Elem.PackageName);
	});
}

void ProjectCleanerUtility::RemoveContentFromDeveloperFolder(TArray<FAssetData>& UnusedAssets)
{
	const FString DeveloperFolderPath = FPaths::ConvertRelativePathToFull(FPaths::GameDevelopersDir());
	const FString CollectionsFolderPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() + TEXT("Collections/"));

	UnusedAssets.RemoveAll([&](const FAssetData& Elem) {
		const FString AssetAbsolutePath = ProjectCleanerHelper::ConvertInternalToAbsolutePath(Elem.PackagePath.ToString());
		return
			FPaths::IsUnderDirectory(AssetAbsolutePath, DeveloperFolderPath) ||
			FPaths::IsUnderDirectory(AssetAbsolutePath, CollectionsFolderPath);
	});	
}
