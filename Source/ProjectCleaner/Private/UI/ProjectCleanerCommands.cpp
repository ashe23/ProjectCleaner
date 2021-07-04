// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "UI/ProjectCleanerCommands.h"
#include "UI/ProjectCleanerStyle.h"
// Engine Headers
#include "Framework/Commands/Commands.h"

#define LOCTEXT_NAMESPACE "FProjectCleanerModule"

FProjectCleanerCommands::FProjectCleanerCommands() : TCommands<FProjectCleanerCommands>(
	TEXT("ProjectCleaner"),
	NSLOCTEXT("Contexts", "ProjectCleaner", "ProjectCleaner Plugin"),
	NAME_None,
	FProjectCleanerStyle::GetStyleSetName()
)
{
}

void FProjectCleanerCommands::RegisterCommands()
{
	UI_COMMAND(
		OpenCleanerWindow,
		"ProjectCleaner",
		"Open Cleaner Tab",
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
		ExcludeAsset,
		"Exclude Asset",
		"Exclude selected assets from deletion list",
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
}

#undef LOCTEXT_NAMESPACE