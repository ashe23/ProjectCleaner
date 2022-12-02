﻿// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/SProjectCleaner.h"
#include "Slate/Tabs/SProjectCleanerFileListView.h"
#include "Slate/Tabs/SProjectCleanerTabIndirect.h"
#include "Slate/Tabs/SProjectCleanerTabUnused.h"
#include "ProjectCleanerScanSettings.h"
#include "ProjectCleanerScanner.h"
#include "ProjectCleanerConstants.h"
#include "ProjectCleanerStyles.h"
#include "ProjectCleanerLibrary.h"
// Engine Headers
#include "ContentBrowserModule.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SWidgetSwitcher.h"

static constexpr int32 WidgetIndexNone = 0;
static constexpr int32 WidgetIndexLoading = 1;

void SProjectCleaner::Construct(const FArguments& InArgs, const TSharedRef<SDockTab>& ConstructUnderMajorTab, const TSharedPtr<SWindow>& ConstructUnderWindow)
{
	ScanSettings = GetMutableDefault<UProjectCleanerScanSettings>();
	if (!ScanSettings.IsValid()) return;

	Scanner = MakeShareable(new FProjectCleanerScanner);
	if (!Scanner.IsValid()) return;

	Scanner.Get()->Scan(ScanSettings);

	// ScanSettings->OnChange().AddLambda([&]()
	// {
	// 	Scanner.Get()->Scan(ScanSettings);
	// });

	FPropertyEditorModule& PropertyEditor = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.bUpdatesFromSelection = false;
	DetailsViewArgs.bLockable = false;
	DetailsViewArgs.bAllowSearch = false;
	DetailsViewArgs.bShowOptions = false;
	DetailsViewArgs.bAllowFavoriteSystem = false;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	DetailsViewArgs.ViewIdentifier = "ProjectCleanerScanSettings";

	ScanSettingsProperty = PropertyEditor.CreateDetailView(DetailsViewArgs);
	ScanSettingsProperty->SetObject(ScanSettings.Get());

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
	];

	TabManager->SetMenuMultiBox(MenuBarBuilder.GetMultiBox());
}

void SProjectCleaner::UpdateView() const
{
	if (TabUnused.IsValid())
	{
		TabUnused.Pin().Get()->UpdateView();
	}
	
	if (TabIndirect.IsValid())
	{
		TabIndirect.Pin().Get()->UpdateView();
	}

	if (TabCorrupted.IsValid())
	{
		TabCorrupted.Pin().Get()->UpdateView(Scanner->GetFilesCorrupted());
	}

	if (TabNonEngine.IsValid())
	{
		TabNonEngine.Pin().Get()->UpdateView(Scanner->GetFilesNonEngine());
	}
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
	return !UProjectCleanerLibrary::IsAssetRegistryWorking();
}

int32 SProjectCleaner::WidgetGetIndex()
{
	return UProjectCleanerLibrary::IsAssetRegistryWorking() ? WidgetIndexLoading : WidgetIndexNone;
}

void SProjectCleaner::MenuBarFillTabs(FMenuBuilder& MenuBuilder, const TSharedPtr<FTabManager> TabManager)
{
	if (!TabManager.IsValid()) return;


#if !WITH_EDITOR
	FGlobalTabmanager::Get()->PopulateTabSpawnerMenu(MenuBuilder, WorkspaceMenu::GetMenuStructure().GetStructureRoot());
#endif

	TabManager->PopulateLocalTabSpawnerMenu(MenuBuilder);
}

void SProjectCleaner::MenuBarFillHelp(FMenuBuilder& MenuBuilder, const TSharedPtr<FTabManager> TabManager)
{
	FUIAction Action;
	Action.ExecuteAction = FExecuteAction::CreateLambda([]()
	{
		if (FPlatformProcess::CanLaunchURL(*ProjectCleanerConstants::UrlWiki))
		{
			FPlatformProcess::LaunchURL(*ProjectCleanerConstants::UrlWiki, nullptr, nullptr);
		}
	});

	MenuBuilder.BeginSection("SectionHelp", FText::FromString(TEXT("Help")));
	MenuBuilder.AddMenuEntry(FText::FromString(TEXT("Documentation")), FText::FromString(TEXT("Open documentation page")), FSlateIcon(), Action, NAME_None);
	MenuBuilder.EndSection();
}

FReply SProjectCleaner::OnBtnScanProjectClick()
{
	if (!Scanner.IsValid()) return FReply::Handled();

	Scanner.Get()->Scan(ScanSettings);

	UpdateView();

	return FReply::Handled();
}

FReply SProjectCleaner::OnBtnCleanProjectClick() const
{
	return FReply::Handled();
}

FReply SProjectCleaner::OnBtnDeleteEmptyFoldersClick() const
{
	return FReply::Handled();
}

