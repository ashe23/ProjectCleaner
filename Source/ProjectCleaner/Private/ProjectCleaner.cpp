// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "ProjectCleaner.h"
#include "ProjectCleanerStyle.h"
#include "ProjectCleanerCommands.h"
#include "ProjectCleanerNotificationManager.h"
#include "ProjectCleanerUtility.h"
#include "Filters/Filter_NotUsedInAnyLevel.h"
#include "Filters/Filter_ExcludedDirectories.h"
#include "Filters/Filter_UsedInSourceCode.h"

// Engine Headers
#include "IContentBrowserSingleton.h"
#include "AssetRegistryModule.h"
#include "Misc/MessageDialog.h"
#include "LevelEditor.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SScrollBox.h"
#include "EditorStyleSet.h"
#include "GenericPlatform/GenericPlatformMisc.h"
#include "AssetRegistry/Public/AssetData.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Editor/ContentBrowser/Public/ContentBrowserModule.h"


DEFINE_LOG_CATEGORY(LogProjectCleaner);

static const FName ProjectCleanerTabName("ProjectCleaner");

#define LOCTEXT_NAMESPACE "FProjectCleanerModule"

#pragma optimize("", off)
void FProjectCleanerModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module

	FProjectCleanerStyle::Initialize();
	FProjectCleanerStyle::ReloadTextures();

	InitModuleComponents();

	// Reserve some space
	UnusedAssets.Reserve(100);
	EmptyFolders.Reserve(50);

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

	delete NotificationManager;
}

void FProjectCleanerModule::PluginButtonClicked()
{
	InitCleaner();

	FGlobalTabmanager::Get()->InvokeTab(ProjectCleanerTabName);
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
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(SScrollBox)
			+ SScrollBox::Slot()
			[
				SNew(SVerticalBox)
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
						        .Text(LOCTEXT("Unused Assets", "Unused Assets - "))
							]
							+ SHorizontalBox::Slot()
							.AutoWidth()
							[
								SNew(STextBlock)
						        .AutoWrapText(true)
						        .Text_Lambda([this]() -> FText
								                {
									                return FText::AsNumber(CleaningStats.UnusedAssetsNum);
								                })
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
						        .Text(LOCTEXT("Total Size", "Total Size - "))
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
		                       .Text(LOCTEXT("Empty Folders", "Empty Folders - "))
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
						.FillWidth(1.0f)
						[
							SNew(SButton)
	                        .HAlign(HAlign_Center)
	                        .VAlign(VAlign_Center)
	                        .Text(FText::FromString("Refresh"))
	                        .OnClicked_Raw(this, &FProjectCleanerModule::RefreshBrowser)
						]
						+ SHorizontalBox::Slot()
						  .FillWidth(1.0f)
						  .Padding(FMargin{40.0f, 0.0f, 40.0f, 0.0f})
						[
							SNew(SButton)
	                        .HAlign(HAlign_Center)
	                        .VAlign(VAlign_Center)
	                        .Text(FText::FromString("Delete Unused Assets"))
							.OnClicked_Raw(this, &FProjectCleanerModule::OnDeleteUnusedAssetsBtnClick)
						]
						+ SHorizontalBox::Slot()
						.FillWidth(1.0f)
						[
							SNew(SButton)
	                        .HAlign(HAlign_Center)
	                        .VAlign(VAlign_Center)
	                        .Text(FText::FromString("Delete Empty Folders"))
							.OnClicked_Raw(this, &FProjectCleanerModule::OnDeleteEmptyFolderClick)
						]
					]
				]
			]
		];
}

