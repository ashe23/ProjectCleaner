// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "PjcFrontendFilters.h"
#include "PjcStyles.h"
#include "PjcSubsystem.h"

FPjcFilterAssetsUsed::FPjcFilterAssetsUsed(TSharedPtr<FFrontendFilterCategory> InCategory) : FFrontendFilter(InCategory) {}

FString FPjcFilterAssetsUsed::GetName() const
{
	return TEXT("Assets Used");
}

FText FPjcFilterAssetsUsed::GetDisplayName() const
{
	return FText::FromString(TEXT("Assets Used"));
}

FText FPjcFilterAssetsUsed::GetToolTipText() const
{
	return FText::FromString(TEXT("Show assets that considered used."));
}

FLinearColor FPjcFilterAssetsUsed::GetColor() const
{
	return FLinearColor::Green;
}

void FPjcFilterAssetsUsed::ActiveStateChanged(bool bActive)
{
	FFrontendFilter::ActiveStateChanged(bActive);

	if (bActive)
	{
		UpdateData();
	}

	if (DelegateFilterChanged.IsBound())
	{
		DelegateFilterChanged.Broadcast(bActive);
	}
}

bool FPjcFilterAssetsUsed::PassesFilter(const FContentBrowserItem& InItem) const
{
	FAssetData AssetData;
	if (!InItem.Legacy_TryGetAssetData(AssetData)) return false;

	return Assets.Contains(AssetData);
}

void FPjcFilterAssetsUsed::UpdateData()
{
	TArray<FAssetData> AssetsUsed;
	UPjcSubsystem::GetAssetsUsed(AssetsUsed, false);

	Assets.Reset();
	Assets.Append(AssetsUsed);
}

FPjcDelegateFilterChanged& FPjcFilterAssetsUsed::OnFilterChanged()
{
	return DelegateFilterChanged;
}

FPjcFilterAssetsPrimary::FPjcFilterAssetsPrimary(TSharedPtr<FFrontendFilterCategory> InCategory) : FFrontendFilter(InCategory) {}

FString FPjcFilterAssetsPrimary::GetName() const
{
	return TEXT("Assets Primary");
}

FText FPjcFilterAssetsPrimary::GetDisplayName() const
{
	return FText::FromString(TEXT("Assets Primary"));
}

FText FPjcFilterAssetsPrimary::GetToolTipText() const
{
	return FText::FromString(TEXT("Show primary assets."));
}

FLinearColor FPjcFilterAssetsPrimary::GetColor() const
{
	return FLinearColor::Red;
}

void FPjcFilterAssetsPrimary::ActiveStateChanged(bool bActive)
{
	FFrontendFilter::ActiveStateChanged(bActive);

	if (bActive)
	{
		UpdateData();
	}

	if (DelegateFilterChanged.IsBound())
	{
		DelegateFilterChanged.Broadcast(bActive);
	}
}

bool FPjcFilterAssetsPrimary::PassesFilter(const FContentBrowserItem& InItem) const
{
	FAssetData AssetData;
	if (!InItem.Legacy_TryGetAssetData(AssetData)) return false;

	return Assets.Contains(AssetData);
}

void FPjcFilterAssetsPrimary::UpdateData()
{
	TArray<FAssetData> AssetsPrimary;
	UPjcSubsystem::GetAssetsPrimary(AssetsPrimary, false);

	Assets.Reset();
	Assets.Append(AssetsPrimary);
}

FPjcDelegateFilterChanged& FPjcFilterAssetsPrimary::OnFilterChanged()
{
	return DelegateFilterChanged;
}

FPjcFilterAssetsIndirect::FPjcFilterAssetsIndirect(TSharedPtr<FFrontendFilterCategory> InCategory) : FFrontendFilter(InCategory) {}

FString FPjcFilterAssetsIndirect::GetName() const
{
	return TEXT("Assets Indirect");
}

FText FPjcFilterAssetsIndirect::GetDisplayName() const
{
	return FText::FromString(TEXT("Assets Indirect"));
}

FText FPjcFilterAssetsIndirect::GetToolTipText() const
{
	return FText::FromString(TEXT("Show assets used in source code or config files."));
}

