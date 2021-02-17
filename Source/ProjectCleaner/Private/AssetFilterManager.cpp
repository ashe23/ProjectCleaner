// Fill out your copyright notice in the Description page of Project Settings.


#include "AssetFilterManager.h"
#include "AssetRegistryModule.h"
#include "AssetRegistry/Public/AssetData.h"
#include "Engine/World.h"

void AssetFilterManager::RemoveLevelAssets(TArray<FAssetData>& AssetContainer)
{
	AssetContainer.RemoveAll([](const FAssetData& Val)
	{
		return
			Val.AssetClass.ToString().Contains("MapBuildDataRegistry") ||
			Val.AssetClass == UWorld::StaticClass()->GetFName();
	});
}
