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
#include "EditorStyleSet.h"
#include "GenericPlatformMisc.h"
#include "Misc/FileHelper.h"
#include "AssetRegistry/Public/AssetData.h"
#include "ProjectCleanerNotificationManager.h"
#include "ProjectCleanerUtility.h"
#include "NotificationManager.h"
#include "SListView.h"
#include "Editor/ContentBrowser/Public/ContentBrowserModule.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SComboBox.h"

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
	ProjectAllSourceFiles.Reserve(100);

	NotificationManager = new ProjectCleanerNotificationManager();

	DirectoryFilterSettings = GetMutableDefault<UDirectoryFilterSettings>();
}

void FProjectCleanerModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	FProjectCleanerStyle::Shutdown();

	FProjectCleanerCommands::Unregister();

	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(ProjectCleanerTabName);

	UnusedAssets.Empty();
	EmptyFolders.Empty();
	ProjectAllSourceFiles.Empty();

	delete NotificationManager;
}

void FProjectCleanerModule::PluginButtonClicked()
{
	// InitCleaner();

	// FGlobalTabmanager::Get()->InvokeTab(ProjectCleanerTabName);

	ProjectCleanerUtility::GetUnusedAssets(UnusedAssets, ProjectAllSourceFiles);

	TArray<FAssetData> RootAssets;
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::GetModuleChecked<FAssetRegistryModule>("AssetRegistry");
	for (const auto& UnusedAsset : UnusedAssets)
	{
		TArray<FName> SoftRefs;
		TArray<FName> HardRefs;
		AssetRegistryModule.Get().GetReferencers(UnusedAsset.PackageName, SoftRefs, EAssetRegistryDependencyType::Soft);
		AssetRegistryModule.Get().GetReferencers(UnusedAsset.PackageName, HardRefs, EAssetRegistryDependencyType::Hard);

		SoftRefs.RemoveAll([&](const FName& Val)
		{
			return Val == UnusedAsset.PackageName;
		});

		HardRefs.RemoveAll([&](const FName& Val)
		{
			return Val == UnusedAsset.PackageName;
		});

		if(HardRefs.Num() > 0) continue;

		if(SoftRefs.Num() > 0)
		{
			for(const auto& SoftRef : SoftRefs)
			{
				TArray<FName> Refs;
				AssetRegistryModule.Get().GetReferencers(SoftRef, Refs, EAssetRegistryDependencyType::Hard);
				
				// if soft refs referencer has same asset and no other as hard ref => add to list
				// This detects cycle
				if(Refs.Num() == 1 && Refs.Contains(UnusedAsset.PackageName))
				{
					FAssetData* AssetData = ProjectCleanerUtility::GetAssetData(SoftRef, UnusedAssets);
					if(AssetData && AssetData->IsValid())
					{
						RootAssets.AddUnique(*AssetData);
						RootAssets.AddUnique(UnusedAsset);
					}
				}				
			}
		}
		else
		{
			RootAssets.AddUnique(UnusedAsset);			
		}

	
	}

	// FAssetRegistryModule& AssetRegistryModule = FModuleManager::GetModuleChecked<FAssetRegistryModule>("AssetRegistry");
	// TArray<FNode> Tree;
	// for (const auto& Asset : UnusedAssets)
	// {
	// 	FNode NewNode;
	// 	NewNode.Asset = Asset;
	// 	AssetRegistryModule.Get().
	// 	                    GetReferencers(Asset.PackageName, NewNode.Parents);
	// 	AssetRegistryModule.Get().
	// 	                    GetDependencies(Asset.PackageName, NewNode.Children);
	// 	NewNode.bCyclic = NewNode.IsCyclic();
	// 	Tree.Add(NewNode);
	// }
	//
	//
	// // finding all assets that are not cyclic and have no parents
	// TArray<FNode> FilteredNodes;
	// for(const auto& Node : Tree)
	// {
	// 	if(!Node.bCyclic && Node.Parents.Num() == 0)
	// 	{
	// 		FilteredNodes.Add(Node);
	// 	}
	//
	// 	if(Node.bCyclic && Node.Parents.Num() == 1)
	// 	{
	// 		FNode NewNode;
	// 		NewNode.Asset = Node.Asset;
	// 		NewNode.Children.Append(Node.Parents);
	// 		NewNode.Children.Append(Node.Children);
	// 		NewNode.bCyclic = false;
	// 		FilteredNodes.Add(NewNode);
	// 	}
	// }

	return;


	TArray<TSharedPtr<FString>> Items;

	const float CommonPadding = 20.0f;

	const FText TipOneText = FText::FromString(
		"Tip : Please close all opened window before running any cleaning operations, so some assets released from memory.");
	const FText TipTwoText = FText::FromString(
		"!!! This process can take some time based on your project sizes and amount assets you used. \n So be patient and a take a cup of coffee until it finished :)");
	const FText TipThreeText = FText::FromString(
		"How plugin works? \n It will delete all assets that never used in any level. \n So before cleaning project try to delete any level(maps) assets that you never used.");

	// spawning new window
	TestWindow = SNew(SWindow)
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
            .ForegroundColor(FEditorStyle::GetSlateColor("DefaultForeground"))
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
			+ SVerticalBox::Slot()
			  .HAlign(HAlign_Center)
			  .AutoHeight()
			[
				SNew(SButton)
	            .ButtonStyle(FEditorStyle::Get(), "HoverHintOnly")
	            .ToolTipText(LOCTEXT("FolderButtonToolTipText", "Choose a directory from this computer"))
	            // .OnClicked( FOnClicked::CreateSP(this, &FDirectoryPathStructCustomization::OnPickDirectory, PathProperty.ToSharedRef(), bRelativeToGameContentDir, bUseRelativePath) )
	            .ContentPadding(2.0f)
	            .ForegroundColor(FSlateColor::UseForeground())
	            .IsFocusable(false)
				[
					SNew(SImage)
	                .Image(FEditorStyle::GetBrush("PropertyWindow.Button_Ellipsis"))
	                .ColorAndOpacity(FSlateColor::UseForeground())
				]
			]
		]
	];

	TestWindow->SetCanTick(true);
	FSlateApplication::Get().AddWindow(TestWindow.ToSharedRef());
}

