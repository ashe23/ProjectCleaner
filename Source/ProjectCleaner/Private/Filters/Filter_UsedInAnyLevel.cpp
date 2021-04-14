#include "Filters/Filter_UsedInAnyLevel.h"

// Engine Headers
#include "AssetRegistryModule.h"
#include "AssetRegistry/Public/AssetData.h"
#include "Engine/World.h"

void Filter_UsedInAnyLevel::Apply(TArray<FAssetData>& Assets)
{
	UsedAssets.Reserve(Assets.Num());
	GetAllUsedAssets(UsedAssets);

	Difference(Assets, UsedAssets);
}

void Filter_UsedInAnyLevel::GetAllUsedAssets(TSet<FName>& Assets) const
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
			Assets.Add(AssetData.PackageName);
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
			Assets.Add(Dependency.PackageName, &bIsAlreadyInSet);
			if (bIsAlreadyInSet == false && Dependency.IsValid())
			{
				PackageNamesToProcess.Add(Dependency.PackageName);
			}
		}
	}
}

void Filter_UsedInAnyLevel::Difference(TArray<FAssetData>& Assets, TSet<FName> FilterSet)
{
	Assets.RemoveAll([&](const FAssetData& Elem)
	{
		return FilterSet.Contains(Elem.PackageName);
	});
}
