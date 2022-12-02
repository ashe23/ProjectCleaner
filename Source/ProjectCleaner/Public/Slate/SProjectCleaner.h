// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectCleanerScanner.h"
#include "IContentBrowserSingleton.h"
#include "Widgets/SCompoundWidget.h"

class UProjectCleanerScanSettings;
class SProjectCleanerTreeView;
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
	void UpdateView() const;
	virtual ~SProjectCleaner() override;
private:
	static bool WidgetEnabled();
	static int32 WidgetGetIndex();

	static void MenuBarFillTabs(FMenuBuilder& MenuBuilder, const TSharedPtr<FTabManager> TabManager);
	static void MenuBarFillHelp(FMenuBuilder& MenuBuilder, const TSharedPtr<FTabManager> TabManager);

	FReply OnBtnScanProjectClick();
	FReply OnBtnCleanProjectClick() const;
	FReply OnBtnDeleteEmptyFoldersClick() const;

	TSharedRef<SDockTab> OnTabSpawnScanSettings(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> OnTabSpawnUnusedAssets(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> OnTabSpawnIndirectAssets(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> OnTabSpawnCorruptedAssets(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> OnTabSpawnNonEngineFiles(const FSpawnTabArgs& Args);
	
	TSharedPtr<FTabManager> TabManager;
	TSharedPtr<FTabManager::FLayout> TabLayout;
	TSharedPtr<FProjectCleanerScanner> Scanner;
	TSharedPtr<IDetailsView> ScanSettingsProperty;
	
	TWeakObjectPtr<UProjectCleanerScanSettings> ScanSettings;
	
	TWeakPtr<SProjectCleanerTabUnused> TabUnused;
	TWeakPtr<SProjectCleanerTabIndirect> TabIndirect;
	TWeakPtr<SProjectCleanerFileListView> TabCorrupted;
	TWeakPtr<SProjectCleanerFileListView> TabNonEngine;
};
