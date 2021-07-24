// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#pragma once

// Engine Headers
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class SProjectCleanerStatisticsUI : public SCompoundWidget
{
public:
	
	SLATE_BEGIN_ARGS(SProjectCleanerStatisticsUI) {}
		SLATE_ARGUMENT(class ProjectCleanerDataManager*, DataManager);
	SLATE_END_ARGS()
	
	void Construct(const FArguments& InArgs);
	void SetDataManager(ProjectCleanerDataManager* DataManagerPtr);
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
    
	/** Data **/
	ProjectCleanerDataManager* DataManager = nullptr;
};