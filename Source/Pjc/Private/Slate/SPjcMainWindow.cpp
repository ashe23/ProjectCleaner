﻿// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/SPjcMainWindow.h"
#include "PjcConstants.h"
#include "PjcStyles.h"
#include "PjcSubsystem.h"
#include "Slate/SPjcTabScanSettings.h"
// Engine Headers
#include "Widgets/Layout/SWidgetSwitcher.h"

void SPjcMainWindow::Construct(const FArguments& InArgs, const TSharedRef<SDockTab>& ConstructUnderMajorTab, const TSharedPtr<SWindow>& ConstructUnderWindow)
{
	TabManager = FGlobalTabmanager::Get()->NewTabManager(ConstructUnderMajorTab);
	const TSharedRef<FWorkspaceItem> AppMenuGroup = TabManager->AddLocalWorkspaceMenuCategory(FText::FromString(PjcConstants::ModuleName.ToString()));

	TabLayout = FTabManager::NewLayout("ProjectCleanerTabLayout")
		->AddArea
		(
			FTabManager::NewPrimaryArea()
			->SetOrientation(Orient_Horizontal)
			->Split
			(
				FTabManager::NewStack()
				->AddTab(PjcConstants::TabScanSettings, ETabState::OpenedTab)
				->SetForegroundTab(PjcConstants::TabScanSettings)
				->SetSizeCoefficient(0.2f)
			)
			->Split(
				FTabManager::NewStack()
				->AddTab(PjcConstants::TabScanInfo, ETabState::OpenedTab)
				// ->AddTab(PjcConstants::TabAssetsBrowser, ETabState::OpenedTab)
				// ->AddTab(PjcConstants::TabFilesNonEngine, ETabState::OpenedTab)
				// ->AddTab(PjcConstants::TabFilesCorrupted, ETabState::OpenedTab)
				->SetForegroundTab(PjcConstants::TabScanInfo)
				->SetSizeCoefficient(0.8f)
			)
		);

	TabManager->RegisterTabSpawner(PjcConstants::TabScanSettings, FOnSpawnTab::CreateRaw(this, &SPjcMainWindow::OnTabScanSettingsSpawn))
	          .SetTooltipText(FText::FromString(TEXT("Open Scan Settings Tab")))
	          .SetDisplayName(FText::FromString(TEXT("Scan Settings")))
	          .SetIcon(FPjcStyles::GetIcon("ProjectCleaner.IconSettings16"))
	          .SetGroup(AppMenuGroup);

	TabManager->RegisterTabSpawner(PjcConstants::TabScanInfo, FOnSpawnTab::CreateRaw(this, &SPjcMainWindow::OnTabScanInfoSpawn))
	          .SetTooltipText(FText::FromString(TEXT("Open Scan Info Tab")))
	          .SetDisplayName(FText::FromString(TEXT("Scan Info")))
	          .SetIcon(FPjcStyles::GetIcon("ProjectCleaner.IconTabAssetsBrowser16"))
	          .SetGroup(AppMenuGroup);
	//
	// TabManager->RegisterTabSpawner(PjcConstants::TabAssetsBrowser, FOnSpawnTab::CreateRaw(this, &SPjcMainWindow::OnTabAssetsBrowserSpawn))
	//           .SetTooltipText(FText::FromString(TEXT("Open Assets Browser Tab")))
	//           .SetDisplayName(FText::FromString(TEXT("Assets Browser")))
	//           .SetIcon(FPjcStyles::GetIcon("ProjectCleaner.IconTabAssetsBrowser16"))
	//           .SetGroup(AppMenuGroup);
	//
	// TabManager->RegisterTabSpawner(PjcConstants::TabFilesNonEngine, FOnSpawnTab::CreateRaw(this, &SPjcMainWindow::OnTabFilesNonEngineSpawn))
	//           .SetTooltipText(FText::FromString(TEXT("Open Non Engine Files Tab")))
	//           .SetDisplayName(FText::FromString(TEXT("Files NonEngine")))
	//           .SetIcon(FPjcStyles::GetIcon("ProjectCleaner.IconTabNonEngine16"))
	//           .SetGroup(AppMenuGroup);
	//
	// TabManager->RegisterTabSpawner(PjcConstants::TabFilesCorrupted, FOnSpawnTab::CreateRaw(this, &SPjcMainWindow::OnTabFilesCorruptedSpawn))
	//           .SetTooltipText(FText::FromString(TEXT("Open Corrupted Files Tab")))
	//           .SetDisplayName(FText::FromString(TEXT("Files Corrupted")))
	//           .SetIcon(FPjcStyles::GetIcon("ProjectCleaner.IconTabCorrupted16"))
	//           .SetGroup(AppMenuGroup);

	FMenuBarBuilder MenuBarBuilder = FMenuBarBuilder(TSharedPtr<FUICommandList>());
	MenuBarBuilder.AddPullDownMenu(
		FText::FromString(TEXT("Tabs")),
		FText::GetEmpty(),
		FNewMenuDelegate::CreateRaw(this, &SPjcMainWindow::CreateMenuBarTabs, TabManager),
		"Window"
	);

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			MenuBarBuilder.MakeWidget()
		]
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			TabManager->RestoreFrom(TabLayout.ToSharedRef(), ConstructUnderWindow).ToSharedRef()
		]
		// SNew(SWidgetSwitcher)
		// .IsEnabled_Raw(this, &SPjcMainWindow::WidgetEnabled)
		// .WidgetIndex_Raw(this, &SPjcMainWindow::WidgetGetIndex)
		// + SWidgetSwitcher::Slot()
		//   .HAlign(HAlign_Fill)
		//   .VAlign(VAlign_Fill)
		// [
		// 	SNew(SVerticalBox)
		// 	+ SVerticalBox::Slot()
		// 	.AutoHeight()
		// 	[
		// 		MenuBarBuilder.MakeWidget()
		// 	]
		// 	+ SVerticalBox::Slot()
		// 	.FillHeight(1.0f)
		// 	[
		// 		TabManager->RestoreFrom(TabLayout.ToSharedRef(), ConstructUnderWindow).ToSharedRef()
		// 	]
		// ]
		// + SWidgetSwitcher::Slot()
		//   .HAlign(HAlign_Fill)
		//   .VAlign(VAlign_Fill)
		// [
		// 	SNew(SHorizontalBox)
		// 	+ SHorizontalBox::Slot()
		// 	  .FillWidth(1.0f)
		// 	  .HAlign(HAlign_Center)
		// 	  .VAlign(VAlign_Center)
		// 	[
		// 		SNew(STextBlock)
		// 		.Justification(ETextJustify::Center)
		// 		.Font(FPjcStyles::GetFont("Light", 30))
		// 		.Text_Raw(this, &SPjcMainWindow::WidgetText)
		// 	]
		// ]
	];

	TabManager->SetMenuMultiBox(MenuBarBuilder.GetMultiBox());
}

