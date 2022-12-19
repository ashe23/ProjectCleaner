// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "FrontendFilters/ProjectCleanerFrontendFilterIndirect.h"
#include "ProjectCleanerSubsystem.h"

FFrontendFilterIndirectAssets::FFrontendFilterIndirectAssets(TSharedPtr<FFrontendFilterCategory> InCategory) : FFrontendFilter(InCategory)
{
}

FString FFrontendFilterIndirectAssets::GetName() const
{
	return TEXT("Indirect Assets");
}

FText FFrontendFilterIndirectAssets::GetDisplayName() const
{
	return FText::FromString(TEXT("Indirect Assets"));
}

FText FFrontendFilterIndirectAssets::GetToolTipText() const
{
	return FText::FromString(TEXT("Show indirects assets. Assets that used in source code files, config files or other files indirectly."));
}

FLinearColor FFrontendFilterIndirectAssets::GetColor() const
{
	return FLinearColor::Blue;
}

void FFrontendFilterIndirectAssets::ActiveStateChanged(bool bActive)
{
	FFrontendFilter::ActiveStateChanged(bActive);

	if (DelegateFilterChanged.IsBound())
	{
		DelegateFilterChanged.Broadcast(bActive);
	}
}

bool FFrontendFilterIndirectAssets::PassesFilter(const FContentBrowserItem& InItem) const
{
	FAssetData ItemAssetData;
	if (InItem.Legacy_TryGetAssetData(ItemAssetData))
	{
		return GEditor->GetEditorSubsystem<UProjectCleanerSubsystem>()->GetScanData().AssetsIndirect.Contains(ItemAssetData);
	}

	return false;
}

FProjectCleanerDelegateFilterChanged& FFrontendFilterIndirectAssets::OnFilterChange()
{
	return DelegateFilterChanged;
}
