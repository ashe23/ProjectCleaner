// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class FProjectCleanerCommands : public TCommands<FProjectCleanerCommands>
{
public:
	FProjectCleanerCommands();
	virtual void RegisterCommands() override;

	/** Data **/
	TSharedPtr<FUICommandInfo> PluginAction;
	TSharedPtr<FUICommandInfo> DeleteAsset;
	TSharedPtr<FUICommandInfo> ExcludeAsset;
	TSharedPtr<FUICommandInfo> IncludeAsset;
};