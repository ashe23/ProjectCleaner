// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "PjcCmds.h"
#include "PjcStyles.h"

#define LOCTEXT_NAMESPACE "FPjc"

FPjcCmds::FPjcCmds() : TCommands(
	TEXT("ProjectCleaner"),
	NSLOCTEXT("Contexts", "ProjectCleaner", "ProjectCleaner Plugin"),
	NAME_None,
	FPjcStyles::GetStyleSetName()
) {}

void FPjcCmds::RegisterCommands()
{
	UI_COMMAND(
		OpenMainWindow,
		"Project Cleaner",
		"Open Project Cleaner Main Window",
		EUserInterfaceActionType::Button,
		FInputChord()
	);
}

#undef LOCTEXT_NAMESPACE
