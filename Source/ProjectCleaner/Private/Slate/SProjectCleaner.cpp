// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/SProjectCleaner.h"
#include "Slate/Tabs/SProjectCleanerTabScanSettings.h"
#include "Slate/Tabs/SProjectCleanerTabUnused.h"
#include "Slate/Tabs/SProjectCleanerTabIndirect.h"
#include "Slate/Tabs/SProjectCleanerTabCorrupted.h"
#include "Slate/Tabs/SProjectCleanerTabNonEngine.h"
#include "Settings/ProjectCleanerScanSettings.h"
#include "Settings/ProjectCleanerExcludeSettings.h"
#include "ProjectCleanerScanner.h"
#include "ProjectCleanerConstants.h"
#include "ProjectCleanerStyles.h"
#include "ProjectCleanerLibrary.h"
// Engine Headers
#include "Widgets/Layout/SWidgetSwitcher.h"

static constexpr int32 WidgetIndexNone = 0;
static constexpr int32 WidgetIndexLoading = 1;
static constexpr int32 WidgetIndexInPlayMode = 2;

void SProjectCleaner::Construct(const FArguments& InArgs, const TSharedRef<SDockTab>& ConstructUnderMajorTab, const TSharedPtr<SWindow>& ConstructUnderWindow)
{
	ScanSettings = GetMutableDefault<UProjectCleanerScanSettings>();
	ExcludeSettings = GetMutableDefault<UProjectCleanerExcludeSettings>();

	check(ScanSettings);
	check(ExcludeSettings);

	Scanner = MakeShareable(new FProjectCleanerScanner(ScanSettings, ExcludeSettings));
	if (!Scanner.IsValid()) return;

	if (ScanSettings->bAutoScan)
	{
		Scanner->Scan();
	}

	Scanner->OnStatusChanged().AddLambda([&](const EProjectCleanerScannerStatus NewStatus)
	{
		TabsRenderOpacity = NewStatus == EProjectCleanerScannerStatus::ScanFinished ? 1.0f : 0.2f;
		bTabsEnabled = NewStatus == EProjectCleanerScannerStatus::ScanFinished;

		TabsUpdateRenderOpacity();
	});

	TabsRenderOpacity = Scanner->GetStatus() == EProjectCleanerScannerStatus::ScanFinished ? 1.0f : 0.2f;
	bTabsEnabled = Scanner->GetStatus() == EProjectCleanerScannerStatus::ScanFinished;

	TabManager = FGlobalTabmanager::Get()->NewTabManager(ConstructUnderMajorTab);
	const TSharedRef<FWorkspaceItem> AppMenuGroup = TabManager->AddLocalWorkspaceMenuCategory(FText::FromString(TEXT("ProjectCleaner")));

	TabManager->RegisterTabSpawner(
		          ProjectCleanerConstants::TabScanSettings,
		          FOnSpawnTab::CreateRaw(
			          this,
			          &SProjectCleaner::OnTabSpawnScanSettings)
	          )
	          .SetTooltipText(FText::FromString(TEXT("Open Scan Settings Tab")))
	          .SetDisplayName(FText::FromString(TEXT("Scan Settings")))
	          .SetIcon(FSlateIcon(FProjectCleanerStyles::GetStyleSetName(), "ProjectCleaner.IconSettings16"))
	          .SetGroup(AppMenuGroup);

	TabManager->RegisterTabSpawner(
		          ProjectCleanerConstants::TabUnusedAssets,
		          FOnSpawnTab::CreateRaw(
			          this,
			          &SProjectCleaner::OnTabSpawnUnusedAssets)
	          )
	          .SetTooltipText(FText::FromString(TEXT("Open Unused Assets Tab")))
	          .SetDisplayName(FText::FromString(TEXT("Unused Assets")))
	          .SetIcon(FSlateIcon(FProjectCleanerStyles::GetStyleSetName(), "ProjectCleaner.IconTabUnused16"))
	          .SetGroup(AppMenuGroup);

	TabManager->RegisterTabSpawner(
		          ProjectCleanerConstants::TabIndirectAssets,
		          FOnSpawnTab::CreateRaw(
			          this,
			          &SProjectCleaner::OnTabSpawnIndirectAssets)
	          )
	          .SetTooltipText(FText::FromString(TEXT("Open Indirect Assets Tab")))
	          .SetDisplayName(FText::FromString(TEXT("Indirect Assets")))
	          .SetIcon(FSlateIcon(FProjectCleanerStyles::GetStyleSetName(), "ProjectCleaner.IconTabIndirect16"))
	          .SetGroup(AppMenuGroup);

	TabManager->RegisterTabSpawner(
		          ProjectCleanerConstants::TabCorruptedAssets,
		          FOnSpawnTab::CreateRaw(
			          this,
			          &SProjectCleaner::OnTabSpawnCorruptedAssets)
	          )
	          .SetTooltipText(FText::FromString(TEXT("Open Corrupted Assets Tab")))
	          .SetDisplayName(FText::FromString(TEXT("Corrupted Assets")))
	          .SetIcon(FSlateIcon(FProjectCleanerStyles::GetStyleSetName(), "ProjectCleaner.IconTabCorrupted16"))
	          .SetGroup(AppMenuGroup);

	TabManager->RegisterTabSpawner(
		          ProjectCleanerConstants::TabNonEngineFiles,
		          FOnSpawnTab::CreateRaw(
			          this,
			          &SProjectCleaner::OnTabSpawnNonEngineFiles)
	          )
	          .SetTooltipText(FText::FromString(TEXT("Open NonEngine Files Tab")))
	          .SetDisplayName(FText::FromString(TEXT("NonEngine Files")))
	          .SetIcon(FSlateIcon(FProjectCleanerStyles::GetStyleSetName(), "ProjectCleaner.IconTabNonEngine16"))
	          .SetGroup(AppMenuGroup);

	TabLayout = FTabManager::NewLayout("ProjectCleanerTabLayout")
		->AddArea
		(
			FTabManager::NewPrimaryArea()
			->SetOrientation(Orient_Horizontal)
			->Split
			(
				FTabManager::NewStack()
				->AddTab(ProjectCleanerConstants::TabScanSettings, ETabState::OpenedTab)
				->SetSizeCoefficient(0.3f)
			)
			->Split(
				FTabManager::NewStack()
				->AddTab(ProjectCleanerConstants::TabUnusedAssets, ETabState::OpenedTab)
				->AddTab(ProjectCleanerConstants::TabIndirectAssets, ETabState::OpenedTab)
				->AddTab(ProjectCleanerConstants::TabCorruptedAssets, ETabState::OpenedTab)
				->AddTab(ProjectCleanerConstants::TabNonEngineFiles, ETabState::OpenedTab)
				->SetSizeCoefficient(0.7f)
				->SetForegroundTab(ProjectCleanerConstants::TabUnusedAssets)
			)
		);

	FMenuBarBuilder MenuBarBuilder = FMenuBarBuilder(TSharedPtr<FUICommandList>());
	MenuBarBuilder.AddPullDownMenu(
		FText::FromString(TEXT("Settings")),
		FText::GetEmpty(),
		FNewMenuDelegate::CreateRaw(this, &SProjectCleaner::MenuBarFillSettings, TabManager),
		"Window"
	);
	MenuBarBuilder.AddPullDownMenu(
		FText::FromString(TEXT("Tabs")),
		FText::GetEmpty(),
		FNewMenuDelegate::CreateStatic(&SProjectCleaner::MenuBarFillTabs, TabManager),
		"Window"
	);
	MenuBarBuilder.AddPullDownMenu(
		FText::FromString(TEXT("Help")),
		FText::GetEmpty(),
		FNewMenuDelegate::CreateStatic(&SProjectCleaner::MenuBarFillHelp, TabManager),
		"Window"
	);

	ChildSlot
	[
		SNew(SWidgetSwitcher)
		.IsEnabled_Static(WidgetEnabled)
		.WidgetIndex_Static(WidgetGetIndex)
		+ SWidgetSwitcher::Slot()
		  .HAlign(HAlign_Fill)
		  .VAlign(VAlign_Fill)
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
				.Font(FProjectCleanerStyles::GetFont("Light", 30))
				.Text(FText::FromString(ProjectCleanerConstants::MsgAssetRegistryStillWorking))
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
				.Font(FProjectCleanerStyles::GetFont("Light", 30))
				.Text(FText::FromString(ProjectCleanerConstants::MsgPlayModeActive))
			]
		]
	];

	TabManager->SetMenuMultiBox(MenuBarBuilder.GetMultiBox());
}

