// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/SProjectCleaner.h"
#include "Slate/Tabs/SProjectCleanerTabScanSettings.h"
#include "ProjectCleanerConstants.h"
#include "ProjectCleanerStyles.h"
// Engine Headers
#include "Widgets/Layout/SWidgetSwitcher.h"

void SProjectCleaner::Construct(const FArguments& InArgs, const TSharedRef<SDockTab>& ConstructUnderMajorTab, const TSharedPtr<SWindow>& ConstructUnderWindow)
{
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

	// FMenuBarBuilder MenuBarBuilder = FMenuBarBuilder(TSharedPtr<FUICommandList>());
	// MenuBarBuilder.AddPullDownMenu(
	// 	FText::FromString(TEXT("Settings")),
	// 	FText::GetEmpty(),
	// 	FNewMenuDelegate::CreateRaw(this, &SProjectCleaner::MenuBarFillSettings, TabManager),
	// 	"Window"
	// );
	// MenuBarBuilder.AddPullDownMenu(
	// 	FText::FromString(TEXT("Tabs")),
	// 	FText::GetEmpty(),
	// 	FNewMenuDelegate::CreateStatic(&SProjectCleaner::MenuBarFillTabs, TabManager),
	// 	"Window"
	// );
	// MenuBarBuilder.AddPullDownMenu(
	// 	FText::FromString(TEXT("Help")),
	// 	FText::GetEmpty(),
	// 	FNewMenuDelegate::CreateStatic(&SProjectCleaner::MenuBarFillHelp, TabManager),
	// 	"Window"
	// );

	ChildSlot
	[
		SNew(SWidgetSwitcher)
		// .IsEnabled_Raw(this, &SProjectCleaner::WidgetEnabled)
		// .WidgetIndex_Raw(this, &SProjectCleaner::WidgetGetIndex)
		+ SWidgetSwitcher::Slot()
		  .HAlign(HAlign_Fill)
		  .VAlign(VAlign_Fill)
		[
			SNew(SVerticalBox)
			// + SVerticalBox::Slot()
			// .AutoHeight()
			// [
			// 	MenuBarBuilder.MakeWidget()
			// ]
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
				// .Text_Raw(this, &SProjectCleaner::WidgetText)
			]
		]
	];
}

SProjectCleaner::~SProjectCleaner()
{
	FGlobalTabmanager::Get()->UnregisterTabSpawner(ProjectCleanerConstants::TabScanSettings);
	// FGlobalTabmanager::Get()->UnregisterTabSpawner(ProjectCleanerConstants::TabUnusedAssets);
	// FGlobalTabmanager::Get()->UnregisterTabSpawner(ProjectCleanerConstants::TabIndirectAssets);
	// FGlobalTabmanager::Get()->UnregisterTabSpawner(ProjectCleanerConstants::TabCorruptedAssets);
	// FGlobalTabmanager::Get()->UnregisterTabSpawner(ProjectCleanerConstants::TabNonEngineFiles);
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
