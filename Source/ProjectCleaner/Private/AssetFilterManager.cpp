// Fill out your copyright notice in the Description page of Project Settings.


#include "AssetFilterManager.h"
#include "ProjectCleanerUtility.h"

// Engine Headers
#include "AssetQueryManager.h"
#include "AssetRegistryModule.h"
#include "AssetRegistry/Public/AssetData.h"
#include "Engine/World.h"
#pragma optimize("", off)

void AssetFilterManager::RemoveLevelAssets(TArray<FAssetData>& AssetContainer)
{
	AssetContainer.RemoveAll([](const FAssetData& Val)
	{
		return IsLevelAsset(Val.AssetClass);
	});
}

bool AssetFilterManager::IsLevelAsset(const FName& ClassName)
{
	return ClassName == "MapBuildDataRegistry" || ClassName == UWorld::StaticClass()->GetFName();
}

void AssetFilterManager::Difference(TArray<FAssetData>& AssetContainer, TSet<FName>& FilterSet)
{
	AssetContainer.RemoveAll([&](const FAssetData& Elem)
	{
		return FilterSet.Contains(Elem.PackageName);
	});
}

void AssetFilterManager::RemoveAllAssetsUsedInSourceFiles(TArray<FAssetData>& AssetContainer,
                                                          TArray<FNode>& AdjacencyList)
{
	TArray<FString> AllSourceFiles;
	ProjectCleanerUtility::FindAllSourceFiles(AllSourceFiles);

	TArray<FName> UsedInSourceFilesRelatedAssets;
	UsedInSourceFilesRelatedAssets.Reserve(100);

	for (const auto& Asset : AssetContainer)
	{
		// checking if current asset used in source files
		if (ProjectCleanerUtility::UsedInSourceFiles(AllSourceFiles, Asset.PackageName))
		{
			// finding him in adjacency list
			FNode* Node = AdjacencyList.FindByPredicate([&](const FNode& Elem)
            {
                return Elem.Asset == Asset.PackageName;
            });

			// and if its valid finding all related assets 
			if (Node)
			{
				AssetQueryManager::FindAllRelatedAssets(*Node, UsedInSourceFilesRelatedAssets, AdjacencyList);
			}
		}
	}

	// removing all assets we found
	AssetContainer.RemoveAll([&](const FAssetData& Val)
    {
        return UsedInSourceFilesRelatedAssets.Contains(Val.PackageName);
    });
}

void AssetFilterManager::IsCyclic(FNode* Node, TArray<FNode>& AdjacencyList, TArray<FNode*>& Visited)
{
	Visited.AddUnique(Node);
}

#pragma optimize("", on)