FLinearColor FPjcFilterAssetsIndirect::GetColor() const
{
	return FLinearColor::White;
}

void FPjcFilterAssetsIndirect::ActiveStateChanged(bool bActive)
{
	FFrontendFilter::ActiveStateChanged(bActive);

	if (bActive)
	{
		UpdateData();
	}

	if (DelegateFilterChanged.IsBound())
	{
		DelegateFilterChanged.Broadcast(bActive);
	}
}

bool FPjcFilterAssetsIndirect::PassesFilter(const FContentBrowserItem& InItem) const
{
	FAssetData AssetData;
	if (!InItem.Legacy_TryGetAssetData(AssetData)) return false;

	return Assets.Contains(AssetData);
}

void FPjcFilterAssetsIndirect::UpdateData()
{
	TArray<FAssetData> AssetsIndirect;
	TArray<FPjcAssetIndirectInfo> AssetIndirectInfos;
	UPjcSubsystem::GetAssetsIndirect(AssetsIndirect, AssetIndirectInfos, false);

	Assets.Reset();
	Assets.Append(AssetsIndirect);
}

FPjcDelegateFilterChanged& FPjcFilterAssetsIndirect::OnFilterChanged()
{
	return DelegateFilterChanged;
}

FPjcFilterAssetsCircular::FPjcFilterAssetsCircular(TSharedPtr<FFrontendFilterCategory> InCategory) : FFrontendFilter(InCategory) {}

FString FPjcFilterAssetsCircular::GetName() const
{
	return TEXT("Assets Circular");
}

FText FPjcFilterAssetsCircular::GetDisplayName() const
{
	return FText::FromString(TEXT("Assets Circular"));
}

FText FPjcFilterAssetsCircular::GetToolTipText() const
{
	return FText::FromString(TEXT("Show assets with circular dependencies"));
}

FLinearColor FPjcFilterAssetsCircular::GetColor() const
{
	return FPjcStyles::Get().GetColor("ProjectCleaner.Color.DarkGray");
}

void FPjcFilterAssetsCircular::ActiveStateChanged(bool bActive)
{
	FFrontendFilter::ActiveStateChanged(bActive);

	if (bActive)
	{
		UpdateData();
	}

	if (DelegateFilterChanged.IsBound())
	{
		DelegateFilterChanged.Broadcast(bActive);
	}
}

bool FPjcFilterAssetsCircular::PassesFilter(const FContentBrowserItem& InItem) const
{
	FAssetData AssetData;
	if (!InItem.Legacy_TryGetAssetData(AssetData)) return false;

	return Assets.Contains(AssetData);
}

void FPjcFilterAssetsCircular::UpdateData()
{
	TArray<FAssetData> AssetsCircular;
	UPjcSubsystem::GetAssetsCircular(AssetsCircular, false);

	Assets.Reset();
	Assets.Append(AssetsCircular);
}

FPjcDelegateFilterChanged& FPjcFilterAssetsCircular::OnFilterChanged()
{
	return DelegateFilterChanged;
}

FPjcFilterAssetsEditor::FPjcFilterAssetsEditor(TSharedPtr<FFrontendFilterCategory> InCategory) : FFrontendFilter(InCategory) {}

FString FPjcFilterAssetsEditor::GetName() const
{
	return TEXT("Assets Editor");
}

FText FPjcFilterAssetsEditor::GetDisplayName() const
{
	return FText::FromString(TEXT("Assets Editor"));
}

FText FPjcFilterAssetsEditor::GetToolTipText() const
{
	return FText::FromString(TEXT("Show editor specific assets"));
}

FLinearColor FPjcFilterAssetsEditor::GetColor() const
{
	return FLinearColor::Blue;
}

void FPjcFilterAssetsEditor::ActiveStateChanged(bool bActive)
{
	FFrontendFilter::ActiveStateChanged(bActive);

	if (bActive)
	{
		UpdateData();
	}

	if (DelegateFilterChanged.IsBound())
	{
		DelegateFilterChanged.Broadcast(bActive);
	}
}

bool FPjcFilterAssetsEditor::PassesFilter(const FContentBrowserItem& InItem) const
{
	FAssetData AssetData;
	if (!InItem.Legacy_TryGetAssetData(AssetData)) return false;

	return Assets.Contains(AssetData);
}

void FPjcFilterAssetsEditor::UpdateData()
{
	TArray<FAssetData> AssetsEditor;
	UPjcSubsystem::GetAssetsEditor(AssetsEditor, false);

	Assets.Reset();
	Assets.Append(AssetsEditor);
}

FPjcDelegateFilterChanged& FPjcFilterAssetsEditor::OnFilterChanged()
{
	return DelegateFilterChanged;
}

FPjcFilterAssetsExcluded::FPjcFilterAssetsExcluded(TSharedPtr<FFrontendFilterCategory> InCategory) : FFrontendFilter(InCategory) {}

FString FPjcFilterAssetsExcluded::GetName() const
{
	return TEXT("Assets Excluded");
}

FText FPjcFilterAssetsExcluded::GetDisplayName() const
{
	return FText::FromString(TEXT("Assets Excluded"));
}

FText FPjcFilterAssetsExcluded::GetToolTipText() const
{
	return FText::FromString(TEXT("Show excluded assets."));
}

FLinearColor FPjcFilterAssetsExcluded::GetColor() const
{
	return FPjcStyles::Get().GetColor("ProjectCleaner.Color.Yellow");
}

void FPjcFilterAssetsExcluded::ActiveStateChanged(bool bActive)
{
	FFrontendFilter::ActiveStateChanged(bActive);

	if (bActive)
	{
		UpdateData();
	}

	if (DelegateFilterChanged.IsBound())
	{
		DelegateFilterChanged.Broadcast(bActive);
	}
}

bool FPjcFilterAssetsExcluded::PassesFilter(const FContentBrowserItem& InItem) const
{
	FAssetData AssetData;
	if (!InItem.Legacy_TryGetAssetData(AssetData)) return false;

	return Assets.Contains(AssetData);
}

void FPjcFilterAssetsExcluded::UpdateData()
{
	TArray<FAssetData> AssetExcluded;
	UPjcSubsystem::GetAssetsExcluded(AssetExcluded, false);

	Assets.Reset();
	Assets.Append(AssetExcluded);
}

FPjcDelegateFilterChanged& FPjcFilterAssetsExcluded::OnFilterChanged()
{
	return DelegateFilterChanged;
}

FPjcFilterAssetsExtReferenced::FPjcFilterAssetsExtReferenced(TSharedPtr<FFrontendFilterCategory> InCategory) : FFrontendFilter(InCategory) {}

FString FPjcFilterAssetsExtReferenced::GetName() const
{
	return TEXT("Assets ExtReferenced");
}

FText FPjcFilterAssetsExtReferenced::GetDisplayName() const
{
	return FText::FromString(TEXT("Assets ExtReferenced"));
}

FText FPjcFilterAssetsExtReferenced::GetToolTipText() const
{
	return FText::FromString(TEXT("Show assets with external referencers outside Content folder"));
}

FLinearColor FPjcFilterAssetsExtReferenced::GetColor() const
{
	return FPjcStyles::Get().GetColor("ProjectCleaner.Color.Violet");
}

void FPjcFilterAssetsExtReferenced::ActiveStateChanged(bool bActive)
{
	FFrontendFilter::ActiveStateChanged(bActive);

	if (bActive)
	{
		UpdateData();
	}

	if (DelegateFilterChanged.IsBound())
	{
		DelegateFilterChanged.Broadcast(bActive);
	}
}

bool FPjcFilterAssetsExtReferenced::PassesFilter(const FContentBrowserItem& InItem) const
{
	FAssetData AssetData;
	if (!InItem.Legacy_TryGetAssetData(AssetData)) return false;

	return Assets.Contains(AssetData);
}

void FPjcFilterAssetsExtReferenced::UpdateData()
{
	TArray<FAssetData> AssetsExtReferenced;
	UPjcSubsystem::GetAssetsExtReferenced(AssetsExtReferenced, false);

	Assets.Reset();
	Assets.Append(AssetsExtReferenced);
}

FPjcDelegateFilterChanged& FPjcFilterAssetsExtReferenced::OnFilterChanged()
{
	return DelegateFilterChanged;
}
