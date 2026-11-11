// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/SPjcTabMain.h"
#include "Slate/PjcTabs.h"
#include "Slate/SPjcTabOverview.h"
#include "Slate/SPjcTabUnused.h"
#include "Slate/SPjcTabIssues.h"
#include "Slate/SPjcTabInspector.h"
#include "Slate/SPjcTabHelp.h"
#include "PjcConstants.h"
#include "PjcSubsystem.h"
#include "PjcStyles.h"
#include "PjcCmds.h"
#include "PjcShim.h"
// Engine Headers
#include "ISettingsModule.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Docking/SDockTab.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Framework/Docking/TabManager.h"
#include "HAL/PlatformProcess.h"

void SPjcTabMain::Construct(
	const FArguments& InArgs,
	const TSharedRef<SDockTab>& ConstructUnderMajorTab,
	const TSharedPtr<SWindow>& ConstructUnderWindow
) {
	TabManager = FGlobalTabmanager::Get()->NewTabManager(ConstructUnderMajorTab);
	PjcTabs::SetMain(TabManager);
	const FText MenuName = FText::FromString(PjcConstants::ModulePjcName.ToString());
	const TSharedRef<FWorkspaceItem> AppMenuGroup = TabManager->AddLocalWorkspaceMenuCategory(MenuName);

	Cmds = MakeShareable(new FUICommandList);

	const auto OpenSettings = [](const FName Container, const FName Section) {
		return FExecuteAction::CreateLambda([Container, Section]() {
			FModuleManager::LoadModuleChecked<ISettingsModule>("Settings").ShowViewer(Container, "Plugins", Section);
		});
	};
	Cmds->MapAction(FPjcCmds::Get().OpenSettingsProject, OpenSettings("Project", "PjcScanSettings"));
	Cmds->MapAction(FPjcCmds::Get().OpenSettingsEditor, OpenSettings("Editor", "PjcEditorSettings"));

	// clang-format off
	const TSharedRef<FTabManager::FStack> Stack = FTabManager::NewStack()
		->AddTab(PjcConstants::TabOverview, ETabState::OpenedTab)
		->AddTab(PjcConstants::TabAssetsUnused, ETabState::OpenedTab)
		->AddTab(PjcConstants::TabInspector, ETabState::OpenedTab)
		->AddTab(PjcConstants::TabIssues, ETabState::OpenedTab)
		->AddTab(PjcConstants::TabHelp, ETabState::OpenedTab)
		->SetForegroundTab(PjcConstants::TabOverview)
		->SetSizeCoefficient(1.0f);
	// clang-format on
	TabLayout = FTabManager::NewLayout("PjcTabLayout")->AddArea(FTabManager::NewPrimaryArea()->SetOrientation(Orient_Horizontal)->Split(Stack));

	TabManager->RegisterTabSpawner(PjcConstants::TabOverview, FOnSpawnTab::CreateRaw(this, &SPjcTabMain::OnTabOverviewSpawn))
		.SetTooltipText(FText::FromString(TEXT("Open Overview Tab")))
		.SetDisplayName(FText::FromString(TEXT("Overview")))
		.SetIcon(FPjcStyles::GetIcon("ProjectCleaner.Icon.Stat16"))
		.SetGroup(AppMenuGroup);

	TabManager->RegisterTabSpawner(PjcConstants::TabIssues, FOnSpawnTab::CreateRaw(this, &SPjcTabMain::OnTabIssuesSpawn))
		.SetTooltipText(FText::FromString(TEXT("Open Issues Tab: redirectors, external files, corrupted files, empty folders")))
		.SetDisplayName(FText::FromString(TEXT("Issues")))
		.SetIcon(FPjcStyles::GetIcon("ProjectCleaner.Icon.Issues16"))
		.SetGroup(AppMenuGroup);

	TabManager->RegisterTabSpawner(PjcConstants::TabAssetsUnused, FOnSpawnTab::CreateRaw(this, &SPjcTabMain::OnTabAssetsUnusedSpawn))
		.SetTooltipText(FText::FromString(TEXT("Open Assets Tab: unused, excluded, used and to review assets")))
		.SetDisplayName(FText::FromString(TEXT("Assets")))
		.SetIcon(FPjcStyles::GetIcon("ProjectCleaner.Icon.PieChart16"))
		.SetGroup(AppMenuGroup);

	TabManager->RegisterTabSpawner(PjcConstants::TabInspector, FOnSpawnTab::CreateRaw(this, &SPjcTabMain::OnTabInspectorSpawn))
		.SetTooltipText(FText::FromString(TEXT("Open Inspector Tab: why an asset is used, from every root down to it")))
		.SetDisplayName(FText::FromString(TEXT("Inspector")))
		.SetIcon(FPjcStyles::GetIcon("ProjectCleaner.Icon.Graph16"))
		.SetGroup(AppMenuGroup);

	TabManager->RegisterTabSpawner(PjcConstants::TabHelp, FOnSpawnTab::CreateRaw(this, &SPjcTabMain::OnTabHelpSpawn))
		.SetTooltipText(FText::FromString(TEXT("Open Help Tab: links, bug report, system info")))
		.SetDisplayName(FText::FromString(TEXT("Help")))
		.SetIcon(FPjcStyles::GetIcon("ProjectCleaner.Icon.Help16"))
		.SetGroup(AppMenuGroup);

	FMenuBarBuilder MenuBarBuilder = FMenuBarBuilder(Cmds);

	const FNewMenuDelegate TabsMenu = FNewMenuDelegate::CreateRaw(this, &SPjcTabMain::CreateMenuBarTabs, TabManager);
	const FNewMenuDelegate SettingsMenu = FNewMenuDelegate::CreateRaw(this, &SPjcTabMain::CreateMenuBarSettings);
	MenuBarBuilder.AddPullDownMenu(FText::FromString(TEXT("Tabs")), FText::GetEmpty(), TabsMenu, "Window");
	MenuBarBuilder.AddPullDownMenu(FText::FromString(TEXT("Settings")), FText::GetEmpty(), SettingsMenu, "Settings");

	// clang-format off
	ChildSlot
	[
		SNew(SOverlay)
		+ SOverlay::Slot()
		[
			SNew(SWidgetSwitcher).WidgetIndex_Raw(this, &SPjcTabMain::GetWidgetIndex)
			+ SWidgetSwitcher::Slot()
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					MenuBarBuilder.MakeWidget()
				]
				+ SVerticalBox::Slot().FillHeight(1.0f)
				[
					TabManager->RestoreFrom(TabLayout.ToSharedRef(), ConstructUnderWindow).ToSharedRef()
				]
			]
			+ SWidgetSwitcher::Slot()
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().FillHeight(1.0f).HAlign(HAlign_Center).VAlign(VAlign_Center)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().HAlign(HAlign_Center).VAlign(VAlign_Center)
					[
						SNew(SBox).WidthOverride(32.0f).HeightOverride(32.0f)
						[
							SNew(SImage).Image(FPjcStyles::GetIcon("ProjectCleaner.Icon.Warning32").GetIcon())
						]
					]
					+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(FMargin{5.0f, 2.0f, 0.0f, 0.0f}).HAlign(HAlign_Center).VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Justification(ETextJustify::Center)
						.ShadowOffset(FVector2D{1.5f, 1.5f})
						.ShadowColorAndOpacity(FLinearColor::Black)
						.Font(FPjcStyles::GetFont("Bold", 18))
						.Text_Raw(this, &SPjcTabMain::GetWidgetWarningText)
						.ColorAndOpacity(FPjcStyles::Get().GetColor("ProjectCleaner.Color.Gray"))
					]
				]
			]
		]
		// a window in front (scan, clean project) dims everything of the plugin behind it
		+ SOverlay::Slot()
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
			.BorderBackgroundColor(FLinearColor {0.0f, 0.0f, 0.0f, 0.6f})
			.Visibility_Lambda([]() { return PjcTabs::IsWindowOpen() ? EVisibility::Visible : EVisibility::Collapsed; })
		]
	];
	// clang-format on

	PjcShim::SetTabManagerMenuMultiBox(TabManager, MenuBarBuilder.GetMultiBox());
}

