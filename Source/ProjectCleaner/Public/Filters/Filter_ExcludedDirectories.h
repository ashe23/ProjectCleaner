#pragma once

#include "Interfaces/Criteria.h"

struct FAssetData;
struct FNode;
class UDirectoryFilterSettings;

class Filter_ExcludedDirectories : public IProjectCleanerFilter
{
public:
	Filter_ExcludedDirectories(UDirectoryFilterSettings* DirectoryFilterSettings, TArray<FNode>& AdjacencyList);
	virtual void Apply(TArray<FAssetData>& Assets) override;
private:
	bool ShouldApplyDirectoryFilters() const;
	void ApplyDirectoryFilters(TArray<FAssetData>& Assets);
	
	UDirectoryFilterSettings* Settings;
	TArray<FNode>& AdjacencyList;
};
