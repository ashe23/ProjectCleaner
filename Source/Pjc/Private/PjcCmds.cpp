// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "PjcCmds.h"
#include "PjcStyles.h"

#define LOCTEXT_NAMESPACE "FPjc"

FPjcCmds::FPjcCmds() :
	TCommands(TEXT("ProjectCleaner"), NSLOCTEXT("Contexts", "ProjectCleaner", "Project Cleaner"), NAME_None, FPjcStyles::GetStyleSetName()) {}

void FPjcCmds::RegisterCommands() {
	UI_COMMAND(TabMain, "Project Cleaner", "Open Project Cleaner Main Window", EUserInterfaceActionType::Button, FInputChord());

	UI_COMMAND(
		OpenSettingsProject,
		"Scan Settings",
		"Project wide rules for used assets and scanned plugins (Project Settings)",
		EUserInterfaceActionType::Button,
		FInputChord()
	);

	UI_COMMAND(
		OpenSettingsEditor, "Editor Preferences", "Toolbar button, redirectors and other preferences", EUserInterfaceActionType::Button, FInputChord()
	);
}

#undef LOCTEXT_NAMESPACE
