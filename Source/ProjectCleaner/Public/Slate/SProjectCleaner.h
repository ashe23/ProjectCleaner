// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
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
	
	void CreateMenuBarSettings(FMenuBuilder& MenuBuilder, const TSharedPtr<FTabManager> TabManagerPtr);
	void CreateMenuBarTabs(FMenuBuilder& MenuBuilder, const TSharedPtr<FTabManager> TabManagerPtr);
	
	TSharedRef<SDockTab> OnTabSpawnScanSettings(const FSpawnTabArgs& Args) const;

	TSharedPtr<FTabManager> TabManager;
	TSharedPtr<FTabManager::FLayout> TabLayout;
	UProjectCleanerSubsystem* SubsystemPtr = nullptr;
};
