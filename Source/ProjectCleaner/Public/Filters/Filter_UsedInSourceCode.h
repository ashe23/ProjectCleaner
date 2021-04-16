#pragma once

#include "Interfaces/Criteria.h"

struct FNode;
struct FAssetData;
struct FSourceCodeFile;
class USourceCodeAsset;

class Filter_UsedInSourceCode : public IProjectCleanerFilter
{
public:
	Filter_UsedInSourceCode(TArray<FSourceCodeFile>& SourceFiles, TArray<FNode>& List, TArray<TWeakObjectPtr<USourceCodeAsset>>& AssetsUsedInSourceCodeUIStructs);
	virtual void Apply(TArray<FAssetData>& Assets) override;
private:
	bool UsedInSourceFiles(const FAssetData& Asset) const;
	
	TArray<FSourceCodeFile>* SourceFiles;
	TArray<FNode>* AdjacencyList;
	TArray<TWeakObjectPtr<USourceCodeAsset>>* AssetsUsedInSourceCodeUIStructs;
};
