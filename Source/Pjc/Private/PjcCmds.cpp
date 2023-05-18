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
		TabMain,
		"Project Cleaner",
		"Open Project Cleaner Main Window",
		EUserInterfaceActionType::Button,
		FInputChord()
	);

	UI_COMMAND(
		ScanProject,
		"Scan Project",
		"Scan project for unused assets and empty folders",
		EUserInterfaceActionType::Button,
		FInputChord()
	);

	UI_COMMAND(
		CleanProject,
		"Clean Project",
		"Clean project from unused assets and empty folders",
		EUserInterfaceActionType::Button,
		FInputChord()
	);

	UI_COMMAND(
		DeleteEmptyFolders,
		"Delete Empty Folders",
		"Delete all empty folders in project. Engine Generated folders will be ignored. (/Game/Developers, /Game/Collection etc).",
		EUserInterfaceActionType::Button,
		FInputChord()
	);

	UI_COMMAND(
		ClearExcludeSettings,
		"Clear Exclude Settings",
		"Clear all exclude settings and rescan project",
		EUserInterfaceActionType::Button,
		FInputChord()
	);

	UI_COMMAND(
		PathsExclude,
		"Exclude ...",
		"Exclude assets in selected paths",
		EUserInterfaceActionType::Button,
		FInputChord()
	);

	UI_COMMAND(
		PathsInclude,
		"Include ...",
		"Include assets in selected paths",
		EUserInterfaceActionType::Button,
		FInputChord()
	);

	UI_COMMAND(
		PathsDelete,
		"Delete ...",
		"Delete all assets in selected paths",
		EUserInterfaceActionType::Button,
		FInputChord()
	);

	UI_COMMAND(
		AssetsExclude,
		"Exclude ...",
		"Exclude selected assets",
		EUserInterfaceActionType::Button,
		FInputChord()
	);

	UI_COMMAND(
		AssetsExcludeByClass,
		"Exclude By Class...",
		"Exclude selected assets by class",
		EUserInterfaceActionType::Button,
		FInputChord()
	);

	UI_COMMAND(
		AssetsInclude,
		"Include ...",
		"Include selected assets",
		EUserInterfaceActionType::Button,
		FInputChord()
	);

	UI_COMMAND(
		AssetsIncludeByClass,
		"Include By Class...",
		"Include selected assets by class",
		EUserInterfaceActionType::Button,
		FInputChord()
	);
	
	UI_COMMAND(
		AssetsDelete,
		"Delete ...",
		"Delete selected assets",
		EUserInterfaceActionType::Button,
		FInputChord()
	);

	UI_COMMAND(
		OpenViewerSizeMap,
		"Open SizeMap ...",
		"Open SizeMap viewer for selected assets",
		EUserInterfaceActionType::Button,
		FInputChord()
	);

	UI_COMMAND(
		OpenViewerReference,
		"Open ReferenceViewer ...",
		"Open ReferenceViewer for selected assets",
		EUserInterfaceActionType::Button,
		FInputChord()
	);

	UI_COMMAND(
		OpenViewerAssetsAudit,
		"Open Asset Audit ...",
		"Open AssetAudit viewer for selected assets",
		EUserInterfaceActionType::Button,
		FInputChord()
	);

	UI_COMMAND(
		OpenViewerAssetsIndirect,
		"Indirect Assets Viewer",
		"Open indirect assets viewer",
		EUserInterfaceActionType::Button,
		FInputChord()
	);

	UI_COMMAND(
		OpenViewerAssetsCorrupted,
		"Corrupted Assets Viewer",
		"Open corrupted assets viewer",
		EUserInterfaceActionType::Button,
		FInputChord()
	);

	UI_COMMAND(
		ItemsCollapseAll,
		"Collapse All ...",
		"Collapse all items",
		EUserInterfaceActionType::Button,
		FInputChord()
	);

	UI_COMMAND(
		ItemsExpandAll,
		"Expand All ...",
		"Expand all items",
		EUserInterfaceActionType::Button,
		FInputChord()
	);
}

#undef LOCTEXT_NAMESPACE
