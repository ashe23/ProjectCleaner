#pragma once

// Engine Headers
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

/**
 * @brief Shows statistics info (UnusedAssets count, Total Size, EmptyFolder count)
 */
class SProjectCleanerBrowserStatisticsUI : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SProjectCleanerBrowserStatisticsUI) {}
		SLATE_ARGUMENT(int32, UnusedAssets)
		SLATE_ARGUMENT(int64, TotalSize)
		SLATE_ARGUMENT(int32, EmptyFolders)
	SLATE_END_ARGS()
	void Construct(const FArguments& InArgs);
private:
	/**
	 * @brief Unused assets count
	 */
	int32 UnusedAssets = 0;
	/**
	 * @brief Total size of unused assets
	 */
	int64 TotalSize = 0;
	/**
	 * @brief Empty folders count
	 */
	int32 EmptyFolders = 0;
};
