#pragma once

#include "Interfaces/Criteria.h"

struct FNode;
struct FAssetData;

class Filter_UsedInSourceCode : public IProjectCleanerFilter
{
public:
	Filter_UsedInSourceCode(TArray<FString>& SourceCodeFilesContents, TArray<FNode>& List);
	virtual void Apply(TArray<FAssetData>& Assets) override;
private:
	bool UsedInSourceFiles(const FAssetData& Asset) const;
	
	TArray<FString>* Files;
	TArray<FNode>* AdjacencyList;
};
