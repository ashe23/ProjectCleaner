// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#pragma once

enum class ERelationType
{
	Reference,
	Dependency
};

struct FAssetNode
{
	FAssetData AssetData;
	TArray<FName> Refs;
	TArray<FName> Deps;
	TArray<FName> LinkedAssets;
	TArray<FAssetData*> LinkedAssetsData;

	// helpers
	TArray<FName> RelatedAssets; // (Refs + Deps)
	bool Visited = false;

	bool HasExternalReferencers() const
	{
		for (const auto& Ref : Refs)
		{
			if (!Ref.ToString().StartsWith("/Game"))
			{
				return true;
			}
		}
		
		return false;
	}
};

class AssetRelationalMap
{
public:
	void Fill(const TArray<FAssetData>& UnusedAssets);
	FAssetNode* FindByPackageName(const FName& PackageName);
	const TArray<FAssetNode>& GetNodes() const;
	void Reset();
private:
	
	
	void GetRelatedAssets(
		const FName& PackageName,
		const ERelationType RelationType,
		TArray<FName>& RelatedAssets
	) const;
	
	void DFS(FAssetNode& Node, FAssetNode& RootNode);
	void ClearVisited();
	
	TArray<FAssetNode> Nodes;
};
