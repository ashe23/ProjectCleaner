// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class UProjectCleanerSubsystem;
class SProjectCleanerTabScanSettings;
class SProjectCleanerTabUnused;
class SProjectCleanerTabIndirect;
class SProjectCleanerTabCorrupted;
class SProjectCleanerTabNonEngine;

// Plugins Main UserInterface
class SProjectCleaner final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SProjectCleaner)
		{
		}

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<SDockTab>& ConstructUnderMajorTab, const TSharedPtr<SWindow>& ConstructUnderWindow);
	virtual ~SProjectCleaner() override;
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

private:
	bool WidgetEnabled() const;
	int32 WidgetGetIndex() const;
	FText WidgetText() const;

	static void MenuBarFillTabs(FMenuBuilder& MenuBuilder, const TSharedPtr<FTabManager> TabManagerPtr);
	void MenuBarFillSettings(FMenuBuilder& MenuBuilder, const TSharedPtr<FTabManager> TabManagerPtr) const;
	static void MenuBarFillHelp(FMenuBuilder& MenuBuilder, const TSharedPtr<FTabManager> TabManagerPtr);

	TSharedRef<SDockTab> OnTabSpawnScanSettings(const FSpawnTabArgs& Args) const;
	TSharedRef<SDockTab> OnTabSpawnUnusedAssets(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> OnTabSpawnIndirectAssets(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> OnTabSpawnCorruptedAssets(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> OnTabSpawnNonEngineFiles(const FSpawnTabArgs& Args);

	TSharedPtr<FTabManager> TabManager;
	TSharedPtr<FTabManager::FLayout> TabLayout;
	UProjectCleanerSubsystem* SubsystemPtr = nullptr;

	// TWeakPtr<SProjectCleanerTabScanSettings> TabScanSettings;
	// TWeakPtr<SProjectCleanerTabUnused> TabUnused;
	// TWeakPtr<SProjectCleanerTabIndirect> TabIndirect;
	// TWeakPtr<SProjectCleanerTabCorrupted> TabCorrupted;
	// TWeakPtr<SProjectCleanerTabNonEngine> TabNonEngine;
};
