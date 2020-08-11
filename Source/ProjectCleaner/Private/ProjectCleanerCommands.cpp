// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectCleanerCommands.h"

#define LOCTEXT_NAMESPACE "FProjectCleanerModule"

void FProjectCleanerCommands::RegisterCommands()
{
	UI_COMMAND(PluginAction, "ProjectCleaner", "Delete All unused assets.", EUserInterfaceActionType::Button, FInputGesture());
}

#undef LOCTEXT_NAMESPACE
