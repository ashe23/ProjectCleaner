// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "FrontendFilters/PjcFrontendFilterAssetsIndirect.h"
#include "PjcSubsystem.h"

FPjcFilterAssetsIndirect::FPjcFilterAssetsIndirect(TSharedPtr<FFrontendFilterCategory> InCategory) : FFrontendFilter(InCategory) {}

FString FPjcFilterAssetsIndirect::GetName() const
{
	return TEXT("Indirect Assets");
}

FText FPjcFilterAssetsIndirect::GetDisplayName() const
{
	return FText::FromString(TEXT("Indirect Assets"));
}

FText FPjcFilterAssetsIndirect::GetToolTipText() const
{
	return FText::FromString(TEXT("Show indirectly used assets"));
}

FLinearColor FPjcFilterAssetsIndirect::GetColor() const
{
	return FLinearColor{FColor::FromHex(TEXT("#577590"))};
}

void FPjcFilterAssetsIndirect::ActiveStateChanged(bool bActive)
{
	FFrontendFilter::ActiveStateChanged(bActive);

	if (DelegateFilterChanged.IsBound())
	{
		DelegateFilterChanged.Broadcast(bActive);
	}

	if (bActive)
	{
		const FPjcScanDataAssets& ScanDataAssets = GEditor->GetEditorSubsystem<UPjcSubsystem>()->GetLastScanDataAssets();

		TArray<FAssetData> AssetsIndirect;
		ScanDataAssets.AssetsIndirect.GetKeys(AssetsIndirect);

		Assets.Empty(AssetsIndirect.Num());
		Assets.Append(AssetsIndirect);
	}
}

bool FPjcFilterAssetsIndirect::PassesFilter(const FContentBrowserItem& InItem) const
{
	FAssetData AssetData;
	if (!InItem.Legacy_TryGetAssetData(AssetData)) return false;

	return Assets.Contains(AssetData);
}

FPjcDelegateFilterChanged& FPjcFilterAssetsIndirect::OnFilterChanged()
{
	return DelegateFilterChanged;
}
