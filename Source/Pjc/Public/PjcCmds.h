// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class FPjcCmds final : public TCommands<FPjcCmds>
{
public:
	FPjcCmds();
	virtual void RegisterCommands() override;

	TSharedPtr<FUICommandInfo> TabMain;
	TSharedPtr<FUICommandInfo> TabAssetsUnusedBtnScan;
	TSharedPtr<FUICommandInfo> TabAssetsUnusedBtnClean;
};
