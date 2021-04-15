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
	ScanProjectFiles();
	return ;
	FScopedSlowTask SlowTask{1.0f, FText::FromString("Scanning. Please wait...")};
	SlowTask.MakeDialog();

	Reset();

	// Querying all assets in project
	ProjectCleanerUtility::GetAllAssets(UnusedAssets);

	// Filtering all assets that are used in any level
	Filter_UsedInAnyLevel UsedInAnyLevel;
	UsedInAnyLevel.Apply(UnusedAssets);
	
	// Based on Remaining assets we create Relational list using AdjacencyList method
	ProjectCleanerUtility::CreateAdjacencyList(UnusedAssets, AdjacencyList, false);

	// Filtering assets that are under directories, which user mark excluded from removal
	Filter_ExcludedDirectories ExcludedDirectories{ExcludeDirectoryFilterSettings, AdjacencyList};
	ExcludedDirectories.Apply(UnusedAssets);

	// Querying all empty folders and non .uasset files in project
	ProjectCleanerUtility::GetEmptyFoldersAndNonUassetFiles(EmptyFolders, NonUassetFiles);
	// Querying all source files in project (.h and .cpp files)
	ProjectCleanerUtility::FindAllSourceFiles(SourceFiles);
	
	// Filtering all assets and their related assets, that used in source code files
	Filter_UsedInSourceCode UsedInSourceCode{SourceFiles, AdjacencyList, AssetsUsedInSourceCodeUIStructs};
	UsedInSourceCode.Apply(UnusedAssets);
	// Filtering all assets that has refs to assets that are outside "Game" folder
	Filter_OutsideGameFolder OutsideGameFolderAssetsFilter{AdjacencyList};
	OutsideGameFolderAssetsFilter.Apply(UnusedAssets);

	// here once more time we updating adjacencyList, it will be used when we delete assets
	AdjacencyList.Empty();
	AdjacencyList.Reserve(UnusedAssets.Num());
	ProjectCleanerUtility::CreateAdjacencyList(UnusedAssets,AdjacencyList, true);
	
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

void FProjectCleanerModule::ScanProjectFiles()
{
	const double StartTime = FPlatformTime::Seconds();
	
	struct DirectoryVisitor : IPlatformFile::FDirectoryVisitor
	{
		//This function is called for every file or directory it finds.
		virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory) override
		{
			if(bIsDirectory)
			{
				DirNum++;
				const auto Path = FString{FilenameOrDirectory} / TEXT("*");
				if (ProjectCleanerUtility::IsEmptyDirectory(Path))
				{
					EmptyDirs.Add(FilenameOrDirectory);
				}
				// todo:ashe23 Decide what to do with Developers and Collection folders	
			}
			else
			{
				FileNum++;
				const auto Extension = FPaths::GetExtension(FilenameOrDirectory);
				if (ProjectCleanerUtility::IsEngineExtension(Extension))
				{
					UassetFiles.Add(FilenameOrDirectory);
				}
				else
				{
					NonUassetFiles.Add(FilenameOrDirectory);
				}
			}
			return true;
		}
		
		int32 FileNum = 0;
		int32 DirNum = 0;
		TSet<FName> EmptyDirs;
		TSet<FName> NonUassetFiles;
		TSet<FName> UassetFiles;
	};

	DirectoryVisitor Visitor;
	
	if (IFileManager::Get().IterateDirectoryRecursively(*FPaths::ProjectContentDir(), Visitor))
	{
		UE_LOG(LogProjectCleaner, Display, TEXT("Found  %d files and %d dir"), Visitor.FileNum, Visitor.DirNum);
		UE_LOG(LogProjectCleaner, Display, TEXT("Found %d empty folders"), Visitor.EmptyDirs.Num());
	}


	ProjectCleanerUtility::GetAllAssets(UnusedAssets);

	TSet<FName> PossibleCorruptedFiles;
	for (const auto& File : Visitor.UassetFiles)
	{
		const bool Contains = UnusedAssets.ContainsByPredicate([&](const FAssetData& Data)
		{
			const auto AbsPath = ProjectCleanerUtility::ConvertRelativeToAbsolutePath(Data.PackageName);
			return AbsPath.Equals(File.ToString());
		});
		
		if (!Contains)
		{
			PossibleCorruptedFiles.Add(File);
		}
	}
	
	const double TimeElapsed = FPlatformTime::Seconds() - StartTime;
	UE_LOG(LogProjectCleaner, Display, TEXT("Time elapsed on scanning : %f"), TimeElapsed);
}

void FProjectCleanerModule::ExcludeAssetsFromDeletionList(const TArray<FAssetData>& Assets) const
{
	UE_LOG(LogProjectCleaner, Display, TEXT("Removing Assets from list %d"), Assets.Num());
	//todo:ashe23
}

#pragma optimize("", on)
#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FProjectCleanerModule, ProjectCleaner)
