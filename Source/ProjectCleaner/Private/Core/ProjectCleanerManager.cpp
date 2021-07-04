// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#include "Core/ProjectCleanerManager.h"
#include "Core/ProjectCleanerUtility.h"
#include "UI/ProjectCleanerNotificationManager.h"
#include "AssetRegistryModule.h"
#include "Misc/ScopedSlowTask.h"
#include "Settings/ContentBrowserSettings.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"

ProjectCleanerManager::ProjectCleanerManager()
{
	CleanerConfigs = GetMutableDefault<UCleanerConfigs>();
	ExcludeOptions = GetMutableDefault<UExcludeOptions>();

	ensure(CleanerConfigs && ExcludeOptions);
}

FProjectCleanerData* ProjectCleanerManager::GetCleanerData()
{
	return &CleanerData;
}

UCleanerConfigs* ProjectCleanerManager::GetCleanerConfigs() const
{
	return CleanerConfigs;
}

UExcludeOptions* ProjectCleanerManager::GetExcludeOptions() const
{
	return ExcludeOptions;
}

TArray<FAssetNode> const* ProjectCleanerManager::GetAdjacencyList() const
{
	return &AdjacencyList;
}

void ProjectCleanerManager::UpdateData()
{
	FScopedSlowTask SlowTask{ 1.0f, FText::FromString("Scanning...") };
	SlowTask.MakeDialog();

	ProjectCleanerUtility::FixupRedirectors();
	ProjectCleanerUtility::SaveAllAssets();
	UpdateAssetRegistry();
	
	CleanerData.Empty();
	AdjacencyList.Empty();
	
	ProjectCleanerUtility::GetPrimaryAssetClasses(CleanerData.PrimaryAssetClasses);
	ProjectCleanerUtility::GetEmptyFolders(CleanerData.EmptyFolders, CleanerConfigs->bScanDeveloperContents);
	ProjectCleanerUtility::GetInvalidFiles(CleanerData.CorruptedFiles, CleanerData.NonEngineFiles);
	ProjectCleanerUtility::GetUnusedAssets(CleanerData.UnusedAssets);
	ProjectCleanerUtility::FindAssetsUsedIndirectly(CleanerData.UnusedAssets, CleanerData.IndirectFileInfos);

	GenerateAdjacencyList();
	RemoveAssetsWithExternalReferencers();
	RemoveAssetsFromDeveloperFolder();
	RemoveIndirectAssets();
	RemoveExcludedAssets();
	
	CleanerData.TotalSize = ProjectCleanerUtility::GetTotalSize(CleanerData.UnusedAssets);

	GetMutableDefault<UContentBrowserSettings>()->SetDisplayDevelopersFolder(CleanerConfigs->bScanDeveloperContents, true);
	GetMutableDefault<UContentBrowserSettings>()->PostEditChange();

	SlowTask.EnterProgressFrame(1.0f);
}

void ProjectCleanerManager::DeleteUnusedAssets()
{
	if (CleanerData.UnusedAssets.Num() == 0) return;

	TArray<FAssetData> Chunk;
	Chunk.Reserve(CleanerConfigs->DeleteChunkLimit);

	const auto ProgressNotification = ProjectCleanerNotificationManager::Add(
		FStandardCleanerText::StartingCleanup,
		SNotificationItem::ECompletionState::CS_Pending
	);

	CleanerData.TotalAssetsNum = CleanerData.UnusedAssets.Num();

	while (AdjacencyList.Num() != 0)
	{
		GetAssetsChunk(Chunk);
		CleanerData.DeletedAssetsNum += ProjectCleanerUtility::DeleteAssets(Chunk);
	
		const uint32 Percent = CleanerData.TotalAssetsNum > 0 ?
			(CleanerData.DeletedAssetsNum * 100.0f) / CleanerData.TotalAssetsNum : 0;
		const FText ProgressText = FText::FromString(
			FString::Printf(
				TEXT("Deleted %d of %d assets. %d %%"),
				CleanerData.DeletedAssetsNum,
				CleanerData.TotalAssetsNum,
				Percent
			)
		);
		
		ProjectCleanerNotificationManager::Update(ProgressNotification, ProgressText);

		CleanerData.UnusedAssets.RemoveAllSwap([&] (const FAssetData& Elem)
		{
			return Chunk.Contains(Elem);
		});

		GenerateAdjacencyList();
		
		Chunk.Reset();
	}

	const FString PostFixText = CleanerData.DeletedAssetsNum > 1 ? TEXT(" assets") : TEXT(" asset");
	const FString FinalText = FString{ "Deleted " } + FString::FromInt(CleanerData.DeletedAssetsNum) + PostFixText;
	ProjectCleanerNotificationManager::Hide(ProgressNotification, FText::FromString(FinalText));
	
	UpdateData();
	
	if (CleanerConfigs->bAutomaticallyDeleteEmptyFolders)
	{
		DeleteEmptyFolders();
	}
	else
	{
		UpdateAssetRegistry();
		FocusOnGameFolder();
	}
}

