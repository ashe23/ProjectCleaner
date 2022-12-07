// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class FProjectCleanerCmds final : public TCommands<FProjectCleanerCmds>
{
public:
	FProjectCleanerCmds();
	virtual void RegisterCommands() override;

	// general
	TSharedPtr<FUICommandInfo> OpenProjectCleanerWindow;

	// tab_non_engine files context menu
	TSharedPtr<FUICommandInfo> TabNonEngineTryOpenFile;
	TSharedPtr<FUICommandInfo> TabNonEngineDeleteFile;

	// tab_indirect context menu
	TSharedPtr<FUICommandInfo> TabIndirectNavigateInContentBrowser;
	TSharedPtr<FUICommandInfo> TabIndirectOpenAsset;
	TSharedPtr<FUICommandInfo> TabIndirectOpenFile;

	// tab_unused_assets context menu
	TSharedPtr<FUICommandInfo> TabUnusedPathExclude;
	TSharedPtr<FUICommandInfo> TabUnusedPathInclude;
	TSharedPtr<FUICommandInfo> TabUnusedPathClean;
	

	
	TSharedPtr<FUICommandInfo> OpenDocs;
	// TSharedPtr<FUICommandInfo> DeleteAsset;
	// TSharedPtr<FUICommandInfo> IncludeAsset;
	// TSharedPtr<FUICommandInfo> IncludePath;
	// TSharedPtr<FUICommandInfo> ExcludeAsset;
	// TSharedPtr<FUICommandInfo> ExcludeByType;
	// TSharedPtr<FUICommandInfo> ExcludePath;
};
