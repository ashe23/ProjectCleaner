// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "ProjectCleaner.h"
#include "ProjectCleanerStyle.h"
#include "ProjectCleanerCommands.h"
#include "Misc/MessageDialog.h"
#include "AssetRegistryModule.h"
#include "FileManager.h"
#include "LevelEditor.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "ObjectTools.h"
#include "AssetRegistry/Public/AssetData.h"
#include "ProjectCleanerNotificationManager.h"
#include "ProjectCleanerUtility.h"
#include "NotificationManager.h"

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
	AssetChunks.Reserve(500);

	NotificationManager = new ProjectCleanerNotificationManager();
	CleanerStatus = ECleanerStatus::None;
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
	AssetChunks.Empty();

	delete NotificationManager;
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
	InitCleaner();

	const float CommonPadding = 20.0f;

	const FText TipOneText = FText::FromString(
		"Tip : Please close all opened window before running any cleaning operations, so some assets released from memory.");
	const FText TipTwoText = FText::FromString(
		"!!! This process can take some time based on your project sizes and amount assets you used. \n So be patient and a take a cup of coffee until it finished :)");
	const FText TipThreeText = FText::FromString(
		"How plugin works? \n It will delete all assets that never used in any level. \n So before cleaning project try to delete any level(maps) assets that you never used.");

	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			// Put your tab content here!
			SNew(SBorder)
            .HAlign(HAlign_Center)
            .Padding(25)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				  .AutoHeight()
				  .HAlign(HAlign_Fill)
				  .VAlign(VAlign_Fill)
				  .Padding(20)
				[
					// First Tip Text
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					  .AutoWidth()
					  .HAlign(HAlign_Center)
					  .VAlign(VAlign_Top)
					[
						SNew(SBorder)
						.HAlign(HAlign_Center)
						.VAlign(VAlign_Center)
						.Padding(CommonPadding)
						.BorderImage(&TipOneBrushColor)
						[
							SNew(STextBlock)
							.Justification(ETextJustify::Center)
	                        .AutoWrapText(true)
							.Text(TipOneText)
						]
					]
				]
				+ SVerticalBox::Slot()
				  .AutoHeight()
				  .HAlign(HAlign_Center)
				[
					// Second Tip Text
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					  .AutoWidth()
					  .HAlign(HAlign_Center)
					  .VAlign(VAlign_Top)
					[
						SNew(SBorder)
	                    .HAlign(HAlign_Center)
	                    .VAlign(VAlign_Center)
	                    .Padding(CommonPadding)
	                    .BorderImage(&TipTwoBrushColor)
						[
							SNew(STextBlock)
	                        .Justification(ETextJustify::Center)
	                        .AutoWrapText(true)
	                        .Text(TipTwoText)
						]
					]
				]
				+ SVerticalBox::Slot()
				  .AutoHeight()
				  .HAlign(HAlign_Center)
				  .Padding(0.0f, 20.0f)
				[
					// Third Tip Text
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					  .AutoWidth()
					  .HAlign(HAlign_Center)
					  .VAlign(VAlign_Top)
					[
						SNew(SBorder)
                        .HAlign(HAlign_Center)
                        .VAlign(VAlign_Center)
                        .Padding(CommonPadding)
                        .BorderImage(&TipTwoBrushColor)
						[
							SNew(STextBlock)
                            .Justification(ETextJustify::Center)
                            .AutoWrapText(true)
                            .Text(TipThreeText)
						]
					]
				]
				+ SVerticalBox::Slot()
				  .AutoHeight()
				  .HAlign(HAlign_Center)
				  .Padding(0.0f, 20.0f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					  .AutoWidth()
					  .HAlign(HAlign_Fill)
					  .VAlign(VAlign_Fill)
					[
						SNew(SButton)
						.HAlign(HAlign_Center)
						.VAlign(VAlign_Center)
						.ContentPadding(10)
						.ButtonColorAndOpacity(FSlateColor{FLinearColor{1.0, 0.0f, 0.006082f, 0.466667f}})
						.OnClicked_Raw(this, &FProjectCleanerModule::OnDeleteUnusedAssetsBtnClick)
						[
							SNew(STextBlock)
							.AutoWrapText(true)
							.ColorAndOpacity(FSlateColor{FLinearColor::White})
							.Text(LOCTEXT("Delete Unused Assets", "Delete Unused Assets"))
						]
					]
					+ SHorizontalBox::Slot()
					  .Padding(20.0f, 0.0f)
					  .AutoWidth()
					[
						SNew(SButton)
						.HAlign(HAlign_Center)
						.VAlign(VAlign_Center)
						.ContentPadding(10)
						.ButtonColorAndOpacity(FSlateColor{FLinearColor{1.0, 0.0f, 0.006082f, 0.466667f}})
	                    .OnClicked_Raw(this, &FProjectCleanerModule::OnDeleteEmptyFolderClick)
						[
							SNew(STextBlock)
	                        .AutoWrapText(true)
	                        .ColorAndOpacity(FSlateColor{FLinearColor::White})
	                        .Text(LOCTEXT("Delete Empty Folders", "Delete Empty Folders"))
						]
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
							.Text_Lambda([this]() -> FText { return FText::AsNumber(CleaningStats.UnusedAssetsNum); })
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
	                    .Text_Lambda([this]() -> FText { return FText::AsMemory(CleaningStats.UnusedAssetsTotalSize); })
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
                        .Text_Lambda([this]() -> FText { return FText::AsNumber(CleaningStats.EmptyFolders); })
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

	// NotificationManager->Show(CleaningStats);

	ProjectCleanerUtility::DeleteEmptyFolders(EmptyFolders);

	DialogText = FText::Format(
		LOCTEXT("PluginButtonDialogText", "Deleted {0} empty folder."),
		CleaningStats.EmptyFolders
	);

	UpdateStats();
	// NotificationManager->Hide();

	FMessageDialog::Open(EAppMsgType::Ok, DialogText);

	return FReply::Handled();
}

FReply FProjectCleanerModule::OnDeleteUnusedAssetsBtnClick()
{
	if (UnusedAssets.Num() == 0)
	{
		const FText DialogText = FText::FromString(FString{"No assets to delete!"});
		FMessageDialog::Open(EAppMsgType::Ok, DialogText);

		return FReply::Handled();
	}

	// Root assets has no referencers
	TArray<FAssetData> RootAssets;
	RootAssets.Reserve(CleaningStats.DeleteChunkSize);
	ProjectCleanerUtility::GetRootAssets(RootAssets, UnusedAssets, CleaningStats);


	NotificationManager->Show(CleaningStats);

	while (RootAssets.Num() > 0)
	{
		const int32 DeletedAssetNum = ProjectCleanerUtility::DeleteAssetsv2(RootAssets);
		if (DeletedAssetNum != RootAssets.Num())
		{
			UE_LOG(LogTemp, Warning, TEXT("Some assets not deleted."));
			CleanerStatus = ECleanerStatus::NotAllAssetsDeleted;
		}

		CleaningStats.DeletedAssetCount += DeletedAssetNum;

		NotificationManager->Update(CleaningStats);

		for (const auto& Asset : RootAssets)
		{
			UnusedAssets.Remove(Asset);
		}

		RootAssets.Empty();
		ProjectCleanerUtility::GetRootAssets(RootAssets, UnusedAssets, CleaningStats);
	}
	// what is left is circular dependent assets that should be deleted
	CleaningStats.DeletedAssetCount += ProjectCleanerUtility::DeleteAssetsv2(UnusedAssets);

	NotificationManager->Update(CleaningStats);

	UpdateStats();
	ProjectCleanerUtility::DeleteEmptyFolders(EmptyFolders);
	UpdateStats();

	NotificationManager->Hide();

	return FReply::Handled();

	FText DialogText;
	if (UnusedAssets.Num() == 0)
	{
		DialogText = FText::FromString(FString{"No assets to delete!"});
	}
	else
	{
		// NotificationManager->Show();

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
	NotificationManager->Hide();

	FMessageDialog::Open(EAppMsgType::Ok, DialogText);

	return FReply::Handled();
}


void FProjectCleanerModule::UpdateStats()
{
	CleaningStats.UnusedAssetsNum = ProjectCleanerUtility::GetUnusedAssetsNum(UnusedAssets);
	CleaningStats.UnusedAssetsTotalSize = ProjectCleanerUtility::GetUnusedAssetsTotalSize(UnusedAssets);
	CleaningStats.EmptyFolders = ProjectCleanerUtility::GetEmptyFoldersNum(EmptyFolders);
	CleaningStats.TotalAssetNum = CleaningStats.UnusedAssetsNum;
	CleaningStats.DeletedAssetCount = 0;
}

void FProjectCleanerModule::InitCleaner()
{
	// before any cleaning operation fixup redirectors
	ProjectCleanerUtility::FixupRedirectors();

	UpdateStats();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FProjectCleanerModule, ProjectCleaner)
