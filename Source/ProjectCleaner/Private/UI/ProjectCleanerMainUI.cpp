// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#include "UI/ProjectCleanerMainUI.h"
#include "UI/ProjectCleanerStyle.h"
#include "UI/ProjectCleanerBrowserStatisticsUI.h"
#include "UI/ProjectCleanerUnusedAssetsBrowserUI.h"
#include "UI/ProjectCleanerNonEngineFilesUI.h"
#include "UI/ProjectCleanerConfigsUI.h"
#include "UI/ProjectCleanerCorruptedFilesUI.h"
#include "UI/ProjectCleanerExcludeOptionsUI.h"
#include "UI/ProjectCleanerIndirectAssetsUI.h"
#include "UI/ProjectCleanerExcludedAssetsUI.h"
// Engine Headers
#include "ToolMenus.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Framework/Docking/TabManager.h"

static const FName UnusedAssetsTab = FName{ TEXT("UnusedAssetsTab") };
static const FName IndirectAssetsTab = FName{ TEXT("IndirectAssetsTab") };
static const FName NonEngineFilesTab = FName{ TEXT("NonEngineFilesTab") };
static const FName CorruptedFilesTab = FName{ TEXT("CorruptedFilesTab") };

void SProjectCleanerMainUI::Construct(const FArguments& InArgs)
{
	InitTabs();

	CleanerManager.UpdateData();

	ensure(TabManager.IsValid());

	const TSharedRef<SWidget> TabContents = TabManager->RestoreFrom(
		TabLayout.ToSharedRef(),
		TSharedPtr<SWindow>()
	).ToSharedRef();

	const auto ExcludedAssetsUIRef = SAssignNew(ExcludedAssetsUI, SProjectCleanerExcludedAssetsUI)
		.ExcludedAssets(&CleanerManager.GetCleanerData()->ExcludedAssets)
		.LinkedAssets(&CleanerManager.GetCleanerData()->LinkedAssets)
		.CleanerConfigs(CleanerManager.GetCleanerConfigs());
	
	ExcludedAssetsUIRef->OnUserIncludedAssets = FOnUserIncludedAsset::CreateRaw(
		this,
		&SProjectCleanerMainUI::OnUserIncludedAssets
	);
	
	ChildSlot
	[
		SNew(SBorder)
		[
			SNew(SSplitter)
			+ SSplitter::Slot()
			.Value(0.35f)
			[
				SNew(SOverlay)
				+ SOverlay::Slot()
				.Padding(FMargin{ 20.0f })
				[
					SNew(SScrollBox)
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
								SAssignNew(StatisticsUI, SProjectCleanerBrowserStatisticsUI)
								.CleanerData(CleanerManager.GetCleanerData())
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
									// .OnClicked_Raw(this, &FProjectCleanerModule::OnDeleteUnusedAssetsBtnClick)
								]
								+ SHorizontalBox::Slot()
								.FillWidth(1.0f)
								[
									SNew(SButton)
									.HAlign(HAlign_Center)
									.VAlign(VAlign_Center)
									.Text(FText::FromString("Delete Empty Folders"))
									// .OnClicked_Raw(this, &FProjectCleanerModule::OnDeleteEmptyFolderClick)
								]
							]
							+ SVerticalBox::Slot()
							.Padding(FMargin{20.0f, 5.0f})
							.AutoHeight()
							[
								SAssignNew(CleanerConfigsUI, SProjectCleanerConfigsUI)
								.CleanerConfigs(CleanerManager.GetCleanerConfigs())
							]
							+ SVerticalBox::Slot()
							.Padding(FMargin{20.0f, 5.0f})
							.AutoHeight()
							[
								SAssignNew(ExcludeOptionUI, SProjectCleanerExcludeOptionsUI)
								.ExcludeOptions(CleanerManager.GetExcludeOptions())
							]
						]
					]
					+ SScrollBox::Slot()
					.Padding(FMargin{0.0f, 20.0f})
					[
						SNew(SBorder)
						.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot()
							.Padding(FMargin{20.0f, 10.0f})
							.AutoHeight()
							[
								ExcludedAssetsUIRef
							]
						]
					]
				]
			]
			+ SSplitter::Slot()
			.Value(0.65f)
			[
				SNew(SOverlay)
				+ SOverlay::Slot()
				.Padding(FMargin{20.0f})
				[
					TabContents
				]
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
			->SetHideTabWell(true)
			->AddTab(UnusedAssetsTab, ETabState::OpenedTab)
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

void SProjectCleanerMainUI::Update()
{
	CleanerManager.UpdateData();

	if (StatisticsUI.IsValid())
	{
		StatisticsUI.Pin()->SetData(*CleanerManager.GetCleanerData());
	}

	if (UnusedAssetsBrowserUI.IsValid())
	{
		UnusedAssetsBrowserUI.Pin()->SetUIData(
			CleanerManager.GetCleanerData()->UnusedAssets,
			CleanerManager.GetCleanerConfigs(),
			CleanerManager.GetCleanerData()->PrimaryAssetClasses
		);
	}

	if (NonEngineFilesUI.IsValid())
	{
		NonEngineFilesUI.Pin()->SetNonEngineFiles(CleanerManager.GetCleanerData()->NonEngineFiles);
	}

	if (CorruptedFilesUI.IsValid())
	{
		CorruptedFilesUI.Pin()->SetCorruptedFiles(CleanerManager.GetCleanerData()->CorruptedFiles);
	}

	if (IndirectAssetsUI.IsValid())
	{
		IndirectAssetsUI.Pin()->SetIndirectFiles(CleanerManager.GetCleanerData()->IndirectFileInfos);
	}

	if (ExcludedAssetsUI.IsValid())
	{
		ExcludedAssetsUI.Pin()->SetUIData(
			CleanerManager.GetCleanerData()->ExcludedAssets,
			CleanerManager.GetCleanerData()->LinkedAssets,
			CleanerManager.GetCleanerConfigs()
		);
	}
}

TSharedRef<SDockTab> SProjectCleanerMainUI::OnUnusedAssetTabSpawn(const FSpawnTabArgs& SpawnTabArgs)
{
	const auto UnusedAssetsUIRef =
		SAssignNew(UnusedAssetsBrowserUI, SProjectCleanerUnusedAssetsBrowserUI)
		.PrimaryAssetClasses(&CleanerManager.GetCleanerData()->PrimaryAssetClasses)
		.UnusedAssets(&CleanerManager.GetCleanerData()->UnusedAssets)
		.CleanerConfigs(CleanerManager.GetCleanerConfigs());
	
	UnusedAssetsUIRef->OnUserDeletedAssets = FOnUserDeletedAssets::CreateRaw(
		this,
		&SProjectCleanerMainUI::OnUserDeletedAssets
	);
	
	UnusedAssetsUIRef->OnUserExcludedAssets = FOnUserExcludedAssets::CreateRaw(
		this,
		&SProjectCleanerMainUI::OnUserExcludedAssets
	);
	
	return SNew(SDockTab)
		.TabRole(ETabRole::PanelTab)
		.OnCanCloseTab_Lambda([] {return false; })
		.Label(NSLOCTEXT("UnusedAssetsTab", "TabTitle", "Unused Assets"))
		[
			UnusedAssetsUIRef
		];
}

TSharedRef<SDockTab> SProjectCleanerMainUI::OnNonEngineFilesTabSpawn(const FSpawnTabArgs& SpawnTabArgs)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::PanelTab)
		.OnCanCloseTab_Lambda([] {return false; })
		.Label(NSLOCTEXT("NonEngineFilesTab", "TabTitle", "Non Engine Files"))
		[
			SAssignNew(NonEngineFilesUI, SProjectCleanerNonEngineFilesUI)
			.NonEngineFiles(&CleanerManager.GetCleanerData()->NonEngineFiles)
		];
}

