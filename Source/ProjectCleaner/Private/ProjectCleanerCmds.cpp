// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "ProjectCleanerCmds.h"
#include "ProjectCleanerStyles.h"
// Engine Headers
#include "Framework/Commands/Commands.h"

#define LOCTEXT_NAMESPACE "FProjectCleanerModule"

FProjectCleanerCmds::FProjectCleanerCmds() : TCommands(
	TEXT("ProjectCleaner"),
	NSLOCTEXT("Contexts", "ProjectCleaner", "ProjectCleaner Plugin"),
	NAME_None,
	FProjectCleanerStyles::GetStyleSetName()
)
{
}

void FProjectCleanerCmds::RegisterCommands()
{
	UI_COMMAND(
		OpenProjectCleanerWindow,
		"Project Cleaner",
		"Open Project Cleaner Window",
		EUserInterfaceActionType::Button,
		FInputChord()
	);
}

#undef LOCTEXT_NAMESPACE
