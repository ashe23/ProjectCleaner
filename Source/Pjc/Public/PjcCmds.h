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

	// Generic Commands
	TSharedPtr<FUICommandInfo> Refresh;
	TSharedPtr<FUICommandInfo> Delete;
	TSharedPtr<FUICommandInfo> Exclude;
	TSharedPtr<FUICommandInfo> ExcludeByExt;
	TSharedPtr<FUICommandInfo> ExcludeByClass;
	TSharedPtr<FUICommandInfo> ClearSelection;

	TSharedPtr<FUICommandInfo> ScanProject;
	TSharedPtr<FUICommandInfo> CleanProject;
	TSharedPtr<FUICommandInfo> DeleteEmptyFolders;
	TSharedPtr<FUICommandInfo> ClearExcludeSettings;
	TSharedPtr<FUICommandInfo> PathsExclude;
	TSharedPtr<FUICommandInfo> PathsInclude;
	TSharedPtr<FUICommandInfo> PathsReveal;
	TSharedPtr<FUICommandInfo> PathsExpandRecursive;
	TSharedPtr<FUICommandInfo> PathsCollapseRecursive;
	TSharedPtr<FUICommandInfo> AssetsExclude;
	TSharedPtr<FUICommandInfo> AssetsExcludeByClass;
	TSharedPtr<FUICommandInfo> AssetsInclude;
	TSharedPtr<FUICommandInfo> AssetsIncludeByClass;
	TSharedPtr<FUICommandInfo> AssetsDelete;
	TSharedPtr<FUICommandInfo> OpenViewerSizeMap;
	TSharedPtr<FUICommandInfo> OpenViewerReference;
	TSharedPtr<FUICommandInfo> OpenViewerAssetsAudit;
	TSharedPtr<FUICommandInfo> OpenViewerAssetsIndirect;
	TSharedPtr<FUICommandInfo> OpenViewerAssetsCorrupted;
	TSharedPtr<FUICommandInfo> OpenGithub;
	TSharedPtr<FUICommandInfo> OpenWiki;
	TSharedPtr<FUICommandInfo> OpenBugReport;
};
