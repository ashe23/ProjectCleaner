// Fill out your copyright notice in the Description page of Project Settings.


#include "AssetFilterManager.h"
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

void AssetFilterManager::Difference(TArray<FAssetData>& FirstContainer, TArray<FAssetData>& SecondContainer)
{
	FirstContainer.RemoveAll([&](const FAssetData& Elem)
	{
		return SecondContainer.Contains(Elem);
	});
}

#pragma optimize("", on)
