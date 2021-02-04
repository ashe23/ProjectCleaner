// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "ProjectCleaner.h"
#include "ProjectCleanerStyle.h"
#include "ProjectCleanerCommands.h"
#include "Misc/MessageDialog.h"
#include "AssetRegistryModule.h"
#include "FileManager.h"
#include "IContentBrowserSingleton.h"
#include "LevelEditor.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "ObjectTools.h"
#include "HAL/PlatformFilemanager.h"
#include "Misc/FileHelper.h"
#include "AssetRegistry/Public/AssetData.h"
#include "ProjectCleanerNotificationManager.h"
#include "ProjectCleanerUtility.h"
#include "NotificationManager.h"
#include "Editor/ContentBrowser/Public/ContentBrowserModule.h"
#include "Framework/Application/SlateApplication.h"

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

	// FGlobalTabmanager::Get()->RegisterNomadTabSpawner(ProjectCleanerTabName,
	//                                                   FOnSpawnTab::CreateRaw(
	// 	                                                  this, &FProjectCleanerModule::OnSpawnPluginTab))
	//                         .SetDisplayName(LOCTEXT("FProjectCleanerTabTitle", "ProjectCleaner"))
	//                         .SetMenuType(ETabSpawnerMenuType::Hidden);

	// Reserve some space
	UnusedAssets.Reserve(100);
	EmptyFolders.Reserve(50);
	ProjectAllSourceFiles.Reserve(100);

	NotificationManager = new ProjectCleanerNotificationManager();
}

void FProjectCleanerModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	FProjectCleanerStyle::Shutdown();

	FProjectCleanerCommands::Unregister();

	// FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(ProjectCleanerTabName);

	UnusedAssets.Empty();
	EmptyFolders.Empty();
	ProjectAllSourceFiles.Empty();

	delete NotificationManager;
}

