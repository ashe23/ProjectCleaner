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
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "ObjectTools.h"
#include "AssetRegistry/Public/AssetData.h"
#include "ProjectCleanerUtility.h"


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
		MenuExtender->AddMenuExtension("WindowLayout", EExtensionHook::After, PluginCommands,
		                               FMenuExtensionDelegate::CreateRaw(
			                               this, &FProjectCleanerModule::AddMenuExtension));

		LevelEditorModule.GetMenuExtensibilityManager()->AddExtender(MenuExtender);
	}

	{
		TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);
		ToolbarExtender->AddToolBarExtension("Settings", EExtensionHook::After, PluginCommands,
		                                     FToolBarExtensionDelegate::CreateRaw(
			                                     this, &FProjectCleanerModule::AddToolbarExtension));

		LevelEditorModule.GetToolBarExtensibilityManager()->AddExtender(ToolbarExtender);
	}

	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(ProjectCleanerTabName,
	                                                  FOnSpawnTab::CreateRaw(
		                                                  this, &FProjectCleanerModule::OnSpawnPluginTab))
	                        .SetDisplayName(LOCTEXT("FProjectCleanerTabTitle", "ProjectCleaner"))
	                        .SetMenuType(ETabSpawnerMenuType::Hidden);

	// Reserve some space
	UnusedAssets.Reserve(100);
	EmptyFolders.Reserve(50);
}

void FProjectCleanerModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	FProjectCleanerStyle::Shutdown();

	FProjectCleanerCommands::Unregister();

	// todo:ashe23 fix focus issue when tab already pinned in editor somewhere
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(ProjectCleanerTabName);

	UnusedAssets.Empty();
	EmptyFolders.Empty();
}

void FProjectCleanerModule::PluginButtonClicked()
{
	FGlobalTabmanager::Get()->InvokeTab(ProjectCleanerTabName);
}

void FProjectCleanerModule::AddMenuExtension(FMenuBuilder& Builder)
{
	Builder.AddMenuEntry(FProjectCleanerCommands::Get().PluginAction);
}


void FProjectCleanerModule::AddToolbarExtension(FToolBarBuilder& Builder)
{
	Builder.AddToolBarButton(FProjectCleanerCommands::Get().PluginAction);
}

TSharedRef<SDockTab> FProjectCleanerModule::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
	// todo:ashe23 too often updates?
	UpdateStats();
	
	return  SNew(SDockTab)
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
				  .Padding(20)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SButton)
						.Text(FText::FromString(TEXT("Delete Unused Assets")))
						.HAlign(HAlign_Left)
						.OnClicked_Raw(this, &FProjectCleanerModule::OnDeleteUnusedAssetsBtnClick)
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SButton)
	                    .Text(FText::FromString(TEXT("Delete Empty Folders")))
	                    .HAlign(HAlign_Left)
	                    .OnClicked_Raw(this, &FProjectCleanerModule::OnDeleteEmptyFolderClick)
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
	                    .Text(LOCTEXT("Unused Assets:", "Unused Assets: "))
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(STextBlock)
						.AutoWrapText(true)
						.Text_Lambda([this] () -> FText { return FText::AsNumber(UnusedAssetsCount); })
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
	                    .Text(LOCTEXT("Unused Assets Size:", "Unused Assets Size: "))
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(STextBlock)
	                    .AutoWrapText(true)
	                    .Text_Lambda([this] () -> FText
	                    {
		                    return FText::AsMemory(UnusedAssetsFilesSize);
	                    })
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
                        .Text(LOCTEXT("Empty Folders:", "Empty Folders: "))
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(STextBlock)
                        .AutoWrapText(true)
                        .Text_Lambda([this] () -> FText { return FText::AsNumber(EmptyFoldersCount); })
					]
				]
			]
		];
}

FReply FProjectCleanerModule::OnDeleteEmptyFolderClick()
{
	FText DialogText;

	if (EmptyFolders.Num() == 0)
	{
		DialogText = FText::FromString(FString{"No empty folders to delete!"});
		FMessageDialog::Open(EAppMsgType::Ok, DialogText);

		return FReply::Handled();
	}

	ProjectCleanerUtility::DeleteEmptyFolders(EmptyFolders);

	DialogText = FText::Format(
		LOCTEXT("PluginButtonDialogText", "Deleted {0} empty folder."),
		EmptyFoldersCount
	);

	UpdateStats();
	FMessageDialog::Open(EAppMsgType::Ok, DialogText);

	return FReply::Handled();
}

FReply FProjectCleanerModule::OnDeleteUnusedAssetsBtnClick()
{
	FText DialogText;
	if (UnusedAssets.Num() == 0)
	{
		DialogText = FText::FromString(FString{"No assets to delete!"});
	}
	else
	{
		const int32 DeletedAssetNum = ProjectCleanerUtility::DeleteUnusedAssets(UnusedAssets);

		// after assets deleted, perform empty directories cleaning automatically
		UpdateStats();
		ProjectCleanerUtility::DeleteEmptyFolders(EmptyFolders);
		
		DialogText = FText::Format(
			LOCTEXT("PluginButtonDialogText", "Deleted {0} assets and {1} empty folders."),
			DeletedAssetNum,
			EmptyFoldersCount
		);
	}

	UpdateStats();
	FMessageDialog::Open(EAppMsgType::Ok, DialogText);

	return FReply::Handled();
}


void FProjectCleanerModule::UpdateStats()
{
	UnusedAssetsCount = ProjectCleanerUtility::GetUnusedAssetsNum(UnusedAssets);
	UnusedAssetsFilesSize = ProjectCleanerUtility::GetUnusedAssetsTotalSize(UnusedAssets);
	EmptyFoldersCount = ProjectCleanerUtility::GetEmptyFoldersNum(EmptyFolders);
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FProjectCleanerModule, ProjectCleaner)
