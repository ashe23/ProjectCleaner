// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#include "Graph/AssetRelationalMap.h"
#include "UI/ProjectCleanerConfigsUI.h"
#include "AssetRegistryModule.h"

void AssetRelationalMap::Rebuild(const TArray<FAssetData>& UnusedAssets, UCleanerConfigs* Configs)
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
	FindRootNodes(Configs);
}

void AssetRelationalMap::FindCircularNodes()
{
	for (const auto& Node : Nodes)
	{
		if (!IsCircularNode(Node)) continue;
		CircularNodes.AddUnique(Node);
	}
}

void AssetRelationalMap::FindRootNodes(UCleanerConfigs* Configs)
{
	int32 DeletionLimit = 20;
	if (Configs && Configs->IsValidLowLevel())
	{
		DeletionLimit = Configs->DeleteChunkLimit;
	}

	for (const auto& Node : Nodes)
	{		
		if (RootNodes.Num() > DeletionLimit) break; // todo:ashe23 chunks size here
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

	RelatedAssets.RemoveAll([&] (const FName& Elem)
	{
		return Elem.IsEqual(PackageName) || !Elem.ToString().StartsWith("/Game");
	});
}

void AssetRelationalMap::DFS(FAssetNode& Node, FAssetNode& RootNode)
{
	if (Node.Visited) return;
	Node.Visited = true;

	for (const auto RelatedAsset : Node.RelatedAssets)
	{
		RootNode.LinkedAssets.AddUnique(RelatedAsset);

		const auto AssetData = FindByPackageName(RelatedAsset);
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

void AssetRelationalMap::FindAssetsByClass(const TArray<UClass*> ExcludedClasses, TArray<FAssetData>& Assets)
{
	for (const auto& Node : Nodes)
	{
		const bool Contains = ExcludedClasses.ContainsByPredicate([&](const UClass* Elem) {
			return Elem && Elem->GetFName().IsEqual(Node.AssetData.AssetClass);
		});
		if (!Contains) continue;
		Assets.Add(Node.AssetData);
	}
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