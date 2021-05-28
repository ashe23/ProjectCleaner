// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#include "ProjectCleaner.h"
#include "ProjectCleanerStyle.h"
#include "ProjectCleanerCommands.h"
#include "ProjectCleanerNotificationManager.h"
#include "ProjectCleanerUtility.h"
#include "UI/ProjectCleanerBrowserStatisticsUI.h"
#include "UI/ProjectCleanerDirectoryExclusionUI.h"
#include "UI/ProjectCleanerUnusedAssetsBrowserUI.h"
#include "UI/ProjectCleanerNonUassetFilesUI.h"
#include "UI/ProjectCleanerSourceCodeAssetsUI.h"
#include "UI/ProjectCleanerCorruptedFilesUI.h"
#include "UI/ProjectCleanerExcludedAssetsUI.h"
// Engine Headers
#include "AssetRegistryModule.h"
#include "ToolMenus.h"
#include "ContentBrowserModule.h"
#include "Misc/MessageDialog.h"
#include "LevelEditor.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SScrollBox.h"
#include "EditorStyleSet.h"
#include "IContentBrowserSingleton.h"
#include "GenericPlatform/GenericPlatformMisc.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Engine/AssetManager.h"
#include "Engine/MapBuildDataRegistry.h"
#include "Framework/Commands/UICommandList.h"
#include "Misc/ScopedSlowTask.h"
#include "Widgets/SWeakWidget.h"

DEFINE_LOG_CATEGORY(LogProjectCleaner);

static const FName ProjectCleanerTabName("ProjectCleaner");

#define LOCTEXT_NAMESPACE "FProjectCleanerModule"

FProjectCleanerModule::FProjectCleanerModule() :
	ExcludeDirectoryFilterSettings(nullptr),
	AssetRegistry(nullptr)
{
}

void FProjectCleanerModule::StartupModule()
{
	// initializing styles
	FProjectCleanerStyle::Initialize();
	FProjectCleanerStyle::ReloadTextures();
	FProjectCleanerCommands::Register();

	// Registering plugin commands
	PluginCommands = MakeShareable(new FUICommandList);
	PluginCommands->MapAction(
		FProjectCleanerCommands::Get().OpenCleanerWindow,
		FExecuteAction::CreateRaw(this, &FProjectCleanerModule::PluginButtonClicked),
		FCanExecuteAction()
	);

	UToolMenus::RegisterStartupCallback(
		FSimpleMulticastDelegate::FDelegate::CreateRaw(
			this,
			&FProjectCleanerModule::RegisterMenus
		)
	);

	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
		ProjectCleanerTabName,
		FOnSpawnTab::CreateRaw(this, &FProjectCleanerModule::OnSpawnPluginTab))
		.SetDisplayName(LOCTEXT("FProjectCleanerTabTitle", "ProjectCleaner"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);

	// initializing some objects
	NotificationManager = MakeShared<ProjectCleanerNotificationManager>();
	ExcludeDirectoryFilterSettings = GetMutableDefault<UExcludeDirectoriesFilterSettings>();
	AssetRegistry = &FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	if (AssetRegistry)
	{
		AssetRegistry->Get().OnFilesLoaded().AddRaw(this, &FProjectCleanerModule::OnFilesLoaded);
	}
}

void FProjectCleanerModule::ShutdownModule()
{
	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(this);
	FProjectCleanerStyle::Shutdown();
	FProjectCleanerCommands::Unregister();
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(ProjectCleanerTabName);
	AssetRegistry = nullptr;
}

bool FProjectCleanerModule::IsGameModule() const
{
	return false;
}

