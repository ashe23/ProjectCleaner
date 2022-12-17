// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class UProjectCleanerSubsystem;

class SProjectCleanerTabScanInfo final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SProjectCleanerTabScanInfo)
		{
		}

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
private:
	void UpdateView();
	void UpdateTreeView();
	void UpdateAssetBrowser();

	UProjectCleanerSubsystem* Subsystem = nullptr;
};
