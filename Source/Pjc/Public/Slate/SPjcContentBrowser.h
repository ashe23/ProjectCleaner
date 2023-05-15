// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ContentBrowserDelegates.h"
#include "Widgets/SCompoundWidget.h"

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
	TSharedRef<SWidget> GetBtnActionsContent();
	TSharedRef<SWidget> GetBtnOptionsContent();
	FSlateColor GetOptionsBtnForegroundColor() const;

private:
	FARFilter Filter;
	TSharedPtr<SComboButton> OptionBtn;
	FSetARFilterDelegate DelegateFilter;
	FRefreshAssetViewDelegate DelegateRefreshView;
	FGetCurrentSelectionDelegate DelegateSelection;
};
