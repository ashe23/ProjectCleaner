// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "ProjectCleanerCommands.h"

#define LOCTEXT_NAMESPACE "FProjectCleanerModule"

void FProjectCleanerCommands::RegisterCommands()
{
	UI_COMMAND(
		PluginAction,
		"ProjectCleaner",
		"Delete unused assets and empty folders.",
		EUserInterfaceActionType::Button, FInputChord()
	);
}

#undef LOCTEXT_NAMESPACE
