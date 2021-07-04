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
	void SetData(FProjectCleanerData& Data);
private:
	/* UI Callbacks */
	FText GetUnusedAssetsNum() const;
	FText GetTotalSize() const;
	FText GetNonEngineFilesNum() const;
	FText GetIndirectAssetsNum() const;
	FText GetEmptyFoldersNum() const;
	FText GetCorruptedFilesNum() const;
    
	/** Data **/
	FProjectCleanerData* CleanerData = nullptr;
};