void ProjectCleanerManager::DeleteEmptyFolders()
{
	if (CleanerData.EmptyFolders.Num() == 0) return;
	
	const FString PostFixText = CleanerData.EmptyFolders.Num() > 1 ? TEXT(" empty folders") : TEXT(" empty folder");
 	const FString DisplayText = FString{ "Deleted " } + FString::FromInt(CleanerData.EmptyFolders.Num()) + PostFixText;
 	
 	if (ProjectCleanerUtility::DeleteEmptyFolders(CleanerData.EmptyFolders))
 	{
 		ProjectCleanerNotificationManager::AddTransient(
 			FText::FromString(DisplayText),
 			SNotificationItem::ECompletionState::CS_Success,
 			5.0f
 		);
 	}
 	else
 	{
 		ProjectCleanerNotificationManager::AddTransient(
 			FText::FromString(FStandardCleanerText::FailedToDeleteSomeFolders),
 			SNotificationItem::ECompletionState::CS_Fail,
 			5.0f
 		);
 	}
 
 	UpdateAssetRegistry();
	FocusOnGameFolder();
}

void ProjectCleanerManager::UpdateAssetRegistry() const
{
	FAssetRegistryModule& AssetRegistry = FModuleManager::GetModuleChecked<FAssetRegistryModule>("AssetRegistry");
	
	TArray<FString> ScanFolders;
	ScanFolders.Add("/Game");

	AssetRegistry.Get().ScanPathsSynchronous(ScanFolders, true);
	AssetRegistry.Get().SearchAllAssets(false);
}

void ProjectCleanerManager::FocusOnGameFolder()
{
	TArray<FString> FocusFolders;
	FocusFolders.Add("/Game");
	
	FContentBrowserModule& ContentBrowser = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	ContentBrowser.Get().SetSelectedPaths(FocusFolders, true);
}

void ProjectCleanerManager::GetRelatedAssets(const FName& PackageName, const ERelationType RelationType, TArray<FName>& RelatedAssets) const
{
	FAssetRegistryModule& AssetRegistry = FModuleManager::GetModuleChecked<FAssetRegistryModule>("AssetRegistry");

	if (RelationType == ERelationType::Reference)
	{
		AssetRegistry.Get().GetReferencers(PackageName, RelatedAssets);
	}
	else
	{
		AssetRegistry.Get().GetDependencies(PackageName, RelatedAssets);
	}

	RelatedAssets.RemoveAll([&] (const FName& Elem)
	{
		return Elem.IsEqual(PackageName);
	});
}

void ProjectCleanerManager::GetLinkedAssets(FAssetNode& Node, FAssetNode& RootNode, TSet<FName>& Visited)
{
	if (Visited.Contains(Node.AssetData.AssetName)) return;

	Visited.Add(Node.AssetData.AssetName);

	// Recursively traversing dependencies
	for (const auto& Dep : Node.Deps)
	{
		const auto& DepAsset = FindByPackageName(Dep);
		if (!DepAsset) continue;
		RootNode.LinkedAssets.AddUnique(DepAsset->AssetData);
		GetLinkedAssets(*DepAsset, RootNode, Visited);
	}
	// Recursively traversing referencers
	for (const auto& Ref : Node.Refs)
	{
		const auto& RefAsset = FindByPackageName(Ref);
		if (!RefAsset) continue;
		RootNode.LinkedAssets.AddUnique(RefAsset->AssetData);
		GetLinkedAssets(*RefAsset, RootNode, Visited);
	}
}

FAssetNode* ProjectCleanerManager::FindByPackageName(const FName& PackageName)
{
	return AdjacencyList.FindByPredicate([&](const FAssetNode& Elem)
	{
		return Elem.AssetData.PackageName.IsEqual(PackageName);
	});
}

