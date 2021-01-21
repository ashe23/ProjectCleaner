// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FToolBarBuilder;
class FMenuBuilder;

class FProjectCleanerModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	
	/** This function will be bound to Command. */
	void PluginButtonClicked();
	
private:

	void AddToolbarExtension(FToolBarBuilder& Builder);
	void AddMenuExtension(FMenuBuilder& Builder);

	void GetAllDependencies(const struct FARFilter& InAssetRegistryFilter, const class IAssetRegistry& AssetRegistry, TSet<FName>& OutDependencySet);
#if WITH_EDITOR
	int32 DeleteUnusedAssets(TArray<FAssetData>& AssetsToDelete);
#endif

private:
	TSharedPtr<class FUICommandList> PluginCommands;
};
