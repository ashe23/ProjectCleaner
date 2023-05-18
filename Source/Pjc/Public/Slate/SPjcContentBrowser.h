// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ContentBrowserDelegates.h"
#include "Widgets/SCompoundWidget.h"

class UPjcSubsystem;
enum class EPjcAssetCategory : uint8;

class SPjcContentBrowser final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SPjcContentBrowser) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	void FilterUpdate(const FARFilter& InFilter);
	void UpdateView(); // todo:ashe23 testing
protected:
	FText GetSummaryText() const;
	TSharedRef<SWidget> CreateToolbar() const;
	TSharedRef<SWidget> GetBtnOptionsContent();
	void CreateContentBrowser();
	void MakeSubmenu(FMenuBuilder& MenuBuilder);
	FSlateColor GetOptionsBtnForegroundColor() const;

private:
	bool bUnusedAssetsMode = true;
	float ThumbnailSize = 0.1f;
	FARFilter Filter;
	UPjcSubsystem* SubsystemPtr = nullptr;
	TSharedPtr<FUICommandList> Cmds;
	TSharedPtr<SComboButton> OptionBtn;
	TSharedPtr<SWidget> ContentBrowserPtr;
	FSetARFilterDelegate DelegateFilter;
	FRefreshAssetViewDelegate DelegateRefreshView;
	FGetCurrentSelectionDelegate DelegateSelection;
};
