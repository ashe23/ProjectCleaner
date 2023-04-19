// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class FPjcCmds final : public TCommands<FPjcCmds>
{
public:
	FPjcCmds();
	virtual void RegisterCommands() override;

	TSharedPtr<FUICommandInfo> OpenMainWindow;
	TSharedPtr<FUICommandInfo> OpenSourceFile;
	TSharedPtr<FUICommandInfo> OpenSizeMap;
	TSharedPtr<FUICommandInfo> OpenReferenceViewer;
	TSharedPtr<FUICommandInfo> OpenAssetAudit;
	TSharedPtr<FUICommandInfo> PathsOpenInFileExplorer;
	TSharedPtr<FUICommandInfo> PathsExclude;
	TSharedPtr<FUICommandInfo> PathsInclude;
	TSharedPtr<FUICommandInfo> PathsEmptyDelete;
	TSharedPtr<FUICommandInfo> AssetsExclude;
	TSharedPtr<FUICommandInfo> AssetsInclude;
	TSharedPtr<FUICommandInfo> AssetsExcludeByClass;
	TSharedPtr<FUICommandInfo> AssetsIncludeByClass;
	TSharedPtr<FUICommandInfo> AssetsDelete;
	TSharedPtr<FUICommandInfo> DeleteFiles;
	TSharedPtr<FUICommandInfo> FilesExclude;
	TSharedPtr<FUICommandInfo> FilesInclude;
};
