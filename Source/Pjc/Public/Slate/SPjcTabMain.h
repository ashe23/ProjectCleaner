// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class SPjcTabMain final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SPjcTabMain) { }

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<SDockTab>& ConstructUnderMajorTab, const TSharedPtr<SWindow>& ConstructUnderWindow);
	virtual ~SPjcTabMain() override;

private:
	int32 GetWidgetIndex() const;
	FText GetWidgetWarningText() const;
	TSharedRef<SDockTab> OnTabAssetsUnusedSpawn(const FSpawnTabArgs& Args) const;
	TSharedRef<SDockTab> OnTabAssetsIndirectSpawn(const FSpawnTabArgs& Args) const;
	TSharedRef<SDockTab> OnTabAssetsCorruptedSpawn(const FSpawnTabArgs& Args) const;
	TSharedRef<SDockTab> OnTabFilesExternalSpawn(const FSpawnTabArgs& Args) const;
	void CreateMenuBarTabs(FMenuBuilder& MenuBuilder, const TSharedPtr<FTabManager> TabManagerPtr);
	void CreateMenuBarHelp(FMenuBuilder& MenuBuilder, const TSharedPtr<FTabManager> TabManagerPtr);

	TSharedPtr<FUICommandList> Cmds;
	TSharedPtr<FTabManager> TabManager;
	TSharedPtr<FTabManager::FLayout> TabLayout;
};
