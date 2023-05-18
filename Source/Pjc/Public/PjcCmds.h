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

	TSharedPtr<FUICommandInfo> ScanProject;
	TSharedPtr<FUICommandInfo> CleanProject;
	TSharedPtr<FUICommandInfo> DeleteEmptyFolders;
	TSharedPtr<FUICommandInfo> ClearExcludeSettings;
	TSharedPtr<FUICommandInfo> ClearSelection;
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
	TSharedPtr<FUICommandInfo> OpenViewerAssetsAudit;
	TSharedPtr<FUICommandInfo> OpenViewerAssetsIndirect;
	TSharedPtr<FUICommandInfo> OpenViewerAssetsCorrupted;
	TSharedPtr<FUICommandInfo> ItemsCollapseAll;
	TSharedPtr<FUICommandInfo> ItemsExpandAll;
	TSharedPtr<FUICommandInfo> ThumbnailSizeTiny;
	TSharedPtr<FUICommandInfo> ThumbnailSizeSmall;
	TSharedPtr<FUICommandInfo> ThumbnailSizeMedium;
	TSharedPtr<FUICommandInfo> ThumbnailSizeLarge;
	TSharedPtr<FUICommandInfo> FilesScan;
	TSharedPtr<FUICommandInfo> FilesDelete;
	TSharedPtr<FUICommandInfo> FilesExclude;
	TSharedPtr<FUICommandInfo> FilesExcludeByExt;
	TSharedPtr<FUICommandInfo> FilesExcludeByPath;
};