SProjectCleaner::~SProjectCleaner()
{
	FGlobalTabmanager::Get()->UnregisterTabSpawner(ProjectCleanerConstants::TabScanSettings);
	FGlobalTabmanager::Get()->UnregisterTabSpawner(ProjectCleanerConstants::TabUnusedAssets);
	FGlobalTabmanager::Get()->UnregisterTabSpawner(ProjectCleanerConstants::TabIndirectAssets);
	FGlobalTabmanager::Get()->UnregisterTabSpawner(ProjectCleanerConstants::TabCorruptedAssets);
	FGlobalTabmanager::Get()->UnregisterTabSpawner(ProjectCleanerConstants::TabNonEngineFiles);
}

bool SProjectCleaner::WidgetEnabled()
{
	if (UProjectCleanerLibrary::AssetRegistryWorking())
	{
		return false;
	}

	if (GEditor && (GEditor->PlayWorld || GIsPlayInEditorWorld))
	{
		return false;
	}

	return true;
}

int32 SProjectCleaner::WidgetGetIndex()
{
	if (UProjectCleanerLibrary::AssetRegistryWorking())
	{
		return WidgetIndexLoading;
	}

	if (GEditor && (GEditor->PlayWorld || GIsPlayInEditorWorld))
	{
		return WidgetIndexInPlayMode;
	}

	return WidgetIndexNone;
}