SPjcTabMain::~SPjcTabMain() {
	FGlobalTabmanager::Get()->UnregisterTabSpawner(PjcConstants::TabOverview);
	FGlobalTabmanager::Get()->UnregisterTabSpawner(PjcConstants::TabAssetsUnused);
	FGlobalTabmanager::Get()->UnregisterTabSpawner(PjcConstants::TabIssues);
	FGlobalTabmanager::Get()->UnregisterTabSpawner(PjcConstants::TabInspector);
	FGlobalTabmanager::Get()->UnregisterTabSpawner(PjcConstants::TabHelp);
}

int32 SPjcTabMain::GetWidgetIndex() const {
	const bool bBusy = UPjcSubsystem::EditorIsInPlayMode() || UPjcSubsystem::GetModuleAssetRegistry().Get().IsLoadingAssets();
	return bBusy ? PjcConstants::WidgetIndexWorking : PjcConstants::WidgetIndexIdle;
}

FText SPjcTabMain::GetWidgetWarningText() const {
	if (UPjcSubsystem::EditorIsInPlayMode()) {
		return FText::FromString(TEXT("Please exit the editor's play mode before performing any operations in the plugin."));
	}

	if (UPjcSubsystem::GetModuleAssetRegistry().Get().IsLoadingAssets()) {
		return FText::FromString(TEXT("Please wait until the Asset Registry has discovered all assets in the project."));
	}

	return FText::FromString(TEXT(""));
}

