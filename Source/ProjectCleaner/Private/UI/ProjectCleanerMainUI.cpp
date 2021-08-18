// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#include "UI/ProjectCleanerMainUI.h"
#include "UI/ProjectCleanerStyle.h"
#include "UI/ProjectCleanerStatisticsUI.h"
#include "UI/ProjectCleanerUnusedAssetsBrowserUI.h"
#include "UI/ProjectCleanerNonEngineFilesUI.h"
#include "UI/ProjectCleanerConfigsUI.h"
#include "UI/ProjectCleanerCorruptedFilesUI.h"
#include "UI/ProjectCleanerIndirectAssetsUI.h"
#include "UI/ProjectCleanerExcludedAssetsUI.h"
#include "Core/ProjectCleanerManager.h"
#include "UI/ProjectCleanerNotificationManager.h"
// Engine Headers
#include "ToolMenus.h"
#include "Components/SlateWrapperTypes.h"
#include "Components/WidgetSwitcher.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Framework/Docking/TabManager.h"

static const FName UnusedAssetsTab = FName{ TEXT("UnusedAssetsTab") };
static const FName IndirectAssetsTab = FName{ TEXT("IndirectAssetsTab") };
static const FName NonEngineFilesTab = FName{ TEXT("NonEngineFilesTab") };
static const FName CorruptedFilesTab = FName{ TEXT("CorruptedFilesTab") };
static const FName ExcludedAssetsTab = FName{ TEXT("ExcludedAssetsTab") };

#define LOCTEXT_NAMESPACE "FProjectCleanerModule"

void SProjectCleanerMainUI::Construct(const FArguments& InArgs)
{
	if (InArgs._CleanerManager)
	{
		SetCleanerManager(InArgs._CleanerManager);
	}

	ensure(CleanerManager);
	CleanerManager->OnCleanerManagerUpdated = FOnCleanerManagerUpdated::CreateRaw(
		this,
		&SProjectCleanerMainUI::OnCleanerManagerUpdated
	);
	
	InitTabs();
	
	ensure(TabManager.IsValid());

	const TSharedRef<SWidget> TabContents = TabManager->RestoreFrom(
		TabLayout.ToSharedRef(),
		TSharedPtr<SWindow>()
	).ToSharedRef();
	
	ChildSlot
	[
		SNew(SWidgetSwitcher)
		.IsEnabled(this, &SProjectCleanerMainUI::IsWidgetEnabled)
		.WidgetIndex_Raw(this , &SProjectCleanerMainUI::GetDefaultWidgetIndex)
		+ SWidgetSwitcher::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.Padding(FMargin{ 20.0f })
			.FillHeight(1.0f)
			[
				SNew(SSplitter)
				.Style(FEditorStyle::Get(), "ContentBrowser.Splitter")
				.PhysicalSplitterHandleSize(5.0f)
				+ SSplitter::Slot()
				.Value(0.35f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SScrollBox)
						.ScrollWhenFocusChanges(EScrollWhenFocusChanges::AnimatedScroll)
						.AnimateWheelScrolling(true)
						.AllowOverscroll(EAllowOverscroll::No)
						+ SScrollBox::Slot()
						[
							SNew(SBorder)
							.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
							[
								SNew(SVerticalBox)
								+ SVerticalBox::Slot()
								.Padding(FMargin{ 20.0f, 20.0f })
								.AutoHeight()
								[
									SAssignNew(StatisticsUI, SProjectCleanerStatisticsUI)
									.CleanerManager(CleanerManager)
								]
								+ SVerticalBox::Slot()
								.Padding(FMargin{ 20.0f, 20.0f })
								.AutoHeight()
								[
									SNew(SHorizontalBox)
									+ SHorizontalBox::Slot()
									.FillWidth(1.0f)
									[
										SNew(SButton)
										.HAlign(HAlign_Center)
										.VAlign(VAlign_Center)
										.Text(FText::FromString("Refresh"))
										.OnClicked_Raw(this, &SProjectCleanerMainUI::OnRefreshBtnClick)
									]
									+ SHorizontalBox::Slot()
									.FillWidth(1.0f)
									.Padding(FMargin{ 40.0f, 0.0f, 40.0f, 0.0f })
									[
										SNew(SButton)
										.HAlign(HAlign_Center)
										.VAlign(VAlign_Center)
										.Text(FText::FromString("Delete Unused Assets"))
										.OnClicked_Raw(this, &SProjectCleanerMainUI::OnDeleteUnusedAssetsBtnClick)
									]
									+ SHorizontalBox::Slot()
									.FillWidth(1.0f)
									[
										SNew(SButton)
										.HAlign(HAlign_Center)
										.VAlign(VAlign_Center)
										.Text(FText::FromString("Delete Empty Folders"))
										.OnClicked_Raw(this, &SProjectCleanerMainUI::OnDeleteEmptyFolderClick)
									]
								]
								+ SVerticalBox::Slot()
								.Padding(FMargin{20.0f, 5.0f})
								.AutoHeight()
								[
									SAssignNew(CleanerConfigsUI, SProjectCleanerConfigsUI)
									.CleanerConfigs(CleanerManager->GetCleanerConfigs())
								]
							]
						]
					]
				]
				+ SSplitter::Slot()
				[
					TabContents
				]
			]
		]
		+ SWidgetSwitcher::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Justification(ETextJustify::Center)
				.Font(FProjectCleanerStyle::Get().GetFontStyle("ProjectCleaner.Font.Light30"))
				.Text(LOCTEXT("project_cleaner_asset_registry_still_working", "Asset registry still working. Please wait while scan completes"))
			]
		]
	];
}