void FProjectCleanerModule::PluginButtonClicked()
{
	// FGlobalTabmanager::Get()->InvokeTab(ProjectCleanerTabName);

	InitCleaner();

	const float CommonPadding = 20.0f;

	const FText TipOneText = FText::FromString(
		"Tip : Please close all opened window before running any cleaning operations, so some assets released from memory.");
	const FText TipTwoText = FText::FromString(
		"!!! This process can take some time based on your project sizes and amount assets you used. \n So be patient and a take a cup of coffee until it finished :)");
	const FText TipThreeText = FText::FromString(
		"How plugin works? \n It will delete all assets that never used in any level. \n So before cleaning project try to delete any level(maps) assets that you never used.");

	// spawning new window
	auto TestWindow = SNew(SWindow)
	.Title(LOCTEXT("ProjectCleaner", "Project Cleaner"))
	.ClientSize(FVector2D{800, 800})
	.SupportsMaximize(true)
	.SupportsMinimize(false)
	// .SizingRule(ESizingRule::Autosized)
	.AutoCenter(EAutoCenter::PrimaryWorkArea)
	[
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


	ParentWindow = FSlateApplication::Get().GetActiveTopLevelWindow();
	if (ParentWindow.IsValid())
	{
		FSlateApplication::Get().AddModalWindow(TestWindow, ParentWindow, false);
	}
}

void FProjectCleanerModule::AddMenuExtension(FMenuBuilder& Builder)
{
	Builder.AddMenuEntry(FProjectCleanerCommands::Get().PluginAction);
}


void FProjectCleanerModule::AddToolbarExtension(FToolBarBuilder& Builder)
{
	Builder.AddToolBarButton(FProjectCleanerCommands::Get().PluginAction);
}

void FProjectCleanerModule::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
	InitCleaner();

	const float CommonPadding = 20.0f;

	const FText TipOneText = FText::FromString(
		"Tip : Please close all opened window before running any cleaning operations, so some assets released from memory.");
	const FText TipTwoText = FText::FromString(
		"!!! This process can take some time based on your project sizes and amount assets you used. \n So be patient and a take a cup of coffee until it finished :)");
	const FText TipThreeText = FText::FromString(
		"How plugin works? \n It will delete all assets that never used in any level. \n So before cleaning project try to delete any level(maps) assets that you never used.");


	// if (TopWindow.IsValid())
	// {
	// 	auto Win = FSlateApplication::Get().AddWindowAsNativeChild(TestWindow, TopWindow.ToSharedRef(), true);
	// 	if (GEditor)
	// 	{
	// 		GEditor->EditorAddModalWindow( Win );
	// 	}
	// }	


	// return SNew(SDockTab)
	// 	.TabRole(ETabRole::NomadTab)
	// 	[
	// 		// Put your tab content here!
	// 		SNew(SBorder)
	//            .HAlign(HAlign_Center)
	//            .Padding(25)
	// 		[
	// 			SNew(SVerticalBox)
	// 			+ SVerticalBox::Slot()
	// 			  .AutoHeight()
	// 			  .HAlign(HAlign_Fill)
	// 			  .VAlign(VAlign_Fill)
	// 			  .Padding(20)
	// 			[
	// 				// First Tip Text
	// 				SNew(SHorizontalBox)
	// 				+ SHorizontalBox::Slot()
	// 				  .AutoWidth()
	// 				  .HAlign(HAlign_Center)
	// 				  .VAlign(VAlign_Top)
	// 				[
	// 					SNew(SBorder)
	// 					.HAlign(HAlign_Center)
	// 					.VAlign(VAlign_Center)
	// 					.Padding(CommonPadding)
	// 					.BorderImage(&TipOneBrushColor)
	// 					[
	// 						SNew(STextBlock)
	// 						.Justification(ETextJustify::Center)
	//                         .AutoWrapText(true)
	// 						.Text(TipOneText)
	// 					]
	// 				]
	// 			]
	// 			+ SVerticalBox::Slot()
	// 			  .AutoHeight()
	// 			  .HAlign(HAlign_Center)
	// 			[
	// 				// Second Tip Text
	// 				SNew(SHorizontalBox)
	// 				+ SHorizontalBox::Slot()
	// 				  .AutoWidth()
	// 				  .HAlign(HAlign_Center)
	// 				  .VAlign(VAlign_Top)
	// 				[
	// 					SNew(SBorder)
	//                     .HAlign(HAlign_Center)
	//                     .VAlign(VAlign_Center)
	//                     .Padding(CommonPadding)
	//                     .BorderImage(&TipTwoBrushColor)
	// 					[
	// 						SNew(STextBlock)
	//                         .Justification(ETextJustify::Center)
	//                         .AutoWrapText(true)
	//                         .Text(TipTwoText)
	// 					]
	// 				]
	// 			]
	// 			+ SVerticalBox::Slot()
	// 			  .AutoHeight()
	// 			  .HAlign(HAlign_Center)
	// 			  .Padding(0.0f, 20.0f)
	// 			[
	// 				// Third Tip Text
	// 				SNew(SHorizontalBox)
	// 				+ SHorizontalBox::Slot()
	// 				  .AutoWidth()
	// 				  .HAlign(HAlign_Center)
	// 				  .VAlign(VAlign_Top)
	// 				[
	// 					SNew(SBorder)
	//                        .HAlign(HAlign_Center)
	//                        .VAlign(VAlign_Center)
	//                        .Padding(CommonPadding)
	//                        .BorderImage(&TipTwoBrushColor)
	// 					[
	// 						SNew(STextBlock)
	//                            .Justification(ETextJustify::Center)
	//                            .AutoWrapText(true)
	//                            .Text(TipThreeText)
	// 					]
	// 				]
	// 			]
	// 			+ SVerticalBox::Slot()
	// 			  .AutoHeight()
	// 			  .HAlign(HAlign_Center)
	// 			  .Padding(0.0f, 20.0f)
	// 			[
	// 				SNew(SHorizontalBox)
	// 				+ SHorizontalBox::Slot()
	// 				  .AutoWidth()
	// 				  .HAlign(HAlign_Fill)
	// 				  .VAlign(VAlign_Fill)
	// 				[
	// 					SNew(SButton)
	// 					.HAlign(HAlign_Center)
	// 					.VAlign(VAlign_Center)
	// 					.ContentPadding(10)
	// 					.ButtonColorAndOpacity(FSlateColor{FLinearColor{1.0, 0.0f, 0.006082f, 0.466667f}})
	// 					.OnClicked_Raw(this, &FProjectCleanerModule::OnDeleteUnusedAssetsBtnClick)
	// 					[
	// 						SNew(STextBlock)
	// 						.AutoWrapText(true)
	// 						.ColorAndOpacity(FSlateColor{FLinearColor::White})
	// 						.Text(LOCTEXT("Delete Unused Assets", "Delete Unused Assets"))
	// 					]
	// 				]
	// 				+ SHorizontalBox::Slot()
	// 				  .Padding(20.0f, 0.0f)
	// 				  .AutoWidth()
	// 				[
	// 					SNew(SButton)
	// 					.HAlign(HAlign_Center)
	// 					.VAlign(VAlign_Center)
	// 					.ContentPadding(10)
	// 					.ButtonColorAndOpacity(FSlateColor{FLinearColor{1.0, 0.0f, 0.006082f, 0.466667f}})
	//                     .OnClicked_Raw(this, &FProjectCleanerModule::OnDeleteEmptyFolderClick)
	// 					[
	// 						SNew(STextBlock)
	//                         .AutoWrapText(true)
	//                         .ColorAndOpacity(FSlateColor{FLinearColor::White})
	//                         .Text(LOCTEXT("Delete Empty Folders", "Delete Empty Folders"))
	// 					]
	// 				]
	// 			]
	// 			+ SVerticalBox::Slot()
	// 			  .HAlign(HAlign_Center)
	// 			  .AutoHeight()
	// 			[
	// 				SNew(SHorizontalBox)
	// 				+ SHorizontalBox::Slot()
	// 				.AutoWidth()
	// 				[
	// 					SNew(STextBlock)
	// 	                    .AutoWrapText(true)
	// 	                    .Text(LOCTEXT("Unused Assets:", "Unused Assets: "))
	// 				]
	// 				+ SHorizontalBox::Slot()
	// 				.AutoWidth()
	// 				[
	// 					SNew(STextBlock)
	// 						.AutoWrapText(true)
	// 						.Text_Lambda([this]() -> FText { return FText::AsNumber(CleaningStats.UnusedAssetsNum); })
	// 				]
	// 			]
	// 			+ SVerticalBox::Slot()
	// 			  .HAlign(HAlign_Center)
	// 			  .AutoHeight()
	// 			[
	// 				SNew(SHorizontalBox)
	// 				+ SHorizontalBox::Slot()
	// 				.AutoWidth()
	// 				[
	// 					SNew(STextBlock)
	//                     .AutoWrapText(true)
	//                     .Text(LOCTEXT("Unused Assets Size:", "Unused Assets Size: "))
	// 				]
	// 				+ SHorizontalBox::Slot()
	// 				.AutoWidth()
	// 				[
	// 					SNew(STextBlock)
	//                     .AutoWrapText(true)
	//                     .Text_Lambda([this]() -> FText { return FText::AsMemory(CleaningStats.UnusedAssetsTotalSize); })
	// 				]
	// 			]
	// 			+ SVerticalBox::Slot()
	// 			  .HAlign(HAlign_Center)
	// 			  .AutoHeight()
	// 			[
	// 				SNew(SHorizontalBox)
	// 				+ SHorizontalBox::Slot()
	// 				.AutoWidth()
	// 				[
	// 					SNew(STextBlock)
	//                        .AutoWrapText(true)
	//                        .Text(LOCTEXT("Empty Folders:", "Empty Folders: "))
	// 				]
	// 				+ SHorizontalBox::Slot()
	// 				.AutoWidth()
	// 				[
	// 					SNew(STextBlock)
	//                        .AutoWrapText(true)
	//                        .Text_Lambda([this]() -> FText { return FText::AsNumber(CleaningStats.EmptyFolders); })
	// 				]
	// 			]
	// 		]
	// 	];
}

