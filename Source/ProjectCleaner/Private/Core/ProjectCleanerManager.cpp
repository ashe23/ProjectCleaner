#include "Core/ProjectCleanerManager.h"
#include "Core/ProjectCleanerUtility.h"
#include "AssetRegistryModule.h"

ProjectCleanerManager::ProjectCleanerManager()
{
	CleanerConfigs = GetMutableDefault<UCleanerConfigs>();
	ExcludeOptions = GetMutableDefault<UExcludeOptions>();
}

FProjectCleanerData* ProjectCleanerManager::GetCleanerData()
{
	return &CleanerData;
}

void ProjectCleanerManager::UpdateData()
{
	ProjectCleanerUtility::GetEmptyFolders(CleanerData.EmptyFolders, CleanerConfigs->bScanDeveloperContents);
	ProjectCleanerUtility::GetInvalidFiles(CleanerData.CorruptedFiles, CleanerData.NonEngineFiles);
	ProjectCleanerUtility::GetUnusedAssets(CleanerData.UnusedAssets);
	// remove assets used indirectly

	// for (const FAssetData& UnusedAsset : CleanerData.UnusedAssets)
	// {
	// 	FAssetNode Node;
	// 	
	// 	GetRelatedAssets(UnusedAsset.PackageName, ERelationType::Reference, Node.Refs);
	// 	GetRelatedAssets(UnusedAsset.PackageName, ERelationType::Dependency, Node.Deps);
	//
	// 	// CleanerData.Nodes.Add(Node);
	// }
}

void ProjectCleanerManager::GetRelatedAssets(const FName& PackageName, const ERelationType RelationType, TArray<FName>& RelatedAssets) const
{
	FAssetRegistryModule& AssetRegistry = FModuleManager::GetModuleChecked<FAssetRegistryModule>("AssetRegistry");

	if (RelationType == ERelationType::Reference)
	{
		AssetRegistry.Get().GetReferencers(PackageName, RelatedAssets);
	}
	else
	{
		AssetRegistry.Get().GetDependencies(PackageName, RelatedAssets);
	}

	RelatedAssets.RemoveAll([&] (const FName& Elem)
	{
		return Elem.IsEqual(PackageName);
	});
}
