// // Copyright Ashot Barkhudaryan. All Rights Reserved.
//
// #include "PjcFrontendFilters.h"
// #include "PjcSettings.h"
// #include "PjcSubsystem.h"
//
//
// FPjcFilterAssetsPrimary::FPjcFilterAssetsPrimary(TSharedPtr<FFrontendFilterCategory> InCategory) : FFrontendFilter(InCategory) {}
//
// FString FPjcFilterAssetsPrimary::GetName() const
// {
// 	return TEXT("Primary Assets");
// }
//
// FText FPjcFilterAssetsPrimary::GetDisplayName() const
// {
// 	return FText::FromString(TEXT("Primary Assets"));
// }
//
// FText FPjcFilterAssetsPrimary::GetToolTipText() const
// {
// 	return FText::FromString(TEXT("Show primary or derived from primary assets"));
// }
//
// FLinearColor FPjcFilterAssetsPrimary::GetColor() const
// {
// 	return FLinearColor::Blue;
// }
//
// void FPjcFilterAssetsPrimary::ActiveStateChanged(bool bActive)
// {
// 	FFrontendFilter::ActiveStateChanged(bActive);
//
// 	if (DelegateFilterChanged.IsBound())
// 	{
// 		DelegateFilterChanged.Broadcast(bActive);
// 	}
//
// 	if (bActive)
// 	{
// 		// UPjcSubsystem::GetAssetsPrimary(Assets);
// 	}
// }
//
// bool FPjcFilterAssetsPrimary::PassesFilter(const FContentBrowserItem& InItem) const
// {
// 	FAssetData AssetData;
// 	if (!InItem.Legacy_TryGetAssetData(AssetData)) return false;
//
// 	return Assets.Contains(AssetData);
// }
//
// FPjcDelegateFilterChanged& FPjcFilterAssetsPrimary::OnFilterChange()
// {
// 	return DelegateFilterChanged;
// }
//
// FPjcFilterAssetsExcluded::FPjcFilterAssetsExcluded(TSharedPtr<FFrontendFilterCategory> InCategory) : FFrontendFilter(InCategory) {}
//
// FString FPjcFilterAssetsExcluded::GetName() const
// {
// 	return TEXT("Excluded Assets");
// }
//
// FText FPjcFilterAssetsExcluded::GetDisplayName() const
// {
// 	return FText::FromString(TEXT("Excluded Assets"));
// }
//
// FText FPjcFilterAssetsExcluded::GetToolTipText() const
// {
// 	return FText::FromString(TEXT("Show excluded assets"));
// }
//
// FLinearColor FPjcFilterAssetsExcluded::GetColor() const
// {
// 	return FLinearColor{FColor::Orange};
// }
//
// void FPjcFilterAssetsExcluded::ActiveStateChanged(bool bActive)
// {
// 	FFrontendFilter::ActiveStateChanged(bActive);
//
// 	if (DelegateFilterChanged.IsBound())
// 	{
// 		DelegateFilterChanged.Broadcast(bActive);
// 	}
//
// 	if (bActive)
// 	{
// 		// UPjcSubsystem::GetAssetsExcluded(Assets);
// 	}
// }
//
// bool FPjcFilterAssetsExcluded::PassesFilter(const FContentBrowserItem& InItem) const
// {
// 	FAssetData AssetData;
// 	if (!InItem.Legacy_TryGetAssetData(AssetData)) return false;
//
// 	return Assets.Contains(AssetData);
// }
//
// FPjcDelegateFilterChanged& FPjcFilterAssetsExcluded::OnFilterChange()
// {
// 	return DelegateFilterChanged;
// }
//
// FPjcFilterAssetsIndirect::FPjcFilterAssetsIndirect(TSharedPtr<FFrontendFilterCategory> InCategory) : FFrontendFilter(InCategory) {}
//
// FString FPjcFilterAssetsIndirect::GetName() const
// {
// 	return TEXT("Indirect Assets");
// }
//
// FText FPjcFilterAssetsIndirect::GetDisplayName() const
// {
// 	return FText::FromString(TEXT("Indirect Assets"));
// }
//
// FText FPjcFilterAssetsIndirect::GetToolTipText() const
// {
// 	return FText::FromString(TEXT("Show assets that used in source code or config files indirectly"));
// }
//
// FLinearColor FPjcFilterAssetsIndirect::GetColor() const
// {
// 	return FLinearColor{FColor::Magenta};
// }
//
// void FPjcFilterAssetsIndirect::ActiveStateChanged(bool bActive)
// {
// 	FFrontendFilter::ActiveStateChanged(bActive);
//
// 	if (DelegateFilterChanged.IsBound())
// 	{
// 		DelegateFilterChanged.Broadcast(bActive);
// 	}
//
// 	if (bActive)
// 	{
// 		// TArray<FPjcAssetIndirectUsageInfo> Infos;
// 		// UPjcSubsystem::GetAssetsIndirectInfos(Infos);
// 		//
// 		// Assets.Empty();
// 		// Assets.Reserve(Infos.Num());
// 		//
// 		// for (const auto& Info : Infos)
// 		// {
// 		// 	Assets.Add(Info.AssetData);
// 		// }
// 	}
// }
//
// bool FPjcFilterAssetsIndirect::PassesFilter(const FContentBrowserItem& InItem) const
// {
// 	FAssetData AssetData;
// 	if (!InItem.Legacy_TryGetAssetData(AssetData)) return false;
//
// 	return Assets.Contains(AssetData);
// }
//
// FPjcDelegateFilterChanged& FPjcFilterAssetsIndirect::OnFilterChange()
// {
// 	return DelegateFilterChanged;
// }
//
// FPjcFilterAssetsExtReferenced::FPjcFilterAssetsExtReferenced(TSharedPtr<FFrontendFilterCategory> InCategory) : FFrontendFilter(InCategory) {}
//
// FString FPjcFilterAssetsExtReferenced::GetName() const
// {
// 	return TEXT("Assets Ext Referenced");
// }
//
// FText FPjcFilterAssetsExtReferenced::GetDisplayName() const
// {
// 	return FText::FromString(TEXT("Assets Ext Referenced"));
// }
//
// FText FPjcFilterAssetsExtReferenced::GetToolTipText() const
// {
// 	return FText::FromString(TEXT("Show assets that have referencers outside Content folder"));
// }
//
// FLinearColor FPjcFilterAssetsExtReferenced::GetColor() const
// {
// 	return FLinearColor{FColor::Purple};
// }
//
// void FPjcFilterAssetsExtReferenced::ActiveStateChanged(bool bActive)
// {
// 	FFrontendFilter::ActiveStateChanged(bActive);
//
// 	if (DelegateFilterChanged.IsBound())
// 	{
// 		DelegateFilterChanged.Broadcast(bActive);
// 	}
// }
//
// bool FPjcFilterAssetsExtReferenced::PassesFilter(const FContentBrowserItem& InItem) const
// {
// 	FAssetData AssetData;
// 	if (!InItem.Legacy_TryGetAssetData(AssetData)) return false;
//
// 	// return UPjcSubsystem::AssetIsExtReferenced(AssetData);
// 	return false;
// }
//
// FPjcDelegateFilterChanged& FPjcFilterAssetsExtReferenced::OnFilterChange()
// {
// 	return DelegateFilterChanged;
// }
//
// FPjcFilterAssetsUsed::FPjcFilterAssetsUsed(TSharedPtr<FFrontendFilterCategory> InCategory) : FFrontendFilter(InCategory) {}
//
// FString FPjcFilterAssetsUsed::GetName() const
// {
// 	return TEXT("Assets Used");
// }
//
// FText FPjcFilterAssetsUsed::GetDisplayName() const
// {
// 	return FText::FromString(TEXT("Assets Used"));
// }
//
// FText FPjcFilterAssetsUsed::GetToolTipText() const
// {
// 	return FText::FromString(TEXT("Show all used assets"));
// }
//
// FLinearColor FPjcFilterAssetsUsed::GetColor() const
// {
// 	return FLinearColor::Green;
// }
//
// void FPjcFilterAssetsUsed::ActiveStateChanged(bool bActive)
// {
// 	FFrontendFilter::ActiveStateChanged(bActive);
//
// 	if (DelegateFilterChanged.IsBound())
// 	{
// 		DelegateFilterChanged.Broadcast(bActive);
// 	}
//
// 	if (bActive)
// 	{
// 		// UPjcSubsystem::GetAssetsUsed(Assets);
// 	}
// }
//
// bool FPjcFilterAssetsUsed::PassesFilter(const FContentBrowserItem& InItem) const
// {
// 	FAssetData AssetData;
// 	if (!InItem.Legacy_TryGetAssetData(AssetData)) return false;
//
// 	return Assets.Contains(AssetData);
// }
//
// FPjcDelegateFilterChanged& FPjcFilterAssetsUsed::OnFilterChange()
// {
// 	return DelegateFilterChanged;
// }