FReply FProjectCleanerModule::OnDeleteUnusedAssetsBtnClick()
{
	// todo:ashe23 close modal window when pressing this button

	if (UnusedAssets.Num() == 0)
	{
		NotificationManager->AddTransient(
			TEXT("There are no assets to delete!"),
			SNotificationItem::ECompletionState::CS_Fail,
			3.0f
		);

		return FReply::Handled();
	}

	// Root assets has no referencers
	TArray<FAssetData> RootAssets;
	RootAssets.Reserve(CleaningStats.DeleteChunkSize);
	ProjectCleanerUtility::GetRootAssets(RootAssets, UnusedAssets, CleaningStats);


	const auto NotificationRef = NotificationManager->Add(
		TEXT("Starting Cleanup. This could take some time, please wait"),
		SNotificationItem::ECompletionState::CS_Pending
	);

	while (RootAssets.Num() > 0)
	{
		CleaningStats.DeletedAssetCount += ProjectCleanerUtility::DeleteAssets(RootAssets);

		NotificationManager->Update(NotificationRef, CleaningStats);

		for (const auto& Asset : RootAssets)
		{
			UnusedAssets.Remove(Asset);
		}

		RootAssets.Empty();
		ProjectCleanerUtility::GetRootAssets(RootAssets, UnusedAssets, CleaningStats);
	}

	// what is left is circular dependent assets that should be deleted
	CleaningStats.DeletedAssetCount += ProjectCleanerUtility::DeleteAssets(UnusedAssets);

	NotificationManager->Update(NotificationRef, CleaningStats);

	NotificationManager->CachedStats = CleaningStats;

	UpdateStats();

	NotificationManager->CachedStats.EmptyFolders = CleaningStats.EmptyFolders;


	ProjectCleanerUtility::DeleteEmptyFolders(EmptyFolders);

	UpdateStats();

	NotificationManager->Hide(NotificationRef);

	FContentBrowserModule& CBModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	TArray<FString> FocusFolders;
	FocusFolders.Add("/Game");
	CBModule.Get().SyncBrowserToFolders(FocusFolders);

	// todo:ashe23 maybe close window, then start cleaning process?
	return FReply::Handled();
}

