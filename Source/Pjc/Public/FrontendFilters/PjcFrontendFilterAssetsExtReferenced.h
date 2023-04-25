// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FrontendFilterBase.h"
#include "PjcDelegates.h"

class FPjcFilterAssetsExtReferenced final : public FFrontendFilter
{
public:
	explicit FPjcFilterAssetsExtReferenced(TSharedPtr<FFrontendFilterCategory> InCategory);
	virtual FString GetName() const override;
	virtual FText GetDisplayName() const override;
	virtual FText GetToolTipText() const override;
	virtual FLinearColor GetColor() const override;
	virtual void ActiveStateChanged(bool bActive) override;
	virtual bool PassesFilter(const FContentBrowserItem& InItem) const override;
	FPjcDelegateFilterChanged& OnFilterChanged();

private:
	FPjcDelegateFilterChanged DelegateFilterChanged;
	TSet<FAssetData> Assets;
};
