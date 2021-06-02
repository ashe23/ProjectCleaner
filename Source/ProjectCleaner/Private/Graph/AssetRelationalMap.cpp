// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#include "Graph/AssetRelationalMap.h"
#include "AssetRegistryModule.h"

void AssetRelationalMap::Rebuild(const TArray<FAssetData>& UnusedAssets)
{
	Reset();
	
	Nodes.Reserve(UnusedAssets.Num());
	RootNodes.Reserve(UnusedAssets.Num());
	CircularNodes.Reserve(UnusedAssets.Num());

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

	for (auto& Node : Nodes)
	{
		for (const auto Rel : Node.LinkedAssets)
		{
			const auto Data = FindByPackageName(Rel);
			if(!Data) continue;
			Node.LinkedAssetsData.Add(&Data->AssetData);
		}
	}

	FindCircularNodes();
	FindRootNodes();
}

void AssetRelationalMap::FindCircularNodes()
{
	for (const auto& Node : Nodes)
	{
		if (!IsCircularNode(Node)) continue;
		CircularNodes.AddUnique(Node);
	}
}

void AssetRelationalMap::FindRootNodes()
{
	for (const auto& Node : Nodes)
	{		
		if (RootNodes.Num() > 20) break; // todo:ashe23 chunks size here
		if (Node.Refs.Num() != 0) continue;
		RootNodes.AddUnique(Node);
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

bool AssetRelationalMap::IsCircularNode(const FAssetNode& Node) const
{
	for (const auto& Ref : Node.Refs)
	{
		if (Node.Deps.Contains(Ref))
		{
			return true;
		}
	}
	return false;
}

FAssetNode* AssetRelationalMap::FindByPackageName(const FName& PackageName)
{
	return Nodes.FindByPredicate([&](const FAssetNode& Elem)
	{
		return Elem.AssetData.PackageName.IsEqual(PackageName);
	});
}

const TArray<FAssetNode>& AssetRelationalMap::GetNodes() const
{
	return Nodes;
}

const TArray<FAssetNode>& AssetRelationalMap::GetCircularNodes() const
{
	return CircularNodes;
}

const TArray<FAssetNode>& AssetRelationalMap::GetRootNodes() const
{
	return RootNodes;
}

void AssetRelationalMap::Reset()
{
	Nodes.Reset();
	RootNodes.Reset();
	CircularNodes.Reset();
}

