// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ContentBrowserDelegates.h"
#include "Widgets/SCompoundWidget.h"

struct FPjcScanDataAssets;
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
	void OnScanAssets(const FPjcScanDataAssets& InScanDataAssets);

	void OnAssetDblClick(const FAssetData& AssetData);

	UPjcSubsystem* SubsystemPtr = nullptr;
	FARFilter Filter;
	FSetARFilterDelegate DelegateFilter;
	FRefreshAssetViewDelegate DelegateRefreshView;
	FGetCurrentSelectionDelegate DelegateSelection;
};
