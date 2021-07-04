// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#pragma once

// Engine Headers
#include "Modules/ModuleInterface.h"
#include "Core/ProjectCleanerManager.h"
#include "CoreMinimal.h"

DECLARE_LOG_CATEGORY_EXTERN(LogProjectCleaner, Log, All);

class FProjectCleanerModule : public IModuleInterface
{
public:
	FProjectCleanerModule();
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	virtual bool IsGameModule() const override;
private:
    /** Module **/	
	void RegisterMenus();
	void PluginButtonClicked();
	TSharedRef<SDockTab> OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs);
	
	/** UI data */
	TSharedPtr<FUICommandList> PluginCommands;
	TWeakPtr<class SProjectCleanerMainUI> CleanerMainUI;
	class ProjectCleanerManager CleanerManager;
	
	/** Other Engine Modules **/
	class FAssetRegistryModule* AssetRegistry;
};