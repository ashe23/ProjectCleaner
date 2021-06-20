// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#include "ProjectCleanerUtility.h"
#include "ProjectCleanerHelper.h"
#include "ProjectCleanerNotificationManager.h"
#include "UI/ProjectCleanerConfigsUI.h"
#include "UI/ProjectCleanerSourceCodeAssetsUI.h"
#include "UI/ProjectCleanerExcludeOptionsUI.h"
#include "Graph/AssetRelationalMap.h"
#include "StructsContainer.h"
// Engine Headers
#include "ObjectTools.h"
#include "FileHelpers.h"
#include "AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "Engine/AssetManager.h"
#include "UObject/ObjectRedirector.h"
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

void ProjectCleanerUtility::RemoveAssetsUsedIndirectly(TArray<FAssetData>& UnusedAssets, AssetRelationalMap& RelationalMap, TArray<TWeakObjectPtr<USourceCodeAsset>>& SourceCodeAssets)
{
	// 1) finding all source code files
	TArray<FSourceCodeFile> SourceCodeFiles;
	ProjectCleanerHelper::GetSourceCodeFilesFromDisk(SourceCodeFiles);

	// 2) parsing files and checking if assets used there
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
		Obj->AssetData = UnusedAsset;
		Obj->SourceCodePath = File->AbsoluteFilePath;
		SourceCodeAssets.Add(Obj);
	}

	// 3) for founded assets find all linked assets
	TSet<FName> LinkedAssets;
	RelationalMap.FindAllLinkedAssets(FoundedAssets, LinkedAssets);
	
	// 4) remove founded assets from unused assets list
	UnusedAssets.RemoveAll([&](const FAssetData& Elem)
	{
		return LinkedAssets.Contains(Elem.PackageName);
	});

	RelationalMap.Rebuild(UnusedAssets);
}

void ProjectCleanerUtility::RemoveAssetsWithExternalReferences(TArray<FAssetData>& UnusedAssets, AssetRelationalMap& RelationalMap)
{
	const auto AssetsWithExternalRefs = RelationalMap.GetAssetsWithExternalRefs();
	if (AssetsWithExternalRefs.Num() == 0) return;

	TArray<FAssetData> Assets;
	Assets.Reserve(AssetsWithExternalRefs.Num());

	for (const auto& Node : AssetsWithExternalRefs)
	{
		Assets.Add(Node.AssetData);
	}
	
	UnusedAssets.RemoveAll([&](const FAssetData& Elem)
	{
		return Assets.Contains(Elem);
	});

	// todo:ashe23 add notification or seperate UI for those type of assets
	RelationalMap.Rebuild(UnusedAssets);
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

bool ProjectCleanerUtility::IsExcludedByPath(const FAssetData& AssetData, const UExcludeOptions& ExcludeOptions)
{
	return ExcludeOptions.Paths.ContainsByPredicate([&](const FDirectoryPath& DirectoryPath)
	{
		return AssetData.PackagePath.ToString().StartsWith(DirectoryPath.Path);
	});
}

bool ProjectCleanerUtility::IsExcludedByClass(const FAssetData& AssetData, const UExcludeOptions& ExcludeOptions)
{
	const UBlueprint* BlueprintAsset = Cast<UBlueprint>(AssetData.GetAsset());
	const bool IsBlueprint = (BlueprintAsset != nullptr);

	FName ClassName;
	FName ParentClassName;

	if (IsBlueprint && BlueprintAsset->GeneratedClass && BlueprintAsset->ParentClass)
	{
		ClassName = BlueprintAsset->GeneratedClass->GetFName();
		ParentClassName = BlueprintAsset->ParentClass->GetFName();
	}
	else
	{
		ClassName = AssetData.AssetClass;
	}
	
	return ExcludeOptions.Classes.ContainsByPredicate([&](const UClass* ElemClass)
	{
		if (!ElemClass) return false;
		return
		ClassName.IsEqual(ElemClass->GetFName()) ||
		(IsBlueprint && (ClassName.IsEqual(ElemClass->GetFName()) || ParentClassName.IsEqual(ElemClass->GetFName())));
	});
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

void ProjectCleanerUtility::RemoveContentFromDeveloperFolder(TArray<FAssetData>& UnusedAssets, AssetRelationalMap& RelationalMap, UCleanerConfigs* CleanerConfigs, const TSharedPtr<ProjectCleanerNotificationManager> NotificationManager)
{
	if (!CleanerConfigs) return;
	if (!CleanerConfigs->IsValidLowLevel()) return;

	{
		TArray<FName> LinkedAssets;
		LinkedAssets.Reserve(UnusedAssets.Num());

		for (const auto& Node : RelationalMap.GetNodes())
		{
			if (Node.HasReferencersInDeveloperFolder())
			{
				LinkedAssets.Append(Node.LinkedAssets);
			}
		}

		if (LinkedAssets.Num() > 0)
		{
			CleanerConfigs->bScanDeveloperContents = true;
			if (NotificationManager.IsValid())
			{
				NotificationManager->AddTransient(TEXT("Some assets have referencers in Developer Contents Folder."), SNotificationItem::ECompletionState::CS_None, 5.0f);
			}
		}
	}

	if (!CleanerConfigs->bScanDeveloperContents)
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

	RelationalMap.Rebuild(UnusedAssets);
}

void ProjectCleanerUtility::RemoveAssetsExcludedByUser(
	TArray<FAssetData>& UnusedAssets,
	AssetRelationalMap& RelationalMap,
	const UExcludeOptions& ExcludeOptions,
	TArray<FAssetData>& ExcludedAssets,
	const TArray<FAssetData>& UserExcludedAssets,
	TArray<FAssetData>& LinkedAssets)
{
	// 1) exclude all assets that user excluded manually
	for (const auto& Asset : UserExcludedAssets)
	{
		ExcludedAssets.AddUnique(Asset);
	}

	// 2) exclude assets filtered by path or class
	for (const auto& Asset : UnusedAssets)
	{
		const bool ExcludedByPath = IsExcludedByPath(Asset, ExcludeOptions);
		const bool ExcludedByClass = IsExcludedByClass(Asset, ExcludeOptions);
		
		if (ExcludedByPath || ExcludedByClass)
		{
			ExcludedAssets.AddUnique(Asset);
		}
	}

	RelationalMap.FindAllLinkedAssets(ExcludedAssets, LinkedAssets);
	UnusedAssets.RemoveAll([&](const FAssetData& Elem)
	{
		return ExcludedAssets.Contains(Elem) || LinkedAssets.Contains(Elem);
	});

	RelationalMap.Rebuild(UnusedAssets);
}
