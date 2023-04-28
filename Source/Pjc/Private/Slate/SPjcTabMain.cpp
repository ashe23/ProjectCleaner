// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/SPjcTabMain.h"
#include "Slate/SPjcTabAssetsInspection.h"
#include "Slate/SPjcTabAssetsUnused.h"
#include "Slate/SPjcTabFilesExternal.h"
#include "PjcConstants.h"
#include "PjcStyles.h"
#include "Libs/PjcLibEditor.h"
// Engine Headers
#include "Widgets/Layout/SWidgetSwitcher.h"

void SPjcTabMain::Construct(const FArguments& InArgs, const TSharedRef<SDockTab>& ConstructUnderMajorTab, const TSharedPtr<SWindow>& ConstructUnderWindow)
{
	TabManager = FGlobalTabmanager::Get()->NewTabManager(ConstructUnderMajorTab);
	const TSharedRef<FWorkspaceItem> AppMenuGroup = TabManager->AddLocalWorkspaceMenuCategory(FText::FromString(PjcConstants::ModulePjcName.ToString()));

	TabLayout = FTabManager::NewLayout("PjcTabLayout")
		->AddArea
		(
			FTabManager::NewPrimaryArea()
			->SetOrientation(Orient_Horizontal)
			->Split
			(
				FTabManager::NewStack()
				->AddTab(PjcConstants::TabAssetsUnused, ETabState::OpenedTab)
				->AddTab(PjcConstants::TabFilesExternal, ETabState::OpenedTab)
				->AddTab(PjcConstants::TabAssetsInspection, ETabState::OpenedTab)
				->SetForegroundTab(PjcConstants::TabAssetsUnused)
				->SetSizeCoefficient(1.0f)
			)
		);

	TabManager->RegisterTabSpawner(PjcConstants::TabAssetsUnused, FOnSpawnTab::CreateRaw(this, &SPjcTabMain::OnTabAssetsUnusedSpawn))
	          .SetTooltipText(FText::FromString(TEXT("Open Unused Assets Tab")))
	          .SetDisplayName(FText::FromString(TEXT("Assets Unused")))
	          .SetIcon(FPjcStyles::GetIcon("ProjectCleaner.Icon.PieChart16"))
	          .SetGroup(AppMenuGroup);

	TabManager->RegisterTabSpawner(PjcConstants::TabAssetsInspection, FOnSpawnTab::CreateRaw(this, &SPjcTabMain::OnTabAssetsInspectionSpawn))
	          .SetTooltipText(FText::FromString(TEXT("Open Assets Inspection Tab")))
	          .SetDisplayName(FText::FromString(TEXT("Assets Inspection")))
	          .SetIcon(FPjcStyles::GetIcon("ProjectCleaner.Icon.Stat16"))
	          .SetGroup(AppMenuGroup);

	TabManager->RegisterTabSpawner(PjcConstants::TabFilesExternal, FOnSpawnTab::CreateRaw(this, &SPjcTabMain::OnTabFilesExternalSpawn))
	          .SetTooltipText(FText::FromString(TEXT("Open External Files Tab")))
	          .SetDisplayName(FText::FromString(TEXT("Files External")))
	          .SetIcon(FPjcStyles::GetIcon("ProjectCleaner.Icon.File16"))
	          .SetGroup(AppMenuGroup);

	FMenuBarBuilder MenuBarBuilder = FMenuBarBuilder(TSharedPtr<FUICommandList>());
	MenuBarBuilder.AddPullDownMenu(
		FText::FromString(TEXT("Tabs")),
		FText::GetEmpty(),
		FNewMenuDelegate::CreateRaw(this, &SPjcTabMain::CreateMenuBarTabs, TabManager),
		"Window"
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

	TabManager->SetMenuMultiBox(MenuBarBuilder.GetMultiBox());
}

SPjcTabMain::~SPjcTabMain()
{
	FGlobalTabmanager::Get()->UnregisterTabSpawner(PjcConstants::TabAssetsUnused);
	FGlobalTabmanager::Get()->UnregisterTabSpawner(PjcConstants::TabFilesExternal);
	FGlobalTabmanager::Get()->UnregisterTabSpawner(PjcConstants::TabAssetsInspection);
}

int32 SPjcTabMain::GetWidgetIndex() const
{
	return FPjcLibEditor::EditorInPlayMode() || FPjcLibEditor::AssetRegistryIsWorking() ? PjcConstants::WidgetIndexWorking : PjcConstants::WidgetIndexIdle;
}

FText SPjcTabMain::GetWidgetWarningText() const
{
	if (FPjcLibEditor::EditorInPlayMode())
	{
		return FText::FromString(TEXT("Please exit the editor's play mode before performing any operations in the plugin."));
	}

	if (FPjcLibEditor::AssetRegistryIsWorking())
	{
		return FText::FromString(TEXT("Please wait until the Asset Registry has discovered all assets in the project."));
	}

	return FText::FromString(TEXT(""));
}

TSharedRef<SDockTab> SPjcTabMain::OnTabAssetsUnusedSpawn(const FSpawnTabArgs& Args) const
{
	return
		SNew(SDockTab)
		.TabRole(PanelTab)
		.Label(FText::FromString(TEXT("Assets Unused")))
		.Icon(FPjcStyles::Get().GetBrush("ProjectCleaner.Icon.PieChart16"))
		[
			SNew(SPjcTabAssetsUnused)
		];
}

TSharedRef<SDockTab> SPjcTabMain::OnTabAssetsInspectionSpawn(const FSpawnTabArgs& Args) const
{
	return
		SNew(SDockTab)
		.TabRole(PanelTab)
		.Label(FText::FromString(TEXT("Assets Inspection")))
		.Icon(FPjcStyles::Get().GetBrush("ProjectCleaner.Icon.Stat16"))
		[
			SNew(SPjcTabAssetsInspection)
		];
}

TSharedRef<SDockTab> SPjcTabMain::OnTabFilesExternalSpawn(const FSpawnTabArgs& Args) const
{
	return
		SNew(SDockTab)
		.TabRole(PanelTab)
		.Label(FText::FromString(TEXT("Files External")))
		.Icon(FPjcStyles::Get().GetBrush("ProjectCleaner.Icon.File16"))
		[
			SNew(SPjcTabFilesExternal)
		];
}

void SPjcTabMain::CreateMenuBarTabs(FMenuBuilder& MenuBuilder, const TSharedPtr<FTabManager> TabManagerPtr)
{
	if (!TabManagerPtr.IsValid()) return;

#if !WITH_EDITOR
	FGlobalTabmanager::Get()->PopulateTabSpawnerMenu(MenuBuilder, WorkspaceMenu::GetMenuStructure().GetStructureRoot());
#endif

	TabManagerPtr->PopulateLocalTabSpawnerMenu(MenuBuilder);
}
