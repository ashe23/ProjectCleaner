// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"
#include "Framework/Commands/UICommandList.h"

DECLARE_LOG_CATEGORY_EXTERN(LogProjectCleaner, Log, All);

class FPjc final : public IModuleInterface
{
  public:

	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	virtual bool SupportsDynamicReloading() override;
	virtual bool IsGameModule() const override;

  private:

	void RegisterMenus();
	void OnSettingsChanged();

	TSharedPtr<FUICommandList> Cmds;
};
