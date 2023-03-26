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

	UI_COMMAND(
		OpenSourceFile,
		"Open Source File...",
		"Try to open file in system default program",
		EUserInterfaceActionType::Button,
		FInputChord()
	);

	UI_COMMAND(
		PathsExclude,
		"Exclude ...",
		"Exclude selected paths",
		EUserInterfaceActionType::Button,
		FInputChord()
	);

	UI_COMMAND(
		PathsInclude,
		"Include ...",
		"Include selected paths",
		EUserInterfaceActionType::Button,
		FInputChord()
	);

	UI_COMMAND(
		PathsEmptyDelete,
		"Delete...",
		"Delete selected empty folders",
		EUserInterfaceActionType::Button,
		FInputChord()
	);

	UI_COMMAND(
		OpenSizeMap,
		"Open SizeMap...",
		"Shows SizeMap viewer for selected assets",
		EUserInterfaceActionType::Button,
		FInputChord()
	);

	UI_COMMAND(
		OpenReferenceViewer,
		"Open ReferenceViewer...",
		"Shows Reference Viewer for selected assets",
		EUserInterfaceActionType::Button,
		FInputChord()
	);

	UI_COMMAND(
		OpenAssetAudit,
		"Open AssetAudit...",
		"Shows AssetAudit for selected assets",
		EUserInterfaceActionType::Button,
		FInputChord()
	);

	UI_COMMAND(
		PathsOpenInFileExplorer,
		"Open In File Explorer...",
		"Open FileExplorer for selected paths",
		EUserInterfaceActionType::Button,
		FInputChord()
	);

	UI_COMMAND(
		AssetsExclude,
		"Exclude...",
		"Excludes selected assets",
		EUserInterfaceActionType::Button,
		FInputChord()
	);

	UI_COMMAND(
		AssetsInclude,
		"Include...",
		"Includes selected assets",
		EUserInterfaceActionType::Button,
		FInputChord()
	);

	UI_COMMAND(
		AssetsExcludeByClass,
		"Exclude By class...",
		"Excludes selected assets by class",
		EUserInterfaceActionType::Button,
		FInputChord()
	);

	UI_COMMAND(
		AssetsIncludeByClass,
		"Include By class...",
		"Included selected assets by class",
		EUserInterfaceActionType::Button,
		FInputChord()
	);

	UI_COMMAND(
		AssetsDelete,
		"Delete...",
		"Deletes selected assets",
		EUserInterfaceActionType::Button,
		FInputChord()
	);

	UI_COMMAND(
		DeleteFiles,
		"Delete...",
		"Deletes selected files",
		EUserInterfaceActionType::Button,
		FInputChord()
	);
}

#undef LOCTEXT_NAMESPACE
