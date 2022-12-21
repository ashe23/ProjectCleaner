// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/SProjectCleaner.h"
#include "Slate/Tabs/SProjectCleanerTabScanSettings.h"
#include "Slate/Tabs/SProjectCleanerTabScanInfo.h"
#include "Slate/Tabs/SProjectCleanerTabIndirectAssets.h"
#include "Slate/Tabs/SProjectCleanerTabCorruptedFiles.h"
#include "Slate/Tabs/SProjectCleanerTabNonEngineFiles.h"
#include "ProjectCleanerConstants.h"
#include "ProjectCleanerStyles.h"
#include "ProjectCleanerSubsystem.h"
// Engine Headers
#include "Settings/ContentBrowserSettings.h"
#include "Widgets/Layout/SWidgetSwitcher.h"

void SProjectCleaner::Construct(const FArguments& InArgs, const TSharedRef<SDockTab>& ConstructUnderMajorTab, const TSharedPtr<SWindow>& ConstructUnderWindow)
{
	if (!GEditor) return;
	SubsystemPtr = GEditor->GetEditorSubsystem<UProjectCleanerSubsystem>();
	if (!SubsystemPtr) return;

	// SubsystemPtr->ProjectScan();

	TabManager = FGlobalTabmanager::Get()->NewTabManager(ConstructUnderMajorTab);
	const TSharedRef<FWorkspaceItem> AppMenuGroup = TabManager->AddLocalWorkspaceMenuCategory(FText::FromString(ProjectCleanerConstants::ModuleName.ToString()));

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
				->AddTab(ProjectCleanerConstants::TabScanInfo, ETabState::OpenedTab)
				->AddTab(ProjectCleanerConstants::TabIndirectAssets, ETabState::OpenedTab)
				->AddTab(ProjectCleanerConstants::TabNonEngineFiles, ETabState::OpenedTab)
				->AddTab(ProjectCleanerConstants::TabCorruptedFiles, ETabState::OpenedTab)
				->SetSizeCoefficient(0.7f)
				->SetForegroundTab(ProjectCleanerConstants::TabScanInfo)
			)
		);

	TabManager->RegisterTabSpawner(ProjectCleanerConstants::TabScanSettings, FOnSpawnTab::CreateRaw(this, &SProjectCleaner::OnTabSpawnScanSettings))
	          .SetTooltipText(FText::FromString(TEXT("Open Scan Settings Tab")))
	          .SetDisplayName(FText::FromString(TEXT("Scan Settings")))
	          .SetIcon(FSlateIcon(FProjectCleanerStyles::GetStyleSetName(), "ProjectCleaner.IconSettings16"))
	          .SetGroup(AppMenuGroup);
	TabManager->RegisterTabSpawner(ProjectCleanerConstants::TabScanInfo, FOnSpawnTab::CreateRaw(this, &SProjectCleaner::OnTabSpawnScanInfo))
	          .SetTooltipText(FText::FromString(TEXT("Open Scan Info Tab")))
	          .SetDisplayName(FText::FromString(TEXT("Scan Info")))
	          .SetIcon(FSlateIcon(FProjectCleanerStyles::GetStyleSetName(), "ProjectCleaner.IconTabScanInfo16"))
	          .SetGroup(AppMenuGroup);
	TabManager->RegisterTabSpawner(ProjectCleanerConstants::TabIndirectAssets, FOnSpawnTab::CreateRaw(this, &SProjectCleaner::OnTabSpawnIndirectAssets))
	          .SetTooltipText(FText::FromString(TEXT("Open Indirect Assets Tab")))
	          .SetDisplayName(FText::FromString(TEXT("Indirect Assets")))
	          .SetIcon(FSlateIcon(FProjectCleanerStyles::GetStyleSetName(), "ProjectCleaner.IconTabIndirect16"))
	          .SetGroup(AppMenuGroup);
	TabManager->RegisterTabSpawner(ProjectCleanerConstants::TabNonEngineFiles, FOnSpawnTab::CreateRaw(this, &SProjectCleaner::OnTabSpawnNonEngineFiles))
	          .SetTooltipText(FText::FromString(TEXT("Open Non Engine Files Tab")))
	          .SetDisplayName(FText::FromString(TEXT("NonEngine Files")))
	          .SetIcon(FSlateIcon(FProjectCleanerStyles::GetStyleSetName(), "ProjectCleaner.IconTabNonEngine16"))
	          .SetGroup(AppMenuGroup);
	TabManager->RegisterTabSpawner(ProjectCleanerConstants::TabCorruptedFiles, FOnSpawnTab::CreateRaw(this, &SProjectCleaner::OnTabSpawnCorruptedFiles))
	          .SetTooltipText(FText::FromString(TEXT("Open Corrupted Files Tab")))
	          .SetDisplayName(FText::FromString(TEXT("Corrupted Files")))
	          .SetIcon(FSlateIcon(FProjectCleanerStyles::GetStyleSetName(), "ProjectCleaner.IconTabCorrupted16"))
	          .SetGroup(AppMenuGroup);

	FMenuBarBuilder MenuBarBuilder = FMenuBarBuilder(TSharedPtr<FUICommandList>());
	MenuBarBuilder.AddPullDownMenu(
		FText::FromString(TEXT("Settings")),
		FText::GetEmpty(),
		FNewMenuDelegate::CreateRaw(this, &SProjectCleaner::CreateMenuBarSettings, TabManager),
		"Window"
	);
	MenuBarBuilder.AddPullDownMenu(
		FText::FromString(TEXT("Tabs")),
		FText::GetEmpty(),
		FNewMenuDelegate::CreateRaw(this, &SProjectCleaner::CreateMenuBarTabs, TabManager),
		"Window"
	);

	ChildSlot
	[
		SNew(SWidgetSwitcher)
		.IsEnabled_Raw(this, &SProjectCleaner::WidgetEnabled)
		.WidgetIndex_Raw(this, &SProjectCleaner::WidgetGetIndex)
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
				.Text_Raw(this, &SProjectCleaner::WidgetText)
			]
		]
	];

	TabManager->SetMenuMultiBox(MenuBarBuilder.GetMultiBox());
}