void FProjectCleanerModule::RegisterMenus()
{
	FToolMenuOwnerScoped OwnerScoped(this);

	UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
	FToolMenuSection& Section = Menu->FindOrAddSection("WindowLayout");
	Section.AddMenuEntryWithCommandList(FProjectCleanerCommands::Get().OpenCleanerWindow, PluginCommands);

	UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar");
	FToolMenuSection& ToolbarSection = ToolbarMenu->FindOrAddSection("Settings");
	FToolMenuEntry& Entry = ToolbarSection.AddEntry(
		FToolMenuEntry::InitToolBarButton(FProjectCleanerCommands::Get().OpenCleanerWindow)
	);
	Entry.SetCommandList(PluginCommands);
}

void FProjectCleanerModule::PluginButtonClicked()
{
	if (!bCanOpenTab)
	{
		if (!NotificationManager) return;
		NotificationManager->AddTransient(
			TEXT("Asset Registry still working! Please wait..."),
			SNotificationItem::CS_Fail,
			3.0f
		);
		return;
	}

	FGlobalTabmanager::Get()->TryInvokeTab(ProjectCleanerTabName);
}

TSharedRef<SDockTab> FProjectCleanerModule::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
	UpdateCleaner();

	const FMargin CommonMargin = FMargin{ 20.0f, 20.0f };

	const auto UnusedAssetsUIRef = SAssignNew(UnusedAssetsBrowserUI, SProjectCleanerUnusedAssetsBrowserUI)
		.UnusedAssets(UnusedAssets);
	UnusedAssetsUIRef->OnUserDeletedAssets = FOnUserDeletedAssets::CreateRaw(
		this,
		&FProjectCleanerModule::OnUserDeletedAssets
	);

	UnusedAssetsUIRef->OnUserExcludedAssets = FOnUserExcludedAssets::CreateRaw(
		this,
		&FProjectCleanerModule::OnUserExcludedAssets
	);

	const auto ExcludedAssetsUIRef = SAssignNew(ExcludedAssetsUI, SProjectCleanerExcludedAssetsUI)
		.ExcludedAssets(ExcludedAssets);
	ExcludedAssetsUIRef->OnUserIncludedAssets = FOnUserIncludedAsset::CreateRaw(
		this,
		&FProjectCleanerModule::OnUserIncludedAssets
	);

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
		.OnClicked_Raw(this, &FProjectCleanerModule::OnRefreshBtnClick)
		]
	+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		.Padding(FMargin{ 40.0f, 0.0f, 40.0f, 0.0f })
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
		.Padding(FMargin(5))
		.AutoHeight()
		[
			ExcludedAssetsUIRef
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
			.NonUassetFiles(NonUAssetFiles)
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
			.SourceCodeAssets(SourceCodeAssets)
		]
		]
	+ SScrollBox::Slot()
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
		.Padding(CommonMargin)
		.AutoHeight()
		[
			SAssignNew(CorruptedFilesUI, SProjectCleanerCorruptedFilesUI)
			.CorruptedFiles(CorruptedFiles)
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
			UnusedAssetsUIRef
		]
		]
		]
		];
}

void FProjectCleanerModule::UpdateCleaner()
{
	ProjectCleanerUtility::SaveAllAssets();

	ProjectCleanerUtility::FixupRedirectors();

	UpdateCleanerData();
}