SProjectCleanerMainUI::~SProjectCleanerMainUI()
{
	TabManager->UnregisterTabSpawner(UnusedAssetsTab);
	TabManager->UnregisterTabSpawner(IndirectAssetsTab);
	TabManager->UnregisterTabSpawner(NonEngineFilesTab);
	TabManager->UnregisterTabSpawner(CorruptedFilesTab);
	TabManager->UnregisterTabSpawner(ExcludedAssetsTab);
	CleanerManager->OnCleanerManagerUpdated.Unbind();
	CleanerManager = nullptr;
}

void SProjectCleanerMainUI::SetCleanerManager(FProjectCleanerManager* CleanerManagerPtr)
{
	if (!CleanerManagerPtr) return;
	CleanerManager = CleanerManagerPtr;
}

void SProjectCleanerMainUI::InitTabs()
{
	const auto DummyTab = SNew(SDockTab).TabRole(ETabRole::NomadTab);
	TabManager = FGlobalTabmanager::Get()->NewTabManager(DummyTab);
	TabManager->SetCanDoDragOperation(false);
	TabLayout = FTabManager::NewLayout("ProjectCleanerTabLayout")
	->AddArea
	(
		FTabManager::NewPrimaryArea()
		->SetOrientation(Orient_Vertical)
		->Split
		(
			FTabManager::NewStack()
			->SetSizeCoefficient(0.4f)
			->AddTab(UnusedAssetsTab, ETabState::OpenedTab)
			->AddTab(ExcludedAssetsTab, ETabState::OpenedTab)
			->AddTab(IndirectAssetsTab, ETabState::OpenedTab)
			->AddTab(NonEngineFilesTab, ETabState::OpenedTab)
			->AddTab(CorruptedFilesTab, ETabState::OpenedTab)
			->SetForegroundTab(UnusedAssetsTab)
		)
	);
	
	TabManager->RegisterTabSpawner(UnusedAssetsTab, FOnSpawnTab::CreateRaw(
		this,
		&SProjectCleanerMainUI::OnUnusedAssetTabSpawn)
	);
	TabManager->RegisterTabSpawner(ExcludedAssetsTab, FOnSpawnTab::CreateRaw(
		this,
		&SProjectCleanerMainUI::OnExcludedAssetsTabSpawn)
	);
	TabManager->RegisterTabSpawner(IndirectAssetsTab, FOnSpawnTab::CreateRaw(
		this,
		&SProjectCleanerMainUI::OnIndirectAssetsTabSpawn)
	);
	TabManager->RegisterTabSpawner(NonEngineFilesTab, FOnSpawnTab::CreateRaw(
		this,
		&SProjectCleanerMainUI::OnNonEngineFilesTabSpawn)
	);
	TabManager->RegisterTabSpawner(CorruptedFilesTab, FOnSpawnTab::CreateRaw(
		this,
		&SProjectCleanerMainUI::OnCorruptedFilesTabSpawn)
	);
}

void SProjectCleanerMainUI::OnCleanerManagerUpdated() const
{
	if (UnusedAssetsBrowserUI.IsValid())
	{
		UnusedAssetsBrowserUI.Pin()->UpdateUI();
	}

	if (ExcludedAssetsUI.IsValid())
	{
		ExcludedAssetsUI.Pin()->UpdateUI();
	}

	if (IndirectAssetsUI.IsValid())
	{
		IndirectAssetsUI.Pin()->UpdateUI();
	}

	if (NonEngineFilesUI.IsValid())
	{
		NonEngineFilesUI.Pin()->UpdateUI();
	}

	if (CorruptedFilesUI.IsValid())
	{
		CorruptedFilesUI.Pin()->UpdateUI();
	}
}

bool SProjectCleanerMainUI::IsWidgetEnabled() const
{
	return !CleanerManager->GetDataManager().IsLoadingAssets();
}

