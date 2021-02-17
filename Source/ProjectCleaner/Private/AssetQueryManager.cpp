// Fill out your copyright notice in the Description page of Project Settings.

#include "AssetQueryManager.h"
#include "AssetRegistryModule.h"
#include "AssetRegistry/Public/AssetData.h"
#include "Engine/World.h"

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
