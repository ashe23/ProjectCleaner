// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "FrontendFilters/PjcFrontendFilterAssetsUsed.h"
#include "PjcSubsystem.h"

FPjcFilterAssetsUsed::FPjcFilterAssetsUsed(TSharedPtr<FFrontendFilterCategory> InCategory) : FFrontendFilter(InCategory) {}

FString FPjcFilterAssetsUsed::GetName() const
{
	return TEXT("Used Assets");
}

FText FPjcFilterAssetsUsed::GetDisplayName() const
{
	return FText::FromString(TEXT("Used Assets"));
}

FText FPjcFilterAssetsUsed::GetToolTipText() const
{
	return FText::FromString(TEXT("Show used assets"));
}

FLinearColor FPjcFilterAssetsUsed::GetColor() const
{
	return FLinearColor{FColor::FromHex(TEXT("#29bf12"))};
}

void FPjcFilterAssetsUsed::ActiveStateChanged(bool bActive)
{
	FFrontendFilter::ActiveStateChanged(bActive);

	if (DelegateFilterChanged.IsBound())
	{
		DelegateFilterChanged.Broadcast(bActive);
	}

	if (bActive)
	{
		const TArray<FAssetData>& AssetsUsed = GEditor->GetEditorSubsystem<UPjcSubsystem>()->GetAssetsUsed();
		Assets.Empty(AssetsUsed.Num());
		Assets.Append(AssetsUsed);
	}
}

bool FPjcFilterAssetsUsed::PassesFilter(const FContentBrowserItem& InItem) const
{
	FAssetData AssetData;
	if (!InItem.Legacy_TryGetAssetData(AssetData)) return false;

	return Assets.Contains(AssetData);
}

FPjcDelegateFilterChanged& FPjcFilterAssetsUsed::OnFilterChanged()
{
	return DelegateFilterChanged;
}
