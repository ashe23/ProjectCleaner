#include "Filters/Filter_OutsideGameFolder.h"
#include "StructsContainer.h"

Filter_OutsideGameFolder::Filter_OutsideGameFolder(TArray<FNode>& List)
{
	AdjacencyList = List;
}

void Filter_OutsideGameFolder::Apply(TArray<FAssetData>& Assets)
{
	if (AdjacencyList.Num() == 0) return;

	TSet<const FNode*> FilteredNodes;
	FilteredNodes.Reserve(Assets.Num());
	for (const auto& Node : AdjacencyList)
	{
		if (Node.HasLinkedAssetsOutsideGameFolder())
		{
			FilteredNodes.Add(&Node);
		}
	}

	// query all linked assets for filtered assets
	TArray<FName> LinkedAssets;
	for(const auto& Node : FilteredNodes)
	{
		FindAllLinkedAssets(*Node, LinkedAssets, AdjacencyList);
	}

	Assets.RemoveAll([&](const FAssetData& Elem)
	{
		return LinkedAssets.Contains(Elem.PackageName);
	});
}

void Filter_OutsideGameFolder::FindAllLinkedAssets(
	const FNode& Node,
	TArray<FName>& LinkedAssets,
	const TArray<FNode>& List
)
{
	LinkedAssets.AddUnique(Node.Asset);
	for (const auto& Adj : Node.LinkedAssets)
	{
		if (!LinkedAssets.Contains(Adj))
		{
			const FNode* NodeRef = List.FindByPredicate([&](const FNode& Elem)
			{
				return Elem.Asset == Adj;
			});

			if (NodeRef)
			{
				FindAllLinkedAssets(*NodeRef, LinkedAssets, List);
			}
		}
	}
}
