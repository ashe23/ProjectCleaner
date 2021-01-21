// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "ProjectCleaner.h"
#include "ProjectCleanerStyle.h"
#include "ProjectCleanerCommands.h"
#include "Misc/MessageDialog.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "AssetRegistryModule.h"
#include "LevelEditor.h"
#include "ObjectTools.h"

static const FName ProjectCleanerTabName("ProjectCleaner");

#define LOCTEXT_NAMESPACE "FProjectCleanerModule"

void FProjectCleanerModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	
	FProjectCleanerStyle::Initialize();
	FProjectCleanerStyle::ReloadTextures();

	FProjectCleanerCommands::Register();
	
	PluginCommands = MakeShareable(new FUICommandList);

	PluginCommands->MapAction(
		FProjectCleanerCommands::Get().PluginAction,
		FExecuteAction::CreateRaw(this, &FProjectCleanerModule::PluginButtonClicked),
		FCanExecuteAction());
		
	FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	
	{
		TSharedPtr<FExtender> MenuExtender = MakeShareable(new FExtender());
		MenuExtender->AddMenuExtension("WindowLayout", EExtensionHook::After, PluginCommands, FMenuExtensionDelegate::CreateRaw(this, &FProjectCleanerModule::AddMenuExtension));

		LevelEditorModule.GetMenuExtensibilityManager()->AddExtender(MenuExtender);
	}
	
	{
		TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);
		ToolbarExtender->AddToolBarExtension("Settings", EExtensionHook::After, PluginCommands, FToolBarExtensionDelegate::CreateRaw(this, &FProjectCleanerModule::AddToolbarExtension));
		
		LevelEditorModule.GetToolBarExtensibilityManager()->AddExtender(ToolbarExtender);
	}
}

void FProjectCleanerModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	FProjectCleanerStyle::Shutdown();

	FProjectCleanerCommands::Unregister();
}

void FProjectCleanerModule::PluginButtonClicked()
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::GetModuleChecked<FAssetRegistryModule>("AssetRegistry");

	// Finding all project assets
	TArray<FAssetData> AllGameAssets;
	AssetRegistryModule.Get().GetAssetsByPath(FName{ "/Game" }, AllGameAssets, true);

	// excluding Build_data and Level assets
	AllGameAssets.RemoveAll([](FAssetData Val) {
		return Val.AssetName.ToString().Contains("_BuiltData") || Val.AssetClass == UWorld::StaticClass()->GetFName();
	});


	// Finding all assets dependecies
	TSet<FName> LevelsDependencies;
	FARFilter Filter;
	Filter.ClassNames.Add(UWorld::StaticClass()->GetFName());
	this->GetAllDependencies(Filter, AssetRegistryModule.Get(), LevelsDependencies);

	// Removing all assets that are used in any level
	AllGameAssets.RemoveAll([&](FAssetData Val) {
		return LevelsDependencies.Contains(Val.PackageName);
	});

	FText DialogText;
	if (AllGameAssets.Num() == 0)
	{
		DialogText = FText::FromString(FString{ "No assets to delete!" });
	}
	else
	{
		int32 DeletedAssetNum = this->DeleteUnusedAssets(AllGameAssets);

		DialogText = FText::Format(
			LOCTEXT("PluginButtonDialogText", "Deleted {0} assets."),
			DeletedAssetNum
		);
	}

	FMessageDialog::Open(EAppMsgType::Ok, DialogText);
}

void FProjectCleanerModule::AddMenuExtension(FMenuBuilder& Builder)
{
	Builder.AddMenuEntry(FProjectCleanerCommands::Get().PluginAction);
}

void FProjectCleanerModule::GetAllDependencies(const FARFilter & InAssetRegistryFilter, const IAssetRegistry & AssetRegistry, TSet<FName>& OutDependencySet)
{
	TArray<FName> PackageNamesToProcess;
	{
		TArray<FAssetData> FoundAssets;
		AssetRegistry.GetAssets(InAssetRegistryFilter, FoundAssets);
		for (const FAssetData& AssetData : FoundAssets)
		{
			PackageNamesToProcess.Add(AssetData.PackageName);
			OutDependencySet.Add(AssetData.PackageName);
		}
	}

	TArray<FAssetIdentifier> AssetDependencies;
	while (PackageNamesToProcess.Num() > 0)
	{
		const FName PackageName = PackageNamesToProcess.Pop(false);
		AssetDependencies.Reset();
		AssetRegistry.GetDependencies(FAssetIdentifier(PackageName), AssetDependencies);
		for (const FAssetIdentifier& Dependency : AssetDependencies)
		{
			bool bIsAlreadyInSet = false;
			OutDependencySet.Add(Dependency.PackageName, &bIsAlreadyInSet);
			if (bIsAlreadyInSet == false)
			{
				PackageNamesToProcess.Add(Dependency.PackageName);
			}
		}
	}
}

#if WITH_EDITOR
int32 FProjectCleanerModule::DeleteUnusedAssets(TArray<FAssetData>& AssetsToDelete)
{
	if (AssetsToDelete.Num() > 0)
	{
		return ObjectTools::DeleteAssets(AssetsToDelete);
	}

	return 0;
}
#endif

void FProjectCleanerModule::AddToolbarExtension(FToolBarBuilder& Builder)
{
	Builder.AddToolBarButton(FProjectCleanerCommands::Get().PluginAction);
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FProjectCleanerModule, ProjectCleaner)