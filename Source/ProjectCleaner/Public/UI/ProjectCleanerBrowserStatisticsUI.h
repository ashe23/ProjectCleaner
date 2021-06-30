// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#pragma once

// Engine Headers
#include "CoreMinimal.h"
#include "StructsContainer.h"
#include "Widgets/SCompoundWidget.h"

class SProjectCleanerBrowserStatisticsUI : public SCompoundWidget
{
public:
	
	SLATE_BEGIN_ARGS(SProjectCleanerBrowserStatisticsUI) {}
		SLATE_ARGUMENT(FProjectCleanerData*, CleanerData);
	SLATE_END_ARGS()
	
	void Construct(const FArguments& InArgs);
	void SetStats(FProjectCleanerData& Data);
private:
	/* UI Callbacks */
	FText GetUnusedAssetsNum() const;
	/** Data **/
	FProjectCleanerData* CleanerData = nullptr;
};