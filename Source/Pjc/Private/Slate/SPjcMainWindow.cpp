// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/SPjcMainWindow.h"
#include "PjcConstants.h"
#include "PjcStyles.h"
#include "Slate/FileBrowser/SPjcFileBrowser.h"
#include "Slate/AssetBrowser/SPjcAssetBrowser.h"

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
				->AddTab(PjcConstants::TabAssetsBrowser, ETabState::OpenedTab)
				->AddTab(PjcConstants::TabFilesBrowser, ETabState::OpenedTab)
				->SetForegroundTab(PjcConstants::TabAssetsBrowser)
				->SetSizeCoefficient(1.0f)
			)
		);

	TabManager->RegisterTabSpawner(PjcConstants::TabAssetsBrowser, FOnSpawnTab::CreateRaw(this, &SPjcMainWindow::OnTabAssetsBrowserSpawn))
	          .SetTooltipText(FText::FromString(TEXT("Open Assets Browser Tab")))
	          .SetDisplayName(FText::FromString(TEXT("Assets Browser")))
	          .SetIcon(FPjcStyles::GetIcon("ProjectCleaner.IconTabAssetsBrowser16"))
	          .SetGroup(AppMenuGroup);

	TabManager->RegisterTabSpawner(PjcConstants::TabFilesBrowser, FOnSpawnTab::CreateRaw(this, &SPjcMainWindow::OnTabFilesBrowserSpawn))
	          .SetTooltipText(FText::FromString(TEXT("Open Files Browser Tab")))
	          .SetDisplayName(FText::FromString(TEXT("File Browser")))
	          .SetIcon(FPjcStyles::GetIcon("ProjectCleaner.IconTabFilesBrowser16"))
	          .SetGroup(AppMenuGroup);

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
		+ SVerticalBox::Slot().AutoHeight()
		[
			MenuBarBuilder.MakeWidget()
		]
		+ SVerticalBox::Slot().FillHeight(1.0f)
		[
			TabManager->RestoreFrom(TabLayout.ToSharedRef(), ConstructUnderWindow).ToSharedRef()
		]
	];

	TabManager->SetMenuMultiBox(MenuBarBuilder.GetMultiBox());
}

SPjcMainWindow::~SPjcMainWindow()
{
	FGlobalTabmanager::Get()->UnregisterTabSpawner(PjcConstants::TabAssetsBrowser);
	FGlobalTabmanager::Get()->UnregisterTabSpawner(PjcConstants::TabFilesBrowser);
}

void SPjcMainWindow::CreateMenuBarTabs(FMenuBuilder& MenuBuilder, const TSharedPtr<FTabManager> TabManagerPtr)
{
	if (!TabManagerPtr.IsValid()) return;

#if !WITH_EDITOR
	FGlobalTabmanager::Get()->PopulateTabSpawnerMenu(MenuBuilder, WorkspaceMenu::GetMenuStructure().GetStructureRoot());
#endif

	TabManagerPtr->PopulateLocalTabSpawnerMenu(MenuBuilder);
}

TSharedRef<SDockTab> SPjcMainWindow::OnTabAssetsBrowserSpawn(const FSpawnTabArgs& Args) const
{
	return
		SNew(SDockTab)
		.TabRole(PanelTab)
		.Label(FText::FromString(TEXT("Assets Browser")))
		.Icon(FPjcStyles::Get().GetBrush("ProjectCleaner.IconTabAssetsBrowser16"))
		[
			SNew(SPjcAssetBrowser)
		];
}

TSharedRef<SDockTab> SPjcMainWindow::OnTabFilesBrowserSpawn(const FSpawnTabArgs& Args) const
{
	return
		SNew(SDockTab)
		.TabRole(PanelTab)
		.Label(FText::FromString(TEXT("File Browser")))
		.Icon(FPjcStyles::Get().GetBrush("ProjectCleaner.IconTabFilesBrowser16"))
		.ToolTipText(FText::FromString(TEXT("Manage external and corrupted asset files in project")))
		[
			SNew(SPjcFileBrowser)
		];
}
