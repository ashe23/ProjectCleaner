// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectCleanerScanner.h"
#include "Widgets/SCompoundWidget.h"

class UProjectCleanerScanSettings;

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
	static bool IsWidgetEnabled();
	static int32 GetWidgetIndex();

	static void MenuBarFillTabs(FMenuBuilder& MenuBuilder, const TSharedPtr<FTabManager> TabManager);
	static void MenuBarFillHelp(FMenuBuilder& MenuBuilder, const TSharedPtr<FTabManager> TabManager);

	TSharedRef<SDockTab> OnTabSpawnScanSettings(const FSpawnTabArgs& Args) const;
	TSharedRef<SDockTab> OnTabSpawnUnusedAssets(const FSpawnTabArgs& Args) const;
	TSharedRef<SDockTab> OnTabSpawnIndirectAssets(const FSpawnTabArgs& Args) const;
	TSharedRef<SDockTab> OnTabSpawnCorruptedAssets(const FSpawnTabArgs& Args) const;
	TSharedRef<SDockTab> OnTabSpawnNonEngineFiles(const FSpawnTabArgs& Args) const;
	TSharedPtr<FTabManager> TabManager;
	TSharedPtr<FTabManager::FLayout> TabLayout;

	// TSharedPtr<FUICommandList> Cmds;
	TWeakObjectPtr<UProjectCleanerScanSettings> ScanSettings;
	TSharedPtr<IDetailsView> ScanSettingsProperty;
};