void FProjectCleanerModule::UpdateCleanerData()
{
	Reset();

	if (!AssetRegistry) return;

	ProjectCleanerUtility::GetEmptyFolders(EmptyFolders);
	ProjectCleanerUtility::GetAllProjectFiles(AllProjectFiles);
	ProjectCleanerUtility::GetInvalidProjectFiles(AssetRegistry, AllProjectFiles, CorruptedFiles, NonUAssetFiles);

	UAssetManager& AssetManager = UAssetManager::Get();
	ProjectCleanerUtility::GetAllPrimaryAssetClasses(AssetManager, PrimaryAssetClasses);
	ProjectCleanerUtility::GetAllAssets(AssetRegistry, UnusedAssets);
	ProjectCleanerUtility::RemovePrimaryAssets(UnusedAssets, PrimaryAssetClasses);
	ProjectCleanerUtility::RemoveUsedAssets(UnusedAssets, PrimaryAssetClasses);
	ProjectCleanerUtility::RemoveMegascansPluginAssetsIfActive(UnusedAssets);

	// filling graphs with unused assets data and creating relational map between them
	RelationalMap.Fill(UnusedAssets);

	ProjectCleanerUtility::RemoveAssetsUsedIndirectly(UnusedAssets, RelationalMap, SourceCodeAssets);

	//ProjectCleanerUtility::RemoveAssetsExcludedByUser(
	//	AssetRegistry,
	//	UnusedAssets,
	//	ExcludedAssets,
	//	RelationalMap,
	//	ExcludeDirectoryFilterSettings
	//);

	// ProjectCleanerUtility::RemoveAssetsWithExternalDependencies(UnusedAssets, AdjacencyList);
	// ProjectCleanerUtility::RemoveAssetsUsedInSourceCode(UnusedAssets, AdjacencyList, SourceFiles, SourceCodeAssets);
	// ProjectCleanerUtility::RemoveAssetsExcludedByUser(UnusedAssets, AdjacencyList, ExcludeDirectoryFilterSettings);

	UpdateStats();

	UE_LOG(LogProjectCleaner, Warning, TEXT("a"));
}

void FProjectCleanerModule::UpdateStats()
{
	CleaningStats.Reset();

	CleaningStats.UnusedAssetsNum = UnusedAssets.Num();
	CleaningStats.EmptyFolders = EmptyFolders.Num();
	CleaningStats.NonUassetFilesNum = NonUAssetFiles.Num();
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
		NonUassetFilesUI.Pin()->SetNonUassetFiles(NonUAssetFiles);
	}

	if (SourceCodeAssetsUI.IsValid())
	{
		SourceCodeAssetsUI.Pin()->SetSourceCodeAssets(SourceCodeAssets);
	}

	if (ExcludedAssetsUI.IsValid())
	{
		ExcludedAssetsUI.Pin()->SetExcludedAssets(ExcludedAssets);
	}

	if (CorruptedFilesUI.IsValid())
	{
		CorruptedFilesUI.Pin()->SetCorruptedFiles(CorruptedFiles);
	}
}

void FProjectCleanerModule::Reset()
{
	UnusedAssets.Reset();
	NonUAssetFiles.Reset();
	SourceCodeAssets.Reset();
	CorruptedFiles.Reset();
	EmptyFolders.Reset();
	AllProjectFiles.Reset();
	ExcludedAssets.Reset();
}

