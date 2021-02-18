// Fill out your copyright notice in the Description page of Project Settings.

#include "AssetQueryManager.h"
#include "AssetFilterManager.h"
#include "ProjectCleanerUtility.h"
#include "UI/SProjectCleanerBrowser.h"
#include "StructsContainer.h"

// Engine Headers
#include "AssetRegistryModule.h"
#include "AssetRegistry/Public/AssetData.h"
#include "Engine/World.h"
#include "Misc/ScopedSlowTask.h"

#pragma optimize("", off)

void AssetQueryManager::GetUnusedAssets(TArray<FAssetData>& AssetContainer,
                                        UDirectoryFilterSettings* DirectoryFilterSettings,
                                        TArray<FNode>& AdjacencyList)
{
	FScopedSlowTask SlowTask{3.0f, FText::FromString("Scanning project for unused assets...")};
	SlowTask.MakeDialog();

	AssetContainer.Reset();
	AdjacencyList.Reset();

	// 1) Filtering and keeping all assets that never used in any levels
	GetAllAssets(AssetContainer);
	AssetFilterManager::RemoveLevelAssets(AssetContainer);
	TSet<FName> Deps;
	GetAllAssetNamesThatUsedInLevel(Deps);
	AssetFilterManager::Difference(AssetContainer, Deps);
	CreateAdjacencyList(AssetContainer, AdjacencyList);

	SlowTask.EnterProgressFrame(1.0f);

	// 2) Filtering assets and their related assets from directories "Exclude This Directories" filter
	if (ShouldApplyDirectoryFilters(DirectoryFilterSettings))
	{
		ApplyDirectoryFilters(AssetContainer, DirectoryFilterSettings, AdjacencyList);
	}

	SlowTask.EnterProgressFrame(1.0f);

	// 3) Filtering also assets that used in source code files via hardlinks, and for every single one, finding
	// related assets and excluding them from deletion list
	AssetFilterManager::RemoveAllAssetsUsedInSourceFiles(AssetContainer, AdjacencyList);

	SlowTask.EnterProgressFrame(1.0f);
}

int64 AssetQueryManager::GetTotalSize(const TArray<FAssetData>& AssetContainer)
{
	int64 Size = 0;
	for (const auto& Asset : AssetContainer)
	{
		FAssetRegistryModule& AssetRegistry = FModuleManager::GetModuleChecked<FAssetRegistryModule>("AssetRegistry");
		const auto AssetPackageData = AssetRegistry.Get().GetAssetPackageData(Asset.PackageName);
		if (!AssetPackageData) continue;
		Size += AssetPackageData->DiskSize;
	}

	return Size;
}

void AssetQueryManager::GetRootAssets(TArray<FAssetData>& RootAssets, TArray<FAssetData>& AssetContainer)
{
	FAssetRegistryModule& AssetRegistry = FModuleManager::GetModuleChecked<FAssetRegistryModule>("AssetRegistry");

	for (const auto& UnusedAsset : AssetContainer)
	{
		TArray<FName> SoftRefs;
		TArray<FName> HardRefs;
		AssetRegistry.Get().GetReferencers(UnusedAsset.PackageName, SoftRefs, EAssetRegistryDependencyType::Soft);
		AssetRegistry.Get().GetReferencers(UnusedAsset.PackageName, HardRefs, EAssetRegistryDependencyType::Hard);

		// sometimes assets has self reference on itself, so we remove them from list
		SoftRefs.RemoveAll([&](const FName& Val)
		{
			return Val == UnusedAsset.PackageName;
		});

		HardRefs.RemoveAll([&](const FName& Val)
		{
			return Val == UnusedAsset.PackageName;
		});

		if (HardRefs.Num() > 0) continue;

		if (SoftRefs.Num() > 0)
		{
			for (const auto& SoftRef : SoftRefs)
			{
				TArray<FName> Refs;
				AssetRegistry.Get().GetReferencers(SoftRef, Refs, EAssetRegistryDependencyType::Hard);

				// if soft refs referencer has same asset and no other as hard ref => add to list
				// This detects cycle
				if (Refs.Num() == 1 && Refs.Contains(UnusedAsset.PackageName))
				{
					FAssetData* AssetData = GetAssetData(SoftRef, AssetContainer);
					if (AssetData && AssetData->IsValid())
					{
						RootAssets.AddUnique(*AssetData);
						RootAssets.AddUnique(UnusedAsset);
					}
				}
			}
		}
		else
		{
			RootAssets.AddUnique(UnusedAsset);
		}
	}
}

void AssetQueryManager::FindAllRelatedAssets(const FNode& Node,
                                             TArray<FName>& RelatedAssets,
                                             const TArray<FNode> AdjacencyList)
{
	RelatedAssets.AddUnique(Node.Asset);
	for (const auto& Adj : Node.AdjacentAssets)
	{
		if (!RelatedAssets.Contains(Adj))
		{
			const FNode* NodeRef = AdjacencyList.FindByPredicate([&](const FNode& Elem)
			{
				return Elem.Asset == Adj;
			});

			if (NodeRef)
			{
				FindAllRelatedAssets(*NodeRef, RelatedAssets, AdjacencyList);
			}
		}
	}
}


