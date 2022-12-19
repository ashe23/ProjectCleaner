// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ContentBrowserDelegates.h"
#include "Widgets/SCompoundWidget.h"

class UProjectCleanerSubsystem;

class SProjectCleanerTabScanInfo final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SProjectCleanerTabScanInfo)
		{
		}

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	void CommandsRegister();

	void TreeViewUpdate();

	FARFilter AssetBrowserCreateFilter() const;

	bool bFilterExcludeActive = false;
	bool bFilterPrimaryActive = false;
	
	TSharedPtr<FUICommandList> Cmds;
	FGetCurrentSelectionDelegate AssetBrowserDelegateSelection;
	FRefreshAssetViewDelegate AssetBrowserDelegateRefreshView;
	FSetARFilterDelegate AssetBrowserDelegateFilter;
	UProjectCleanerSubsystem* SubsystemPtr = nullptr;
};