SProjectCleaner::~SProjectCleaner()
{
	FGlobalTabmanager::Get()->UnregisterTabSpawner(ProjectCleanerConstants::TabScanSettings);
	FGlobalTabmanager::Get()->UnregisterTabSpawner(ProjectCleanerConstants::TabScanInfo);
	FGlobalTabmanager::Get()->UnregisterTabSpawner(ProjectCleanerConstants::TabIndirectAssets);
	FGlobalTabmanager::Get()->UnregisterTabSpawner(ProjectCleanerConstants::TabNonEngineFiles);
	FGlobalTabmanager::Get()->UnregisterTabSpawner(ProjectCleanerConstants::TabCorruptedFiles);
}

bool SProjectCleaner::WidgetEnabled() const
{
	if (!SubsystemPtr) return false;

	if (SubsystemPtr->AssetRegistryWorking() || SubsystemPtr->EditorInPlayMode())
	{
		return false;
	}

	return true;
}

int32 SProjectCleaner::WidgetGetIndex() const
{
	if (!SubsystemPtr) return ProjectCleanerConstants::WidgetIndexWorking;

	if (SubsystemPtr->AssetRegistryWorking() || SubsystemPtr->EditorInPlayMode())
	{
		return ProjectCleanerConstants::WidgetIndexWorking;
	}

	return ProjectCleanerConstants::WidgetIndexIdle;
}

FText SProjectCleaner::WidgetText() const
{
	if (!SubsystemPtr) return FText::FromString(TEXT(""));

	if (SubsystemPtr->AssetRegistryWorking())
	{
		return FText::FromString(TEXT("The AssetRegistry is still working. Please wait for the scan to finish"));
	}

	if (SubsystemPtr->EditorInPlayMode())
	{
		return FText::FromString(TEXT("Please stop play mode in the editor before doing any operations in the plugin."));
	}

	// if (SubsystemPtr->ScanningProject())
	// {
	// 	return FText::FromString(TEXT("Scanning Project ..."));
	// }
	//
	// if (SubsystemPtr->CleaningProject())
	// {
	// 	return FText::FromString(TEXT("Cleaning Project ..."));
	// }

	return FText::FromString(TEXT(""));
}

