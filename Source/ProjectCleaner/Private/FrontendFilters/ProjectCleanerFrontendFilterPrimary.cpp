// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "FrontendFilters/ProjectCleanerFrontendFilterPrimary.h"
#include "ProjectCleanerSubsystem.h"

FFrontendFilterPrimaryAssets::FFrontendFilterPrimaryAssets(TSharedPtr<FFrontendFilterCategory> InCategory) : FFrontendFilter(InCategory)
{
}

FString FFrontendFilterPrimaryAssets::GetName() const
{
	return TEXT("Primary Assets");
}

FText FFrontendFilterPrimaryAssets::GetDisplayName() const
{
	return FText::FromString(TEXT("Primary Assets"));
}

FText FFrontendFilterPrimaryAssets::GetToolTipText() const
{
	return FText::FromString(TEXT("Show primary assets"));
}

FLinearColor FFrontendFilterPrimaryAssets::GetColor() const
{
	return FLinearColor::White;
}

void FFrontendFilterPrimaryAssets::ActiveStateChanged(bool bActive)
{
	FFrontendFilter::ActiveStateChanged(bActive);

	if (DelegateFilterChanged.IsBound())
	{
		DelegateFilterChanged.Broadcast(bActive);
	}
}

bool FFrontendFilterPrimaryAssets::PassesFilter(const FContentBrowserItem& InItem) const
{
	FAssetData ItemAssetData;
	if (InItem.Legacy_TryGetAssetData(ItemAssetData))
	{
		return GEditor->GetEditorSubsystem<UProjectCleanerSubsystem>()->GetScanData().AssetsPrimary.Contains(ItemAssetData);
	}

	return false;
}

FProjectCleanerDelegateFilterChanged& FFrontendFilterPrimaryAssets::OnFilterChange()
{
	return DelegateFilterChanged;
}
