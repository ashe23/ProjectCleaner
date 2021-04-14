#pragma once

#include "Interfaces/Criteria.h"
struct FNode;

/**
 * @brief Filters all assets that has link to assets that are outside of "Game" folder
 * Example is Megascans Plugin which has default materials in /Engine/Plugins/Megascans/Content folder
 * and other materials built based on that materials.
 * 
 * We must exclude that asset to prevent any asset corruption.
 * ( UE4 will show "Assets has Non-Displayable Referencer" Message, if you try to delete those assets)
 * 
 * Best approach would be if user deactivates plugin, cleans all assets and then reactivates it.
 * But i doubt that anyone would do it.
 */
class Filter_OutsideGameFolder : public IProjectCleanerFilter
{
public:
	Filter_OutsideGameFolder(TArray<FNode>& List);
	virtual void Apply(TArray<FAssetData>& Assets) override;

private:
	TArray<FNode> AdjacencyList;
};
