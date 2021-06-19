// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "ProjectCleanerHelper.h"

class UCleanerConfigs;

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

	bool operator==(const FAssetNode& OtherNode) const
	{
		return AssetData == OtherNode.AssetData;
	}

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

	bool HasReferencersInDeveloperFolder() const
	{
		const FString DeveloperFolderPath = FPaths::ConvertRelativePathToFull(FPaths::GameDevelopersDir());
		const FString CollectionsFolderPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() + TEXT("Collections/"));

		for (const auto& Ref : Refs)
		{
			const auto AbsPath = ProjectCleanerHelper::ConvertInternalToAbsolutePath(Ref.ToString());
			const bool IsUnderDir = FPaths::IsUnderDirectory(AbsPath, DeveloperFolderPath) || FPaths::IsUnderDirectory(AbsPath, CollectionsFolderPath);
			if (IsUnderDir)
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
	void Rebuild(const TArray<FAssetData>& UnusedAssets, UCleanerConfigs* Configs);
	FAssetNode* FindByPackageName(const FName& PackageName);
	void FindAssetsByClass(const TArray<UClass*> ExcludedClasses, TArray<FAssetData>& Assets);
	void FindAllLinkedAssets(const TSet<FName>& Assets, TSet<FName>& LinkedAssets);
	const TArray<FAssetNode>& GetNodes() const;
	const TArray<FAssetNode>& GetCircularNodes() const;
	const TArray<FAssetNode>& GetRootNodes() const;
	void Reset();
private:
	void GetRelatedAssets(
		const FName& PackageName,
		const ERelationType RelationType,
		TArray<FName>& RelatedAssets
	) const;
	void FindCircularNodes();
	void FindRootNodes(UCleanerConfigs* Configs);
	void DFS(FAssetNode& Node, FAssetNode& RootNode);
	void ClearVisited();
	bool IsCircularNode(const FAssetNode& Node) const;
	
	TArray<FAssetNode> Nodes;
	TArray<FAssetNode> CircularNodes;
	TArray<FAssetNode> RootNodes;
};
