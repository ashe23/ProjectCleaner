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
		AssetLocateInBrowser,
		"Locate in ContentBrowser",
		"Show selected assets in main ContentBrowser",
		EUserInterfaceActionType::Button,
		FInputChord()
	);

	UI_COMMAND(
		AssetLocateInExplorer,
		"Locate in FileExplorer",
		"Show selected assets locations in FileExplorer",
		EUserInterfaceActionType::Button,
		FInputChord()
	);

	UI_COMMAND(
		AssetExclude,
		"Exclude",
		"Exclude selected assets",
		EUserInterfaceActionType::Button,
		FInputChord()
	);

	UI_COMMAND(
		AssetExcludeByType,
		"Exclude By Class",
		"Exclude assets of selected classes",
		EUserInterfaceActionType::Button,
		FInputChord()
	);

	UI_COMMAND(
		AssetDelete,
		"Delete",
		"Delete selected assets",
		EUserInterfaceActionType::Button,
		FInputChord()
	);

	UI_COMMAND(
		AssetDeleteLinked,
		"Delete Linked",
		"Delete selected assets plus all unused linked assets",
		EUserInterfaceActionType::Button,
		FInputChord()
	);

	UI_COMMAND(
		TabIndirectAssetOpen,
		"Open Asset",
		"Open asset editor",
		EUserInterfaceActionType::Button,
		FInputChord()
	);

	UI_COMMAND(
		TabIndirectFileOpen,
		"Open File",
		"Try to open file in default system program",
		EUserInterfaceActionType::Button,
		FInputChord()
	);

	UI_COMMAND(
		TabIndirectLocateInBrowser,
		"Locate In ContentBrowser",
		"Show selected assets in main ContentBrowser",
		EUserInterfaceActionType::Button,
		FInputChord()
	);
	
	UI_COMMAND(
		TabNonEngineTryOpenFile,
		"Open File",
		"Try to open file in default system program",
		EUserInterfaceActionType::Button,
		FInputChord()
	);

	UI_COMMAND(
		TabNonEngineDeleteFile,
		"Delete File",
		"TDelete selected file from FileExplorer",
		EUserInterfaceActionType::Button,
		FInputChord()
	);
}

#undef LOCTEXT_NAMESPACE
