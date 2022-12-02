﻿// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectCleanerTypes.h"
#include "Widgets/SCompoundWidget.h"

struct FProjectCleanerScanner;
class SProjectCleanerTreeView;
class UProjectCleanerScanSettings;

class SProjectCleanerTabUnused final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SProjectCleanerTabUnused){}
		SLATE_ARGUMENT(TSharedPtr<FProjectCleanerScanner>, Scanner)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	void UpdateView();
private:
	void OnTreeViewSelectionChange(const TSharedPtr<FProjectCleanerTreeViewItem>& Item);
	
	TSharedPtr<FProjectCleanerScanner> Scanner;
	TWeakObjectPtr<UProjectCleanerScanSettings> ScanSettings;
	TSharedPtr<SProjectCleanerTreeView> ProjectCleanerTreeView;
};