TSharedRef<SDockTab> SProjectCleanerMainUI::OnCorruptedFilesTabSpawn(const FSpawnTabArgs& SpawnTabArgs)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::PanelTab)
		.OnCanCloseTab_Lambda([] {return false; })
		.Label(NSLOCTEXT("CorruptedFilesTab", "TabTitle", "Corrupted Files"))
		[
			SAssignNew(CorruptedFilesUI, SProjectCleanerCorruptedFilesUI)
			.CorruptedFiles(&CleanerManager.GetCleanerData()->CorruptedFiles)
		];

}

TSharedRef<SDockTab> SProjectCleanerMainUI::OnIndirectAssetsTabSpawn(const FSpawnTabArgs& SpawnTabArgs)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::PanelTab)
		.OnCanCloseTab_Lambda([] {return false; })
		.Label(NSLOCTEXT("IndirectAssetsTab", "TabTitle", "Assets Used Indirectly"))
		[
			SAssignNew(IndirectAssetsUI, SProjectCleanerIndirectAssetsUI)
			.IndirectFileInfos(&CleanerManager.GetCleanerData()->IndirectFileInfos)
		];
}

void SProjectCleanerMainUI::OnUserDeletedAssets()
{
	// todo
}

void SProjectCleanerMainUI::OnUserExcludedAssets(const TArray<FAssetData>& Assets)
{
	if (!Assets.Num()) return;
	
	for (const auto& Asset : Assets)
	{
		CleanerManager.GetCleanerData()->UserExcludedAssets.AddUnique(Asset);
	}
	
	Update();
}

void SProjectCleanerMainUI::OnUserIncludedAssets(const TArray<FAssetData>& Assets, const bool CleanFilters)
{
	// todo
}

FReply SProjectCleanerMainUI::OnRefreshBtnClick()
{
	Update();
	
	return FReply::Handled();
}

