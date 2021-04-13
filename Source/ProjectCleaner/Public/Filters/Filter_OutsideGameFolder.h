#pragma once

#include "Interfaces/Criteria.h"
struct FNode;

class Filter_OutsideGameFolder : public IProjectCleanerFilter
{
public:
	Filter_OutsideGameFolder(TArray<FNode>& List);
	virtual void Apply(TArray<FAssetData>& Assets) override;

private:
	static void FindAllLinkedAssets(const FNode& Node, TArray<FName>& LinkedAssets, const TArray<FNode>& List);
	TArray<FNode> AdjacencyList;
};
