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
		ViewReferences,
		"Reference Viewer...",
		"Launches the reference viewer showing the selected assets' references",
		EUserInterfaceActionType::Button,
		FInputChord()
	);
}

#undef LOCTEXT_NAMESPACE