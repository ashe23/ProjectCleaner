// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectCleaner.h"
#include "ProjectCleanerStyle.h"
#include "ProjectCleanerCommands.h"
#include "Misc/MessageDialog.h"
#include "ToolMenus.h"
#include "AssetRegistryModule.h"
#include "ObjectTools.h"
#include "ContentBrowser/Private/FrontendFilters.h"
#include "Containers/Set.h"

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

	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FProjectCleanerModule::RegisterMenus));
}

void FProjectCleanerModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	UToolMenus::UnRegisterStartupCallback(this);

	UToolMenus::UnregisterOwner(this);

	FProjectCleanerStyle::Shutdown();

	FProjectCleanerCommands::Unregister();
}

void FProjectCleanerModule::PluginButtonClicked()
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::GetModuleChecked<FAssetRegistryModule>("AssetRegistry");

	// Finding all project assets
	TArray<FAssetData> AllGameAsssets;
	AssetRegistryModule.Get().GetAssetsByPath(FName{ "/Game" }, AllGameAsssets, true);

	// excluding Build_data and Level assets
	AllGameAsssets.RemoveAll([](FAssetData Val) {
		return Val.AssetName.ToString().Contains("_BuiltData") || Val.AssetClass == UWorld::StaticClass()->GetFName();
	});


	// Finding all assets dependecies
	TSet<FName> LevelsDependencies;
	FARFilter Filter;
	Filter.ClassNames.Add(UWorld::StaticClass()->GetFName());
	this->GetAllDependencies(Filter, AssetRegistryModule.Get(), LevelsDependencies);

	// Removing all assets that are used in any level
	AllGameAsssets.RemoveAll([&](FAssetData Val) {
		return LevelsDependencies.Contains(Val.PackageName);
	});

	FText DialogText;
	if (AllGameAsssets.Num() == 0)
	{
		DialogText = FText::FromString(FString{ "No assets to delete!" });
	}
	else
	{
		int32 DeletedAsssetNum = this->DeleteUnusedAssets(AllGameAsssets);

		DialogText = FText::Format(
								LOCTEXT("PluginButtonDialogText", "Deleted {0} assets."),
								DeletedAsssetNum
						   );
	}

	FMessageDialog::Open(EAppMsgType::Ok, DialogText);
}

void FProjectCleanerModule::RegisterMenus()
{
	// Owner will be used for cleanup in call to UToolMenus::UnregisterOwner
	FToolMenuOwnerScoped OwnerScoped(this);

	{
		UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
		{
			FToolMenuSection& Section = Menu->FindOrAddSection("WindowLayout");
			Section.AddMenuEntryWithCommandList(FProjectCleanerCommands::Get().PluginAction, PluginCommands);
		}
	}

	{
		UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar");
		{
			FToolMenuSection& Section = ToolbarMenu->FindOrAddSection("Settings");
			{
				FToolMenuEntry& Entry = Section.AddEntry(FToolMenuEntry::InitToolBarButton(FProjectCleanerCommands::Get().PluginAction));
				Entry.SetCommandList(PluginCommands);
			}
		}
	}
}

void FProjectCleanerModule::GetAllDependencies(const FARFilter& InAssetRegistryFilter, const IAssetRegistry& AssetRegistry, TSet<FName>& OutDependencySet)
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
		int32 DeletedAssetsNum = ObjectTools::DeleteAssets(AssetsToDelete);
		return DeletedAssetsNum;
	}

	return 0;
}
#endif

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FProjectCleanerModule, ProjectCleaner)