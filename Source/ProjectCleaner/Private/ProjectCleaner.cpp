// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "ProjectCleaner.h"
#include "ProjectCleanerStyle.h"
#include "ProjectCleanerCommands.h"
#include "ProjectCleanerNotificationManager.h"
#include "ProjectCleanerUtility.h"
#include "Filters/Filter_UsedInAnyLevel.h"
#include "Filters/Filter_ExcludedDirectories.h"
#include "Filters/Filter_UsedInSourceCode.h"
#include "Filters/Filter_OutsideGameFolder.h"
#include "UI/ProjectCleanerBrowserCommands.h"
#include "UI/ProjectCleanerNonUassetFilesUI.h"
// Engine Headers
#include "IContentBrowserSingleton.h"
#include "AssetRegistryModule.h"
#include "Misc/MessageDialog.h"
#include "LevelEditor.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SScrollBox.h"
#include "EditorStyleSet.h"
#include "GenericPlatform/GenericPlatformMisc.h"
#include "AssetRegistry/Public/AssetData.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Editor/ContentBrowser/Public/ContentBrowserModule.h"
#include "ContentBrowserDelegates.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Framework/Commands/UICommandList.h"
#include "Misc/ScopedSlowTask.h"
#include "Widgets/SWeakWidget.h"

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

	ExcludeDirectoryFilterSettings = GetMutableDefault<UExcludeDirectoriesFilterSettings>();
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

bool FProjectCleanerModule::IsGameModule() const
{
	return false;
}

void FProjectCleanerModule::PluginButtonClicked()
{
	UpdateCleaner();

	FGlobalTabmanager::Get()->InvokeTab(ProjectCleanerTabName);
}

void FProjectCleanerModule::AddMenuExtension(FMenuBuilder& Builder)
{
	Builder.AddMenuEntry(FProjectCleanerCommands::Get().PluginAction);
}

FReply FProjectCleanerModule::RefreshBrowser()
{
	UpdateCleaner();
	
	return FReply::Handled();
}


void FProjectCleanerModule::AddToolbarExtension(FToolBarBuilder& Builder)
{
	Builder.AddToolBarButton(FProjectCleanerCommands::Get().PluginAction);
}

TSharedRef<SDockTab> FProjectCleanerModule::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
	UpdateCleaner();
	
	const FMargin CommonMargin = FMargin{20.0f, 20.0f};

	return SNew(SDockTab)
		.TabRole(ETabRole::MajorTab)
		[
			SNew(SSplitter)
			+ SSplitter::Slot()
			.Value(0.35f)
			[
				SNew(SScrollBox)
				+ SScrollBox::Slot()
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					  .Padding(CommonMargin)
					  .AutoHeight()
					[
						SAssignNew(ProjectCleanerBrowserStatisticsUI, SProjectCleanerBrowserStatisticsUI)
						.Stats(CleaningStats)
					]
					+ SVerticalBox::Slot()
					  .Padding(CommonMargin)
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
				+ SScrollBox::Slot()
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					  .Padding(CommonMargin)
					  .AutoHeight()
					[
						SAssignNew(ProjectCleanerDirectoryExclusionUI, SProjectCleanerDirectoryExclusionUI)
						.ExcludeDirectoriesFilterSettings(ExcludeDirectoryFilterSettings)
					]
				]
				+ SScrollBox::Slot()
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					  .Padding(CommonMargin)
					  .AutoHeight()
					[
						SAssignNew(ProjectCleanerNonUassetFilesUI, SProjectCleanerNonUassetFilesUI)
						.NonUassetFiles(NonUassetFiles)
					]
				]
				+ SScrollBox::Slot()
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					  .Padding(CommonMargin)
					  .AutoHeight()
					[
						SAssignNew(ProjectCleanerAssetsUsedInSourceCodeUI, SProjectCleanerAssetsUsedInSourceCodeUI)
						.AssetsUsedInSourceCode(AssetsUsedInSourceCodeUIStructs)
					]
				]
			]
			+ SSplitter::Slot()
			.Value(0.65f)
			[
				SNew(SScrollBox)
				+ SScrollBox::Slot()
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					  .Padding(CommonMargin)
					  .AutoHeight()
					[
						SAssignNew(ProjectCleanerUnusedAssetsBrowserUI, SProjectCleanerUnusedAssetsBrowserUI)
						.UnusedAssets(UnusedAssets)
					]
				]
			]
		];
}

