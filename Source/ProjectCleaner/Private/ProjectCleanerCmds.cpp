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

	UI_COMMAND(
		PathExclude,
		"Exclude Path",
		"Exclude selected paths",
		EUserInterfaceActionType::Button,
		FInputChord()
	);

	UI_COMMAND(
		PathShowInExplorer,
		"Open File Explorer",
		"Open selected path if file explorer",
		EUserInterfaceActionType::Button,
		FInputChord()
	);

	UI_COMMAND(
		AssetExclude,
		"Exclude Asset",
		"Exclude selected assets",
		EUserInterfaceActionType::Button,
		FInputChord()
	);

	UI_COMMAND(
		AssetExcludeByType,
		"Exclude Assets By Type",
		"Exclude assets of selected types",
		EUserInterfaceActionType::Button,
		FInputChord()
	);
}

#undef LOCTEXT_NAMESPACE
