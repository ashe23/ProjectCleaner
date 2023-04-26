// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ContentBrowserDelegates.h"
#include "Widgets/SCompoundWidget.h"

class SPjcTreeView;
class UPjcSubsystem;

class SPjcAssetBrowser final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SPjcAssetBrowser) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~SPjcAssetBrowser() override;

private:
	FReply OnBtnScanAssetsClick() const;
	void OnScanAssets();
	void OnPathSelectionChanged(const TArray<FName>& InSelectedPaths);
	void FilterUpdate();
	void OnAssetDblClick(const FAssetData& AssetData);
	void OnFilterUsedChanged(const bool bActive);
	void OnFilterPrimaryChanged(const bool bActive);
	void OnFilterIndirectChanged(const bool bActive);
	void OnFilterEditorChanged(const bool bActive);
	void OnFilterExtReferencedChanged(const bool bActive);
	void OnFilterExcludedChanged(const bool bActive);
	void CmdsRegister();
	bool AnyFilterEnabled() const;

	TSharedPtr<SWidget> OnGetAssetContextMenu(const TArray<FAssetData>& Assets) const;

	UPjcSubsystem* SubsystemPtr = nullptr;
	FARFilter Filter;
	TSharedPtr<FUICommandList> Cmds;
	TSharedPtr<SPjcTreeView> TreeViewPtr;
	TArray<FName> SelectedPaths;
	FSetARFilterDelegate DelegateFilter;
	FRefreshAssetViewDelegate DelegateRefreshView;
	FGetCurrentSelectionDelegate DelegateSelection;

	bool bFilterUsedActive = false;
	bool bFilterPrimaryActive = false;
	bool bFilterIndirectActive = false;
	bool bFilterEditorActive = false;
	bool bFilterExtReferencedActive = false;
	bool bFilterExcludedActive = false;
};
