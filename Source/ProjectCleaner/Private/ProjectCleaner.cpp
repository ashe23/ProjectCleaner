// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "ProjectCleaner.h"
#include "ProjectCleanerStyle.h"
#include "ProjectCleanerCommands.h"
#include "ProjectCleanerNotificationManager.h"
#include "ProjectCleanerUtility.h"
#include "Filters/Filter_NotUsedInAnyLevel.h"
#include "Filters/Filter_ExcludedDirectories.h"
#include "Filters/Filter_UsedInSourceCode.h"
#include "UI/ProjectCleanerBrowserCommands.h"
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
#include "ContentBrowserDelegates.h"
#include "AssetManagerEditorModule.h"
#include "Framework/Commands/UICommandList.h"
#include "Toolkits/GlobalEditorCommonCommands.h"


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
	NonUProjectFilesSettings = GetMutableDefault<UNonUProjectFiles>();
	UnusedAssetsUIContainerSettings = GetMutableDefault<UUnusedAssetsUIContainer>();

	// UI
	NonProjectFilesInfo = GetMutableDefault<UNonProjectFilesInfo>();
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
	const auto ProjectCleanerBrowserPtr = SAssignNew(ProjectCleanerBrowserUI, SProjectCleanerBrowser)
		.DirectoryFilterSettings(DirectoryFilterSettings)
		.NonProjectFiles(NonUProjectFilesSettings)
		.UnusedAssets(UnusedAssetsUIContainerSettings);

	// const auto ProjectCleanerBrowserStatisticsUI = SAssignNew(ProjectCleanerBrowserStatisticsUI, SProjectCleanerBrowserStatisticsUI);
	FMargin CommonMargin = FMargin{40.0f,20.0f};
	
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(SSplitter)
			+SSplitter::Slot()
			.Value(0.3f)			
			[
				SAssignNew(ProjectCleanerBrowserStatisticsUI, SProjectCleanerBrowserStatisticsUI)
				.UnusedAssets(CleaningStats.UnusedAssetsNum)
				.TotalSize(CleaningStats.UnusedAssetsTotalSize)
				.EmptyFolders(CleaningStats.EmptyFolders)
			]
			+SSplitter::Slot()
			.Value(0.7f)
			[
				SAssignNew(ProjectCleanerBrowserNonProjectFilesUI, SProjectCleanerBrowserNonProjectFilesUI)
				.NonProjectFiles(NonProjectFilesInfo)
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

	bool bFailedWhileDeletingAsset = false;
	while (UnusedAssets.Num() > 0 && !bFailedWhileDeletingAsset)
	{
		ProjectCleanerUtility::GetRootAssets(RootAssets, UnusedAssets);

		if (RootAssets.Num() == 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("Error while deleting assets! Try to restart editor and try again."));
			// todo:ashe23
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

TSharedPtr<SWidget> FProjectCleanerModule::OnGetAssetContextMenu(const TArray<FAssetData>& SelectedAssets)
{
	FMenuBuilder MenuBuilder(true, PluginCommands);

	MenuBuilder.BeginSection(TEXT("Asset"), NSLOCTEXT("ReferenceViewerSchema", "AssetSectionLabel", "Asset"));
	{
		MenuBuilder.AddMenuEntry(FGlobalEditorCommonCommands::Get().FindInContentBrowser);
		// MenuBuilder.AddMenuEntry(FAssetManagerEditorCommands::Get().OpenSelectedInAssetEditor);
	}
	MenuBuilder.EndSection();

	// MenuBuilder.BeginSection(TEXT("References"), NSLOCTEXT("ReferenceViewerSchema", "ReferencesSectionLabel", "References"));
	// {
	// 	MenuBuilder.AddMenuEntry(FAssetManagerEditorCommands::Get().ViewReferences);
	// 	MenuBuilder.AddMenuEntry(FAssetManagerEditorCommands::Get().ViewSizeMap);
	// }
	// MenuBuilder.EndSection();

	return MenuBuilder.MakeWidget();
}

void FProjectCleanerModule::FindInContentBrowser() const
{
	UE_LOG(LogProjectCleaner, Warning, TEXT("Finding in content browser"));
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

	ProjectCleanerUtility::GetEmptyFoldersAndNonProjectFiles(EmptyFolders, NonProjectFiles);
	ProjectCleanerUtility::FindAllSourceFiles(SourceFiles);
	ProjectCleanerUtility::GetAllAssets(UnusedAssets);
	ProjectCleanerUtility::CreateAdjacencyList(UnusedAssets, AdjacencyList);

	NonUProjectFilesSettings->Files.Append(NonProjectFiles);

	TArray<FString> AbsEmptyFolders;
	AbsEmptyFolders.Reserve(EmptyFolders.Num());

	for (auto& Folder : EmptyFolders)
	{
		AbsEmptyFolders.Add(FPaths::ConvertRelativePathToFull(Folder));
	}

	NonUProjectFilesSettings->EmptyFolders = AbsEmptyFolders;
	TMap<FString, FString> T;
	T.Add(TEXT("AA"), TEXT("BB"));
	T.Add(TEXT("AAclass"), TEXT("BB.cpp"));
	NonUProjectFilesSettings->UsedSourceFiles = T;


	// filters
	Filter_NotUsedInAnyLevel NotUsedInAnyLevel;
	NotUsedInAnyLevel.Apply(UnusedAssets);

	Filter_ExcludedDirectories ExcludedDirectories{DirectoryFilterSettings, AdjacencyList};
	ExcludedDirectories.Apply(UnusedAssets);

	Filter_UsedInSourceCode UsedInSourceCode{SourceFiles, AdjacencyList};
	UsedInSourceCode.Apply(UnusedAssets);

	UnusedAssetsUIContainerSettings->UnusedAssets = &UnusedAssets;
	NonProjectFilesInfo->Files = NonProjectFiles;
	NonProjectFilesInfo->EmptyFolders = EmptyFolders;
	
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
	NonUProjectFilesSettings->Files.Reset();
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

FString FProjectCleanerModule::GetStringValueForCustomColumn(FAssetData& AssetData, FName ColumnName) const
{
	FString OutValue;
	IAssetManagerEditorModule::Get().GetStringValueForCustomColumn(AssetData, ColumnName, OutValue);
	return OutValue;
}

FText FProjectCleanerModule::GetDisplayTextForCustomColumn(FAssetData& AssetData, FName ColumnName) const
{
	FText OutValue;
	IAssetManagerEditorModule::Get().GetDisplayTextForCustomColumn(AssetData, ColumnName, OutValue);
	return OutValue;
}
#pragma optimize("", on)
#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FProjectCleanerModule, ProjectCleaner)
