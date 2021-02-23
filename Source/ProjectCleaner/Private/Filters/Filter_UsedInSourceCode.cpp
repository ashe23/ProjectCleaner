#include "Filters/Filter_UsedInSourceCode.h"
#include "ProjectCleanerUtility.h"
#include "StructsContainer.h"


Filter_UsedInSourceCode::Filter_UsedInSourceCode(TArray<FString>& SourceCodeFilesContents, TArray<FNode>& List)
{
	AdjacencyList = &List;
	Files = &SourceCodeFilesContents;
}

void Filter_UsedInSourceCode::Apply(TArray<FAssetData>& Assets)
{
	RemoveAllAssetsUsedInSourceFiles(Assets);
}

bool Filter_UsedInSourceCode::UsedInSourceFiles(const FAssetData& Asset)
{
	for (const auto& File : *Files)
	{
		if (
			(File.Find(Asset.PackageName.ToString()) != -1) ||
			File.Find(Asset.PackagePath.ToString()) != -1
		)
		{
			return true;
		}
	}

	return false;
}

void Filter_UsedInSourceCode::RemoveAllAssetsUsedInSourceFiles(TArray<FAssetData>& AssetContainer)
{
	TArray<FName> UsedInSourceFilesRelatedAssets;
	UsedInSourceFilesRelatedAssets.Reserve(100);

	for (const auto& Asset : AssetContainer)
	{
		// checking if current asset used in source files
		if (UsedInSourceFiles(Asset))
		{
			// finding him in adjacency list
			FNode* Node = AdjacencyList->FindByPredicate([&](const FNode& Elem)
			{
				return Elem.Asset == Asset.PackageName;
			});

			// and if its valid finding all related assets 
			if (Node)
			{
				ProjectCleanerUtility::FindAllRelatedAssets(*Node, UsedInSourceFilesRelatedAssets, *AdjacencyList);
			}
		}
	}

	if (UsedInSourceFilesRelatedAssets.Num() == 0) return;


	// removing all assets we found
	AssetContainer.RemoveAll([&](const FAssetData& Val)
	{
		return UsedInSourceFilesRelatedAssets.Contains(Val.PackageName);
	});
}
