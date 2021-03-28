#include "UI/ProjectCleanerBrowserCommands.h"

#define LOCTEXT_NAMESPACE "ProjectCleanerBrowserCommands"

FProjectCleanerBrowserCommands::FProjectCleanerBrowserCommands() : TCommands<FProjectCleanerBrowserCommands>(
	"ProjectCleanerBrowserCommands",
	NSLOCTEXT("Contexts", "ProjectCleanerBrowserCommands", "Project Cleaner"),
	NAME_None,
	FEditorStyle::GetStyleSetName()
)
{
}

void FProjectCleanerBrowserCommands::RegisterCommands()
{
	UI_COMMAND(
		DeleteAsset,
		"Delete Asset...",
		"Deletes Selected Asset",
		EUserInterfaceActionType::Button,
		FInputChord()
	);

	UI_COMMAND(
        ExcludeFromDeletion,
        "Dont delete this Asset...",
        "Exclude asset from deletion list",
        EUserInterfaceActionType::Button,
        FInputChord()
    );
}

#undef LOCTEXT_NAMESPACE