void FProjectCleanerModule::InitModuleComponents()
{
	FProjectCleanerCommands::Register();

	PluginCommands = MakeShareable(new FUICommandList);

	PluginCommands->MapAction(
		FProjectCleanerCommands::Get().PluginAction,
		FExecuteAction::CreateRaw(this, &FProjectCleanerModule::PluginButtonClicked),
		FCanExecuteAction()
	);

	FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");

	{
		TSharedPtr<FExtender> MenuExtender = MakeShareable(new FExtender());
		MenuExtender->AddMenuExtension(
			"WindowLayout",
			EExtensionHook::After,
			PluginCommands,
			FMenuExtensionDelegate::CreateRaw(this, &FProjectCleanerModule::AddMenuExtension)
		);

		LevelEditorModule.GetMenuExtensibilityManager()->AddExtender(MenuExtender);
	}

	{
		TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);
		ToolbarExtender->AddToolBarExtension(
			"Settings",
			EExtensionHook::After,
			PluginCommands,
			FToolBarExtensionDelegate::CreateRaw(this, &FProjectCleanerModule::AddToolbarExtension)
		);

		LevelEditorModule.GetToolBarExtensibilityManager()->AddExtender(ToolbarExtender);
	}

	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
		                        ProjectCleanerTabName,
		                        FOnSpawnTab::CreateRaw(this, &FProjectCleanerModule::OnSpawnPluginTab)
	                        )
	                        .SetDisplayName(LOCTEXT("FProjectCleanerTabTitle", "ProjectCleaner"))
	                        .SetMenuType(ETabSpawnerMenuType::Hidden);
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
	RootAssets.Reserve(UnusedAssets.Num());

	const auto NotificationRef = NotificationManager->Add(
		StandardCleanerText.StartingCleanup.ToString(),
		SNotificationItem::ECompletionState::CS_Pending
	);

	bool bFailedWhileDeletingAsset = false;
	while (UnusedAssets.Num() > 0 && !bFailedWhileDeletingAsset)
	{
		ProjectCleanerUtility::GetRootAssets(RootAssets, UnusedAssets);

		if (RootAssets.Num() == 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("Error while deleting assets! Try to restart editor and try again.")); // todo:ashe23
			break;
		}

		const auto AssetsDeleted = ProjectCleanerUtility::DeleteAssets(RootAssets);
		if (AssetsDeleted != RootAssets.Num())
		{
			bFailedWhileDeletingAsset = true;
		}
		CleaningStats.DeletedAssetCount += AssetsDeleted;


		NotificationManager->Update(NotificationRef, CleaningStats);

		UnusedAssets.RemoveAll([&](const FAssetData& Elem)
		{
			return RootAssets.Contains(Elem);
		});
		//
		RootAssets.Reset();
	}


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
	Reset();
	
	ProjectCleanerUtility::GetEmptyFoldersAndNonProjectFiles(EmptyFolders,NonProjectFiles);
	ProjectCleanerUtility::FindAllSourceFiles(SourceFiles);
	ProjectCleanerUtility::GetAllAssets(UnusedAssets);
	ProjectCleanerUtility::CreateAdjacencyList(UnusedAssets, AdjacencyList);
	
	// filters
	Filter_NotUsedInAnyLevel NotUsedInAnyLevel;
	NotUsedInAnyLevel.Apply(UnusedAssets);

	Filter_ExcludedDirectories ExcludedDirectories{DirectoryFilterSettings, AdjacencyList};
	ExcludedDirectories.Apply(UnusedAssets);

	Filter_UsedInSourceCode UsedInSourceCode{SourceFiles, AdjacencyList};
	UsedInSourceCode.Apply(UnusedAssets);

	CleaningStats.UnusedAssetsNum = UnusedAssets.Num();
	CleaningStats.UnusedAssetsTotalSize = ProjectCleanerUtility::GetTotalSize(UnusedAssets);
	CleaningStats.EmptyFolders = EmptyFolders.Num();
	CleaningStats.TotalAssetNum = CleaningStats.UnusedAssetsNum;
	CleaningStats.DeletedAssetCount = 0;
	
	if (NonProjectFiles.Num() > 0)
	{
		UE_LOG(LogProjectCleaner, Warning, TEXT("Non UAsset file list:"));
		for (const auto& Asset : NonProjectFiles)
		{
			UE_LOG(LogProjectCleaner, Warning, TEXT("%s"), *Asset);
		}
	
		FNotificationInfo Info(StandardCleanerText.NonUAssetFilesFound);
		Info.ExpireDuration = 10.0f;
		Info.Hyperlink = FSimpleDelegate::CreateStatic([]()
		{
			FGlobalTabmanager::Get()->InvokeTab(FName("OutputLog"));
		});
		Info.HyperlinkText = LOCTEXT("ShowOutputLogHyperlink", "Show Output Log");
		FSlateNotificationManager::Get().AddNotification(Info);
	}
}

void FProjectCleanerModule::InitCleaner()
{
	ProjectCleanerUtility::SaveAllAssets();

	ProjectCleanerUtility::FixupRedirectors();

	UpdateStats();
}

void FProjectCleanerModule::Reset()
{
	UnusedAssets.Reset();
	// AllSourceFiles.Reset();
	// SourceCodeFilesContent.Reset();
	SourceFiles.Reset();
	NonProjectFiles.Reset();
	EmptyFolders.Reset();
	AdjacencyList.Reset();
}

void FProjectCleanerModule::UpdateContentBrowser() const
{
	TArray<FString> FocusFolders;
	FocusFolders.Add("/Game");

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::GetModuleChecked<FAssetRegistryModule>("AssetRegistry");
	AssetRegistryModule.Get().ScanPathsSynchronous(FocusFolders, true);
	AssetRegistryModule.Get().SearchAllAssets(true);

	FContentBrowserModule& CBModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	CBModule.Get().SyncBrowserToFolders(FocusFolders);
}
#pragma optimize("", on)
#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FProjectCleanerModule, ProjectCleaner)