void AssetQueryManager::GetAllAssets(TArray<FAssetData>& AssetContainer)
{
	FAssetRegistryModule& AssetRegistry = FModuleManager::GetModuleChecked<FAssetRegistryModule>("AssetRegistry");
	AssetRegistry.Get().GetAssetsByPath(FName{"/Game"}, AssetContainer, true);
}

void AssetQueryManager::GetAllAssetNamesThatUsedInLevel(TSet<FName>& DependencyContainer)
{
	FAssetRegistryModule& AssetRegistry = FModuleManager::GetModuleChecked<FAssetRegistryModule>("AssetRegistry");

	FARFilter Filter;
	Filter.ClassNames.Add(UWorld::StaticClass()->GetFName());
	Filter.PackagePaths.Add("/Game");
	Filter.bRecursivePaths = true;

	TArray<FName> PackageNamesToProcess;
	{
		TArray<FAssetData> FoundAssets;
		AssetRegistry.Get().GetAssets(Filter, FoundAssets);
		for (const FAssetData& AssetData : FoundAssets)
		{
			PackageNamesToProcess.Add(AssetData.PackageName);
			DependencyContainer.Add(AssetData.PackageName);
		}
	}

	TArray<FAssetIdentifier> AssetDependencies;
	while (PackageNamesToProcess.Num() > 0)
	{
		const FName PackageName = PackageNamesToProcess.Pop(false);
		AssetDependencies.Reset();
		AssetRegistry.Get().GetDependencies(FAssetIdentifier(PackageName), AssetDependencies);
		for (const FAssetIdentifier& Dependency : AssetDependencies)
		{
			bool bIsAlreadyInSet = false;
			DependencyContainer.Add(Dependency.PackageName, &bIsAlreadyInSet);
			if (bIsAlreadyInSet == false && Dependency.IsValid())
			{
				PackageNamesToProcess.Add(Dependency.PackageName);
			}
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

bool AssetQueryManager::ShouldApplyDirectoryFilters(UDirectoryFilterSettings* DirectoryFilterSettings)
{
	if (!DirectoryFilterSettings) return false;

	return DirectoryFilterSettings->DirectoryPaths.Num() > 0;
}

void AssetQueryManager::ApplyDirectoryFilters(TArray<FAssetData>& AssetContainer,
                                              UDirectoryFilterSettings* DirectoryFilterSettings,
                                              TArray<FNode>& AdjacencyList)
{
	if (AssetContainer.Num() == 0) return;

	// TArray<FNode> AdjacencyList;
	// CreateAdjacencyList(AssetContainer, AdjacencyList);

	// query list of assets in given directory paths in filter section
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::GetModuleChecked<FAssetRegistryModule>("AssetRegistry");
	TArray<FAssetData> ProcessingAssets;
	for (const auto& Filter : DirectoryFilterSettings->DirectoryPaths)
	{
		TArray<FAssetData> AssetsInPath;
		AssetRegistryModule.Get().GetAssetsByPath(FName{*Filter.Path}, AssetsInPath, true);

		for (const auto& Asset : AssetsInPath)
		{
			ProcessingAssets.AddUnique(Asset);
		}
	}

	// query all related assets that contains in given directory paths
	TArray<FName> RelatedAssets;
	for (const auto& Asset : ProcessingAssets)
	{
		const auto AssetNode = AdjacencyList.FindByPredicate([&](const FNode& Elem)
		{
			return Elem.Asset == Asset.PackageName;
		});
		if (AssetNode)
		{
			FindAllRelatedAssets(*AssetNode, RelatedAssets, AdjacencyList);
		}
	}

	// remove them from deletion list
	AssetContainer.RemoveAll([&](const FAssetData& Elem)
	{
		return RelatedAssets.Contains(Elem.PackageName);
	});
}

void AssetQueryManager::CreateAdjacencyList(TArray<FAssetData>& AssetContainer, TArray<FNode>& AdjacencyList)
{
	if (AssetContainer.Num() == 0) return;

	FAssetRegistryModule& AssetRegistry = FModuleManager::GetModuleChecked<FAssetRegistryModule>("AssetRegistry");

	for (const auto& Asset : AssetContainer)
	{
		FNode Node;
		Node.Asset = Asset.PackageName;
		TArray<FName> Deps;
		TArray<FName> Refs;
		AssetRegistry.Get().GetDependencies(Asset.PackageName, Deps);
		AssetRegistry.Get().GetReferencers(Asset.PackageName, Refs);

		for (const auto& Dep : Deps)
		{
			FAssetData* UnusedAsset = AssetContainer.FindByPredicate([&](const FAssetData& Elem)
			{
				return Elem.PackageName == Dep;
			});
			if (UnusedAsset && UnusedAsset->PackageName != Asset.PackageName)
			{
				Node.AdjacentAssets.AddUnique(Dep);
			}
		}

		for (const auto& Ref : Refs)
		{
			FAssetData* UnusedAsset = AssetContainer.FindByPredicate([&](const FAssetData& Elem)
			{
				return Elem.PackageName == Ref;
			});
			if (UnusedAsset && UnusedAsset->PackageName != Asset.PackageName)
			{
				Node.AdjacentAssets.AddUnique(Ref);
			}
		}

		AdjacencyList.Add(Node);
	}
}


#pragma optimize("", on)
