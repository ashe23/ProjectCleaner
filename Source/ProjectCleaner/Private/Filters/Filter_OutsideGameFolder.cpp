#include "Filters/Filter_OutsideGameFolder.h"
#include "StructsContainer.h"


#pragma optimize("", off)
Filter_OutsideGameFolder::Filter_OutsideGameFolder(TArray<FNode>& List)
{
	AdjacencyList = List;
}

void Filter_OutsideGameFolder::Apply(TArray<FAssetData>& Assets)
{
	if (AdjacencyList.Num() == 0) return;

	for (const auto& Node : AdjacencyList)
	{
		if (Node.HasLinkedAssetsOutsideGameFolder())
		{
			Assets.RemoveAll([&](const FAssetData& Asset)
            {
                return Asset.PackageName.IsEqual(Node.Asset);
            });
		}
	}
}
#pragma optimize("", on)