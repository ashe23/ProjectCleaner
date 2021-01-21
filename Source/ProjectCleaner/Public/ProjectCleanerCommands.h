// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "ProjectCleanerStyle.h"

class FProjectCleanerCommands : public TCommands<FProjectCleanerCommands>
{
public:

	FProjectCleanerCommands()
		: TCommands<FProjectCleanerCommands>(TEXT("ProjectCleaner"), NSLOCTEXT("Contexts", "ProjectCleaner", "ProjectCleaner Plugin"), NAME_None, FProjectCleanerStyle::GetStyleSetName())
	{
	}

	// TCommands<> interface
	virtual void RegisterCommands() override;

public:
	TSharedPtr< FUICommandInfo > PluginAction;
};
