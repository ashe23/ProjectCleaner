#include "Core/ProjectCleanerManager.h"
#include "Core/ProjectCleanerUtility.h"
#include "AssetRegistryModule.h"
#include "Misc/ScopedSlowTask.h"

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

void ProjectCleanerManager::UpdateData()
{
	FScopedSlowTask SlowTask{ 1.0f, FText::FromString("Scanning...") };
	SlowTask.MakeDialog();
	
	CleanerData.Empty();
	AdjacencyList.Empty();
	
	ProjectCleanerUtility::GetPrimaryAssetClasses(CleanerData.PrimaryAssetClasses);
	ProjectCleanerUtility::GetEmptyFolders(CleanerData.EmptyFolders, CleanerConfigs->bScanDeveloperContents);
	ProjectCleanerUtility::GetInvalidFiles(CleanerData.CorruptedFiles, CleanerData.NonEngineFiles);
	ProjectCleanerUtility::GetUnusedAssets(CleanerData.UnusedAssets);
	ProjectCleanerUtility::FindAssetsUsedIndirectly(CleanerData.UnusedAssets, CleanerData.IndirectFileInfos);

	GenerateAdjacencyList();
	FindAllLinkedAssets();

	TSet<FAssetData> AssetsToRemove;
	AssetsToRemove.Reserve(CleanerData.UnusedAssets.Num());

	// indirect files
	for (const auto& IndirectFileInfo : CleanerData.IndirectFileInfos)
	{
		AssetsToRemove.Add(IndirectFileInfo.AssetData);

		FAssetNode* Node = FindByPackageName(IndirectFileInfo.AssetData.PackageName);
		if (!Node) continue;

		for (const auto& LinkedAsset : Node->LinkedAssets)
		{
			AssetsToRemove.Add(LinkedAsset);
		}
	}

	// user excluded assets
	for (const auto& Asset : CleanerData.UserExcludedAssets)
	{
		CleanerData.ExcludedAssets.AddUnique(Asset);
	}

	// excluded by path or class
	for (const auto& Asset : CleanerData.UnusedAssets)
	{
		const bool ExcludedByPath = ProjectCleanerUtility::IsExcludedByPath(Asset, *ExcludeOptions);
		const bool ExcludedByClass = ProjectCleanerUtility::IsExcludedByClass(Asset, *ExcludeOptions);

		if (ExcludedByPath || ExcludedByClass)
		{
			CleanerData.ExcludedAssets.AddUnique(Asset);
		}
	}

	for (const auto& ExcludedAsset : CleanerData.ExcludedAssets)
	{
		AssetsToRemove.Add(ExcludedAsset);

		FAssetNode* Node = FindByPackageName(ExcludedAsset.PackageName);
		if (!Node) continue;

		for (const auto& LinkedAsset : Node->LinkedAssets)
		{
			AssetsToRemove.Add(LinkedAsset);
		}
	}
	
	CleanerData.UnusedAssets.RemoveAllSwap([&] (const FAssetData& Elem)
	{
		return AssetsToRemove.Contains(Elem);
	});
	
	CleanerData.TotalSize = ProjectCleanerUtility::GetTotalSize(CleanerData.UnusedAssets);

	SlowTask.EnterProgressFrame(1.0f);
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
		return Elem.IsEqual(PackageName); // todo:ashe23 remove non "Game" folder content?
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
	for (const FAssetData& UnusedAsset : CleanerData.UnusedAssets)
	{
		FAssetNode Node;
		Node.AssetData = UnusedAsset;
		
		GetRelatedAssets(UnusedAsset.PackageName, ERelationType::Reference, Node.Refs);
		GetRelatedAssets(UnusedAsset.PackageName, ERelationType::Dependency, Node.Deps);
	
		AdjacencyList.Add(Node);
	}
}

void ProjectCleanerManager::FindAllLinkedAssets()
{
	for (auto& Node : AdjacencyList)
	{
		TSet<FName> Visited;
		GetLinkedAssets(Node, Node, Visited);
		Visited.Reset();
	}
}
