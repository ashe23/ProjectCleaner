// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "FrontendFilters/PjcFrontendFilterAssetsPrimary.h"
#include "PjcSubsystem.h"

FPjcFilterAssetsPrimary::FPjcFilterAssetsPrimary(TSharedPtr<FFrontendFilterCategory> InCategory) : FFrontendFilter(InCategory) {}

FString FPjcFilterAssetsPrimary::GetName() const
{
	return TEXT("Primary Assets");
}

FText FPjcFilterAssetsPrimary::GetDisplayName() const
{
	return FText::FromString(TEXT("Primary Assets"));
}

FText FPjcFilterAssetsPrimary::GetToolTipText() const
{
	return FText::FromString(TEXT("Show primary or derived from primary assets"));
}

FLinearColor FPjcFilterAssetsPrimary::GetColor() const
{
	return FLinearColor{FColor::FromHex(TEXT("#F94144"))};
}

void FPjcFilterAssetsPrimary::ActiveStateChanged(bool bActive)
{
	FFrontendFilter::ActiveStateChanged(bActive);

	if (DelegateFilterChanged.IsBound())
	{
		DelegateFilterChanged.Broadcast(bActive);
	}

	if (bActive)
	{
		const FPjcScanDataAssets& ScanDataAssets = GEditor->GetEditorSubsystem<UPjcSubsystem>()->GetLastScanDataAssets();

		Assets.Empty(ScanDataAssets.AssetsPrimary.Num());
		Assets.Append(ScanDataAssets.AssetsPrimary);
	}
}

bool FPjcFilterAssetsPrimary::PassesFilter(const FContentBrowserItem& InItem) const
{
	FAssetData AssetData;
	if (!InItem.Legacy_TryGetAssetData(AssetData)) return false;

	return Assets.Contains(AssetData);
}

FPjcDelegateFilterChanged& FPjcFilterAssetsPrimary::OnFilterChanged()
{
	return DelegateFilterChanged;
}
