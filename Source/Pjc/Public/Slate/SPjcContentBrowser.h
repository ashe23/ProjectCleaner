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

protected:
	FText GetSummaryText() const;
	TSharedRef<SWidget> CreateToolbar() const;
	TSharedRef<SWidget> GetBtnOptionsContent();
	void CreateContentBrowser();
	FSlateColor GetOptionsBtnForegroundColor() const;

private:
	bool bUnusedAssetsMode = true;
	FARFilter Filter;
	UPjcSubsystem* SubsystemPtr = nullptr;
	TSharedPtr<FUICommandList> Cmds;
	TSharedPtr<SComboButton> OptionBtn;
	TSharedPtr<SWidget> ContentBrowserPtr;
	FSetARFilterDelegate DelegateFilter;
	FRefreshAssetViewDelegate DelegateRefreshView;
	FGetCurrentSelectionDelegate DelegateSelection;
};