void FProjectCleanerModule::AddMenuExtension(FMenuBuilder& Builder)
{
	Builder.AddMenuEntry(FProjectCleanerCommands::Get().PluginAction);
}

FReply FProjectCleanerModule::RefreshBrowser()
{
	UpdateStats();

	return FReply::Handled();
}


void FProjectCleanerModule::AddToolbarExtension(FToolBarBuilder& Builder)
{
	Builder.AddToolBarButton(FProjectCleanerCommands::Get().PluginAction);
}

TSharedRef<SDockTab> FProjectCleanerModule::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
	const FText TipOneText = FText::FromString(
		"How to use?\nDelete all unused levels first. (If any asset used in level it wont be deleted).\n"
	);

	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			  .AutoHeight()
			  .Padding(FMargin(20))
			[
				SNew(SBorder)
                .Padding(FMargin(10))
                .BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
                .VAlign(VAlign_Center)
                .HAlign(HAlign_Center)
				[
					SNew(STextBlock)
                    .Justification(ETextJustify::Center)
                    .AutoWrapText(true)
                    .Text(TipOneText)
				]
			]
			+ SVerticalBox::Slot()
			  .Padding(FMargin(20))
			  .AutoHeight()
			[
				SNew(SBorder)
	            .Padding(FMargin(5))
	            .BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.HAlign(HAlign_Center)
					[
						// Unused Assets
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
					// stats
					+ SVerticalBox::Slot()
					  .HAlign(HAlign_Center)
					  .AutoHeight()
					[
						// Total size
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(STextBlock)
					        .AutoWrapText(true)
					        .Text(LOCTEXT("Total Size:", "Total Size: "))
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(STextBlock)
					        .AutoWrapText(true)
					        .Text_Lambda([this]() -> FText
							                {
								                return FText::AsMemory(CleaningStats.UnusedAssetsTotalSize);
							                })
						]
					]
					// stats
					+ SVerticalBox::Slot()
					  .HAlign(HAlign_Center)
					  .AutoHeight()
					[
						// Empty folders count
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
			]
			+ SVerticalBox::Slot()
			  .AutoHeight()
			  .Padding(FMargin(20))
			[
				SAssignNew(ProjectCleanerBrowserUI, SProjectCleanerBrowser)
				.DirectoryFilterSettings(DirectoryFilterSettings)
			]
			// action buttons
			+ SVerticalBox::Slot()
			  .Padding(FMargin(20))
			  .AutoHeight()
			[
				SNew(SBorder)
                .Padding(FMargin(10))
                .BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					// .AutoWidth()
					.FillWidth(1.0f)
					// .Padding(FMargin(0.0f, 0.0f, 20.0f, 0.0f))
					[
						SNew(SButton)
                        .HAlign(HAlign_Center)
                        .VAlign(VAlign_Center)
                        .Text(FText::FromString("Refresh"))
                        .OnClicked_Raw(this, &FProjectCleanerModule::RefreshBrowser)
					]
					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					// .AutoWidth()
					[
						SNew(SButton)
                        .HAlign(HAlign_Center)
                        .VAlign(VAlign_Center)
                        .Text(FText::FromString("Delete Unused Assets"))
						// .OnClicked_Raw(this, &FProjectCleanerModule::OnDeleteUnusedAssetsBtnClick)
					]
					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					// .AutoWidth()
					// .Padding(FMargin(20.0f, 0.0f))
					[
						SNew(SButton)
                        .HAlign(HAlign_Center)
                        .VAlign(VAlign_Center)
                        .Text(FText::FromString("Delete Empty Folders"))
						// .OnClicked_Raw(this, &FProjectCleanerModule::OnDeleteEmptyFolderClick)
					]
				]
			]
		];
	// InitCleaner();
	//
	// const float CommonPadding = 20.0f;
	//
	// const FText TipOneText = FText::FromString(
	// 	"Tip : Please close all opened window before running any cleaning operations, so some assets released from memory.");
	// const FText TipTwoText = FText::FromString(
	// 	"!!! This process can take some time based on your project sizes and amount assets you used. \n So be patient and a take a cup of coffee until it finished :)");
	// const FText TipThreeText = FText::FromString(
	// 	"How plugin works? \n It will delete all assets that never used in any level. \n So before cleaning project try to delete any level(maps) assets that you never used.");

	//
	// if (TopWindow.IsValid())
	// {
	// 	auto Win = FSlateApplication::Get().AddWindowAsNativeChild(TestWindow, TopWindow.ToSharedRef(), true);
	// 	if (GEditor)
	// 	{
	// 		GEditor->EditorAddModalWindow( Win );
	// 	}
	// }	
	//
	//
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
	// SNew(SHorizontalBox)
	// + SHorizontalBox::Slot()
	// .AutoWidth()
	// [
	// 	SNew(STextBlock)
	//                  .AutoWrapText(true)
	//                  .Text(LOCTEXT("Unused Assets:", "Unused Assets: "))
	// ]
	// + SHorizontalBox::Slot()
	// .AutoWidth()
	// [
	// 	SNew(STextBlock)
	// 		.AutoWrapText(true)
	// 		.Text_Lambda([this]() -> FText { return FText::AsNumber(CleaningStats.UnusedAssetsNum); })
	// ]
	// 			]
	// + SVerticalBox::Slot()
	//   .HAlign(HAlign_Center)
	//   .AutoHeight()
	// [
	// 	SNew(SHorizontalBox)
	// 	+ SHorizontalBox::Slot()
	// 	.AutoWidth()
	// 	[
	// 		SNew(STextBlock)
	//                  .AutoWrapText(true)
	//                  .Text(LOCTEXT("Unused Assets Size:", "Unused Assets Size: "))
	// 	]
	// 	+ SHorizontalBox::Slot()
	// 	.AutoWidth()
	// 	[
	// 		SNew(STextBlock)
	//                  .AutoWrapText(true)
	//                  .Text_Lambda([this]() -> FText { return FText::AsMemory(CleaningStats.UnusedAssetsTotalSize); })
	// 	]
	// ]
	// 			+ SVerticalBox::Slot()
	// 			  .HAlign(HAlign_Center)
	// 			  .AutoHeight()
	// 			[
	// SNew(SHorizontalBox)
	// + SHorizontalBox::Slot()
	// .AutoWidth()
	// [
	// 	SNew(STextBlock)
	//                    .AutoWrapText(true)
	//                    .Text(LOCTEXT("Empty Folders:", "Empty Folders: "))
	// ]
	// + SHorizontalBox::Slot()
	// .AutoWidth()
	// [
	// 	SNew(STextBlock)
	//                    .AutoWrapText(true)
	//                    .Text_Lambda([this]() -> FText { return FText::AsNumber(CleaningStats.EmptyFolders); })
	// ]
	// 			]
	// 		]
	// 	];
}

