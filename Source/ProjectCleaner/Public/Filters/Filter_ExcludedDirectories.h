#pragma once

#include "Interfaces/Criteria.h"

struct FAssetData;
struct FNode;
class UExcludeDirectoriesFilterSettings;

class Filter_ExcludedDirectories : public IProjectCleanerFilter
{
public:
	Filter_ExcludedDirectories(UExcludeDirectoriesFilterSettings* DirectoryFilterSettings, TArray<FNode>& List);
	virtual void Apply(TArray<FAssetData>& Assets) override;
private:
	bool ShouldApplyDirectoryFilters() const;
	void ApplyDirectoryFilters(TArray<FAssetData>& Assets);

	UExcludeDirectoriesFilterSettings* Settings;
	TArray<FNode>* AdjacencyList;
};
