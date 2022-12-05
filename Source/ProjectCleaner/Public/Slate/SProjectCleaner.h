// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectCleanerScanner.h"
#include "Widgets/SCompoundWidget.h"

class UProjectCleanerScanSettings;
class SProjectCleanerTreeView;
class SProjectCleanerTabScanSettings;
class SProjectCleanerTabCorrupted;
class SProjectCleanerTabNonEngine;
class SProjectCleanerTabUnused;
class SProjectCleanerTabIndirect;
class SProjectCleanerFileListView;

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

	static void MenuBarFillTabs(FMenuBuilder& MenuBuilder, const TSharedPtr<FTabManager> TabManager);
	static void MenuBarFillHelp(FMenuBuilder& MenuBuilder, const TSharedPtr<FTabManager> TabManager);

	TSharedRef<SDockTab> OnTabSpawnScanSettings(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> OnTabSpawnUnusedAssets(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> OnTabSpawnIndirectAssets(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> OnTabSpawnCorruptedAssets(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> OnTabSpawnNonEngineFiles(const FSpawnTabArgs& Args);

	TSharedPtr<FTabManager> TabManager;
	TSharedPtr<FTabManager::FLayout> TabLayout;
	TSharedPtr<FProjectCleanerScanner> Scanner;

	TWeakObjectPtr<UProjectCleanerScanSettings> ScanSettings;

	TWeakPtr<SProjectCleanerTabScanSettings> TabScanSettings;
	TWeakPtr<SProjectCleanerTabUnused> TabUnused;
	TWeakPtr<SProjectCleanerTabIndirect> TabIndirect;
	TWeakPtr<SProjectCleanerTabCorrupted> TabCorrupted;
	TWeakPtr<SProjectCleanerTabNonEngine> TabNonEngine;
};
