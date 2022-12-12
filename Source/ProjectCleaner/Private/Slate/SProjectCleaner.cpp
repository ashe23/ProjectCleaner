// // Copyright Ashot Barkhudaryan. All Rights Reserved.
//
// #include "Slate/SProjectCleaner.h"
// #include "Slate/Tabs/SProjectCleanerTabScanSettings.h"
// #include "Slate/Tabs/SProjectCleanerTabUnused.h"
// #include "Slate/Tabs/SProjectCleanerTabIndirect.h"
// #include "Slate/Tabs/SProjectCleanerTabCorrupted.h"
// #include "Slate/Tabs/SProjectCleanerTabNonEngine.h"
// #include "ProjectCleanerConstants.h"
// #include "ProjectCleanerStyles.h"
// #include "ProjectCleanerLibrary.h"
// #include "ProjectCleanerSubsystem.h"
// // Engine Headers
// #include "Widgets/Layout/SWidgetSwitcher.h"
//
// static constexpr int32 WidgetIndexIdle = 0;
// static constexpr int32 WidgetIndexWorking = 1;
//
// void SProjectCleaner::Construct(const FArguments& InArgs, const TSharedRef<SDockTab>& ConstructUnderMajorTab, const TSharedPtr<SWindow>& ConstructUnderWindow)
// {
// 	if (!GEditor) return;
//
// 	SubsystemPtr = GEditor->GetEditorSubsystem<UProjectCleanerSubsystem>();
// 	check(SubsystemPtr);
//
// 	SubsystemPtr->ProjectScan();
//
// 	TabManager = FGlobalTabmanager::Get()->NewTabManager(ConstructUnderMajorTab);
// 	const TSharedRef<FWorkspaceItem> AppMenuGroup = TabManager->AddLocalWorkspaceMenuCategory(FText::FromString(TEXT("ProjectCleaner")));
//
// 	TabManager->RegisterTabSpawner(
// 		          ProjectCleanerConstants::TabScanSettings,
// 		          FOnSpawnTab::CreateRaw(
// 			          this,
// 			          &SProjectCleaner::OnTabSpawnScanSettings)
// 	          )
// 	          .SetTooltipText(FText::FromString(TEXT("Open Scan Settings Tab")))
// 	          .SetDisplayName(FText::FromString(TEXT("Scan Settings")))
// 	          .SetIcon(FSlateIcon(FProjectCleanerStyles::GetStyleSetName(), "ProjectCleaner.IconSettings16"))
// 	          .SetGroup(AppMenuGroup);
//
// 	TabManager->RegisterTabSpawner(
// 		          ProjectCleanerConstants::TabUnusedAssets,
// 		          FOnSpawnTab::CreateRaw(
// 			          this,
// 			          &SProjectCleaner::OnTabSpawnUnusedAssets)
// 	          )
// 	          .SetTooltipText(FText::FromString(TEXT("Open Unused Assets Tab")))
// 	          .SetDisplayName(FText::FromString(TEXT("Unused Assets")))
// 	          .SetIcon(FSlateIcon(FProjectCleanerStyles::GetStyleSetName(), "ProjectCleaner.IconTabUnused16"))
// 	          .SetGroup(AppMenuGroup);
//
// 	TabManager->RegisterTabSpawner(
// 		          ProjectCleanerConstants::TabIndirectAssets,
// 		          FOnSpawnTab::CreateRaw(
// 			          this,
// 			          &SProjectCleaner::OnTabSpawnIndirectAssets)
// 	          )
// 	          .SetTooltipText(FText::FromString(TEXT("Open Indirect Assets Tab")))
// 	          .SetDisplayName(FText::FromString(TEXT("Indirect Assets")))
// 	          .SetIcon(FSlateIcon(FProjectCleanerStyles::GetStyleSetName(), "ProjectCleaner.IconTabIndirect16"))
// 	          .SetGroup(AppMenuGroup);
//
// 	TabManager->RegisterTabSpawner(
// 		          ProjectCleanerConstants::TabCorruptedAssets,
// 		          FOnSpawnTab::CreateRaw(
// 			          this,
// 			          &SProjectCleaner::OnTabSpawnCorruptedAssets)
// 	          )
// 	          .SetTooltipText(FText::FromString(TEXT("Open Corrupted Assets Tab")))
// 	          .SetDisplayName(FText::FromString(TEXT("Corrupted Assets")))
// 	          .SetIcon(FSlateIcon(FProjectCleanerStyles::GetStyleSetName(), "ProjectCleaner.IconTabCorrupted16"))
// 	          .SetGroup(AppMenuGroup);
//
// 	TabManager->RegisterTabSpawner(
// 		          ProjectCleanerConstants::TabNonEngineFiles,
// 		          FOnSpawnTab::CreateRaw(
// 			          this,
// 			          &SProjectCleaner::OnTabSpawnNonEngineFiles)
// 	          )
// 	          .SetTooltipText(FText::FromString(TEXT("Open NonEngine Files Tab")))
// 	          .SetDisplayName(FText::FromString(TEXT("NonEngine Files")))
// 	          .SetIcon(FSlateIcon(FProjectCleanerStyles::GetStyleSetName(), "ProjectCleaner.IconTabNonEngine16"))
// 	          .SetGroup(AppMenuGroup);
//
// 	TabLayout = FTabManager::NewLayout("ProjectCleanerTabLayout")
// 		->AddArea
// 		(
// 			FTabManager::NewPrimaryArea()
// 			->SetOrientation(Orient_Horizontal)
// 			->Split
// 			(
// 				FTabManager::NewStack()
// 				->AddTab(ProjectCleanerConstants::TabScanSettings, ETabState::OpenedTab)
// 				->SetSizeCoefficient(0.3f)
// 			)
// 			->Split(
// 				FTabManager::NewStack()
// 				->AddTab(ProjectCleanerConstants::TabUnusedAssets, ETabState::OpenedTab)
// 				->AddTab(ProjectCleanerConstants::TabIndirectAssets, ETabState::OpenedTab)
// 				->AddTab(ProjectCleanerConstants::TabCorruptedAssets, ETabState::OpenedTab)
// 				->AddTab(ProjectCleanerConstants::TabNonEngineFiles, ETabState::OpenedTab)
// 				->SetSizeCoefficient(0.7f)
// 				->SetForegroundTab(ProjectCleanerConstants::TabUnusedAssets)
// 			)
// 		);
//
// 	FMenuBarBuilder MenuBarBuilder = FMenuBarBuilder(TSharedPtr<FUICommandList>());
// 	MenuBarBuilder.AddPullDownMenu(
// 		FText::FromString(TEXT("Settings")),
// 		FText::GetEmpty(),
// 		FNewMenuDelegate::CreateRaw(this, &SProjectCleaner::MenuBarFillSettings, TabManager),
// 		"Window"
// 	);
// 	MenuBarBuilder.AddPullDownMenu(
// 		FText::FromString(TEXT("Tabs")),
// 		FText::GetEmpty(),
// 		FNewMenuDelegate::CreateStatic(&SProjectCleaner::MenuBarFillTabs, TabManager),
// 		"Window"
// 	);
// 	MenuBarBuilder.AddPullDownMenu(
// 		FText::FromString(TEXT("Help")),
// 		FText::GetEmpty(),
// 		FNewMenuDelegate::CreateStatic(&SProjectCleaner::MenuBarFillHelp, TabManager),
// 		"Window"
// 	);
//
// 	ChildSlot
// 	[
// 		SNew(SWidgetSwitcher)
// 		.IsEnabled_Raw(this, &SProjectCleaner::WidgetEnabled)
// 		.WidgetIndex_Raw(this, &SProjectCleaner::WidgetGetIndex)
// 		+ SWidgetSwitcher::Slot()
// 		  .HAlign(HAlign_Fill)
// 		  .VAlign(VAlign_Fill)
// 		[
// 			SNew(SVerticalBox)
// 			+ SVerticalBox::Slot()
// 			.AutoHeight()
// 			[
// 				MenuBarBuilder.MakeWidget()
// 			]
// 			+ SVerticalBox::Slot()
// 			.FillHeight(1.0f)
// 			[
// 				TabManager->RestoreFrom(TabLayout.ToSharedRef(), ConstructUnderWindow).ToSharedRef()
// 			]
// 		]
// 		+ SWidgetSwitcher::Slot()
// 		  .HAlign(HAlign_Fill)
// 		  .VAlign(VAlign_Fill)
// 		[
// 			SNew(SHorizontalBox)
// 			+ SHorizontalBox::Slot()
// 			  .FillWidth(1.0f)
// 			  .HAlign(HAlign_Center)
// 			  .VAlign(VAlign_Center)
// 			[
// 				SNew(STextBlock)
// 				.Justification(ETextJustify::Center)
// 				.Font(FProjectCleanerStyles::GetFont("Light", 30))
// 				.Text_Raw(this, &SProjectCleaner::WidgetText)
// 			]
// 		]
// 	];
//
// 	TabManager->SetMenuMultiBox(MenuBarBuilder.GetMultiBox());
// }
//
// SProjectCleaner::~SProjectCleaner()
// {
// 	FGlobalTabmanager::Get()->UnregisterTabSpawner(ProjectCleanerConstants::TabScanSettings);
// 	FGlobalTabmanager::Get()->UnregisterTabSpawner(ProjectCleanerConstants::TabUnusedAssets);
// 	FGlobalTabmanager::Get()->UnregisterTabSpawner(ProjectCleanerConstants::TabIndirectAssets);
// 	FGlobalTabmanager::Get()->UnregisterTabSpawner(ProjectCleanerConstants::TabCorruptedAssets);
// 	FGlobalTabmanager::Get()->UnregisterTabSpawner(ProjectCleanerConstants::TabNonEngineFiles);
// }
//
// void SProjectCleaner::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
// {
// 	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
//
// 	if (!SubsystemPtr) return;
//
// 	SubsystemPtr->CheckEditorState();
// }
//
// bool SProjectCleaner::WidgetEnabled() const
// {
// 	if (!SubsystemPtr) return false;
//
// 	if (SubsystemPtr->GetEditorState() != EProjectCleanerEditorState::Idle)
// 	{
// 		return false;
// 	}
//
// 	if (SubsystemPtr->GetScanState() != EProjectCleanerScanState::Idle)
// 	{
// 		return false;
// 	}
//
// 	return true;
// }
//
// int32 SProjectCleaner::WidgetGetIndex() const
// {
// 	if (!SubsystemPtr) return WidgetIndexWorking;
//
// 	if (
// 		SubsystemPtr->GetEditorState() != EProjectCleanerEditorState::Idle ||
// 		SubsystemPtr->GetScanState() != EProjectCleanerScanState::Idle
// 	)
// 	{
// 		return WidgetIndexWorking;
// 	}
//
// 	return WidgetIndexIdle;
// }
//
// FText SProjectCleaner::WidgetText() const
// {
// 	if (!SubsystemPtr) return FText::FromString(TEXT("ProjectCleanerSubsystem Invalid"));
//
// 	if (SubsystemPtr->GetEditorState() == EProjectCleanerEditorState::PlayMode)
// 	{
// 		return FText::FromString(ProjectCleanerConstants::MsgPlayModeActive);
// 	}
//
// 	if (SubsystemPtr->GetEditorState() == EProjectCleanerEditorState::AssetRegistryWorking)
// 	{
// 		return FText::FromString(ProjectCleanerConstants::MsgAssetRegistryStillWorking);
// 	}
//
// 	if (SubsystemPtr->GetScanState() == EProjectCleanerScanState::Scanning)
// 	{
// 		return FText::FromString(ProjectCleanerConstants::MsgScanning);
// 	}
//
// 	if (SubsystemPtr->GetScanState() == EProjectCleanerScanState::Cleaning)
// 	{
// 		return FText::FromString(ProjectCleanerConstants::MsgCleaning);
// 	}
//
// 	return FText::FromString(TEXT(""));
// }
//
// void SProjectCleaner::MenuBarFillTabs(FMenuBuilder& MenuBuilder, const TSharedPtr<FTabManager> TabManagerPtr)
// {
// 	if (!TabManagerPtr.IsValid()) return;
//
//
// #if !WITH_EDITOR
// 	FGlobalTabmanager::Get()->PopulateTabSpawnerMenu(MenuBuilder, WorkspaceMenu::GetMenuStructure().GetStructureRoot());
// #endif
//
// 	TabManagerPtr->PopulateLocalTabSpawnerMenu(MenuBuilder);
// }
//
// void SProjectCleaner::MenuBarFillSettings(FMenuBuilder& MenuBuilder, const TSharedPtr<FTabManager> TabManagerPtr) const
// {
// 	FUIAction ActionAutoDeleteEmptyFolders;
// 	ActionAutoDeleteEmptyFolders.ExecuteAction = FExecuteAction::CreateLambda([&]()
// 	{
// 		SubsystemPtr->bAutoCleanEmptyFolders = !SubsystemPtr->bAutoCleanEmptyFolders;
// 		SubsystemPtr->PostEditChange();
// 	});
// 	ActionAutoDeleteEmptyFolders.CanExecuteAction = FCanExecuteAction::CreateLambda([&]()
// 	{
// 		return SubsystemPtr != nullptr;
// 	});
// 	ActionAutoDeleteEmptyFolders.GetActionCheckState = FGetActionCheckState::CreateLambda([&]()
// 	{
// 		return SubsystemPtr->bAutoCleanEmptyFolders ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
// 	});
//
// 	MenuBuilder.BeginSection(NAME_None);
// 	MenuBuilder.AddMenuEntry(
// 		FText::FromString(TEXT("Auto Clean Empty Folders")),
// 		FText::FromString(TEXT("Automatically delete empty folders after cleaning a project of unused assets. By default, it is enabled.")),
// 		FSlateIcon(),
// 		ActionAutoDeleteEmptyFolders,
// 		NAME_None,
// 		EUserInterfaceActionType::ToggleButton
// 	);
// 	MenuBuilder.EndSection();
// }
//
// void SProjectCleaner::MenuBarFillHelp(FMenuBuilder& MenuBuilder, const TSharedPtr<FTabManager> TabManagerPtr)
// {
// 	FUIAction Action;
// 	Action.ExecuteAction = FExecuteAction::CreateLambda([]()
// 	{
// 		FPlatformProcess::LaunchURL(*ProjectCleanerConstants::UrlWiki, nullptr, nullptr);
// 	});
// 	Action.CanExecuteAction = FCanExecuteAction::CreateLambda([]()
// 	{
// 		return FPlatformProcess::CanLaunchURL(*ProjectCleanerConstants::UrlWiki);
// 	});
//
// 	MenuBuilder.BeginSection("SectionHelp", FText::FromString(TEXT("Help")));
// 	MenuBuilder.AddMenuEntry(FText::FromString(TEXT("Wiki")), FText::FromString(TEXT("Open wiki page on github")), FSlateIcon(), Action, NAME_None);
// 	MenuBuilder.EndSection();
// }
//
// TSharedRef<SDockTab> SProjectCleaner::OnTabSpawnScanSettings(const FSpawnTabArgs& Args) const
// {
// 	return
// 		SNew(SDockTab)
// 		.TabRole(PanelTab)
// 		.Label(FText::FromString(TEXT("Scan Settings")))
// 		.Icon(FProjectCleanerStyles::Get().GetBrush("ProjectCleaner.IconSettings16"))
// 		[
// 			SNew(SProjectCleanerTabScanSettings)
// 		];
// }
//
// TSharedRef<SDockTab> SProjectCleaner::OnTabSpawnUnusedAssets(const FSpawnTabArgs& Args)
// {
// 	return
// 		SNew(SDockTab)
// 		.TabRole(PanelTab)
// 		.Label(FText::FromString(TEXT("Unused Assets")))
// 		.Icon(FProjectCleanerStyles::Get().GetBrush("ProjectCleaner.IconTabUnused16"))
// 		[
// 			SNew(STextBlock)
// 			// SAssignNew(TabUnused, SProjectCleanerTabUnused)
// 			// .Scanner(Scanner)
// 			// .RenderOpacity(TabsRenderOpacity)
// 			// .IsEnabled(this, &SProjectCleaner::TabsEnabled)
// 		];
// }
//
// TSharedRef<SDockTab> SProjectCleaner::OnTabSpawnIndirectAssets(const FSpawnTabArgs& Args)
// {
// 	return
// 		SNew(SDockTab)
// 		.TabRole(PanelTab)
// 		.Label(FText::FromString(TEXT("Indirect Assets")))
// 		.Icon(FProjectCleanerStyles::Get().GetBrush("ProjectCleaner.IconTabIndirect16"))
// 		[
// 			SNew(SProjectCleanerTabIndirect)
// 			// SAssignNew(TabIndirect, SProjectCleanerTabIndirect)
// 			// .Scanner(Scanner)
// 			// .RenderOpacity(TabsRenderOpacity)
// 			// .IsEnabled(this, &SProjectCleaner::TabsEnabled)
// 		];
// }
//
// TSharedRef<SDockTab> SProjectCleaner::OnTabSpawnCorruptedAssets(const FSpawnTabArgs& Args)
// {
// 	return
// 		SNew(SDockTab)
// 		.TabRole(PanelTab)
// 		.Label(FText::FromString(TEXT("Corrupted Assets")))
// 		.Icon(FProjectCleanerStyles::Get().GetBrush("ProjectCleaner.IconTabCorrupted16"))
// 		[
// 			SNew(SProjectCleanerTabCorrupted)
// 			// SAssignNew(TabCorrupted, SProjectCleanerTabCorrupted)
// 			// .Scanner(Scanner)
// 			// .RenderOpacity(TabsRenderOpacity)
// 			// .IsEnabled(this, &SProjectCleaner::TabsEnabled)
// 		];
// }
//
// TSharedRef<SDockTab> SProjectCleaner::OnTabSpawnNonEngineFiles(const FSpawnTabArgs& Args)
// {
// 	return
// 		SNew(SDockTab)
// 		.TabRole(PanelTab)
// 		.Label(FText::FromString(TEXT("NonEngine Files")))
// 		.Icon(FProjectCleanerStyles::Get().GetBrush("ProjectCleaner.IconTabNonEngine16"))
// 		[
// 			SNew(SProjectCleanerTabNonEngine)
// 			// SAssignNew(TabNonEngine, SProjectCleanerTabNonEngine)
// 			// .Scanner(Scanner)
// 			// .RenderOpacity(TabsRenderOpacity)
// 			// .IsEnabled(this, &SProjectCleaner::TabsEnabled)
// 		];
// }