int32 SProjectCleanerMainUI::GetDefaultWidgetIndex() const
{
	if (CleanerManager->GetDataManager().IsLoadingAssets())
	{
		return 1;
	}

	return 0;
}

TSharedRef<SDockTab> SProjectCleanerMainUI::OnUnusedAssetTabSpawn(const FSpawnTabArgs& SpawnTabArgs)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		.Label(NSLOCTEXT("UnusedAssetsTab", "TabTitle", "Unused Assets"))
		.ShouldAutosize(true)
		[
			SNew(SOverlay)
			+ SOverlay::Slot()
			.Padding(20.0f)
			[
				SAssignNew(UnusedAssetsBrowserUI, SProjectCleanerUnusedAssetsBrowserUI)
				.CleanerManager(CleanerManager)
			]
		];
}

TSharedRef<SDockTab> SProjectCleanerMainUI::OnExcludedAssetsTabSpawn(const FSpawnTabArgs& SpawnTabArgs)
{
	return SNew(SDockTab)		
		.TabRole(ETabRole::NomadTab)
		.Label(NSLOCTEXT("ExcludedAssetsTab", "TabTitle", "Excluded Assets"))
		.ShouldAutosize(true)
		[
			SNew(SOverlay)
			+ SOverlay::Slot()
			.Padding(20.0f)
			[
				SAssignNew(ExcludedAssetsUI, SProjectCleanerExcludedAssetsUI)
				.CleanerManager(CleanerManager)
			]
		];
}

TSharedRef<SDockTab> SProjectCleanerMainUI::OnNonEngineFilesTabSpawn(const FSpawnTabArgs& SpawnTabArgs)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		.Label(NSLOCTEXT("NonEngineFilesTab", "TabTitle", "Non Engine Files"))
		[
			SAssignNew(NonEngineFilesUI, SProjectCleanerNonEngineFilesUI)
			.CleanerManager(CleanerManager)
		];
}

TSharedRef<SDockTab> SProjectCleanerMainUI::OnCorruptedFilesTabSpawn(const FSpawnTabArgs& SpawnTabArgs)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		.Label(NSLOCTEXT("CorruptedAssetsTab", "TabTitle", "Corrupted Assets"))
		[
			SAssignNew(CorruptedFilesUI, SProjectCleanerCorruptedFilesUI)
			.CleanerManager(CleanerManager)
		];
}

TSharedRef<SDockTab> SProjectCleanerMainUI::OnIndirectAssetsTabSpawn(const FSpawnTabArgs& SpawnTabArgs)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		.Label(NSLOCTEXT("IndirectAssetsTab", "TabTitle", "Indirectly used assets"))
		[
			SAssignNew(IndirectAssetsUI, SProjectCleanerIndirectAssetsUI)
			.CleanerManager(CleanerManager)
		];
}

FReply SProjectCleanerMainUI::OnRefreshBtnClick() const
{
	CleanerManager->Update();

	return FReply::Handled();
}

FReply SProjectCleanerMainUI::OnDeleteUnusedAssetsBtnClick() const
{
	if (CleanerManager->GetUnusedAssets().Num() == 0)
	{
		ProjectCleanerNotificationManager::AddTransient(
			FText::FromString(FStandardCleanerText::NoAssetsToDelete),
			SNotificationItem::ECompletionState::CS_Fail,
			3.0f
		);
	
		return FReply::Handled();
	}
	
	const auto ConfirmationWindowStatus = ProjectCleanerNotificationManager::ShowConfirmationWindow(
		FText::FromString(FStandardCleanerText::AssetsDeleteWindowTitle),
		FText::FromString(FStandardCleanerText::AssetsDeleteWindowContent)
	);
	if (ProjectCleanerNotificationManager::IsConfirmationWindowCanceled(ConfirmationWindowStatus))
	{
		return FReply::Handled();
	}
	
	CleanerManager->DeleteAllUnusedAssets();
	
	return FReply::Handled();
}

FReply SProjectCleanerMainUI::OnDeleteEmptyFolderClick() const
{
	if (CleanerManager->GetEmptyFolders().Num() == 0)
	{
		ProjectCleanerNotificationManager::AddTransient(
			FText::FromString(FStandardCleanerText::NoEmptyFolderToDelete),
			SNotificationItem::ECompletionState::CS_Fail,
			3.0f
		);
		
		return FReply::Handled();
	}
	
	const auto ConfirmationWindowStatus = ProjectCleanerNotificationManager::ShowConfirmationWindow(
		FText::FromString(FStandardCleanerText::EmptyFolderWindowTitle),
		FText::FromString(FStandardCleanerText::EmptyFolderWindowContent)
	);
	if (ProjectCleanerNotificationManager::IsConfirmationWindowCanceled(ConfirmationWindowStatus))
	{
		return FReply::Handled();
	}
	
	CleanerManager->DeleteEmptyFolders();

	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE