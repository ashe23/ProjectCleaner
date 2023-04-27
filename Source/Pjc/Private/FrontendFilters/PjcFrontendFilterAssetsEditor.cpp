// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "FrontendFilters/PjcFrontendFilterAssetsEditor.h"
#include "PjcSubsystem.h"

FPjcFilterAssetsEditor::FPjcFilterAssetsEditor(TSharedPtr<FFrontendFilterCategory> InCategory) : FFrontendFilter(InCategory) {}

FString FPjcFilterAssetsEditor::GetName() const
{
	return TEXT("Editor Assets");
}

FText FPjcFilterAssetsEditor::GetDisplayName() const
{
	return FText::FromString(TEXT("Editor Assets"));
}

FText FPjcFilterAssetsEditor::GetToolTipText() const
{
	return FText::FromString(TEXT("Show editor specific assets"));
}

FLinearColor FPjcFilterAssetsEditor::GetColor() const
{
	return FLinearColor::Black;
}

void FPjcFilterAssetsEditor::ActiveStateChanged(bool bActive)
{
	FFrontendFilter::ActiveStateChanged(bActive);

	if (DelegateFilterChanged.IsBound())
	{
		DelegateFilterChanged.Broadcast(bActive);
	}

	if (bActive)
	{
		const TArray<FAssetData>& AssetsEditor = GEditor->GetEditorSubsystem<UPjcSubsystem>()->GetAssetsEditor();
		Assets.Empty(AssetsEditor.Num());
		Assets.Append(AssetsEditor);
	}
}

bool FPjcFilterAssetsEditor::PassesFilter(const FContentBrowserItem& InItem) const
{
	FAssetData AssetData;
	if (!InItem.Legacy_TryGetAssetData(AssetData)) return false;

	return Assets.Contains(AssetData);
}

FPjcDelegateFilterChanged& FPjcFilterAssetsEditor::OnFilterChanged()
{
	return DelegateFilterChanged;
}
