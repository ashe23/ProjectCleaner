// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "ProjectCleaner.h"
#include "ProjectCleanerStyle.h"
#include "ProjectCleanerCommands.h"
#include "ProjectCleanerNotificationManager.h"
#include "ProjectCleanerUtility.h"
#include "UI/ProjectCleanerBrowserCommands.h"
#include "UI/ProjectCleanerNonUassetFilesUI.h"
// Engine Headers
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
#include "Engine/AssetManager.h"
#include "Engine/MapBuildDataRegistry.h"
#include "Engine/StreamableManager.h"
#include "Framework/Commands/UICommandList.h"
#include "Misc/ScopedSlowTask.h"
#include "Widgets/SWeakWidget.h"
#include "ShaderCompiler.h"

DEFINE_LOG_CATEGORY(LogProjectCleaner);

static const FName ProjectCleanerTabName("ProjectCleaner");

#define LOCTEXT_NAMESPACE "FProjectCleanerModule"

#pragma optimize("", off)
void FProjectCleanerModule::StartupModule()
{
	FProjectCleanerStyle::Initialize();
	FProjectCleanerStyle::ReloadTextures();

	InitModuleComponents();

	NotificationManager = MakeShared<ProjectCleanerNotificationManager>();
	ExcludeDirectoryFilterSettings = GetMutableDefault<UExcludeDirectoriesFilterSettings>();
}

void FProjectCleanerModule::ShutdownModule()
{	
	FProjectCleanerStyle::Shutdown();

	FProjectCleanerCommands::Unregister();

	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(ProjectCleanerTabName);
}

bool FProjectCleanerModule::IsGameModule() const
{
	return false;
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

void FProjectCleanerModule::AddMenuExtension(FMenuBuilder& Builder)
{
	Builder.AddMenuEntry(FProjectCleanerCommands::Get().PluginAction);
}

void FProjectCleanerModule::AddToolbarExtension(FToolBarBuilder& Builder)
{
	Builder.AddToolBarButton(FProjectCleanerCommands::Get().PluginAction);
}

void FProjectCleanerModule::PluginButtonClicked()
{
	UpdateCleaner();

	FGlobalTabmanager::Get()->InvokeTab(ProjectCleanerTabName);
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
						SAssignNew(StatisticsUI, SProjectCleanerBrowserStatisticsUI)
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
						SAssignNew(DirectoryExclusionUI, SProjectCleanerDirectoryExclusionUI)
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
						SAssignNew(NonUassetFilesUI, SProjectCleanerNonUassetFilesUI)
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
						SAssignNew(SourceCodeAssetsUI, SProjectCleanerSourceCodeAssetsUI)
						.AssetsUsedInSourceCode(SourceCodeAssets)
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
						SAssignNew(UnusedAssetsBrowserUI, SProjectCleanerUnusedAssetsBrowserUI)
						.UnusedAssets(UnusedAssets)
					]
				]
			]
		];
}

FReply FProjectCleanerModule::RefreshBrowser()
{
	UpdateCleaner();
	
	return FReply::Handled();
}

FReply FProjectCleanerModule::OnDeleteUnusedAssetsBtnClick()
{
	if (!NotificationManager.IsValid())
	{
		UE_LOG(LogProjectCleaner, Error, TEXT("Notification Manager is not valid"));
		return FReply::Handled();
	}
	
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
		if (RootAssets.Num() == 0)
		{
			// todo:ashe23 this should not happen, refactor this later
			// just delete remaining assets
			RootAssets = UnusedAssets;
		}

		// Loading assets and find corrupted files
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

		// after chunk of assets deleted, we must update adjacency list
		AdjacencyList.Empty();
		AdjacencyList.Reserve(UnusedAssets.Num());
		ProjectCleanerUtility::CreateAdjacencyList(UnusedAssets, AdjacencyList, true);
		
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
	UpdateCleanerData();

	// Delete all empty folder
	ProjectCleanerUtility::DeleteEmptyFolders(EmptyFolders);
	NotificationManager->CachedStats.EmptyFolders = EmptyFolders.Num();
	NotificationManager->Hide(NotificationRef);
	
	return FReply::Handled();
}