void ProjectCleanerManager::GenerateAdjacencyList()
{
	AdjacencyList.Empty();
	
	for (const FAssetData& UnusedAsset : CleanerData.UnusedAssets)
	{
		FAssetNode Node;
		Node.AssetData = UnusedAsset;
		
		GetRelatedAssets(UnusedAsset.PackageName, ERelationType::Reference, Node.Refs);
		GetRelatedAssets(UnusedAsset.PackageName, ERelationType::Dependency, Node.Deps);
	
		AdjacencyList.Add(Node);
	}

	for (auto& Node : AdjacencyList)
	{
		TSet<FName> Visited;
		GetLinkedAssets(Node, Node, Visited);
		Visited.Reset();
	}
}

bool ProjectCleanerManager::IsExcludedByPath(const FAssetData& AssetData)
{
	if (!ExcludeOptions) return false;
	
	return ExcludeOptions->Paths.ContainsByPredicate([&](const FDirectoryPath& DirectoryPath)
	{
		return AssetData.PackagePath.ToString().StartsWith(DirectoryPath.Path);
	});
}

bool ProjectCleanerManager::IsExcludedByClass(const FAssetData& AssetData)
{
	if (!ExcludeOptions) return false;
	
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
	
	return ExcludeOptions->Classes.ContainsByPredicate([&](const UClass* ElemClass)
	{
		if (!ElemClass) return false;
		return ClassName.IsEqual(ElemClass->GetFName()) ||
		(IsBlueprint &&
			(
				ClassName.IsEqual(ElemClass->GetFName()) ||
				ParentClassName.IsEqual(ElemClass->GetFName()) ||
				ElemClass->GetFName().IsEqual(UBlueprint::StaticClass()->GetFName())
			)
		);
	});
}

bool ProjectCleanerManager::IsCircular(const FAssetNode& AssetNode)
{
	return AssetNode.Refs.ContainsByPredicate([&] (const FName& Elem)
	{
		return AssetNode.Deps.Contains(Elem);
	});
}

bool ProjectCleanerManager::HasReferencers(const FAssetNode& AssetNode)
{
	return AssetNode.Refs.Num() > 0;
}

bool ProjectCleanerManager::IsUnderDeveloperFolder(const FString& PackagePath) const
{
	const FString InPath = ProjectCleanerUtility::ConvertInternalToAbsolutePath(PackagePath);
	const FString DeveloperFolderPath = FPaths::ConvertRelativePathToFull(FPaths::GameDevelopersDir());
	const FString CollectionsFolderPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() + TEXT("Collections/"));

	return
		FPaths::IsUnderDirectory(InPath, DeveloperFolderPath) ||
		FPaths::IsUnderDirectory(InPath, CollectionsFolderPath);
}

bool ProjectCleanerManager::HasReferencersInDeveloperFolder(const FAssetNode& AssetNode) const
{
	for (const auto& Ref : AssetNode.Refs)
	{
		if (IsUnderDeveloperFolder(Ref.ToString()))
		{
			return true;
		}
	}

	return false;
}

bool ProjectCleanerManager::HasExternalReferencers(const FAssetNode& AssetNode)
{
	return AssetNode.Refs.ContainsByPredicate([&] (const FName& Elem)
	{
		return !Elem.ToString().StartsWith("/Game");
	});
}

void ProjectCleanerManager::RemoveAssetsWithExternalReferencers()
{
	TSet<FAssetData> FilteredAssets;
	FilteredAssets.Reserve(AdjacencyList.Num());
	
	for (const auto& AssetNode : AdjacencyList)
	{
		if (!HasExternalReferencers(AssetNode)) continue;
		FilteredAssets.Add(AssetNode.AssetData);

		for (const auto& LinkedAsset : AssetNode.LinkedAssets)
		{
			FilteredAssets.Add(LinkedAsset);
		}
	}
	
	CleanerData.UnusedAssets.RemoveAllSwap([&] (const FAssetData& Elem)
	{
		return FilteredAssets.Contains(Elem);
	});

	AdjacencyList.RemoveAllSwap([&](const FAssetNode& Elem)
	{
		return FilteredAssets.Contains(Elem.AssetData);
	});
}


