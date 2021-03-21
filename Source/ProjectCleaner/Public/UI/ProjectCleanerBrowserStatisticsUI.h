#pragma once

// Engine Headers
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

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
	int32 UnusedAssets = 0;
	int64 TotalSize = 0;
	int32 EmptyFolders = 0;
};
