// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#pragma once

// Engine Headers
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class SProjectCleanerStatisticsUI : public SCompoundWidget
{
public:
	
	SLATE_BEGIN_ARGS(SProjectCleanerStatisticsUI) {}
		SLATE_ARGUMENT(class FProjectCleanerManager*, CleanerManager);
	SLATE_END_ARGS()
	
	void Construct(const FArguments& InArgs);
	void SetCleanerManager(FProjectCleanerManager* CleanerManagerPtr);
private:
	/* UI Callbacks */
	FText GetAllAssetsNum() const;
	FText GetUnusedAssetsNum() const;
	FText GetTotalProjectSize() const;
	FText GetTotalUnusedAssetsSize() const;
	FText GetNonEngineFilesNum() const;
	FText GetIndirectAssetsNum() const;
	FText GetEmptyFoldersNum() const;
	FText GetCorruptedAssetsNum() const;
	FText GetExcludedAssetsNum() const;
	TOptional<float> GetPercentRatio() const;
	FSlateColor GetProgressBarColor() const;
	FText GetProgressBarText() const;
	
	/** Data **/
	FProjectCleanerManager* CleanerManager = nullptr;
};