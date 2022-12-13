// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/SProjectCleaner.h"
#include "Slate/Tabs/SProjectCleanerTabScanSettings.h"
#include "ProjectCleanerConstants.h"
#include "ProjectCleanerStyles.h"
#include "ProjectCleanerSubsystem.h"
// Engine Headers
#include "Slate/Tabs/SProjectCleanerTabUnusedAssets.h"
#include "Widgets/Layout/SWidgetSwitcher.h"

void SProjectCleaner::Construct(const FArguments& InArgs, const TSharedRef<SDockTab>& ConstructUnderMajorTab, const TSharedPtr<SWindow>& ConstructUnderWindow)
{
	if (!GEditor) return;
	SubsystemPtr = GEditor->GetEditorSubsystem<UProjectCleanerSubsystem>();
	if (!SubsystemPtr) return;

	SubsystemPtr->ProjectScan();

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
				->AddTab(ProjectCleanerConstants::TabUnusedAssets, ETabState::OpenedTab)
				->AddTab(ProjectCleanerConstants::TabIndirectAssets, ETabState::OpenedTab)
				->AddTab(ProjectCleanerConstants::TabCorruptedAssets, ETabState::OpenedTab)
				->AddTab(ProjectCleanerConstants::TabNonEngineFiles, ETabState::OpenedTab)
				->SetSizeCoefficient(0.7f)
				->SetForegroundTab(ProjectCleanerConstants::TabUnusedAssets)
			)
		);

	TabManager->RegisterTabSpawner(ProjectCleanerConstants::TabScanSettings, FOnSpawnTab::CreateRaw(this, &SProjectCleaner::OnTabSpawnScanSettings))
	          .SetTooltipText(FText::FromString(TEXT("Open Scan Settings Tab")))
	          .SetDisplayName(FText::FromString(TEXT("Scan Settings")))
	          .SetIcon(FSlateIcon(FProjectCleanerStyles::GetStyleSetName(), "ProjectCleaner.IconSettings16"))
	          .SetGroup(AppMenuGroup);
	TabManager->RegisterTabSpawner(ProjectCleanerConstants::TabUnusedAssets, FOnSpawnTab::CreateRaw(this, &SProjectCleaner::OnTabSpawnUnusedAssets))
	          .SetTooltipText(FText::FromString(TEXT("Open Unused Assets Tab")))
	          .SetDisplayName(FText::FromString(TEXT("Unused Assets")))
	          .SetIcon(FSlateIcon(FProjectCleanerStyles::GetStyleSetName(), "ProjectCleaner.IconTabUnused16"))
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
	FGlobalTabmanager::Get()->UnregisterTabSpawner(ProjectCleanerConstants::TabUnusedAssets);
	// FGlobalTabmanager::Get()->UnregisterTabSpawner(ProjectCleanerConstants::TabIndirectAssets);
	// FGlobalTabmanager::Get()->UnregisterTabSpawner(ProjectCleanerConstants::TabCorruptedAssets);
	// FGlobalTabmanager::Get()->UnregisterTabSpawner(ProjectCleanerConstants::TabNonEngineFiles);
}

bool SProjectCleaner::WidgetEnabled() const
{
	if (!SubsystemPtr) return false;

	if (
		SubsystemPtr->IsAssetRegistryWorking() ||
		SubsystemPtr->IsEditorInPlayMode() ||
		SubsystemPtr->IsScanningProject() ||
		SubsystemPtr->IsCleaningProject()
	)
	{
		return false;
	}

	return true;
}

int32 SProjectCleaner::WidgetGetIndex() const
{
	if (!SubsystemPtr) return ProjectCleanerConstants::WidgetIndexWorking;

	if (
		SubsystemPtr->IsAssetRegistryWorking() ||
		SubsystemPtr->IsEditorInPlayMode() ||
		SubsystemPtr->IsScanningProject() ||
		SubsystemPtr->IsCleaningProject()
	)
	{
		return ProjectCleanerConstants::WidgetIndexWorking;
	}

	return ProjectCleanerConstants::WidgetIndexIdle;
}

FText SProjectCleaner::WidgetText() const
{
	if (!SubsystemPtr) return FText::FromString(TEXT(""));

	if (SubsystemPtr->IsAssetRegistryWorking())
	{
		return FText::FromString(TEXT("The AssetRegistry is still working. Please wait for the scan to finish"));
	}

	if (SubsystemPtr->IsEditorInPlayMode())
	{
		return FText::FromString(TEXT("Please stop play mode in the editor before doing any operations in the plugin."));
	}

	if (SubsystemPtr->IsScanningProject())
	{
		return FText::FromString(TEXT("Scanning Project ..."));
	}

	if (SubsystemPtr->IsCleaningProject())
	{
		return FText::FromString(TEXT("Cleaning Project ..."));
	}

	return FText::FromString(TEXT(""));
}

void SProjectCleaner::CreateMenuBarSettings(FMenuBuilder& MenuBuilder, const TSharedPtr<FTabManager> TabManagerPtr) const
{
	FUIAction ActionAutoDeleteEmptyFolders;
	ActionAutoDeleteEmptyFolders.ExecuteAction = FExecuteAction::CreateLambda([&]()
	{
		if (!SubsystemPtr) return;

		SubsystemPtr->bAutoCleanEmptyFolders = !SubsystemPtr->bAutoCleanEmptyFolders;
		SubsystemPtr->PostEditChange();
	});
	ActionAutoDeleteEmptyFolders.CanExecuteAction = FCanExecuteAction::CreateLambda([&]()
	{
		return SubsystemPtr != nullptr;
	});
	ActionAutoDeleteEmptyFolders.GetActionCheckState = FGetActionCheckState::CreateLambda([&]()
	{
		return SubsystemPtr && SubsystemPtr->bAutoCleanEmptyFolders ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
	});

	MenuBuilder.BeginSection(NAME_None);
	MenuBuilder.AddMenuEntry(
		FText::FromString(TEXT("Auto Clean Empty Folders")),
		FText::FromString(TEXT("Automatically delete empty folders after cleaning a project of unused assets. By default, it is enabled.")),
		FSlateIcon(),
		ActionAutoDeleteEmptyFolders,
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

TSharedRef<SDockTab> SProjectCleaner::OnTabSpawnUnusedAssets(const FSpawnTabArgs& Args) const
{
	return
		SNew(SDockTab)
		.TabRole(PanelTab)
		.Label(FText::FromString(TEXT("Unused Assets")))
		.Icon(FProjectCleanerStyles::Get().GetBrush("ProjectCleaner.IconTabUnused16"))
		[
			SNew(SProjectCleanerTabUnusedAssets)
		];
}
