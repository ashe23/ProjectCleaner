// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/SPjcTabMain.h"
#include "Slate/SPjcTabAssetsUnused.h"
#include "Slate/SPjcTabAssetsIndirect.h"
#include "Slate/SPjcTabAssetsCorrupted.h"
#include "Slate/SPjcTabFilesExternal.h"
#include "PjcConstants.h"
#include "PjcSubsystem.h"
#include "PjcStyles.h"
#include "PjcCmds.h"
// Engine Headers
#include "Widgets/Layout/SWidgetSwitcher.h"

void SPjcTabMain::Construct(const FArguments& InArgs, const TSharedRef<SDockTab>& ConstructUnderMajorTab, const TSharedPtr<SWindow>& ConstructUnderWindow)
{
	TabManager = FGlobalTabmanager::Get()->NewTabManager(ConstructUnderMajorTab);
	const TSharedRef<FWorkspaceItem> AppMenuGroup = TabManager->AddLocalWorkspaceMenuCategory(FText::FromString(PjcConstants::ModulePjcName.ToString()));

	Cmds = MakeShareable(new FUICommandList);

	Cmds->MapAction(FPjcCmds::Get().OpenGithub, FExecuteAction::CreateLambda([]()
	{
		FPlatformProcess::LaunchURL(*PjcConstants::UrlGithub, nullptr, nullptr);
	}));

	Cmds->MapAction(FPjcCmds::Get().OpenWiki, FExecuteAction::CreateLambda([]()
	{
		FPlatformProcess::LaunchURL(*PjcConstants::UrlDocs, nullptr, nullptr);
	}));

	Cmds->MapAction(FPjcCmds::Get().OpenBugReport, FExecuteAction::CreateLambda([]()
	{
		FPlatformProcess::LaunchURL(*PjcConstants::UrlIssueTracker, nullptr, nullptr);
	}));

	TabLayout = FTabManager::NewLayout("PjcTabLayout")
		->AddArea
		(
			FTabManager::NewPrimaryArea()
			->SetOrientation(Orient_Horizontal)
			->Split
			(
				FTabManager::NewStack()
				->AddTab(PjcConstants::TabAssetsUnused, ETabState::OpenedTab)
				->AddTab(PjcConstants::TabAssetsIndirect, ETabState::OpenedTab)
				->AddTab(PjcConstants::TabAssetsCorrupted, ETabState::OpenedTab)
				->AddTab(PjcConstants::TabFilesExternal, ETabState::OpenedTab)
				->SetForegroundTab(PjcConstants::TabAssetsUnused)
				->SetSizeCoefficient(1.0f)
			)
		);

	TabManager->RegisterTabSpawner(PjcConstants::TabAssetsUnused, FOnSpawnTab::CreateRaw(this, &SPjcTabMain::OnTabAssetsUnusedSpawn))
	          .SetTooltipText(FText::FromString(TEXT("Open Unused Assets Tab")))
	          .SetDisplayName(FText::FromString(TEXT("Assets Unused")))
	          .SetIcon(FPjcStyles::GetIcon("ProjectCleaner.Icon.PieChart16"))
	          .SetGroup(AppMenuGroup);

	TabManager->RegisterTabSpawner(PjcConstants::TabAssetsIndirect, FOnSpawnTab::CreateRaw(this, &SPjcTabMain::OnTabAssetsIndirectSpawn))
	          .SetTooltipText(FText::FromString(TEXT("Open Indirect Assets Tab")))
	          .SetDisplayName(FText::FromString(TEXT("Assets Indirect")))
	          .SetIcon(FPjcStyles::GetIcon("ProjectCleaner.Icon.Arrows16"))
	          .SetGroup(AppMenuGroup);

	TabManager->RegisterTabSpawner(PjcConstants::TabAssetsCorrupted, FOnSpawnTab::CreateRaw(this, &SPjcTabMain::OnTabAssetsCorruptedSpawn))
	          .SetTooltipText(FText::FromString(TEXT("Open Corrupted Assets Tab")))
	          .SetDisplayName(FText::FromString(TEXT("Assets Corrupted")))
	          .SetIcon(FPjcStyles::GetIcon("ProjectCleaner.Icon.CorruptedFile16"))
	          .SetGroup(AppMenuGroup);

	TabManager->RegisterTabSpawner(PjcConstants::TabFilesExternal, FOnSpawnTab::CreateRaw(this, &SPjcTabMain::OnTabFilesExternalSpawn))
	          .SetTooltipText(FText::FromString(TEXT("Open External Files Tab")))
	          .SetDisplayName(FText::FromString(TEXT("Files External")))
	          .SetIcon(FPjcStyles::GetIcon("ProjectCleaner.Icon.File16"))
	          .SetGroup(AppMenuGroup);

	FMenuBarBuilder MenuBarBuilder = FMenuBarBuilder(Cmds);

	MenuBarBuilder.AddPullDownMenu(
		FText::FromString(TEXT("Tabs")),
		FText::GetEmpty(),
		FNewMenuDelegate::CreateRaw(this, &SPjcTabMain::CreateMenuBarTabs, TabManager),
		"Window"
	);
	MenuBarBuilder.AddPullDownMenu(
		FText::FromString(TEXT("Help")),
		FText::GetEmpty(),
		FNewMenuDelegate::CreateRaw(this, &SPjcTabMain::CreateMenuBarHelp, TabManager),
		"Tabs"
	);

	ChildSlot
	[
		SNew(SWidgetSwitcher)
		.WidgetIndex_Raw(this, &SPjcTabMain::GetWidgetIndex)
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
	];

	TabManager->SetMenuMultiBox(MenuBarBuilder.GetMultiBox(), nullptr);
}

