// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Framework/Commands/UICommandList.h"
#include "Framework/Docking/TabManager.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/SWindow.h"

class SPjcTabMain final : public SCompoundWidget
{
  public:

	SLATE_BEGIN_ARGS(SPjcTabMain) {}

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<SDockTab>& ConstructUnderMajorTab, const TSharedPtr<SWindow>& ConstructUnderWindow);
	virtual ~SPjcTabMain() override;

  private:

	int32 GetWidgetIndex() const;
	FText GetWidgetWarningText() const;
	TSharedRef<SDockTab> OnTabAssetsUnusedSpawn(const FSpawnTabArgs& Args) const;
	TSharedRef<SDockTab> OnTabIssuesSpawn(const FSpawnTabArgs& Args) const;
	TSharedRef<SDockTab> OnTabInspectorSpawn(const FSpawnTabArgs& Args) const;
	TSharedRef<SDockTab> OnTabHelpSpawn(const FSpawnTabArgs& Args) const;
	void CreateMenuBarTabs(FMenuBuilder& MenuBuilder, const TSharedPtr<FTabManager> TabManagerPtr);
	void CreateMenuBarSettings(FMenuBuilder& MenuBuilder);

	TSharedPtr<FUICommandList> Cmds;
	TSharedPtr<FTabManager> TabManager;
	TSharedPtr<FTabManager::FLayout> TabLayout;
};
