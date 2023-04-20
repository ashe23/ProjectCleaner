// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class SPjcMainWindow final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SPjcMainWindow) { }

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<SDockTab>& ConstructUnderMajorTab, const TSharedPtr<SWindow>& ConstructUnderWindow);
	virtual ~SPjcMainWindow() override;

private:
	void CreateMenuBarTabs(FMenuBuilder& MenuBuilder, const TSharedPtr<FTabManager> TabManagerPtr);

	TSharedRef<SDockTab> OnTabAssetsBrowserSpawn(const FSpawnTabArgs& Args) const;
	TSharedRef<SDockTab> OnTabFilesBrowserSpawn(const FSpawnTabArgs& Args) const;

	TSharedPtr<FTabManager> TabManager;
	TSharedPtr<FTabManager::FLayout> TabLayout;
};
