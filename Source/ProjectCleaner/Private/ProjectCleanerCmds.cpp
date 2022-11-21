// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "ProjectCleanerCmds.h"
#include "ProjectCleanerStyles.h"
// #include "UI/ProjectCleanerCommands.h"
// #include "UI/ProjectCleanerStyle.h"
// Engine Headers
#include "Framework/Commands/Commands.h"

#define LOCTEXT_NAMESPACE "FProjectCleanerModule"

FProjectCleanerCmds::FProjectCleanerCmds() : TCommands<FProjectCleanerCmds>(
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
		Cmd_OpenCleanerWindow,
		"ProjectCleaner",
		"Open Project Cleaner Window",
		EUserInterfaceActionType::Button,
		FInputChord()
	);

	UI_COMMAND(
		DeleteAsset,
		"Delete Asset",
		"Delete selected assets",
		EUserInterfaceActionType::Button,
		FInputChord()
	);

	UI_COMMAND(
		IncludeAsset,
		"Include Asset",
		"Include selected assets to deletion list",
		EUserInterfaceActionType::Button,
		FInputChord()
	);

	UI_COMMAND(
		IncludePath,
		"Include Assets in path",
		"Include all assets in selected path",
		EUserInterfaceActionType::Button,
		FInputChord()
	);

	UI_COMMAND(
		ExcludeAsset,
		"Exclude Assets",
		"Exclude selected assets from deletion list",
		EUserInterfaceActionType::Button,
		FInputChord()
	);

	UI_COMMAND(
		ExcludeByType,
		"Exclude Assets of this type",
		"Excludes all assets of this type",
		EUserInterfaceActionType::Button,
		FInputChord()
	);

	UI_COMMAND(
		ExcludePath,
		"Exclude Assets in path",
		"Excludes all assets in selected path",
		EUserInterfaceActionType::Button,
		FInputChord()
	);
}

#undef LOCTEXT_NAMESPACE
