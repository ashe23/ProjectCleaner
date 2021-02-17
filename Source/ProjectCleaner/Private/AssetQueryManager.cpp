// Fill out your copyright notice in the Description page of Project Settings.

#include "AssetQueryManager.h"
#include "AssetRegistryModule.h"
#include "AssetRegistry/Public/AssetData.h"
#include "Engine/World.h"

#pragma optimize("", off)

void AssetQueryManager::GetAllAssets(TArray<FAssetData>& AssetContainer)
{
	FAssetRegistryModule& AssetRegistry = FModuleManager::GetModuleChecked<FAssetRegistryModule>("AssetRegistry");
	AssetRegistry.Get().GetAssetsByPath(FName{"/Game"}, AssetContainer, true);
}

void AssetQueryManager::GetAssets(TArray<FAssetData>& AssetContainer, const FName& Path, bool bRecursive)
{
	FAssetRegistryModule& AssetRegistry = FModuleManager::GetModuleChecked<FAssetRegistryModule>("AssetRegistry");
	AssetRegistry.Get().GetAssetsByPath(Path, AssetContainer, bRecursive);
}

void AssetQueryManager::GetLevelAssets(TArray<FAssetData>& AssetContainer)
{
	FAssetRegistryModule& AssetRegistry = FModuleManager::GetModuleChecked<FAssetRegistryModule>("AssetRegistry");

	FARFilter Filter;
	Filter.ClassNames.Add(UWorld::StaticClass()->GetFName());
	Filter.PackagePaths.Add(FName{"/Game"});
	Filter.bRecursivePaths = true;

	AssetRegistry.Get().GetAssets(Filter, AssetContainer);
}

void AssetQueryManager::GetDependencies(TArray<FAssetData>& AssetContainer, TArray<FAssetData>& DependencyContainer)
{
	FAssetRegistryModule& AssetRegistry = FModuleManager::GetModuleChecked<FAssetRegistryModule>("AssetRegistry");

	{
		TArray<FName> Deps;
		Deps.Reserve(200);
		
		for (const auto& Asset : AssetContainer)
		{
			AssetRegistry.Get().GetDependencies(Asset.PackageName, Deps);

			for(const auto& Dep : Deps)
			{
				FAssetData* AssetData = GetAssetData(Dep,AssetContainer);
				if(AssetData)
				{
					DependencyContainer.AddUnique(*AssetData);
				}
			}

			Deps.Reset();
		}
	}
}

FAssetData* AssetQueryManager::GetAssetData(const FName& AssetName, TArray<FAssetData>& AssetContainer)
{
	return AssetContainer.FindByPredicate([&](const FAssetData& Val)
    {
        return Val.PackageName == AssetName;
    });
}

#pragma optimize("", on)
