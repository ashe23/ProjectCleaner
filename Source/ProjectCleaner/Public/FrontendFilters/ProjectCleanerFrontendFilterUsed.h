// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectCleanerDelegates.h"
#include "FrontendFilterBase.h"

class FFrontendFilterUsedAssets final : public FFrontendFilter
{
public:
	explicit FFrontendFilterUsedAssets(TSharedPtr<FFrontendFilterCategory> InCategory);
	virtual FString GetName() const override;
	virtual FText GetDisplayName() const override;
	virtual FText GetToolTipText() const override;
	virtual FLinearColor GetColor() const override;
	virtual void ActiveStateChanged(bool bActive) override;
	virtual bool PassesFilter(const FContentBrowserItem& InItem) const override;

	FProjectCleanerDelegateFilterChanged& OnFilterChange();

private:
	FProjectCleanerDelegateFilterChanged DelegateFilterChanged;
};
