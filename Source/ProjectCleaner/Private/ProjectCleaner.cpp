// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "ProjectCleaner.h"
#include "ProjectCleanerStyle.h"
#include "ProjectCleanerCommands.h"
#include "Misc/MessageDialog.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "AssetRegistryModule.h"
#include "FileManager.h"
#include "LevelEditor.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "ObjectTools.h"
#include "AssetRegistry/Public/AssetData.h"

static const FName ProjectCleanerTabName("ProjectCleaner");

#define LOCTEXT_NAMESPACE "FProjectCleanerModule"

#pragma optimize("", off)

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
	
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(ProjectCleanerTabName, FOnSpawnTab::CreateRaw(this, &FProjectCleanerModule::OnSpawnPluginTab))
    .SetDisplayName(LOCTEXT("FProjectCleanerTabTitle", "ProjectCleaner"))
    .SetMenuType(ETabSpawnerMenuType::Hidden);
}

void FProjectCleanerModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	FProjectCleanerStyle::Shutdown();

	FProjectCleanerCommands::Unregister();

	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(ProjectCleanerTabName);
}

void FProjectCleanerModule::PluginButtonClicked()
{
	FGlobalTabmanager::Get()->InvokeTab(ProjectCleanerTabName);
	// FAssetRegistryModule& AssetRegistryModule = FModuleManager::GetModuleChecked<FAssetRegistryModule>("AssetRegistry");
	//
	// TArray<FAssetData> AllGameAssets;
	// FindAllGameAssets(AllGameAssets);
	// RemoveLevelAssets(AllGameAssets);
	//
	//
	// // Finding all assets and their dependencies that used in levels
	// TSet<FName> LevelsDependencies;
	// FARFilter Filter;
	// Filter.ClassNames.Add(UWorld::StaticClass()->GetFName());
	// this->GetAllDependencies(Filter, AssetRegistryModule.Get(), LevelsDependencies);
	//
	// // Removing all assets that are used in any level
	// AllGameAssets.RemoveAll([&](const FAssetData& Val) {
	// 	return LevelsDependencies.Contains(Val.PackageName);
	// });
	//
	// TArray<FName> DirectoriesToDelete;
	//
	// for (const auto& Asset : AllGameAssets)
	// {
	// 	DirectoriesToDelete.AddUnique(Asset.PackagePath);
	// }
	//
	// for(const auto& Directory : DirectoriesToDelete)
	// {
	// 	UE_LOG(LogTemp, Warning, TEXT("%s"), *Directory.ToString());
	// }
	//
	// UE_LOG(LogTemp, Warning, TEXT("Directories Num: %d"), DirectoriesToDelete.Num());
	// UE_LOG(LogTemp, Warning, TEXT("Assets Num: %d"), AllGameAssets.Num());
	//
	//
	// // excluding Build_data and Level assets
	// // AllGameAssets.RemoveAll([](FAssetData Val) {
	// // 	return Val.AssetName.ToString().Contains("_BuiltData") || Val.AssetClass == UWorld::StaticClass()->GetFName();
	// // });
	// //
	// //
	// //
	// //
	// FText DialogText;
	// if (AllGameAssets.Num() == 0)
	// {
	// 	DialogText = FText::FromString(FString{ "No assets to delete!" });
	// }
	// else
	// {
	// 	const int32 DeletedAssetNum = this->DeleteUnusedAssets(AllGameAssets);
	// 	this->DeleteEmptyFolder(DirectoriesToDelete);
	// 	
	//
	// 	DialogText = FText::Format(
	// 		LOCTEXT("PluginButtonDialogText", "Deleted {0} assets."),
	// 		DeletedAssetNum
	// 	);
	// }
	//
	// FMessageDialog::Open(EAppMsgType::Ok, DialogText);
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

// #if WITH_EDITOR
int32 FProjectCleanerModule::DeleteUnusedAssets(TArray<FAssetData>& AssetsToDelete)
{	
	if (AssetsToDelete.Num() > 0)
	{
		return ObjectTools::DeleteAssets(AssetsToDelete);
	}

	return 0;
}
// #endif

void FProjectCleanerModule::AddToolbarExtension(FToolBarBuilder& Builder)
{
	Builder.AddToolBarButton(FProjectCleanerCommands::Get().PluginAction);
}

void FProjectCleanerModule::FindAllGameAssets(TArray<FAssetData>& GameAssetsContainer) const
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::GetModuleChecked<FAssetRegistryModule>("AssetRegistry");
	AssetRegistryModule.Get().GetAssetsByPath(FName{ "/Game" }, GameAssetsContainer, true);
}