void ProjectCleanerManager::RemoveAssetsFromDeveloperFolder()
{
	for (const auto& Node : AdjacencyList)
	{
		if (!IsUnderDeveloperFolder(Node.AssetData.PackagePath.ToString()) && HasReferencersInDeveloperFolder(Node))
		{
			ProjectCleanerNotificationManager::AddTransient(
				FText::FromString(FStandardCleanerText::SomeAssetsHaveRefsInDevFolder),
				SNotificationItem::ECompletionState::CS_None,
				5.0f
			);
			CleanerConfigs->bScanDeveloperContents = true;
			break;
		}
	}
	
	if (!CleanerConfigs->bScanDeveloperContents)
	{
		CleanerData.UnusedAssets.RemoveAllSwap([&](const FAssetData& Elem) {
			return IsUnderDeveloperFolder(Elem.PackagePath.ToString());
		});

		AdjacencyList.RemoveAllSwap([&] (const FAssetNode& Elem)
		{
			return IsUnderDeveloperFolder(Elem.AssetData.PackagePath.ToString());
		});
	}
}

void ProjectCleanerManager::RemoveIndirectAssets()
{
	TSet<FAssetData> FilteredAssets;
	FilteredAssets.Reserve(CleanerData.UnusedAssets.Num());
	
	for (const auto& IndirectFileInfo : CleanerData.IndirectFileInfos)
	{
		FilteredAssets.Add(IndirectFileInfo.AssetData);

		FAssetNode* Node = FindByPackageName(IndirectFileInfo.AssetData.PackageName);
		if (!Node) continue;
		
		for (const auto& LinkedAsset : Node->LinkedAssets)
		{
			FilteredAssets.Add(LinkedAsset);
		}
	}

	FilteredAssets.Shrink();

	CleanerData.UnusedAssets.RemoveAllSwap([&] (const FAssetData& Elem)
	{
		return FilteredAssets.Contains(Elem);
	});

	AdjacencyList.RemoveAllSwap([&] (const FAssetNode& Elem)
	{
		return FilteredAssets.Contains(Elem.AssetData);
	});
}

void ProjectCleanerManager::RemoveExcludedAssets()
{
	// excluded by user
	for (const auto& Asset : CleanerData.UserExcludedAssets)
	{
		CleanerData.ExcludedAssets.AddUnique(Asset);
	}

	// excluded by path or class
	for (const auto& Asset : CleanerData.UnusedAssets)
	{
		if (!IsExcludedByPath(Asset) && !IsExcludedByClass(Asset)) continue;
		
		CleanerData.ExcludedAssets.AddUnique(Asset);
	}

	for (const auto& ExcludedAsset : CleanerData.ExcludedAssets)
	{
		FAssetNode* Node = FindByPackageName(ExcludedAsset.PackageName);
		if (!Node) continue;
		
		for (const auto& LinkedAsset : Node->LinkedAssets)
		{
			CleanerData.LinkedAssets.Add(LinkedAsset);
		}
	}

	CleanerData.LinkedAssets.RemoveAllSwap([&] (const FAssetData& Elem)
	{
		return CleanerData.ExcludedAssets.Contains(Elem);
	});
	
	CleanerData.UnusedAssets.RemoveAllSwap([&] (const FAssetData& Elem)
	{
		return CleanerData.ExcludedAssets.Contains(Elem) || CleanerData.LinkedAssets.Contains(Elem);
	});

	AdjacencyList.RemoveAllSwap([&] (const FAssetNode& Elem)
	{
		return CleanerData.ExcludedAssets.Contains(Elem.AssetData) || CleanerData.LinkedAssets.Contains(Elem.AssetData);
	});
}

void ProjectCleanerManager::GetAssetsChunk(TArray<FAssetData>& Chunk)
{
	Chunk.Reserve(CleanerData.UnusedAssets.Num());
	// 1) trying to find circular assets
	for (const auto& AssetNode : AdjacencyList)
	{
		if (IsCircular(AssetNode))
		{
			Chunk.Add(AssetNode.AssetData);
		}
	}
	
	if (Chunk.Num() > 0) return;
	
	// 2) trying to find assets without referencers
	for (const auto& AssetNode : AdjacencyList)
	{
		if (Chunk.Num() > CleanerConfigs->DeleteChunkLimit) break;
		if (!HasReferencers(AssetNode))
		{
			Chunk.Add(AssetNode.AssetData);
		}
	}
	
	if (Chunk.Num() > 0) return;
	
	for (const auto& AssetNode : AdjacencyList)
	{
		if (Chunk.Num() > CleanerConfigs->DeleteChunkLimit) break;
		Chunk.Add(AssetNode.AssetData);
	}
}
