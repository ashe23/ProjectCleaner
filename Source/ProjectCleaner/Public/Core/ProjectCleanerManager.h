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
	void UpdateData();
private:
	void GetRelatedAssets(const FName& PackageName, const ERelationType RelationType, TArray<FName>& RelatedAssets) const;
	void GetLinkedAssets(FAssetNode& Node, FAssetNode& RootNode, TSet<FName>& Visited);
	FAssetNode* FindByPackageName(const FName& PackageName);
	void GenerateAdjacencyList();
	void FindAllLinkedAssets();

	FProjectCleanerData CleanerData;
	UCleanerConfigs* CleanerConfigs;
	UExcludeOptions* ExcludeOptions;
	TArray<FAssetNode> AdjacencyList;
};
