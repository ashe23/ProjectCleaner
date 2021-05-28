#include "Graph/AssetRelationalMap.h"
#include "AssetRegistryModule.h"

void AssetRelationalMap::Fill(const TArray<FAssetData>& UnusedAssets)
{
	Nodes.Reserve(UnusedAssets.Num());

	for (const auto& UnusedAsset : UnusedAssets)
	{
		FAssetNode Node;
		Node.AssetData = UnusedAsset;

		GetRelatedAssets(UnusedAsset.PackageName, ERelationType::Reference, Node.Refs);
		GetRelatedAssets(UnusedAsset.PackageName, ERelationType::Dependency, Node.Deps);

		Node.RelatedAssets.Append(Node.Refs);
		Node.RelatedAssets.Append(Node.Deps);
		
		Nodes.Add(Node);
	}

	for (auto& Node : Nodes)
	{
		DFS(Node, Node);
		ClearVisited();
	}
}

void AssetRelationalMap::GetRelatedAssets(
	const FName& PackageName,
	const ERelationType RelationType,
	TArray<FName>& RelatedAssets) const
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

	// todo:ashe23 maybe remove also assets that are outside "/Game" folder
	RelatedAssets.RemoveAll([&] (const FName& Elem)
	{
		return Elem.IsEqual(PackageName);
	});
}

void AssetRelationalMap::DFS(FAssetNode& Node, FAssetNode& RootNode)
{
	if (Node.Visited) return;
	Node.Visited = true;

	for (const auto Rel : Node.RelatedAssets)
	{
		RootNode.LinkedAssets.AddUnique(Rel);

		const auto AssetData = FindByPackageName(Rel);
		if(!AssetData) continue;
		DFS(*AssetData, RootNode);
	}
}

void AssetRelationalMap::ClearVisited()
{
	for (auto& Node : Nodes)
	{
		Node.Visited = false;
	}
}

FAssetNode* AssetRelationalMap::FindByPackageName(const FName& PackageName)
{
	return Nodes.FindByPredicate([&](const FAssetNode& Elem)
	{
		return Elem.AssetData.PackageName.IsEqual(PackageName);
	});
}

