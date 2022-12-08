// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectCleanerScanner.h"
#include "Widgets/SCompoundWidget.h"

class UProjectCleanerScanSettings;
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

private:
	static bool WidgetEnabled();
	static int32 WidgetGetIndex();

	static void MenuBarFillTabs(FMenuBuilder& MenuBuilder, const TSharedPtr<FTabManager> TabManagerPtr);
	void MenuBarFillSettings(FMenuBuilder& MenuBuilder, const TSharedPtr<FTabManager> TabManagerPtr) const;
	static void MenuBarFillHelp(FMenuBuilder& MenuBuilder, const TSharedPtr<FTabManager> TabManagerPtr);

	TSharedRef<SDockTab> OnTabSpawnScanSettings(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> OnTabSpawnUnusedAssets(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> OnTabSpawnIndirectAssets(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> OnTabSpawnCorruptedAssets(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> OnTabSpawnNonEngineFiles(const FSpawnTabArgs& Args);

	bool TabsEnabled() const;

	TSharedPtr<FTabManager> TabManager;
	TSharedPtr<FTabManager::FLayout> TabLayout;
	TSharedPtr<FProjectCleanerScanner> Scanner;

	TWeakObjectPtr<UProjectCleanerScanSettings> ScanSettings;
	TWeakObjectPtr<UProjectCleanerExcludeSettings> ExcludeSettings;

	TWeakPtr<SProjectCleanerTabScanSettings> TabScanSettings;
	TWeakPtr<SProjectCleanerTabUnused> TabUnused;
	TWeakPtr<SProjectCleanerTabIndirect> TabIndirect;
	TWeakPtr<SProjectCleanerTabCorrupted> TabCorrupted;
	TWeakPtr<SProjectCleanerTabNonEngine> TabNonEngine;

	float TabsRenderOpacity = 1.0f;
	bool bTabsEnabled = false;
};


