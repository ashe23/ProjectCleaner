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
		ClearSelection,
		"Clear Selection",
		"Clear any selection",
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
		PathsDelete,
		"Delete",
		"Delete all assets in selected paths",
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
		ItemsCollapseAll,
		"Collapse All",
		"Collapse all items",
		EUserInterfaceActionType::Button,
		FInputChord()
	);

	UI_COMMAND(
		ItemsExpandAll,
		"Expand All",
		"Expand all items",
		EUserInterfaceActionType::Button,
		FInputChord()
	);

	UI_COMMAND(
		ThumbnailSizeTiny,
		"Tiny",
		"Thumbnail tiny size",
		EUserInterfaceActionType::Button,
		FInputChord()
	);

	UI_COMMAND(
		ThumbnailSizeSmall,
		"Small",
		"Thumbnail small size",
		EUserInterfaceActionType::Button,
		FInputChord()
	);

	UI_COMMAND(
		ThumbnailSizeMedium,
		"Medium",
		"Thumbnail medium size",
		EUserInterfaceActionType::Button,
		FInputChord()
	);

	UI_COMMAND(
		ThumbnailSizeLarge,
		"Large",
		"Thumbnail large size",
		EUserInterfaceActionType::Button,
		FInputChord()
	);

	UI_COMMAND(
		FilesScan,
		"Scan",
		"Scan files",
		EUserInterfaceActionType::Button,
		FInputChord()
	);

	UI_COMMAND(
		FilesDelete,
		"Delete",
		"Delete selected files",
		EUserInterfaceActionType::Button,
		FInputChord()
	);

	UI_COMMAND(
		FilesExclude,
		"Exclude",
		"Exclude selected files from scanning",
		EUserInterfaceActionType::Button,
		FInputChord()
	);

	UI_COMMAND(
		FilesExcludeByPath,
		"Exclude By Path",
		"Exclude selected files from scanning by their paths",
		EUserInterfaceActionType::Button,
		FInputChord()
	);

	UI_COMMAND(
		FilesExcludeByExt,
		"Exclude By Extension",
		"Exclude selected files from scanning, by their extensions",
		EUserInterfaceActionType::Button,
		FInputChord()
	);
}

#undef LOCTEXT_NAMESPACE