FReply FProjectCleanerModule::OnDeleteUnusedAssetsBtnClick()
{
	if (UnusedAssets.Num() == 0)
	{
		NotificationManager->AddTransient(
			StandardCleanerText.NoAssetsToDelete.ToString(),
			SNotificationItem::ECompletionState::CS_Fail,
			3.0f
		);

		return FReply::Handled();
	}

	const auto ConfirmationWindowStatus = ShowConfirmationWindow(
		StandardCleanerText.AssetsDeleteWindowTitle,
		StandardCleanerText.AssetsDeleteWindowContent
	);
	if (IsConfirmationWindowCanceled(ConfirmationWindowStatus))
	{
		return FReply::Handled();
	}

	// Root assets has no referencers
	TArray<FAssetData> RootAssets;
	RootAssets.Reserve(CleaningStats.DeleteChunkSize);
	ProjectCleanerUtility::GetRootAssets(RootAssets, UnusedAssets, CleaningStats);

	const auto NotificationRef = NotificationManager->Add(
		StandardCleanerText.StartingCleanup.ToString(),
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

	CleaningStats.DeletedAssetCount += ProjectCleanerUtility::DeleteAssets(UnusedAssets);

	NotificationManager->Update(NotificationRef, CleaningStats);

	NotificationManager->CachedStats = CleaningStats;
	UpdateStats();
	NotificationManager->CachedStats.EmptyFolders = CleaningStats.EmptyFolders;
	ProjectCleanerUtility::DeleteEmptyFolders(EmptyFolders);
	NotificationManager->Hide(NotificationRef);
	UpdateStats();

	UpdateContentBrowser();

	return FReply::Handled();
}

EAppReturnType::Type FProjectCleanerModule::ShowConfirmationWindow(const FText& Title, const FText& ContentText) const
{
	return FMessageDialog::Open(
		EAppMsgType::YesNo,
		ContentText,
		&Title
	);
}

bool FProjectCleanerModule::IsConfirmationWindowCanceled(EAppReturnType::Type Status)
{
	return Status == EAppReturnType::Type::No || Status == EAppReturnType::Cancel;
}

FReply FProjectCleanerModule::OnDeleteEmptyFolderClick()
{
	if (EmptyFolders.Num() == 0)
	{
		NotificationManager->AddTransient(
			StandardCleanerText.NoEmptyFolderToDelete.ToString(),
			SNotificationItem::ECompletionState::CS_Fail,
			3.0f
		);

		return FReply::Handled();
	}

	const auto ConfirmationWindowStatus = ShowConfirmationWindow(
		StandardCleanerText.EmptyFolderWindowTitle,
		StandardCleanerText.EmptyFolderWindowContent
	);
	if (IsConfirmationWindowCanceled(ConfirmationWindowStatus))
	{
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

	UpdateContentBrowser();

	return FReply::Handled();
}


void FProjectCleanerModule::UpdateStats()
{
	ProjectCleanerUtility::GetUnusedAssets(UnusedAssets, ProjectAllSourceFiles);
	ProjectCleanerUtility::GetEmptyFoldersNum(EmptyFolders, NonProjectFiles);

	if (ShouldApplyDirectoryFilters())
	{
		ApplyDirectoryFilters();
	}

	CleaningStats.UnusedAssetsNum = UnusedAssets.Num();
	CleaningStats.UnusedAssetsTotalSize = ProjectCleanerUtility::GetUnusedAssetsTotalSize(UnusedAssets);
	CleaningStats.EmptyFolders = EmptyFolders.Num();
	CleaningStats.TotalAssetNum = CleaningStats.UnusedAssetsNum;
	CleaningStats.DeletedAssetCount = 0;
}

void FProjectCleanerModule::InitCleaner()
{
	ProjectCleanerUtility::SaveAllAssets();

	ProjectCleanerUtility::FixupRedirectors();

	UpdateStats();
}

void FProjectCleanerModule::UpdateContentBrowser() const
{
	FContentBrowserModule& CBModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	TArray<FString> FocusFolders;
	FocusFolders.Add("/Game");
	CBModule.Get().SyncBrowserToFolders(FocusFolders);


	FAssetRegistryModule& AssetRegistryModule = FModuleManager::GetModuleChecked<FAssetRegistryModule>("AssetRegistry");
	AssetRegistryModule.Get().ScanPathsSynchronous(FocusFolders, true);
	AssetRegistryModule.Get().SearchAllAssets(true);
}

bool FProjectCleanerModule::ShouldApplyDirectoryFilters() const
{
	if (!DirectoryFilterSettings) return false;

	return DirectoryFilterSettings->DirectoryFilterPath.Num() > 0;
}

void FProjectCleanerModule::ApplyDirectoryFilters()
{
	if (UnusedAssets.Num() == 0) return;

	// unused assets filter
	auto CopyAssets = UnusedAssets;
	for (int32 i = 0; i < CopyAssets.Num(); ++i)
	{
		for (const auto& Dir : DirectoryFilterSettings->DirectoryFilterPath)
		{
			const auto PackagePath = CopyAssets[i].PackagePath.ToString();
			const bool Contains = PackagePath.Contains(Dir.Path);
			if (Contains)
			{
				ProjectCleanerUtility::RemoveAllDependenciesFromList(CopyAssets[i], UnusedAssets);
				UnusedAssets.Remove(CopyAssets[i]);
				break;
			}
		}
	}

	// empty folders filter
	auto CopyFolders = EmptyFolders;
	const auto RootDir = FPaths::ProjectContentDir();
	for (int32 i = 0; i < CopyFolders.Num(); ++i)
	{
		auto FolderPath = CopyFolders[i];
		auto Replaced = FolderPath.Replace(*RootDir, TEXT("/Game/"));

		for (const auto& Dir : DirectoryFilterSettings->DirectoryFilterPath)
		{
			if (Replaced.Contains(Dir.Path))
			{
				EmptyFolders.Remove(CopyFolders[i]);
				break;
			}
		}
	}
}

#pragma optimize("", on)
#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FProjectCleanerModule, ProjectCleaner)
