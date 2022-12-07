// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "Modules/ModuleInterface.h"
#include "CoreMinimal.h"

DECLARE_LOG_CATEGORY_EXTERN(LogProjectCleaner, Log, All);

class FProjectCleanerModule final : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	virtual bool IsGameModule() const override;
	virtual bool SupportsDynamicReloading() override;

private:
	static void RegisterStyles();
	void RegisterCmds();
	void RegisterMenus();
	void RegisterTabs() const;

	static void UnregisterStyles();
	static void UnregisterCmds();
	void UnregisterMenus();
	static void UnregisterTabs();

	TSharedPtr<FUICommandList> Cmds;
};