SPjcMainWindow::~SPjcMainWindow()
{
	FGlobalTabmanager::Get()->UnregisterTabSpawner(PjcConstants::TabScanSettings);
	FGlobalTabmanager::Get()->UnregisterTabSpawner(PjcConstants::TabScanInfo);
}

// bool SPjcMainWindow::WidgetEnabled() const
// {
// 	return GEditor->GetEditorSubsystem<UPjcSubsystem>()->GetScannerState() == EPjcScannerState::Idle;
// }
//
// int32 SPjcMainWindow::WidgetGetIndex() const
// {
// 	return GEditor->GetEditorSubsystem<UPjcSubsystem>()->GetScannerState() == EPjcScannerState::Idle ? PjcConstants::WidgetIndexIdle : PjcConstants::WidgetIndexWorking;
// }
//
// FText SPjcMainWindow::WidgetText() const
// {
// 	return FText::FromString(GEditor->GetEditorSubsystem<UPjcSubsystem>()->GetScannerErrMsg());
// 	// if (UPjcSubsystem::EditorIsInPlayMode())
// 	// {
// 	// 	return FText::FromString(TEXT("Please stop play mode in the editor before doing any operations in the plugin."));
// 	// }
// 	//
// 	// if (UPjcSubsystem::AssetRegistryIsWorking())
// 	// {
// 	// 	return FText::FromString(TEXT("The AssetRegistry is still working. Please wait for the scan to finish"));
// 	// }
//
// 	// return FText::FromString(TEXT(""));
// }