FReply FProjectCleanerModule::OnDeleteEmptyFolderClick()
{
	if (!NotificationManager.IsValid())
	{
		UE_LOG(LogProjectCleaner, Error, TEXT("Notification Manager is not valid"));
		return FReply::Handled();
	}
	
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

void FProjectCleanerModule::OpenCorruptedFilesWindow()
{
	const auto CorruptedFilesWindow =
	SNew(SWindow)
	.Title(LOCTEXT("corruptedfileswindow", "Failed to delete this assets"))
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
		SAssignNew(CorruptedFilesUI, SProjectCleanerCorruptedFilesUI)
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
	
	StreamableManager = &UAssetManager::GetStreamableManager();
	if (!StreamableManager) return;
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

void FProjectCleanerModule::UpdateStats()
{
	CleaningStats.Reset();
	
	CleaningStats.UnusedAssetsNum = UnusedAssets.Num();
	CleaningStats.EmptyFolders = EmptyFolders.Num();
	CleaningStats.NonUassetFilesNum = NonUassetFiles.Num();
	CleaningStats.SourceCodeAssetsNum = SourceCodeAssets.Num();
	CleaningStats.UnusedAssetsTotalSize = ProjectCleanerUtility::GetTotalSize(UnusedAssets);
	CleaningStats.CorruptedFilesNum = CorruptedFiles.Num();
	CleaningStats.TotalAssetNum = CleaningStats.UnusedAssetsNum;

	if (StatisticsUI.IsValid())
	{
		StatisticsUI.Pin()->SetStats(CleaningStats);
	}

	if (UnusedAssetsBrowserUI.IsValid())
	{
		UnusedAssetsBrowserUI.Pin()->SetUnusedAssets(UnusedAssets);
	}

	if (NonUassetFilesUI.IsValid())
	{
		NonUassetFilesUI.Pin()->SetNonUassetFiles(NonUassetFiles);
	}
	
	if (SourceCodeAssetsUI.IsValid())
	{
		SourceCodeAssetsUI.Pin()->SetAssetsUsedInSourceCode(SourceCodeAssets);
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
	SourceCodeAssets.Reset();
	CorruptedFiles.Reset();
	EmptyFolders.Reset();
	AdjacencyList.Reset();
}

void FProjectCleanerModule::UpdateCleanerData()
{
	Reset();
	
	FScopedSlowTask SlowTask{1.0f, FText::FromString("Scanning. Please wait...")};
	SlowTask.MakeDialog();
	
	const double StartTime = FPlatformTime::Seconds();

	struct FDirectoryVisitor : IPlatformFile::FDirectoryVisitor
	{
		virtual bool Visit(const TCHAR* FileName, bool bIsDirectory) override
		{
			if(!bIsDirectory)
			{
				FileNum++;
				const auto Extension = FPaths::GetExtension(FileName);
				if (ProjectCleanerUtility::IsEngineExtension(Extension))
				{
					UassetFiles.Add(FileName);
				}
				else
				{
					NonUassetFiles.Add(FileName);
				}
			}
			
			return true;
		}
		
		int32 FileNum = 0;
		TSet<FName> NonUassetFiles;
		TSet<FName> UassetFiles;
	};

	FDirectoryVisitor Visitor;
	if (!IFileManager::Get().IterateDirectoryRecursively(*FPaths::ProjectContentDir(), Visitor))
	{
		UE_LOG(LogProjectCleaner, Error, TEXT("Failed to scan project directories! Restart Editor and try again."));
		return;
	}

	// at this point we got all non uasset and uasset files
	ProjectCleanerUtility::GetEmptyFolders(EmptyFolders);
	
	NonUassetFiles = Visitor.NonUassetFiles;
	
	UnusedAssets.Reserve(Visitor.FileNum);
	ProjectCleanerUtility::GetAllAssets(UnusedAssets);
	
	TSet<FName> PossiblyCorruptedFiles;	
	ProjectCleanerUtility::CheckForCorruptedFiles(UnusedAssets, Visitor.UassetFiles, PossiblyCorruptedFiles);
	
	ProjectCleanerUtility::RemoveUsedAssets(UnusedAssets);
	ProjectCleanerUtility::RemoveAssetsWithExternalDependencies(UnusedAssets, AdjacencyList);
	ProjectCleanerUtility::RemoveAssetsUsedInSourceCode(UnusedAssets, AdjacencyList, SourceFiles, SourceCodeAssets);
	ProjectCleanerUtility::RemoveAssetsExcludedByUser(UnusedAssets, AdjacencyList, ExcludeDirectoryFilterSettings);
	
	const double TimeElapsed = FPlatformTime::Seconds() - StartTime;
	UE_LOG(LogProjectCleaner, Display, TEXT("Time elapsed on scanning : %f"), TimeElapsed);

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

void FProjectCleanerModule::ExcludeAssetsFromDeletionList(const TArray<FAssetData>& Assets) const
{
	UE_LOG(LogProjectCleaner, Display, TEXT("Removing Assets from list %d"), Assets.Num());
	// todo:ashe23
}

#pragma optimize("", on)
#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FProjectCleanerModule, ProjectCleaner)
