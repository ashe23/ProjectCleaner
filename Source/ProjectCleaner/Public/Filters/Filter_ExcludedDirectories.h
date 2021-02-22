#pragma once

#include "Interfaces/Criteria.h"

struct FAssetData;
struct FNode;
class UDirectoryFilterSettings;

class Filter_ExcludedDirectories : public IProjectCleanerFilter
{
public:
	Filter_ExcludedDirectories(UDirectoryFilterSettings* DirectoryFilterSettings, TArray<FNode>* List);
	virtual void Apply(TArray<FAssetData>& Assets) override;
private:
	bool ShouldApplyDirectoryFilters() const;
	void ApplyDirectoryFilters(TArray<FAssetData>& Assets);
	void FindAllRelatedAssets(const FNode& Node,
	                          TArray<FName>& RelatedAssets,
	                          const TArray<FNode>& List);
	void CreateAdjacencyList(TArray<FAssetData>& Assets, TArray<FNode>& List);

	UDirectoryFilterSettings* Settings;
	TArray<FNode>* AdjacencyList;
};