void FProjectCleanerModule::InitModuleComponents()
{
	FProjectCleanerCommands::Register();
	FProjectCleanerBrowserCommands::Register();

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

	StreamableManager = &UAssetManager::GetStreamableManager();
	
	bool bFailedWhileDeletingAsset = false;
	while(UnusedAssets.Num() > 0)
	{
		ProjectCleanerUtility::GetRootAssets(RootAssets, UnusedAssets, AdjacencyList);
		if(RootAssets.Num() == 0)
		{
			break;
		}

		// Loading assets and finding corrupted files
		TArray<FAssetData> CorruptedFilesContainer;
		FindCorruptedAssets(RootAssets, CorruptedFilesContainer);
		if (CorruptedFilesContainer.Num() > 0)
		{
			bFailedWhileDeletingAsset = true;
			CorruptedFiles.Append(CorruptedFilesContainer);
	
			UnusedAssets.RemoveAll([&](const FAssetData& Asset)
			{
				return CorruptedFiles.Contains(Asset);
			});
	
			RootAssets.RemoveAll([&](const FAssetData& Asset)
			{
				return CorruptedFiles.Contains(Asset);
			});
		}

		const auto DeletedAssets = ProjectCleanerUtility::DeleteAssets(RootAssets);
		if (DeletedAssets != RootAssets.Num())
		{
			bFailedWhileDeletingAsset = true;
			CorruptedFiles.Append(RootAssets);
			break;
		}

		CleaningStats.DeletedAssetCount += DeletedAssets;
		NotificationManager->Update(NotificationRef, CleaningStats);

		UnusedAssets.RemoveAll([&](const FAssetData& Elem)
		{
			return RootAssets.Contains(Elem);
		});

		AdjacencyList.Empty();
		ProjectCleanerUtility::CreateAdjacencyListV2(UnusedAssets, AdjacencyList, true);
		
		RootAssets.Reset();
		CorruptedFilesContainer.Empty();
	}
	
	if (bFailedWhileDeletingAsset)
	{
		if(CorruptedFiles.Num() > 0)
		{
			OpenCorruptedFilesWindow();
		}
	}
	
	NotificationManager->CachedStats = CleaningStats;
	NotificationManager->Hide(NotificationRef);
	UpdateCleanerData();

	// bool bFailedWhileDeletingAsset = false;
	// StreamableManager = &UAssetManager::GetStreamableManager();
	// while (UnusedAssets.Num() > 0)
	// {
	// 	ProjectCleanerUtility::GetRootAssets(RootAssets, UnusedAssets);
	//
	// 	if (RootAssets.Num() == 0)
	// 	{
	// 		break;
	// 		// todo:ashe23 happens when megascans plugin enabled
	// 		// todo:ashe23 think what to do in this situation.
	// 		
	// 	}
	//
	// 	// Loading assets and finding corrupted files
	// 	TArray<FAssetData> CorruptedFilesContainer;
	// 	FindCorruptedAssets(RootAssets, CorruptedFilesContainer);
	// 	if (CorruptedFilesContainer.Num() > 0)
	// 	{
	// 		bFailedWhileDeletingAsset = true;
	// 		CorruptedFiles.Append(CorruptedFilesContainer);
	//
	// 		UnusedAssets.RemoveAll([&](const FAssetData& Asset)
	// 		{
	// 			return CorruptedFiles.Contains(Asset);
	// 		});
	//
	// 		RootAssets.RemoveAll([&](const FAssetData& Asset)
	// 		{
	// 			return CorruptedFiles.Contains(Asset);
	// 		});
	// 	}
	//
	// 	const auto DeletedAssets = ProjectCleanerUtility::DeleteAssets(RootAssets);
	// 	if (DeletedAssets != RootAssets.Num())
	// 	{
	// 		bFailedWhileDeletingAsset = true;
	// 		CorruptedFiles.Append(RootAssets);
	// 		break;
	// 	}
	// 	
	// 	CleaningStats.DeletedAssetCount += DeletedAssets;
	// 	NotificationManager->Update(NotificationRef, CleaningStats);	
	//
	// 	UnusedAssets.RemoveAll([&](const FAssetData& Elem)
	// 	{
	// 		return RootAssets.Contains(Elem);
	// 	});
	// 	
	// 	RootAssets.Reset();
	// 	CorruptedFilesContainer.Empty();
	// }
	//
	// if (bFailedWhileDeletingAsset)
	// {
	// 	if(CorruptedFiles.Num() > 0)
	// 	{
	// 		OpenCorruptedFilesWindow();
	// 	}
	//
	// 	//
	// 	// FNotificationInfo Info(LOCTEXT("ErrorWhileDeleting",
	// 	// 							   "Error occured while deleting assets! Try to restart editor and try again."));
	// 	// Info.ExpireDuration = 5.0f;
	// 	// FSlateNotificationManager::Get().AddNotification(Info);
	// }
	//
	// NotificationManager->Update(NotificationRef, CleaningStats);
	// NotificationManager->CachedStats = CleaningStats;
	// UpdateCleanerData();
	//
	// // Now Deleting Empty Folders
	// NotificationManager->CachedStats.EmptyFolders = CleaningStats.EmptyFolders;
	// ProjectCleanerUtility::DeleteEmptyFolders(EmptyFolders);
	// NotificationManager->Hide(NotificationRef);
	//
	// UpdateCleanerData();
	// UpdateContentBrowser();

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

void FProjectCleanerModule::OnAssetsLoaded()
{
	// todo:ashe23 THIS FUNCTION WILL BE REMOVED
	//
	// 
	// first we loading all assets, to find which assets not loading correctly,
	// that could be corrupted files - that wont be fixed with in engine tools
	// for (const auto& ObjectPath : ObjectPaths)
	// {
	// 	TSoftObjectPtr<UObject> ObjectSoftPtr{ObjectPath};
	// 	if (!ObjectSoftPtr.Get())
	// 	{
	// 		// finding this asset in list
	// 		FAssetData* CorruptedAsset = UnusedAssets.FindByPredicate([&](const FAssetData& Asset)
	// 		{
	// 			return Asset.ObjectPath.IsEqual(ObjectPath.GetAssetPathName());
	// 		});
	// 		if (CorruptedAsset)
	// 		{
	// 			CorruptedFilesPtrs.Add(CorruptedAsset);
	// 		}
	// 	}
	// 	// unloading asset immediately
	// 	StreamableManager->Unload(ObjectPath);
	// }
	//
	// UnusedAssets.RemoveAll([&](const FAssetData& Asset)
	// {
	// 	return CorruptedFilesPtrs.Contains(&Asset);
	// });
	//
	// UnusedAssetsPtrs.Reserve(UnusedAssets.Num());
	// for (auto& Asset : UnusedAssets)
	// {
	// 	FAssetData* AssetDataPtr = &Asset;
	// 	UnusedAssetsPtrs.Add(AssetDataPtr);
	// }
	//
	//
	// CleaningStats.UnusedAssetsNum = UnusedAssets.Num();
	// CleaningStats.UnusedAssetsTotalSize = ProjectCleanerUtility::GetTotalSize(UnusedAssets);
	// CleaningStats.EmptyFolders = EmptyFolders.Num();
	// CleaningStats.NonUassetFilesNum = NonUassetFiles.Num();
	// CleaningStats.AssetsUsedInSourceCodeFilesNum = AssetsUsedInSourceCodeUIStructs.Num();
	// CleaningStats.TotalAssetNum = CleaningStats.UnusedAssetsNum;
	// CleaningStats.DeletedAssetCount = 0;
	//
	// // UI updates
	// if (ProjectCleanerBrowserStatisticsUI.IsValid())
	// {
	// 	ProjectCleanerBrowserStatisticsUI.Pin()->SetStats(CleaningStats);
	// }
	//
	// if (ProjectCleanerNonUassetFilesUI.IsValid())
	// {
	// 	ProjectCleanerNonUassetFilesUI.Pin()->SetNonUassetFiles(NonUassetFiles);
	// }
	//
	// if (ProjectCleanerUnusedAssetsBrowserUI.IsValid())
	// {
	// 	ProjectCleanerUnusedAssetsBrowserUI.Pin()->SetUnusedAssets(UnusedAssetsPtrs);
	// }
	//
	// if (ProjectCleanerCorruptedFilesUI.IsValid())
	// {
	// 	ProjectCleanerCorruptedFilesUI.Pin()->SetCorruptedFiles(CorruptedFilesPtrs);
	// }
	//
	// if (ProjectCleanerAssetsUsedInSourceCodeUI.IsValid())
	// {
	// 	ProjectCleanerAssetsUsedInSourceCodeUI.Pin()->SetAssetsUsedInSourceCode(AssetsUsedInSourceCodeUIStructs);
	// }
}

void FProjectCleanerModule::OpenCorruptedFilesWindow()
{
	const auto CorruptedFilesWindow =
	SNew(SWindow)
	.Title(LOCTEXT("corruptedfileswindow", "Corrupted Files"))
	.ClientSize(FVector2D(800.0f, 800.0f))
	.MinWidth(800.0f)
	.MinHeight(800.0f)
	.SupportsMaximize(false)
	.SupportsMinimize(false)
	.SizingRule(ESizingRule::Autosized)
	.AutoCenter(EAutoCenter::PrimaryWorkArea)
	.IsInitiallyMaximized(false)
	.bDragAnywhere(true)
	[
		SAssignNew(ProjectCleanerCorruptedFilesUI, SProjectCleanerCorruptedFilesUI)
		.CorruptedFiles(CorruptedFiles)
	];
			
	const TSharedPtr<SWindow> TopWindow = FSlateApplication::Get().GetActiveTopLevelWindow();
	if (TopWindow.IsValid())
	{
		//Add as Native
		FSlateApplication::Get().AddWindowAsNativeChild(
			CorruptedFilesWindow,
			TopWindow.ToSharedRef(),
			true
		);
	}
	else
	{
		//Default in case no top window
		FSlateApplication::Get().AddWindow(CorruptedFilesWindow);
	}
			
	if(GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->AddViewportWidgetContent(SNew(SWeakWidget).PossiblyNullContent(CorruptedFilesWindow));
	}
}

void FProjectCleanerModule::FindCorruptedAssets(const TArray<FAssetData>& Assets, TArray<FAssetData>& CorruptedAssets)
{
	TArray<FSoftObjectPath> LoadedAssets;
	LoadedAssets.Reserve(Assets.Num());
	CorruptedAssets.Reserve(Assets.Num());
	for (const auto& Asset : Assets)
	{
		LoadedAssets.Add(FSoftObjectPath{Asset.ObjectPath});
	}

	StreamableManager->RequestSyncLoad(LoadedAssets);

	for (const auto& LoadedAsset : LoadedAssets)
	{
		TSoftObjectPtr<UObject> ObjectSoftPtr{LoadedAsset};
		if (!ObjectSoftPtr.Get())
		{
			// finding this asset in list
			FAssetData* CorruptedAsset = UnusedAssets.FindByPredicate([&](const FAssetData& Asset)
			{
				return Asset.ObjectPath.IsEqual(LoadedAsset.GetAssetPathName());
			});
			if (CorruptedAsset && CorruptedAsset->IsValid())
			{
				CorruptedAssets.Add(*CorruptedAsset);
			}
		}
	}
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

	UpdateCleanerData();
	UpdateContentBrowser();

	return FReply::Handled();
}


void FProjectCleanerModule::UpdateStats()
{
	CleaningStats.Reset();
	
	CleaningStats.UnusedAssetsNum = UnusedAssets.Num();
	CleaningStats.EmptyFolders = EmptyFolders.Num();
	CleaningStats.NonUassetFilesNum = NonUassetFiles.Num();
	CleaningStats.AssetsUsedInSourceCodeFilesNum = AssetsUsedInSourceCodeUIStructs.Num();
	CleaningStats.UnusedAssetsTotalSize = ProjectCleanerUtility::GetTotalSize(UnusedAssets);
	CleaningStats.TotalAssetNum = CleaningStats.UnusedAssetsNum;

	if (ProjectCleanerBrowserStatisticsUI.IsValid())
	{
		ProjectCleanerBrowserStatisticsUI.Pin()->SetStats(CleaningStats);
	}

	if (ProjectCleanerUnusedAssetsBrowserUI.IsValid())
	{
		ProjectCleanerUnusedAssetsBrowserUI.Pin()->SetUnusedAssets(UnusedAssets);
	}

	if (ProjectCleanerNonUassetFilesUI.IsValid())
	{
		ProjectCleanerNonUassetFilesUI.Pin()->SetNonUassetFiles(NonUassetFiles);
	}
	
	if (ProjectCleanerAssetsUsedInSourceCodeUI.IsValid())
	{
		ProjectCleanerAssetsUsedInSourceCodeUI.Pin()->SetAssetsUsedInSourceCode(AssetsUsedInSourceCodeUIStructs);
	}
}

void FProjectCleanerModule::UpdateCleaner()
{
	ProjectCleanerUtility::SaveAllAssets();

	ProjectCleanerUtility::FixupRedirectors();

	UpdateCleanerData();
}

void FProjectCleanerModule::Reset()
{
	UnusedAssets.Reset();
	SourceFiles.Reset();
	NonUassetFiles.Reset();
	AssetsUsedInSourceCodeUIStructs.Reset();
	EmptyFolders.Reset();
	AdjacencyList.Reset();
}

void FProjectCleanerModule::UpdateCleanerData()
{
	FScopedSlowTask SlowTask{1.0f, FText::FromString("Scanning. Please wait...")};
	SlowTask.MakeDialog();

	Reset();

	// Querying all assets in project
	ProjectCleanerUtility::GetAllAssets(UnusedAssets);

	// Filtering all assets that are used in any level
	Filter_UsedInAnyLevel UsedInAnyLevel;
	UsedInAnyLevel.Apply(UnusedAssets);

	// Filtering assets that are under directories, which user mark excluded from removal
	Filter_ExcludedDirectories ExcludedDirectories{ExcludeDirectoryFilterSettings, AdjacencyList};
	ExcludedDirectories.Apply(UnusedAssets);

	// Querying all empty folders and non .uasset files in project
	ProjectCleanerUtility::GetEmptyFoldersAndNonUassetFiles(EmptyFolders, NonUassetFiles);
	// Querying all source files in project (.h and .cpp files)
	ProjectCleanerUtility::FindAllSourceFiles(SourceFiles);
	
	// Based on Remaining assets we create Relational list using AdjacencyList method
	ProjectCleanerUtility::CreateAdjacencyListV2(UnusedAssets, AdjacencyList, false);

	// Filtering all assets and their related assets, that used in source code files
	Filter_UsedInSourceCode UsedInSourceCode{SourceFiles, AdjacencyList, AssetsUsedInSourceCodeUIStructs};
	UsedInSourceCode.Apply(UnusedAssets);
	// Filtering all assets that has refs to assets that are outside "Game" folder
	Filter_OutsideGameFolder OutsideGameFolderAssetsFilter{AdjacencyList};
	OutsideGameFolderAssetsFilter.Apply(UnusedAssets);

	// here once more time we updating adjacencyList, its will be used when we delete assets
	AdjacencyList.Empty();
	AdjacencyList.Reserve(UnusedAssets.Num());
	ProjectCleanerUtility::CreateAdjacencyListV2(UnusedAssets,AdjacencyList, true);
	
	UpdateStats();
	
	SlowTask.EnterProgressFrame(1.0f);
}

void FProjectCleanerModule::UpdateContentBrowser() const
{
	TArray<FString> FocusFolders;
	FocusFolders.Add("/Game");

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::GetModuleChecked<FAssetRegistryModule>("AssetRegistry");
	AssetRegistryModule.Get().ScanPathsSynchronous(FocusFolders, true);
	AssetRegistryModule.Get().SearchAllAssets(true);

	// FContentBrowserModule& CBModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	// CBModule.Get().SyncBrowserToFolders(FocusFolders);
}

#pragma optimize("", on)
#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FProjectCleanerModule, ProjectCleaner)
