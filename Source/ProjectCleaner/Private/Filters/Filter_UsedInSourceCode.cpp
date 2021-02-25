#include "Filters/Filter_UsedInSourceCode.h"
#include "ProjectCleanerUtility.h"
#include "StructsContainer.h"

#pragma optimize("", off)
Filter_UsedInSourceCode::Filter_UsedInSourceCode(TArray<FString>& SourceCodeFilesContents, TArray<FNode>& List)
{
	AdjacencyList = &List;
	Files = &SourceCodeFilesContents;
}

void Filter_UsedInSourceCode::Apply(TArray<FAssetData>& Assets)
{
	TArray<FName> UsedInSourceFilesRelatedAssets;
	UsedInSourceFilesRelatedAssets.Reserve(100);

	for (const auto& Asset : Assets)
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
	Assets.RemoveAll([&](const FAssetData& Val)
	{
		return UsedInSourceFilesRelatedAssets.Contains(Val.PackageName);
	});
}

bool Filter_UsedInSourceCode::UsedInSourceFiles(const FAssetData& Asset) const
{
	// todo:ashe23 change detection method
	for (const auto& File : *Files)
	{
		FString f = Asset.AssetName.ToString();
		f.InsertAt(0, TEXT("\""));
		f.Append(TEXT("\""));
		
		if (
			File.Contains(Asset.PackageName.ToString()) ||
			File.Contains(f)
		)
		{
			return true;
		}
	}

	return false;
}
#pragma optimize("", on)
