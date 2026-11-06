// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"

class FPjcCmds final : public TCommands<FPjcCmds>
{
  public:

	FPjcCmds();
	virtual void RegisterCommands() override;

	// Tabs
	TSharedPtr<FUICommandInfo> TabMain;

	// Generic Commands

	TSharedPtr<FUICommandInfo> ScanProject;
	TSharedPtr<FUICommandInfo> OpenGithub;
	TSharedPtr<FUICommandInfo> OpenWiki;
	TSharedPtr<FUICommandInfo> OpenBugReport;
	TSharedPtr<FUICommandInfo> OpenSettingsProject;
	TSharedPtr<FUICommandInfo> OpenSettingsEditor;
};
