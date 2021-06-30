// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#include "UI/ProjectCleanerMainUI.h"
#include "UI/ProjectCleanerStyle.h"
#include "UI/ProjectCleanerBrowserStatisticsUI.h"
#include "UI/ProjectCleanerUnusedAssetsBrowserUI.h"
#include "UI/ProjectCleanerNonEngineFilesUI.h"
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
									// .OnClicked_Raw(this, &FProjectCleanerModule::OnRefreshBtnClick)
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
							// + SVerticalBox::Slot()
							// .Padding(FMargin{20.0f, 5.0f})
							// .AutoHeight()
							// [
							// 	SAssignNew(CleanerConfigsUI, SProjectCleanerConfigsUI)
							// 	// .CleanerConfigs(CleanerConfigs)
							// ]
							// + SVerticalBox::Slot()
							// .Padding(FMargin{20.0f, 5.0f})
							// .AutoHeight()
							// [
							// 	SAssignNew(ExcludeOptionUI, SProjectCleanerExcludeOptionsUI)
							// 	// .ExcludeOptions(ExcludeOptions)
							// ]
						]
					]
					// + SScrollBox::Slot()
					// .Padding(FMargin{0.0f, 20.0f})
					// [
					// 	SNew(SBorder)
					// 	.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
					// 	[
					// 		SNew(SVerticalBox)
					// 		+ SVerticalBox::Slot()
					// 		.Padding(FMargin{20.0f, 10.0f})
					// 		.AutoHeight()
					// 		[
					// 			ExcludedAssetsUIRef
					// 		]
					// 	]
					// ]
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
	// this is for TabManager initialization only
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

TSharedRef<SDockTab> SProjectCleanerMainUI::OnUnusedAssetTabSpawn(const FSpawnTabArgs& SpawnTabArgs)
{
	return SNew(SDockTab).TabRole(ETabRole::PanelTab);
}

TSharedRef<SDockTab> SProjectCleanerMainUI::OnNonEngineFilesTabSpawn(const FSpawnTabArgs& SpawnTabArgs)
{
	return SNew(SDockTab).TabRole(ETabRole::PanelTab);
}

TSharedRef<SDockTab> SProjectCleanerMainUI::OnCorruptedFilesTabSpawn(const FSpawnTabArgs& SpawnTabArgs)
{
	return SNew(SDockTab).TabRole(ETabRole::PanelTab);
}

TSharedRef<SDockTab> SProjectCleanerMainUI::OnIndirectAssetsTabSpawn(const FSpawnTabArgs& SpawnTabArgs)
{
	return SNew(SDockTab).TabRole(ETabRole::PanelTab);
}