void FProjectCleanerModule::RemoveLevelAssets(TArray<FAssetData>& GameAssetsContainer) const
{
	GameAssetsContainer.RemoveAll([](FAssetData Val) {
		return Val.AssetName.ToString().Contains("_BuiltData") || Val.AssetClass == UWorld::StaticClass()->GetFName();
	});
}


void FProjectCleanerModule::DeleteEmptyFolder(const TArray<FName>& DirectoriesToDelete)
{
	for(const auto& Directory : DirectoriesToDelete)
	{
		FString Dir = Directory.ToString();
		Dir.RemoveFromStart(TEXT("/Game/"));
		
		const FString DirectoryPath = FPaths::ProjectContentDir() + Dir;		
		IFileManager::Get().DeleteDirectory(*DirectoryPath, false);		
	}	
}


TSharedRef<SDockTab> FProjectCleanerModule::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
	FText WidgetText = FText::Format(
        LOCTEXT("WindowWidgetText", "Add code to {0} in {1} to override this window's contents"),
        FText::FromString(TEXT("FTestPluginModule::OnSpawnPluginTab")),
        FText::FromString(TEXT("TestPlugin.cpp"))
        );

	return SNew(SDockTab)
        .TabRole(ETabRole::NomadTab)
        [
            // Put your tab content here!
            SNew(SBorder)
            .HAlign(HAlign_Center)
            .Padding(25)
            // .VAlign(VAlign_Center)
            [
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Fill)
				.Padding(50)
				[
					SNew(SHorizontalBox)					
					+ SHorizontalBox::Slot()
					.AutoWidth()					
					[
						SNew(SButton)
						.Text(FText::FromString(TEXT("Delete Unused Assets")))
						.HAlign(HAlign_Left)
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SButton)
	                    .Text(FText::FromString(TEXT("Organize Project")))
	                    .HAlign(HAlign_Left)
	                    .OnClicked_Raw(this, &FProjectCleanerModule::OnProjectOrganizeBtnClick)
					]
				]
				+ SVerticalBox::Slot()
				.HAlign(HAlign_Center)
				.AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
	                .AutoWidth()
	                [
	                    SNew(STextBlock)
	                    .AutoWrapText(true)
	                    .Text(LOCTEXT("Unused Assets:", "999"))
	                ]               
				]
				+ SVerticalBox::Slot()
                .HAlign(HAlign_Center)
                .AutoHeight()
                [
                    SNew(SHorizontalBox)
                    + SHorizontalBox::Slot()
                    .AutoWidth()
                    [
                        SNew(STextBlock)
                        .AutoWrapText(true)
                        .Text(LOCTEXT("Empty Folders:", "9"))
                    ]               
                ]
                + SVerticalBox::Slot()
                .HAlign(HAlign_Center)
                .AutoHeight()
                [
                    SNew(SHorizontalBox)
                    + SHorizontalBox::Slot()
                    .AutoWidth()
                    [
                        SNew(STextBlock)
                        .AutoWrapText(true)
                        .Text(LOCTEXT("Unused Assets Size:", "9.9 MB"))
                    ]               
                ]
            ]
        ];
}


FReply FProjectCleanerModule::OnProjectOrganizeBtnClick()
{
	UE_LOG(LogTemp, Warning, TEXT("PRojectOrganzieBtnClicked!"));
	return FReply::Handled();
}
#pragma optimize("", on)
#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FProjectCleanerModule, ProjectCleaner)