// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "PjcFrontendFilters.h"

FPjcFilterAssetsUsed::FPjcFilterAssetsUsed(TSharedPtr<FFrontendFilterCategory> InCategory) : FFrontendFilter(InCategory) {}

FString FPjcFilterAssetsUsed::GetName() const
{
	return TEXT("Assets Used");
}

FText FPjcFilterAssetsUsed::GetDisplayName() const
{
	return FText::FromString(TEXT("Assets Used"));
}

FText FPjcFilterAssetsUsed::GetToolTipText() const
{
	return FText::FromString(TEXT("Show assets that considered used."));
}

FLinearColor FPjcFilterAssetsUsed::GetColor() const
{
	return FLinearColor::Green; // todo:ashe23 change color later
}

void FPjcFilterAssetsUsed::ActiveStateChanged(bool bActive)
{
	FFrontendFilter::ActiveStateChanged(bActive);

	if (DelegateFilterChanged.IsBound())
	{
		DelegateFilterChanged.Broadcast(bActive);
	}
	// todo:ashe23 query used assets here 
}

bool FPjcFilterAssetsUsed::PassesFilter(const FContentBrowserItem& InItem) const
{
	FAssetData AssetData;
	if (InItem.Legacy_TryGetAssetData(AssetData)) return false;

	return Assets.Contains(AssetData);
}

FPjcDelegateFilterChanged& FPjcFilterAssetsUsed::OnFilterChanged()
{
	return DelegateFilterChanged;
}
