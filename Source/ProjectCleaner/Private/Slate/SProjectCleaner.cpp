// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/SProjectCleaner.h"
#include "Slate/Tabs/SProjectCleanerTabScanSettings.h"
#include "Slate/Tabs/SProjectCleanerTabScanInfo.h"
#include "Slate/Tabs/SProjectCleanerTabIndirectAssets.h"
#include "Slate/Tabs/SProjectCleanerTabCorruptedFiles.h"
#include "Slate/Tabs/SProjectCleanerTabNonEngineFiles.h"
#include "ProjectCleanerConstants.h"
#include "ProjectCleanerStyles.h"
#include "Libs/ProjectCleanerLibAsset.h"
#include "Libs/ProjectCleanerLibEditor.h"
// Engine Headers
#include "Widgets/Layout/SWidgetSwitcher.h"

void SProjectCleaner::Construct(const FArguments& InArgs, const TSharedRef<SDockTab>& ConstructUnderMajorTab, const TSharedPtr<SWindow>& ConstructUnderWindow)
{
	SubsystemPtr = GEditor->GetEditorSubsystem<UProjectCleanerSubsystem>();
	check(SubsystemPtr);

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
		FText::FromString(TEXT("Tabs")),
		FText::GetEmpty(),
		FNewMenuDelegate::CreateStatic(&SProjectCleaner::CreateMenuBarTabs, TabManager),
		"Window"
	);

	ChildSlot
	[
		SNew(SWidgetSwitcher)
		.IsEnabled_Static(&SProjectCleaner::WidgetEnabled)
		.WidgetIndex_Static(&SProjectCleaner::WidgetGetIndex)
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

bool SProjectCleaner::WidgetEnabled()
{
	return !(UProjectCleanerLibAsset::AssetRegistryWorking() && UProjectCleanerLibEditor::EditorInPlayMode());
}

int32 SProjectCleaner::WidgetGetIndex()
{
	if (UProjectCleanerLibAsset::AssetRegistryWorking() || UProjectCleanerLibEditor::EditorInPlayMode())
	{
		return ProjectCleanerConstants::WidgetIndexWorking;
	}

	return ProjectCleanerConstants::WidgetIndexIdle;
}

FText SProjectCleaner::WidgetText() const
{
	if (UProjectCleanerLibAsset::AssetRegistryWorking())
	{
		return FText::FromString(TEXT("The AssetRegistry is still working. Please wait for the scan to finish"));
	}

	if (UProjectCleanerLibEditor::EditorInPlayMode())
	{
		return FText::FromString(TEXT("Please stop play mode in the editor before doing any operations in the plugin."));
	}

	return FText::FromString(TEXT(""));
}

void SProjectCleaner::CreateMenuBarTabs(FMenuBuilder& MenuBuilder, const TSharedPtr<FTabManager> TabManagerPtr)
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
			// SNew(SProjectCleanerTabScanInfo)
			SNew(STextBlock)
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
			// SNew(SProjectCleanerTabIndirect)
			SNew(STextBlock)
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
			// SNew(SProjectCleanerTabNonEngine)
			SNew(STextBlock)
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
			// SNew(SProjectCleanerTabCorrupted)
			SNew(STextBlock)
		];
}