void SProjectCleaner::CreateMenuBarSettings(FMenuBuilder& MenuBuilder, const TSharedPtr<FTabManager> TabManagerPtr) const
{
	MenuBuilder.BeginSection(TEXT("SectionGeneral"), FText::FromString(TEXT("General Settings")));
	MenuBuilder.AddMenuEntry(
		FText::FromString(TEXT("Realtime Thumbnails")),
		FText::FromString(TEXT("Enabled realtime thumbnails in asset browser. By default is disabled")),
		FSlateIcon(),
		FUIAction
		(
			FExecuteAction::CreateLambda([&]()
			{
				if (!SubsystemPtr) return;

				SubsystemPtr->bShowRealtimeThumbnails = !SubsystemPtr->bShowRealtimeThumbnails;
				SubsystemPtr->PostEditChange();

				UContentBrowserSettings* Settings = GetMutableDefault<UContentBrowserSettings>();
				if (!Settings) return;

				Settings->RealTimeThumbnails = SubsystemPtr->bShowRealtimeThumbnails;
				Settings->PostEditChange();

				// todo:ashe23 must update asset browser ui
			}),
			FCanExecuteAction::CreateLambda([&]()
			{
				return SubsystemPtr != nullptr;
			}),
			FGetActionCheckState::CreateLambda([&]()
			{
				return SubsystemPtr && SubsystemPtr->bShowRealtimeThumbnails ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
			})
		),
		NAME_None,
		EUserInterfaceActionType::ToggleButton
	);
	MenuBuilder.EndSection();

	MenuBuilder.BeginSection(TEXT("SectionClean"), FText::FromString(TEXT("Clean Settings")));
	MenuBuilder.AddMenuEntry(
		FText::FromString(TEXT("Auto Clean Empty Folders")),
		FText::FromString(TEXT("Automatically delete empty folders after cleaning a project of unused assets. By default, it is enabled.")),
		FSlateIcon(),
		FUIAction
		(
			FExecuteAction::CreateLambda([&]()
			{
				if (!SubsystemPtr) return;

				SubsystemPtr->bAutoCleanEmptyFolders = !SubsystemPtr->bAutoCleanEmptyFolders;
				SubsystemPtr->PostEditChange();
			}),
			FCanExecuteAction::CreateLambda([&]()
			{
				return SubsystemPtr != nullptr;
			}),
			FGetActionCheckState::CreateLambda([&]()
			{
				return SubsystemPtr && SubsystemPtr->bAutoCleanEmptyFolders ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
			})
		),
		NAME_None,
		EUserInterfaceActionType::ToggleButton
	);
	MenuBuilder.EndSection();
}

void SProjectCleaner::CreateMenuBarTabs(FMenuBuilder& MenuBuilder, const TSharedPtr<FTabManager> TabManagerPtr) const
{
	if (!TabManagerPtr.IsValid()) return;

#if !WITH_EDITOR
	FGlobalTabmanager::Get()->PopulateTabSpawnerMenu(MenuBuilder, WorkspaceMenu::GetMenuStructure().GetStructureRoot());
#endif

	TabManagerPtr->PopulateLocalTabSpawnerMenu(MenuBuilder);
}

TSharedRef<SDockTab> SProjectCleaner::OnTabSpawnScanSettings(const FSpawnTabArgs& Args) const
{
	return
		SNew(SDockTab)
		.TabRole(PanelTab)
		.Label(FText::FromString(TEXT("Scan Settings")))
		.Icon(FProjectCleanerStyles::Get().GetBrush("ProjectCleaner.IconSettings16"))
		[
			SNew(SProjectCleanerTabScanSettings)
		];
}

TSharedRef<SDockTab> SProjectCleaner::OnTabSpawnScanInfo(const FSpawnTabArgs& Args) const
{
	return
		SNew(SDockTab)
		.TabRole(PanelTab)
		.Label(FText::FromString(TEXT("Scan Info")))
		.Icon(FProjectCleanerStyles::Get().GetBrush("ProjectCleaner.IconTabScanInfo16"))
		[
			SNew(SProjectCleanerTabScanInfo)
		];
}

TSharedRef<SDockTab> SProjectCleaner::OnTabSpawnIndirectAssets(const FSpawnTabArgs& Args) const
{
	return
		SNew(SDockTab)
		.TabRole(PanelTab)
		.Label(FText::FromString(TEXT("Indirect Assets")))
		.Icon(FProjectCleanerStyles::Get().GetBrush("ProjectCleaner.IconTabIndirect16"))
		[
			SNew(SProjectCleanerTabIndirect)
		];
}

TSharedRef<SDockTab> SProjectCleaner::OnTabSpawnNonEngineFiles(const FSpawnTabArgs& Args) const
{
	return
		SNew(SDockTab)
		.TabRole(PanelTab)
		.Label(FText::FromString(TEXT("NonEngine Files")))
		.Icon(FProjectCleanerStyles::Get().GetBrush("ProjectCleaner.IconTabNonEngine16"))
		[
			SNew(SProjectCleanerTabNonEngine)
		];
}

TSharedRef<SDockTab> SProjectCleaner::OnTabSpawnCorruptedFiles(const FSpawnTabArgs& Args) const
{
	return
		SNew(SDockTab)
		.TabRole(PanelTab)
		.Label(FText::FromString(TEXT("Corrupted Files")))
		.Icon(FProjectCleanerStyles::Get().GetBrush("ProjectCleaner.IconTabCorrupted16"))
		[
			SNew(SProjectCleanerTabCorrupted)
		];
}