void FProjectCleanerModule::UpdateContentBrowser() const
{
	TArray<FString> FocusFolders;
	FocusFolders.Add("/Game");

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::GetModuleChecked<FAssetRegistryModule>("AssetRegistry");
	AssetRegistryModule.Get().ScanPathsSynchronous(FocusFolders, true);
	AssetRegistryModule.Get().SearchAllAssets(true);

	FContentBrowserModule& CBModule = FModuleManager::Get().GetModuleChecked<FContentBrowserModule>("ContentBrowser");
	CBModule.Get().SetSelectedPaths(FocusFolders, true);
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

void FProjectCleanerModule::OnUserDeletedAssets()
{
	UpdateCleaner();
}

void FProjectCleanerModule::OnUserExcludedAssets(const TArray<FAssetData>& Assets)
{
	if (!Assets.Num()) return;
	
	/*TArray<FAssetData> FilteredAssets;
	FilteredAssets.Reserve(Assets.Num());

	for (const auto& Asset : Assets)
	{
		ExcludedAssets.Add(Asset);
		const auto Data = RelationalMap.FindByPackageName(Asset.PackageName);
		if (!Data) continue;
		for (const auto& LinkedAsset : Data->LinkedAssetsData)
		{
			ExcludedAssets.Add(*LinkedAsset);
		}
	}*/
	
	UpdateCleanerData();
}

void FProjectCleanerModule::OnUserIncludedAssets(const TArray<FAssetData>& Assets)
{
	if (!Assets.Num()) return;

	/*TArray<FAssetData> FilteredAssets;
	FilteredAssets.Reserve(Assets.Num());

	for (const auto& Asset : Assets)
	{
		ExcludedAssets.Remove(Asset);
		const auto Data = RelationalMap.FindByPackageName(Asset.PackageName);
		if (!Data) continue;
		for (const auto& LinkedAsset : Data->LinkedAssetsData)
		{
			ExcludedAssets.Remove(*LinkedAsset);
		}
	}*/

	UpdateCleanerData();
}

FReply FProjectCleanerModule::OnRefreshBtnClick()
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

	const auto CleaningNotificationPtr = NotificationManager->Add(
		StandardCleanerText.StartingCleanup.ToString(),
		SNotificationItem::ECompletionState::CS_Pending
	);

	while (UnusedAssets.Num() > 0)
	{
		//ProjectCleanerUtility::GetRootAssets(RootAssets, UnusedAssets, AdjacencyList);

		// Before we delete assets
		// Loading assets and find corrupted files
		// todo:ashe23 handle assets that failed to delete different way?
		for (const auto& Asset : RootAssets)
		{
			const auto LoadedAsset = Asset.GetAsset();
			if (!LoadedAsset)
			{
				CorruptedFiles.Add(Asset.PackageName);
			}
		}

		if (CorruptedFiles.Num() > 0)
		{
			UnusedAssets.RemoveAll([&](const FAssetData& Asset)
				{
					return CorruptedFiles.Contains(Asset.PackageName);
				});

			RootAssets.RemoveAll([&](const FAssetData& Asset)
				{
					return CorruptedFiles.Contains(Asset.PackageName);
				});
		}

		// Remaining assets are valid so we trying to delete them
		CleaningStats.DeletedAssetCount += ProjectCleanerUtility::DeleteAssets(RootAssets);
		NotificationManager->Update(CleaningNotificationPtr, CleaningStats);

		UnusedAssets.RemoveAll([&](const FAssetData& Elem)
			{
				return RootAssets.Contains(Elem);
			});

		// after chunk of assets deleted, we must update adjacency list
		//ProjectCleanerUtility::CreateAdjacencyList(UnusedAssets, AdjacencyList, true);
		RootAssets.Reset();
	}

	NotificationManager->Hide(
		CleaningNotificationPtr,
		FText::FromString(FString::Printf(TEXT("Deleted %d assets."), CleaningStats.DeletedAssetCount))
	);

	UpdateCleanerData();

	// Delete all empty folder
	ProjectCleanerUtility::DeleteEmptyFolders(EmptyFolders);
	const FString PostFixText = CleaningStats.EmptyFolders > 1 ? TEXT(" empty folders") : TEXT(" empty folder");
	const FString DisplayText = FString{ "Deleted " } + FString::FromInt(CleaningStats.EmptyFolders) + PostFixText;
	NotificationManager->AddTransient(
		DisplayText,
		SNotificationItem::ECompletionState::CS_Success,
		10.0f
	);

	UpdateCleanerData();
	UpdateContentBrowser();

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
	const FString DisplayText = FString{ "Deleted " } + FString::FromInt(CleaningStats.EmptyFolders) + PostFixText;
	NotificationManager->AddTransient(
		DisplayText,
		SNotificationItem::ECompletionState::CS_Success,
		5.0f
	);

	UpdateCleanerData();
	UpdateContentBrowser();

	return FReply::Handled();
}

void FProjectCleanerModule::OnFilesLoaded()
{
	bCanOpenTab = true;
}

void FProjectCleanerModule::OpenCorruptedFilesWindow()
{
	const auto CorruptedFilesWindow =
		SNew(SWindow)
		.Title(LOCTEXT("corrupted_files_window", "Corrupted Files"))
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

	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->AddViewportWidgetContent(SNew(SWeakWidget).PossiblyNullContent(CorruptedFilesWindow));
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FProjectCleanerModule, ProjectCleaner)