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
		Refresh,
		"Refresh",
		"Refresh",
		EUserInterfaceActionType::Button,
		FInputChord()
	);

	UI_COMMAND(
		Delete,
		"Delete",
		"Delete Selected",
		EUserInterfaceActionType::Button,
		FInputChord()
	);

	UI_COMMAND(
		Exclude,
		"Exclude",
		"Exclude Selected",
		EUserInterfaceActionType::Button,
		FInputChord()
	);

	UI_COMMAND(
		ExcludeByExt,
		"Exclude By Ext",
		"Exclude Selected Files By Extension",
		EUserInterfaceActionType::Button,
		FInputChord()
	);

	UI_COMMAND(
		ExcludeByClass,
		"Exclude By Class",
		"Exclude Selected By Class",
		EUserInterfaceActionType::Button,
		FInputChord()
	);

	UI_COMMAND(
		ClearSelection,
		"Clear Selection",
		"Clear Any Selection",
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
		"Delete all empty folders in project. Excluded and Engine Generated folders will be ignored.",
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
		"Exclude",
		"Exclude assets in selected folders",
		EUserInterfaceActionType::Button,
		FInputChord()
	);

	UI_COMMAND(
		PathsInclude,
		"Include",
		"Include assets in selected folders",
		EUserInterfaceActionType::Button,
		FInputChord()
	);

	UI_COMMAND(
		PathsReveal,
		"Reveal in FileExplorer",
		"Show selected folders in FileExplorer",
		EUserInterfaceActionType::Button,
		FInputChord()
	);

	UI_COMMAND(
		PathsExpandRecursive,
		"Expand Recursive",
		"Expand selected folders recursively",
		EUserInterfaceActionType::Button,
		FInputChord()
	);

	UI_COMMAND(
		PathsCollapseRecursive,
		"Collapse Recursive",
		"Collapse selected folders recursively",
		EUserInterfaceActionType::Button,
		FInputChord()
	);

	UI_COMMAND(
		AssetsExclude,
		"Exclude",
		"Exclude selected assets",
		EUserInterfaceActionType::Button,
		FInputChord()
	);

	UI_COMMAND(
		AssetsExcludeByClass,
		"Exclude By Class",
		"Exclude selected assets by class",
		EUserInterfaceActionType::Button,
		FInputChord()
	);

	UI_COMMAND(
		AssetsInclude,
		"Include",
		"Include selected assets",
		EUserInterfaceActionType::Button,
		FInputChord()
	);

	UI_COMMAND(
		AssetsIncludeByClass,
		"Include By Class",
		"Include selected assets by class",
		EUserInterfaceActionType::Button,
		FInputChord()
	);

	UI_COMMAND(
		AssetsDelete,
		"Delete",
		"Delete selected assets",
		EUserInterfaceActionType::Button,
		FInputChord()
	);

	UI_COMMAND(
		OpenViewerSizeMap,
		"SizeMap",
		"Open SizeMap viewer for selected assets",
		EUserInterfaceActionType::Button,
		FInputChord()
	);

	UI_COMMAND(
		OpenViewerReference,
		"ReferenceViewer",
		"Open ReferenceViewer for selected assets",
		EUserInterfaceActionType::Button,
		FInputChord()
	);

	UI_COMMAND(
		OpenViewerAssetsAudit,
		"Asset Audit",
		"Open AssetAudit viewer for selected assets",
		EUserInterfaceActionType::Button,
		FInputChord()
	);

	UI_COMMAND(
		OpenViewerAssetsIndirect,
		"Indirect Assets",
		"Open indirect assets viewer",
		EUserInterfaceActionType::Button,
		FInputChord()
	);

	UI_COMMAND(
		OpenViewerAssetsCorrupted,
		"Corrupted Assets",
		"Open corrupted assets viewer",
		EUserInterfaceActionType::Button,
		FInputChord()
	);

	UI_COMMAND(
		OpenGithub,
		"Go to Github",
		"Open plugins github repository",
		EUserInterfaceActionType::Button,
		FInputChord()
	);

	UI_COMMAND(
		OpenWiki,
		"Go to Wiki",
		"Open plugin documenation",
		EUserInterfaceActionType::Button,
		FInputChord()
	);

	UI_COMMAND(
		OpenBugReport,
		"Report Bug",
		"Open the GitHub issue tracker to report a bug.",
		EUserInterfaceActionType::Button,
		FInputChord()
	);
}

#undef LOCTEXT_NAMESPACE
