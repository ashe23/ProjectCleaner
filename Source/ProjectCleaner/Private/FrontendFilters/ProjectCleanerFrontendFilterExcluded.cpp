// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "FrontendFilters/ProjectCleanerFrontendFilterExcluded.h"
#include "ProjectCleanerSubsystem.h"

FFrontendFilterExcludedAssets::FFrontendFilterExcludedAssets(TSharedPtr<FFrontendFilterCategory> InCategory) : FFrontendFilter(InCategory)
{
}

FString FFrontendFilterExcludedAssets::GetName() const
{
	return TEXT("Excluded Assets");
}

FText FFrontendFilterExcludedAssets::GetDisplayName() const
{
	return FText::FromString(TEXT("Excluded Assets"));
}

FText FFrontendFilterExcludedAssets::GetToolTipText() const
{
	return FText::FromString(TEXT("Show excluded assets"));
}

FLinearColor FFrontendFilterExcludedAssets::GetColor() const
{
	return FLinearColor{FColor::FromHex(TEXT("#ffcb77"))};
}

void FFrontendFilterExcludedAssets::ActiveStateChanged(bool bActive)
{
	FFrontendFilter::ActiveStateChanged(bActive);

	if (DelegateFilterChanged.IsBound())
	{
		DelegateFilterChanged.Broadcast(bActive);
	}
}

bool FFrontendFilterExcludedAssets::PassesFilter(const FContentBrowserItem& InItem) const
{
	FAssetData ItemAssetData;
	if (InItem.Legacy_TryGetAssetData(ItemAssetData))
	{
		return GEditor->GetEditorSubsystem<UProjectCleanerSubsystem>()->GetScanData().AssetsExcluded.Contains(ItemAssetData);
	}

	return false;
}

FProjectCleanerDelegateFilterChanged& FFrontendFilterExcludedAssets::OnFilterChange()
{
	return DelegateFilterChanged;
}