TSharedRef<SDockTab> SProjectCleaner::OnTabSpawnScanSettings(const FSpawnTabArgs& Args)
{
	return
		SNew(SDockTab)
		.TabRole(PanelTab)
		.Label(FText::FromString(TEXT("Scan Settings")))
		.Icon(FProjectCleanerStyles::Get().GetBrush("ProjectCleaner.IconSettings16"))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			  .AutoHeight()
			  .Padding(FMargin{5.0f})
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				  .AutoWidth()
				  .Padding(FMargin{0.0f, 0.0f, 5.0f, 0.0f})
				[
					SNew(SButton)
					.ContentPadding(FMargin{5.0f})
					.OnClicked_Raw(this, &SProjectCleaner::OnBtnScanProjectClick)
					.ButtonColorAndOpacity(FProjectCleanerStyles::Get().GetColor("ProjectCleaner.Color.Blue"))
					[
						SNew(STextBlock)
						.Justification(ETextJustify::Center)
						.ToolTipText(FText::FromString(TEXT("Scan project for unused assets, empty folders, non engine files or corrupted assets")))
						.ColorAndOpacity(FProjectCleanerStyles::Get().GetColor("ProjectCleaner.Color.White"))
						.ShadowOffset(FVector2D{1.5f, 1.5f})
						.ShadowColorAndOpacity(FLinearColor::Black)
						.Font(FProjectCleanerStyles::GetFont("Bold", 10))
						.Text(FText::FromString(TEXT("Scan Project")))
					]
				]
				+ SHorizontalBox::Slot()
				  .AutoWidth()
				  .Padding(FMargin{0.0f, 0.0f, 5.0f, 0.0f})
				[
					SNew(SButton)
					.ContentPadding(FMargin{5.0f})
					.OnClicked_Raw(this, &SProjectCleaner::OnBtnCleanProjectClick)
					.ButtonColorAndOpacity(FProjectCleanerStyles::Get().GetColor("ProjectCleaner.Color.Red"))
					[
						SNew(STextBlock)
						.Justification(ETextJustify::Center)
						.ToolTipText(FText::FromString(TEXT("Remove all unused assets and empty folders in project. This wont delete any excluded asset")))
						.ColorAndOpacity(FProjectCleanerStyles::Get().GetColor("ProjectCleaner.Color.White"))
						.ShadowOffset(FVector2D{1.5f, 1.5f})
						.ShadowColorAndOpacity(FLinearColor::Black)
						.Font(FProjectCleanerStyles::GetFont("Bold", 10))
						.Text(FText::FromString(TEXT("Clean Project")))
					]
				]
				+ SHorizontalBox::Slot()
				  .AutoWidth()
				  .Padding(FMargin{0.0f, 0.0f, 5.0f, 0.0f})
				[
					SNew(SButton)
					.ContentPadding(FMargin{5.0f})
					.OnClicked_Raw(this, &SProjectCleaner::OnBtnDeleteEmptyFoldersClick)
					.ButtonColorAndOpacity(FProjectCleanerStyles::Get().GetColor("ProjectCleaner.Color.Red"))
					[
						SNew(STextBlock)
						.Justification(ETextJustify::Center)
						.ToolTipText(FText::FromString(TEXT("Remove all empty folders in project")))
						.ColorAndOpacity(FProjectCleanerStyles::Get().GetColor("ProjectCleaner.Color.White"))
						.ShadowOffset(FVector2D{1.5f, 1.5f})
						.ShadowColorAndOpacity(FLinearColor::Black)
						.Font(FProjectCleanerStyles::GetFont("Bold", 10))
						.Text(FText::FromString(TEXT("Delete Empty Folders")))
					]
				]
			]
			+ SVerticalBox::Slot()
			  .Padding(FMargin{5.0f})
			  .AutoHeight()
			  [
			  	SNew(STextBlock)
			  	.Font(FProjectCleanerStyles::GetFont("Light", 8))
			  	.Text(FText::FromString(TEXT("Please Scan Project after changing scan settings")))
			  ]
			+ SVerticalBox::Slot()
			  .Padding(FMargin{5.0f})
			  .FillHeight(1.0f)
			[
				SNew(SBox)
				[
					SNew(SScrollBox)
					.ScrollWhenFocusChanges(EScrollWhenFocusChanges::NoScroll)
					.AnimateWheelScrolling(true)
					.AllowOverscroll(EAllowOverscroll::No)
					+ SScrollBox::Slot()
					[
						ScanSettingsProperty.ToSharedRef()
					]
				]
			]
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
			SAssignNew(TabCorrupted, SProjectCleanerFileListView)
			.Title(TEXT("List of possibly corrupted assets, that exist in Content folder, but not loaded by engine"))
			.Description(TEXT("In order to fix, try to reload project and see if its loaded. Otherwise close editor and remove them manually from explorer"))
			.Files(Scanner.Get()->GetFilesCorrupted())
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
			SAssignNew(TabNonEngine, SProjectCleanerFileListView)
			.Title(TEXT("List of Non Engine files inside Content folder"))
			.Description(TEXT(
				                                                     "Sometimes you will see empty folder in ContentBrowser, which you cant delete. Its because its contains some non engine files visible only in Explorer. So make sure you delete all unnecessary files from list below to cleanup empty folders."))
			.Files(Scanner.Get()->GetFilesNonEngine())
		];
}