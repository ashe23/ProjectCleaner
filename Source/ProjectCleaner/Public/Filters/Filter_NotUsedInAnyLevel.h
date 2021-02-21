#pragma once

#include "Interfaces/Criteria.h"

/**
 * @brief This filter removes all assets that used in any level from given list of assets
 */
class Filter_NotUsedInAnyLevel : public IProjectCleanerFilter
{
public:
	/**
	 * @brief Applies filter on given list of assets
	 * @param Assets 
	 */
	virtual void Apply(TArray<FAssetData>& Assets) override;
private:
	/**
	 * @brief Finds all assets that used in any "Level" asset
	 * @param Assets 
	 */
	void GetAllUsedAssets(TSet<FName>& Assets) const;
	/**
	 * @brief Returns AssetContainer after applying difference operation with FilterSet
	 * @param Assets 
	 * @param FilterSet 
	 */
	void Difference(TArray<FAssetData>& Assets, TSet<FName> FilterSet);
	
	/**
	 * @brief Container for all used assets in project
	 */
	TSet<FName> UsedAssets;
};
