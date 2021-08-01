// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#pragma once

// Engine Headers
#include "Modules/ModuleInterface.h"
#include "CoreMinimal.h"

DECLARE_LOG_CATEGORY_EXTERN(LogProjectCleaner, Log, All);

class FProjectCleanerModule : public IModuleInterface
{
public:
	/* IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	virtual bool IsGameModule() const override;
private:
    /* Module */	
	void RegisterMenus();
	static void PluginButtonClicked();
	TSharedRef<SDockTab> OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs) const;
	
	/* UI data */
	TSharedPtr<FUICommandList> PluginCommands;
};