#include "Filters/Filter_ExcludedDirectories.h"
#include "UI/ProjectCleanerDirectoryExclusionUI.h"
#include "StructsContainer.h"
#include "ProjectCleanerUtility.h"

// Engine Headers
#include "AssetRegistry/Public/AssetData.h"
#include "AssetRegistryModule.h"

Filter_ExcludedDirectories::Filter_ExcludedDirectories(UExcludeDirectoriesFilterSettings* DirectoryFilterSettings,
                                                       TArray<FNode>& List)
{
	Settings = DirectoryFilterSettings;
	this->AdjacencyList = &List;
}

void Filter_ExcludedDirectories::Apply(TArray<FAssetData>& Assets)
{
	if (ShouldApplyDirectoryFilters())
	{
		ApplyDirectoryFilters(Assets);
	}
}

bool Filter_ExcludedDirectories::ShouldApplyDirectoryFilters() const
{
	if (!Settings) return false;

	return Settings->Paths.Num() > 0;
}

void Filter_ExcludedDirectories::ApplyDirectoryFilters(TArray<FAssetData>& Assets)
{
	if (Assets.Num() == 0) return;

	// query list of assets in given directory paths in filter section
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::GetModuleChecked<FAssetRegistryModule>("AssetRegistry");
	TArray<FAssetData> ProcessingAssets;
	for (const auto& Filter : Settings->Paths)
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
		const auto AssetNode = AdjacencyList->FindByPredicate([&](const FNode& Elem)
		{
			return Elem.Asset == Asset.PackageName;
		});
		if (AssetNode)
		{
			ProjectCleanerUtility::FindAllRelatedAssets(*AssetNode, RelatedAssets, *AdjacencyList);
		}
	}

	// remove them from deletion list
	Assets.RemoveAll([&](const FAssetData& Elem)
	{
		return RelatedAssets.Contains(Elem.PackageName);
	});
}