void SProjectCleaner::MenuBarFillTabs(FMenuBuilder& MenuBuilder, const TSharedPtr<FTabManager> TabManagerPtr)
{
	if (!TabManagerPtr.IsValid()) return;


#if !WITH_EDITOR
	FGlobalTabmanager::Get()->PopulateTabSpawnerMenu(MenuBuilder, WorkspaceMenu::GetMenuStructure().GetStructureRoot());
#endif

	TabManagerPtr->PopulateLocalTabSpawnerMenu(MenuBuilder);
}

void SProjectCleaner::MenuBarFillSettings(FMenuBuilder& MenuBuilder, const TSharedPtr<FTabManager> TabManagerPtr) const
{
	FUIAction ActionAutoScan;
	ActionAutoScan.ExecuteAction = FExecuteAction::CreateLambda([&]()
	{
		ScanSettings->bAutoScan = !ScanSettings->bAutoScan;
		ScanSettings->PostEditChange();
	});
	ActionAutoScan.CanExecuteAction = FCanExecuteAction::CreateLambda([&]()
	{
		return ScanSettings != nullptr;
	});
	ActionAutoScan.GetActionCheckState = FGetActionCheckState::CreateLambda([&]()
	{
		return ScanSettings->bAutoScan ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
	});
	MenuBuilder.BeginSection("SectionHelp", FText::FromString(TEXT("Scan Settings")));
	MenuBuilder.AddMenuEntry(
		FText::FromString(TEXT("Auto Scan")),
		FText::FromString(TEXT("Automatically scan the project when settings change. On large projects, this can be unfavorable. By default, it is disabled.")),
		FSlateIcon(),
		ActionAutoScan,
		NAME_None,
		EUserInterfaceActionType::ToggleButton
	);

	FUIAction ActionAutoDeleteEmptyFolders;
	ActionAutoDeleteEmptyFolders.ExecuteAction = FExecuteAction::CreateLambda([&]()
	{
		ScanSettings->bAutoDeleteEmptyFolders = !ScanSettings->bAutoDeleteEmptyFolders;
		ScanSettings->PostEditChange();
	});
	ActionAutoDeleteEmptyFolders.CanExecuteAction = FCanExecuteAction::CreateLambda([&]()
	{
		return ScanSettings != nullptr;
	});
	ActionAutoDeleteEmptyFolders.GetActionCheckState = FGetActionCheckState::CreateLambda([&]()
	{
		return ScanSettings->bAutoDeleteEmptyFolders ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
	});
	MenuBuilder.AddMenuEntry(
		FText::FromString(TEXT("Auto Delete Empty Folders")),
		FText::FromString(TEXT("Automatically delete empty folders after cleaning a project of unused assets. By default, it is enabled.")),
		FSlateIcon(),
		ActionAutoDeleteEmptyFolders,
		NAME_None,
		EUserInterfaceActionType::ToggleButton
	);

	FUIAction ActionScanDevContent;
	ActionScanDevContent.ExecuteAction = FExecuteAction::CreateLambda([&]()
	{
		ScanSettings->bScanDeveloperContents = !ScanSettings->bScanDeveloperContents;
		ScanSettings->PostEditChange();
	});
	ActionScanDevContent.CanExecuteAction = FCanExecuteAction::CreateLambda([&]()
	{
		return ScanSettings != nullptr;
	});
	ActionScanDevContent.GetActionCheckState = FGetActionCheckState::CreateLambda([&]()
	{
		return ScanSettings->bScanDeveloperContents ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
	});
	MenuBuilder.AddMenuEntry(
		FText::FromString(TEXT("Scan Developers Content")),
		FText::FromString(TEXT("Scan the 'Developers' folder for unused assets. By default, it is disabled.")),
		FSlateIcon(),
		ActionScanDevContent,
		NAME_None,
		EUserInterfaceActionType::ToggleButton
	);
	MenuBuilder.EndSection();
}

