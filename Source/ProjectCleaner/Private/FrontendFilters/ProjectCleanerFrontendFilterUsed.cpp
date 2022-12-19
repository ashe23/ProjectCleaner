// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "FrontendFilters/ProjectCleanerFrontendFilterUsed.h"
#include "ProjectCleanerSubsystem.h"

FFrontendFilterUsedAssets::FFrontendFilterUsedAssets(TSharedPtr<FFrontendFilterCategory> InCategory) : FFrontendFilter(InCategory)
{
}

FString FFrontendFilterUsedAssets::GetName() const
{
	return TEXT("Used Assets");
}

FText FFrontendFilterUsedAssets::GetDisplayName() const
{
	return FText::FromString(TEXT("Used Assets"));
}

FText FFrontendFilterUsedAssets::GetToolTipText() const
{
	return FText::FromString(TEXT("Show used assets"));
}

FLinearColor FFrontendFilterUsedAssets::GetColor() const
{
	return FLinearColor::Green;
}

void FFrontendFilterUsedAssets::ActiveStateChanged(bool bActive)
{
	FFrontendFilter::ActiveStateChanged(bActive);

	if (DelegateFilterChanged.IsBound())
	{
		DelegateFilterChanged.Broadcast(bActive);
	}
}

bool FFrontendFilterUsedAssets::PassesFilter(const FContentBrowserItem& InItem) const
{
	FAssetData ItemAssetData;
	if (InItem.Legacy_TryGetAssetData(ItemAssetData))
	{
		return GEditor->GetEditorSubsystem<UProjectCleanerSubsystem>()->GetScanData().AssetsUsed.Contains(ItemAssetData);
	}

	return false;
}

FProjectCleanerDelegateFilterChanged& FFrontendFilterUsedAssets::OnFilterChange()
{
	return DelegateFilterChanged;
}
