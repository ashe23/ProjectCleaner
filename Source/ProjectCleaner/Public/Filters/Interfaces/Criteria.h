#pragma once

#include "AssetRegistry/Public/AssetData.h"

/**
 * @brief Interface for Asset filters
 */
class IProjectCleanerFilter
{
public:
	virtual void Apply(TArray<FAssetData>& Assets) = 0;

	virtual ~IProjectCleanerFilter()
	{
	}
};
