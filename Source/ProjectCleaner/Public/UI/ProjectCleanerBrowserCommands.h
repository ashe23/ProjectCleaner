#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"

class FProjectCleanerBrowserCommands : public TCommands<FProjectCleanerBrowserCommands>
{
public:
	FProjectCleanerBrowserCommands();

	virtual void RegisterCommands() override;

	TSharedPtr<FUICommandInfo> DeleteAsset;
	TSharedPtr<FUICommandInfo> ExcludeFromDeletion;
	TSharedPtr<FUICommandInfo> MarkUnused;
};
