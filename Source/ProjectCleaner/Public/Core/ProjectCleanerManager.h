// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "StructsContainer.h"

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
	TArray<FAssetData> LinkedAssets;
};

class ProjectCleanerManager
{
public:
	ProjectCleanerManager();
	FProjectCleanerData* GetCleanerData();
	UCleanerConfigs* GetCleanerConfigs() const;
	UExcludeOptions* GetExcludeOptions() const;
	TArray<FAssetNode>const * GetAdjacencyList() const;
	void UpdateData();
	void DeleteUnusedAssets();
	void DeleteEmptyFolders();
	void UpdateContentBrowser() const;
private:
	void GetRelatedAssets(const FName& PackageName, const ERelationType RelationType, TArray<FName>& RelatedAssets) const;
	void GetLinkedAssets(FAssetNode& Node, FAssetNode& RootNode, TSet<FName>& Visited);
	FAssetNode* FindByPackageName(const FName& PackageName);
	void GenerateAdjacencyList();
	bool IsExcludedByPath(const FAssetData& AssetData);
	bool IsExcludedByClass(const FAssetData& AssetData);
	bool IsCircular(const FAssetNode& AssetNode) const;
	bool HasReferencers(const FAssetNode& AssetNode) const;
	bool IsUnderDeveloperFolder(const FString& PackagePath) const;
	bool HasReferencersInDeveloperFolder(const FAssetNode& AssetNode) const;
	void RemoveAssetsFromDeveloperFolder();
	void RemoveIndirectAssets();
	void RemoveExcludedAssets();
	void GetAssetsChunk(TArray<FAssetData>& Chunk);

	FProjectCleanerData CleanerData;
	UCleanerConfigs* CleanerConfigs;
	UExcludeOptions* ExcludeOptions;
	TArray<FAssetNode> AdjacencyList;
};
