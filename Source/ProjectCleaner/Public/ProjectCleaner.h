// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "Modules/ModuleInterface.h"
#include "CoreMinimal.h"

DECLARE_LOG_CATEGORY_EXTERN(LogProjectCleaner, Log, All);

// struct FProjectCleanerScanner;

class FProjectCleanerModule final : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	virtual bool IsGameModule() const override;
	virtual bool SupportsDynamicReloading() override;
	// static const TSharedPtr<FProjectCleanerScanner>& GetScanner();
private:
	static void RegisterStyles();
	void RegisterCmds();
	void RegisterMenus();
	void RegisterTabs() const;
	// void RegisterScanner() const;

	static void UnregisterStyles();
	static void UnregisterCmds();
	void UnregisterMenus();
	static void UnregisterTabs();
	// void UnregisterScanner() const;

	TSharedPtr<FUICommandList> Cmds;
	// static TSharedPtr<FProjectCleanerScanner> ScannerInstance;
};