void SPjcMainWindow::CreateMenuBarTabs(FMenuBuilder& MenuBuilder, const TSharedPtr<FTabManager> TabManagerPtr)
{
	if (!TabManagerPtr.IsValid()) return;

#if !WITH_EDITOR
	FGlobalTabmanager::Get()->PopulateTabSpawnerMenu(MenuBuilder, WorkspaceMenu::GetMenuStructure().GetStructureRoot());
#endif

	TabManagerPtr->PopulateLocalTabSpawnerMenu(MenuBuilder);
}

TSharedRef<SDockTab> SPjcMainWindow::OnTabScanSettingsSpawn(const FSpawnTabArgs& Args) const
{
	return
		SNew(SDockTab)
		.TabRole(PanelTab)
		.Label(FText::FromString(TEXT("Scan Settings")))
		.Icon(FPjcStyles::Get().GetBrush("ProjectCleaner.IconSettings16"))
		[
			SNew(SPjcTabScanSettings)
		];
}

TSharedRef<SDockTab> SPjcMainWindow::OnTabScanInfoSpawn(const FSpawnTabArgs& Args) const
{
	return
		SNew(SDockTab)
		.TabRole(PanelTab)
		.Label(FText::FromString(TEXT("Scan Info")))
		.Icon(FPjcStyles::Get().GetBrush("ProjectCleaner.IconTabAssetsBrowser16"))
		.ToolTipText(FText::FromString(TEXT("Show detailed view of unused, used and excluded assets")))
		[
			SNew(STextBlock)
			// TabScanInfoPtr.ToSharedRef()
		];
}

TSharedRef<SDockTab> SPjcMainWindow::OnTabAssetsBrowserSpawn(const FSpawnTabArgs& Args) const
{
	return
		SNew(SDockTab)
		.TabRole(PanelTab)
		.Label(FText::FromString(TEXT("Assets Browser")))
		.Icon(FPjcStyles::Get().GetBrush("ProjectCleaner.IconTabAssetsBrowser16"))
		[
			SNew(STextBlock)
			// TabAssetsBrowserPtr.ToSharedRef()
		];
}

TSharedRef<SDockTab> SPjcMainWindow::OnTabFilesNonEngineSpawn(const FSpawnTabArgs& Args) const
{
	return
		SNew(SDockTab)
		.TabRole(PanelTab)
		.Label(FText::FromString(TEXT("Files NonEngine")))
		.Icon(FPjcStyles::Get().GetBrush("ProjectCleaner.IconTabNonEngine16"))
		.ToolTipText(FText::FromString(TEXT("List of non engine files inside Content folder")))
		[
			SNew(STextBlock)
			// TabFilesNonEnginePtr.ToSharedRef()
		];
}

TSharedRef<SDockTab> SPjcMainWindow::OnTabFilesCorruptedSpawn(const FSpawnTabArgs& Args) const
{
	return
		SNew(SDockTab)
		.TabRole(PanelTab)
		.Label(FText::FromString(TEXT("Files Corrupted")))
		.Icon(FPjcStyles::Get().GetBrush("ProjectCleaner.IconTabCorrupted16"))
		.ToolTipText(FText::FromString(TEXT("List of corrupted assets")))
		[
			SNew(STextBlock)
			// TabFilesCorruptedPtr.ToSharedRef()
		];
}