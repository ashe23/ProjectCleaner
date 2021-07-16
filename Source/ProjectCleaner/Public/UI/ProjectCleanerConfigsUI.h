// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class UCleanerConfigs;

class SProjectCleanerConfigsUI : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SProjectCleanerConfigsUI) {}
		SLATE_ARGUMENT(UCleanerConfigs*, CleanerConfigs)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	void SetCleanerConfigs(UCleanerConfigs* Configs);
private:
	TSharedPtr<IDetailsView> ConfigsProperty;
	UCleanerConfigs* CleanerConfigs = nullptr;
};