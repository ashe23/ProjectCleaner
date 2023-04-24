// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

// class SPjcContentBrowser;
// class SPjcTreeView;

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
	void OnScanAssets(const FPjcScanDataAssets& InScaDataAssets);

	UPjcSubsystem* SubsystemPtr = nullptr;
	// TSharedPtr<SPjcTreeView> TreeView;
	// TSharedPtr<SPjcContentBrowser> ContentBrowser;
};