SPjcTabMain::~SPjcTabMain()
{
	FGlobalTabmanager::Get()->UnregisterTabSpawner(PjcConstants::TabAssetsUnused);
	FGlobalTabmanager::Get()->UnregisterTabSpawner(PjcConstants::TabAssetsIndirect);
	FGlobalTabmanager::Get()->UnregisterTabSpawner(PjcConstants::TabAssetsCorrupted);
	FGlobalTabmanager::Get()->UnregisterTabSpawner(PjcConstants::TabFilesExternal);
}

int32 SPjcTabMain::GetWidgetIndex() const
{
	return UPjcSubsystem::EditorIsInPlayMode() || UPjcSubsystem::GetModuleAssetRegistry().Get().IsLoadingAssets() ? PjcConstants::WidgetIndexWorking : PjcConstants::WidgetIndexIdle;
}

FText SPjcTabMain::GetWidgetWarningText() const
{
	if (UPjcSubsystem::EditorIsInPlayMode())
	{
		return FText::FromString(TEXT("Please exit the editor's play mode before performing any operations in the plugin."));
	}

	if (UPjcSubsystem::GetModuleAssetRegistry().Get().IsLoadingAssets())
	{
		return FText::FromString(TEXT("Please wait until the Asset Registry has discovered all assets in the project."));
	}

	return FText::FromString(TEXT(""));
}

TSharedRef<SDockTab> SPjcTabMain::OnTabAssetsUnusedSpawn(const FSpawnTabArgs& Args) const
{
	const auto Frontend =
		SNew(SDockTab)
		.TabRole(PanelTab)
		.Label(FText::FromString(TEXT("Assets Unused")))
		[
			SNew(SPjcTabAssetsUnused)
		];

	Frontend->SetTabIcon(FPjcStyles::Get().GetBrush("ProjectCleaner.Icon.PieChart16"));
	return Frontend;
}

TSharedRef<SDockTab> SPjcTabMain::OnTabAssetsIndirectSpawn(const FSpawnTabArgs& Args) const
{
	const auto Frontend =
		SNew(SDockTab)
		.TabRole(PanelTab)
		.Label(FText::FromString(TEXT("Assets Indirect")))
		[
			SNew(SPjcTabAssetsIndirect)
		];

	Frontend->SetTabIcon(FPjcStyles::Get().GetBrush("ProjectCleaner.Icon.Arrows16"));
	return Frontend;
}

TSharedRef<SDockTab> SPjcTabMain::OnTabAssetsCorruptedSpawn(const FSpawnTabArgs& Args) const
{
	const auto Frontend =
		SNew(SDockTab)
		.TabRole(PanelTab)
		.Label(FText::FromString(TEXT("Assets Corrupted")))
		[
			SNew(SPjcTabAssetsCorrupted)
		];

	Frontend->SetTabIcon(FPjcStyles::Get().GetBrush("ProjectCleaner.Icon.CorruptedFile16"));
	return Frontend;
}

TSharedRef<SDockTab> SPjcTabMain::OnTabFilesExternalSpawn(const FSpawnTabArgs& Args) const
{
	const auto Frontend =
		SNew(SDockTab)
		.TabRole(PanelTab)
		.Label(FText::FromString(TEXT("Files External")))
		[
			SNew(SPjcTabFilesExternal)
		];

	Frontend->SetTabIcon(FPjcStyles::Get().GetBrush("ProjectCleaner.Icon.File16"));
	return Frontend;
}

void SPjcTabMain::CreateMenuBarTabs(FMenuBuilder& MenuBuilder, const TSharedPtr<FTabManager> TabManagerPtr)
{
	if (!TabManagerPtr.IsValid()) return;

#if !WITH_EDITOR
	FGlobalTabmanager::Get()->PopulateTabSpawnerMenu(MenuBuilder, WorkspaceMenu::GetMenuStructure().GetStructureRoot());
#endif

	TabManagerPtr->PopulateLocalTabSpawnerMenu(MenuBuilder);
}

void SPjcTabMain::CreateMenuBarHelp(FMenuBuilder& MenuBuilder, const TSharedPtr<FTabManager> TabManagerPtr)
{
	MenuBuilder.AddMenuEntry(FPjcCmds::Get().OpenGithub);
	MenuBuilder.AddMenuEntry(FPjcCmds::Get().OpenWiki);
	MenuBuilder.AddSeparator();
	MenuBuilder.AddMenuEntry(FPjcCmds::Get().OpenBugReport);
}
