// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#pragma once

// Engine Headers
#include "CoreMinimal.h"
#include "StructsContainer.h"
#include "Widgets/SCompoundWidget.h"

/**
 * @brief Shows statistics info (UnusedAssets count, Total Size, EmptyFolder count, etc.)
 */
class SProjectCleanerBrowserStatisticsUI : public SCompoundWidget
{
public:
	
	SLATE_BEGIN_ARGS(SProjectCleanerBrowserStatisticsUI) {}
		SLATE_ARGUMENT(FCleaningStats, Stats);
	SLATE_END_ARGS()
	
	void Construct(const FArguments& InArgs);
	void SetStats(const FCleaningStats& NewStats);
	FCleaningStats GetStats() const;

private:
	void RefreshUIContent();
	
	/** Data **/
	FCleaningStats Stats;
	TSharedRef<SWidget> WidgetRef = SNullWidget::NullWidget;
};
