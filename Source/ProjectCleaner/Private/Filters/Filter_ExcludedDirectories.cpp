#include "Filters/Filter_ExcludedDirectories.h"
#include "UI/SProjectCleanerBrowser.h"
#include "StructsContainer.h"

// Engine Headers
#include "AssetRegistry/Public/AssetData.h"
#include "AssetRegistryModule.h"

Filter_ExcludedDirectories::Filter_ExcludedDirectories(UDirectoryFilterSettings* DirectoryFilterSettings,
                                                       TArray<FNode>* List)
{
	Settings = DirectoryFilterSettings;
	this->AdjacencyList = List;
}

void Filter_ExcludedDirectories::Apply(TArray<FAssetData>& Assets)
{
	if (ShouldApplyDirectoryFilters())
	{
		CreateAdjacencyList(Assets, *AdjacencyList);
		ApplyDirectoryFilters(Assets);
	}
}

bool Filter_ExcludedDirectories::ShouldApplyDirectoryFilters() const
{
	if (!Settings) return false;

	return Settings->DirectoryPaths.Num() > 0;
}

void Filter_ExcludedDirectories::ApplyDirectoryFilters(TArray<FAssetData>& Assets)
{
	if (Assets.Num() == 0) return;

	// query list of assets in given directory paths in filter section
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::GetModuleChecked<FAssetRegistryModule>("AssetRegistry");
	TArray<FAssetData> ProcessingAssets;
	for (const auto& Filter : Settings->DirectoryPaths)
	{
		TArray<FAssetData> AssetsInPath;
		AssetRegistryModule.Get().GetAssetsByPath(FName{*Filter.Path}, AssetsInPath, true);

		for (const auto& Asset : AssetsInPath)
		{
			ProcessingAssets.AddUnique(Asset);
		}
	}

	// query all related assets that contains in given directory paths
	TArray<FName> RelatedAssets;
	for (const auto& Asset : ProcessingAssets)
	{
		const auto AssetNode = AdjacencyList->FindByPredicate([&](const FNode& Elem)
		{
			return Elem.Asset == Asset.PackageName;
		});
		if (AssetNode)
		{
			FindAllRelatedAssets(*AssetNode, RelatedAssets, *AdjacencyList);
		}
	}

	// remove them from deletion list
	Assets.RemoveAll([&](const FAssetData& Elem)
	{
		return RelatedAssets.Contains(Elem.PackageName);
	});
}

void Filter_ExcludedDirectories::FindAllRelatedAssets(const FNode& Node,
                                             TArray<FName>& RelatedAssets,
                                             const TArray<FNode>& List)
{
	RelatedAssets.AddUnique(Node.Asset);
	for (const auto& Adj : Node.AdjacentAssets)
	{
		if (!RelatedAssets.Contains(Adj))
		{
			const FNode* NodeRef = List.FindByPredicate([&](const FNode& Elem)
            {
                return Elem.Asset == Adj;
            });

			if (NodeRef)
			{
				FindAllRelatedAssets(*NodeRef, RelatedAssets, List);
			}
		}
	}
}

void Filter_ExcludedDirectories::CreateAdjacencyList(TArray<FAssetData>& Assets, TArray<FNode>& List)
{
	if (Assets.Num() == 0) return;

	FAssetRegistryModule& AssetRegistry = FModuleManager::GetModuleChecked<FAssetRegistryModule>("AssetRegistry");

	for (const auto& Asset : Assets)
	{
		FNode Node;
		Node.Asset = Asset.PackageName;
		TArray<FName> Deps;
		TArray<FName> Refs;
		AssetRegistry.Get().GetDependencies(Asset.PackageName, Deps);
		AssetRegistry.Get().GetReferencers(Asset.PackageName, Refs);

		for (const auto& Dep : Deps)
		{
			FAssetData* UnusedAsset = Assets.FindByPredicate([&](const FAssetData& Elem)
            {
                return Elem.PackageName == Dep;
            });
			if (UnusedAsset && UnusedAsset->PackageName != Asset.PackageName)
			{
				Node.AdjacentAssets.AddUnique(Dep);
			}
		}

		for (const auto& Ref : Refs)
		{
			FAssetData* UnusedAsset = Assets.FindByPredicate([&](const FAssetData& Elem)
            {
                return Elem.PackageName == Ref;
            });
			if (UnusedAsset && UnusedAsset->PackageName != Asset.PackageName)
			{
				Node.AdjacentAssets.AddUnique(Ref);
			}
		}

		List.Add(Node);
	}
}
