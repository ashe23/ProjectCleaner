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
	
	TSharedPtr<FUICommandInfo> PathExclude;
	TSharedPtr<FUICommandInfo> PathShowInExplorer;
	TSharedPtr<FUICommandInfo> AssetExclude;
	TSharedPtr<FUICommandInfo> AssetExcludeByType;
};