FReply FProjectCleanerModule::CloseModalWindow() const
{
	if (ParentWindow.IsValid())
	{
		ParentWindow->RequestDestroyWindow();
	}

	return FReply::Handled();
}

FReply FProjectCleanerModule::OnDeleteEmptyFolderClick()
{
	// todo:ashe23 close modal window when pressing this button

	if (EmptyFolders.Num() == 0)
	{
		NotificationManager->AddTransient(
			FString{"There are no empty folders to delete!"},
			SNotificationItem::ECompletionState::CS_Fail,
			3.0f
		);

		return FReply::Handled();
	}

	ProjectCleanerUtility::DeleteEmptyFolders(EmptyFolders);

	const FString PostFixText = CleaningStats.EmptyFolders > 1 ? TEXT(" empty folders") : TEXT(" empty folder");
	const FString DisplayText = FString{"Deleted "} + FString::FromInt(CleaningStats.EmptyFolders) + PostFixText;
	NotificationManager->AddTransient(
		DisplayText,
		SNotificationItem::ECompletionState::CS_Success,
		5.0f
	);

	UpdateStats();

	FContentBrowserModule& CBModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	TArray<FString> FocusFolders;
	FocusFolders.Add("/Game");
	CBModule.Get().SyncBrowserToFolders(FocusFolders);

	return FReply::Handled();
}


void FProjectCleanerModule::UpdateStats()
{
	CleaningStats.UnusedAssetsNum = ProjectCleanerUtility::GetUnusedAssetsNum(UnusedAssets, ProjectAllSourceFiles);
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