TSharedRef<SDockTab> SPjcTabMain::OnTabOverviewSpawn(const FSpawnTabArgs& Args) const {
	// clang-format off
	const auto Frontend = SNew(SDockTab).TabRole(PanelTab).Label(FText::FromString(TEXT("Overview")))
		[
			SNew(SPjcTabOverview).TabManager(TabManager)
		];
	// clang-format on

	Frontend->SetTabIcon(FPjcStyles::Get().GetBrush("ProjectCleaner.Icon.Stat16"));

	return Frontend;
}

TSharedRef<SDockTab> SPjcTabMain::OnTabIssuesSpawn(const FSpawnTabArgs& Args) const {
	// clang-format off
	const auto Frontend = SNew(SDockTab).TabRole(PanelTab).Label(FText::FromString(TEXT("Issues")))
		[
			SNew(SPjcTabIssues).TabManager(TabManager)
		];
	// clang-format on

	Frontend->SetTabIcon(FPjcStyles::Get().GetBrush("ProjectCleaner.Icon.Issues16"));

	return Frontend;
}

TSharedRef<SDockTab> SPjcTabMain::OnTabAssetsUnusedSpawn(const FSpawnTabArgs& Args) const {
	// clang-format off
	const auto Frontend = SNew(SDockTab).TabRole(PanelTab).Label(FText::FromString(TEXT("Assets")))
		[
			SNew(SPjcTabUnused).TabManager(TabManager)
		];
	// clang-format on

	Frontend->SetTabIcon(FPjcStyles::Get().GetBrush("ProjectCleaner.Icon.PieChart16"));

	return Frontend;
}

TSharedRef<SDockTab> SPjcTabMain::OnTabInspectorSpawn(const FSpawnTabArgs& Args) const {
	// clang-format off
	const auto Frontend = SNew(SDockTab).TabRole(PanelTab).Label(FText::FromString(TEXT("Inspector")))
		[
			SNew(SPjcTabInspector).TabManager(TabManager)
		];
	// clang-format on

	Frontend->SetTabIcon(FPjcStyles::Get().GetBrush("ProjectCleaner.Icon.Graph16"));

	return Frontend;
}

TSharedRef<SDockTab> SPjcTabMain::OnTabHelpSpawn(const FSpawnTabArgs& Args) const {
	// clang-format off
	const auto Frontend = SNew(SDockTab).TabRole(PanelTab).Label(FText::FromString(TEXT("Help")))
		[
			SNew(SPjcTabHelp)
		];
	// clang-format on

	Frontend->SetTabIcon(FPjcStyles::Get().GetBrush("ProjectCleaner.Icon.Help16"));

	return Frontend;
}

void SPjcTabMain::CreateMenuBarTabs(FMenuBuilder& MenuBuilder, const TSharedPtr<FTabManager> TabManagerPtr) {
	if (!TabManagerPtr.IsValid()) return;

#if !WITH_EDITOR
	FGlobalTabmanager::Get()->PopulateTabSpawnerMenu(MenuBuilder, WorkspaceMenu::GetMenuStructure().GetStructureRoot());
#endif

	TabManagerPtr->PopulateLocalTabSpawnerMenu(MenuBuilder);
}

void SPjcTabMain::CreateMenuBarSettings(FMenuBuilder& MenuBuilder) {
	MenuBuilder.AddMenuEntry(FPjcCmds::Get().OpenSettingsProject);
	MenuBuilder.AddMenuEntry(FPjcCmds::Get().OpenSettingsEditor);
}
