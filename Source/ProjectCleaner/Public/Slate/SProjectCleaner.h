﻿// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectCleanerTypes.h"
#include "Widgets/SCompoundWidget.h"

class UProjectCleanerSubsystem;

class SProjectCleaner final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SProjectCleaner)
		{
		}

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<SDockTab>& ConstructUnderMajorTab, const TSharedPtr<SWindow>& ConstructUnderWindow);
	virtual ~SProjectCleaner() override;

private:
	bool WidgetEnabled() const;
	int32 WidgetGetIndex() const;
	FText WidgetText() const;

	static void CreateMenuBarTabs(FMenuBuilder& MenuBuilder, const TSharedPtr<FTabManager> TabManagerPtr);

	TSharedRef<SDockTab> OnTabSpawnScanSettings(const FSpawnTabArgs& Args) const;
	TSharedRef<SDockTab> OnTabSpawnScanInfo(const FSpawnTabArgs& Args) const;
	TSharedRef<SDockTab> OnTabSpawnIndirectAssets(const FSpawnTabArgs& Args) const;
	TSharedRef<SDockTab> OnTabSpawnNonEngineFiles(const FSpawnTabArgs& Args) const;
	TSharedRef<SDockTab> OnTabSpawnCorruptedFiles(const FSpawnTabArgs& Args) const;
	TSharedRef<SDockTab> OnTabSpawnAssetsTableView(const FSpawnTabArgs& Args) const;

	TSharedPtr<FTabManager> TabManager;
	TSharedPtr<FTabManager::FLayout> TabLayout;
	FProjectCleanerScanData ScanData;
	UProjectCleanerSubsystem* Subsystem = nullptr;
};
