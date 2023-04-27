// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "FrontendFilters/PjcFrontendFilterAssetsExtReferenced.h"
#include "PjcSubsystem.h"

FPjcFilterAssetsExtReferenced::FPjcFilterAssetsExtReferenced(TSharedPtr<FFrontendFilterCategory> InCategory) : FFrontendFilter(InCategory) {}

FString FPjcFilterAssetsExtReferenced::GetName() const
{
	return TEXT("ExtReferenced Assets");
}

FText FPjcFilterAssetsExtReferenced::GetDisplayName() const
{
	return FText::FromString(TEXT("ExtReferenced Assets"));
}

FText FPjcFilterAssetsExtReferenced::GetToolTipText() const
{
	return FText::FromString(TEXT("Show assets that have external referencers outside 'Content' folder"));
}

FLinearColor FPjcFilterAssetsExtReferenced::GetColor() const
{
	return FLinearColor{FColor::FromHex(TEXT("#F3722C"))};
}

void FPjcFilterAssetsExtReferenced::ActiveStateChanged(bool bActive)
{
	FFrontendFilter::ActiveStateChanged(bActive);

	if (DelegateFilterChanged.IsBound())
	{
		DelegateFilterChanged.Broadcast(bActive);
	}

	if (bActive)
	{
		const TArray<FAssetData>& AssetExtReferenced = GEditor->GetEditorSubsystem<UPjcSubsystem>()->GetAssetsExtReferenced();
		Assets.Empty(AssetExtReferenced.Num());
		Assets.Append(AssetExtReferenced);
	}
}

bool FPjcFilterAssetsExtReferenced::PassesFilter(const FContentBrowserItem& InItem) const
{
	FAssetData AssetData;
	if (!InItem.Legacy_TryGetAssetData(AssetData)) return false;

	return Assets.Contains(AssetData);
}

FPjcDelegateFilterChanged& FPjcFilterAssetsExtReferenced::OnFilterChanged()
{
	return DelegateFilterChanged;
}