void SProjectCleaner::MenuBarFillHelp(FMenuBuilder& MenuBuilder, const TSharedPtr<FTabManager> TabManagerPtr)
{
	FUIAction Action;
	Action.ExecuteAction = FExecuteAction::CreateLambda([]()
	{
		FPlatformProcess::LaunchURL(*ProjectCleanerConstants::UrlWiki, nullptr, nullptr);
	});
	Action.CanExecuteAction = FCanExecuteAction::CreateLambda([]()
	{
		return FPlatformProcess::CanLaunchURL(*ProjectCleanerConstants::UrlWiki);
	});

	MenuBuilder.BeginSection("SectionHelp", FText::FromString(TEXT("Help")));
	MenuBuilder.AddMenuEntry(FText::FromString(TEXT("Wiki")), FText::FromString(TEXT("Open wiki page on github")), FSlateIcon(), Action, NAME_None);
	MenuBuilder.EndSection();
}

TSharedRef<SDockTab> SProjectCleaner::OnTabSpawnScanSettings(const FSpawnTabArgs& Args)
{
	return
		SNew(SDockTab)
		.TabRole(PanelTab)
		.Label(FText::FromString(TEXT("Scan Settings")))
		.Icon(FProjectCleanerStyles::Get().GetBrush("ProjectCleaner.IconSettings16"))
		[
			SAssignNew(TabScanSettings, SProjectCleanerTabScanSettings).Scanner(Scanner)
		];
}

TSharedRef<SDockTab> SProjectCleaner::OnTabSpawnUnusedAssets(const FSpawnTabArgs& Args)
{
	return
		SNew(SDockTab)
		.TabRole(PanelTab)
		.Label(FText::FromString(TEXT("Unused Assets")))
		.Icon(FProjectCleanerStyles::Get().GetBrush("ProjectCleaner.IconTabUnused16"))
		[
			SAssignNew(TabUnused, SProjectCleanerTabUnused)
			.Scanner(Scanner)
			.RenderOpacity(TabsRenderOpacity)
			.IsEnabled(this, &SProjectCleaner::TabsEnabled)
		];
}

TSharedRef<SDockTab> SProjectCleaner::OnTabSpawnIndirectAssets(const FSpawnTabArgs& Args)
{
	return
		SNew(SDockTab)
		.TabRole(PanelTab)
		.Label(FText::FromString(TEXT("Indirect Assets")))
		.Icon(FProjectCleanerStyles::Get().GetBrush("ProjectCleaner.IconTabIndirect16"))
		[
			SAssignNew(TabIndirect, SProjectCleanerTabIndirect)
			.Scanner(Scanner)
			.RenderOpacity(TabsRenderOpacity)
			.IsEnabled(this, &SProjectCleaner::TabsEnabled)
		];
}

TSharedRef<SDockTab> SProjectCleaner::OnTabSpawnCorruptedAssets(const FSpawnTabArgs& Args)
{
	return
		SNew(SDockTab)
		.TabRole(PanelTab)
		.Label(FText::FromString(TEXT("Corrupted Assets")))
		.Icon(FProjectCleanerStyles::Get().GetBrush("ProjectCleaner.IconTabCorrupted16"))
		[
			SAssignNew(TabCorrupted, SProjectCleanerTabCorrupted)
			.Scanner(Scanner)
			.RenderOpacity(TabsRenderOpacity)
			.IsEnabled(this, &SProjectCleaner::TabsEnabled)
		];
}

TSharedRef<SDockTab> SProjectCleaner::OnTabSpawnNonEngineFiles(const FSpawnTabArgs& Args)
{
	return
		SNew(SDockTab)
		.TabRole(PanelTab)
		.Label(FText::FromString(TEXT("NonEngine Files")))
		.Icon(FProjectCleanerStyles::Get().GetBrush("ProjectCleaner.IconTabNonEngine16"))
		[
			SAssignNew(TabNonEngine, SProjectCleanerTabNonEngine)
			.Scanner(Scanner)
			.RenderOpacity(TabsRenderOpacity)
			.IsEnabled(this, &SProjectCleaner::TabsEnabled)
		];
}

bool SProjectCleaner::TabsEnabled() const
{
	return bTabsEnabled;
}

void SProjectCleaner::TabsUpdateRenderOpacity() const
{
	if (TabUnused.IsValid())
	{
		TabUnused.Pin()->SetRenderOpacity(TabsRenderOpacity);	
	}
	
	if (TabIndirect.IsValid())
	{
		TabIndirect.Pin()->SetRenderOpacity(TabsRenderOpacity);
	}

	if (TabCorrupted.IsValid())
	{
		TabCorrupted.Pin()->SetRenderOpacity(TabsRenderOpacity);
	}

	if (TabNonEngine.IsValid())
	{
		TabNonEngine.Pin()->SetRenderOpacity(TabsRenderOpacity);
	}
}
