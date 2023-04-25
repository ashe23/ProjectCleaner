// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "FrontendFilters/PjcFrontendFilterAssetsExcluded.h"
#include "PjcSubsystem.h"

FPjcFilterAssetsExcluded::FPjcFilterAssetsExcluded(TSharedPtr<FFrontendFilterCategory> InCategory) : FFrontendFilter(InCategory) {}

FString FPjcFilterAssetsExcluded::GetName() const
{
	return TEXT("Excluded Assets");
}

FText FPjcFilterAssetsExcluded::GetDisplayName() const
{
	return FText::FromString(TEXT("Excluded Assets"));
}

FText FPjcFilterAssetsExcluded::GetToolTipText() const
{
	return FText::FromString(TEXT("Show excluded assets"));
}

FLinearColor FPjcFilterAssetsExcluded::GetColor() const
{
	return FLinearColor{FColor::FromHex(TEXT("#F9C74F"))};
}

void FPjcFilterAssetsExcluded::ActiveStateChanged(bool bActive)
{
	FFrontendFilter::ActiveStateChanged(bActive);

	if (DelegateFilterChanged.IsBound())
	{
		DelegateFilterChanged.Broadcast(bActive);
	}

	if (bActive)
	{
		const FPjcScanDataAssets& ScanDataAssets = GEditor->GetEditorSubsystem<UPjcSubsystem>()->GetLastScanDataAssets();

		Assets.Empty(ScanDataAssets.AssetsExcluded.Num());
		Assets.Append(ScanDataAssets.AssetsExcluded);
	}
}

bool FPjcFilterAssetsExcluded::PassesFilter(const FContentBrowserItem& InItem) const
{
	FAssetData AssetData;
	if (!InItem.Legacy_TryGetAssetData(AssetData)) return false;

	return Assets.Contains(AssetData);
}

FPjcDelegateFilterChanged& FPjcFilterAssetsExcluded::OnFilterChanged()
{
	return DelegateFilterChanged;
}
