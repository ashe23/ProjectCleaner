// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class FProjectCleanerCmds final : public TCommands<FProjectCleanerCmds>
{
public:
	FProjectCleanerCmds();
	virtual void RegisterCommands() override;

	TSharedPtr<FUICommandInfo> Cmd_OpenCleanerWindow;
	TSharedPtr<FUICommandInfo> Cmd_ShowInContentBrowser;
	TSharedPtr<FUICommandInfo> Cmd_OpenAsset;
	
	TSharedPtr<FUICommandInfo> DeleteAsset;
	TSharedPtr<FUICommandInfo> IncludeAsset;
	TSharedPtr<FUICommandInfo> IncludePath;
	TSharedPtr<FUICommandInfo> ExcludeAsset;
	TSharedPtr<FUICommandInfo> ExcludeByType;
	TSharedPtr<FUICommandInfo> ExcludePath;
};
