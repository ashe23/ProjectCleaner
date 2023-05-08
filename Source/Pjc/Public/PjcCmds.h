// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class FPjcCmds final : public TCommands<FPjcCmds>
{
public:
	FPjcCmds();
	virtual void RegisterCommands() override;

	// Tabs
	TSharedPtr<FUICommandInfo> TabMain;

	// Settings
	TSharedPtr<FUICommandInfo> AutoCleanEmptyFolders;

	
	TSharedPtr<FUICommandInfo> TabAssetsUnusedBtnScan;
	TSharedPtr<FUICommandInfo> TabAssetsUnusedBtnClean;
	TSharedPtr<FUICommandInfo> PathsExclude;
	TSharedPtr<FUICommandInfo> PathsInclude;
	TSharedPtr<FUICommandInfo> PathsDelete;
	TSharedPtr<FUICommandInfo> AssetsExclude;
	TSharedPtr<FUICommandInfo> AssetsExcludeByClass;
	TSharedPtr<FUICommandInfo> AssetsInclude;
	TSharedPtr<FUICommandInfo> AssetsIncludeByClass;
	TSharedPtr<FUICommandInfo> AssetsDelete;
	TSharedPtr<FUICommandInfo> OpenViewerSizeMap;
	TSharedPtr<FUICommandInfo> OpenViewerReference;
	TSharedPtr<FUICommandInfo> OpenViewerAudit;
	TSharedPtr<FUICommandInfo> ItemsCollapseAll;
	TSharedPtr<FUICommandInfo> ItemsExpandAll;
};
