// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ContentBrowserDelegates.h"
#include "Widgets/SCompoundWidget.h"

class UProjectCleanerSubsystem;
class SProjectCleanerTreeView;

class SProjectCleanerTabScanInfo final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SProjectCleanerTabScanInfo)
		{
		}

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~SProjectCleanerTabScanInfo() override;
private:
	void CommandsRegister();
	void OnProjectScanned() const;
	void OnTreeViewPathSelected(const TSet<FString>& InSelectedPaths);
	FARFilter AssetBrowserCreateFilter() const;

	bool FilterAnyEnabled() const;
	bool FilterAllDisabled() const;
	bool FilterAllEnabled() const;

	bool bFilterExcludeActive = false;
	bool bFilterPrimaryActive = false;
	bool bFilterUsedActive = false;
	bool bFilterIndirectActive = false;

	TSet<FString> SelectedPaths;
	TSharedPtr<FUICommandList> Cmds;
	FGetCurrentSelectionDelegate AssetBrowserDelegateSelection;
	FRefreshAssetViewDelegate AssetBrowserDelegateRefreshView;
	FSetARFilterDelegate AssetBrowserDelegateFilter;
	UProjectCleanerSubsystem* SubsystemPtr = nullptr;
};
