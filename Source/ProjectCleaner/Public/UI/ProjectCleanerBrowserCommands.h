#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"

class FProjectCleanerBrowserCommands : public TCommands<FProjectCleanerBrowserCommands>
{
public:
	FProjectCleanerBrowserCommands();

	virtual void RegisterCommands() override;

	TSharedPtr<FUICommandInfo> ViewReferences;
	// todo:ashe23 Add delete assets option
};
