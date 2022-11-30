// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/ProjectCleanerWindowMain.h"
#include "Slate/ProjectCleanerWindowIndirectAssets.h"
#include "Slate/ProjectCleanerFileListView.h"
#include "Slate/ProjectCleanerAssetBrowser.h"
#include "Slate/TreeView/SProjectCleanerTreeView.h"
#include "ProjectCleanerStyles.h"
#include "ProjectCleanerLibrary.h"
#include "ProjectCleanerConstants.h"
#include "ProjectCleanerScanSettings.h"
// Engine Headers
#include "ProjectCleaner.h"
#include "Widgets/Input/SHyperlink.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SWidgetSwitcher.h"

static constexpr int32 WidgetIndexNone = 0;
static constexpr int32 WidgetIndexLoading = 1;

void SProjectCleanerWindowMain::Construct(const FArguments& InArgs)
{
	ScanSettings = GetMutableDefault<UProjectCleanerScanSettings>();
	check(ScanSettings.IsValid());

	TabsRegister();

	ProjectCleanerScanner = MakeShareable(new FProjectCleanerScanner());
	ProjectCleanerScanner->Scan(ScanSettings);

	ScanSettings->OnChange().AddLambda([&]()
	{
		ProjectCleanerScanner->Scan(ScanSettings);
	});

	FPropertyEditorModule& PropertyEditor = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.bUpdatesFromSelection = false;
	DetailsViewArgs.bLockable = false;
	DetailsViewArgs.bAllowSearch = false;
	DetailsViewArgs.bShowOptions = false;
	DetailsViewArgs.bAllowFavoriteSystem = false;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	DetailsViewArgs.ViewIdentifier = "ProjectCleanerScanSettings";

	const TSharedRef<IDetailsView> ScanSettingsProperty = PropertyEditor.CreateDetailView(DetailsViewArgs);
	ScanSettingsProperty->SetObject(ScanSettings.Get());
	
	ChildSlot
	[
		SNew(SWidgetSwitcher)
		.IsEnabled_Static(IsWidgetEnabled)
		.WidgetIndex_Static(GetWidgetIndex)
		+ SWidgetSwitcher::Slot()
		  .HAlign(HAlign_Fill)
		  .VAlign(VAlign_Fill)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			  .AutoHeight()
			  .Padding(FMargin{10.0f})
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
						.OnClicked_Raw(this, &SProjectCleanerWindowMain::OnBtnScanProjectClicked)
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
						.OnClicked_Raw(this, &SProjectCleanerWindowMain::OnBtnCleanProjectClicked)
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
						.OnClicked_Raw(this, &SProjectCleanerWindowMain::OnBtnDeleteEmptyFoldersClicked)
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
			]
			+ SVerticalBox::Slot()
			  .FillHeight(1.0f)
			  .Padding(FMargin{10.0f})
			[
				SNew(SSplitter)
				.Style(FEditorStyle::Get(), "ContentBrowser.Splitter")
				.PhysicalSplitterHandleSize(5.0f)
				+ SSplitter::Slot()
				.Value(0.3f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					  .FillHeight(1.0f)
					  .Padding(FMargin{5.0f})
					[
						SNew(SScrollBox)
						.ScrollWhenFocusChanges(EScrollWhenFocusChanges::NoScroll)
						.AnimateWheelScrolling(true)
						.AllowOverscroll(EAllowOverscroll::No)
						+ SScrollBox::Slot()
						[
							ScanSettingsProperty
						]
					]
				]
				+ SSplitter::Slot()
				[
					TabManager->RestoreFrom(TabLayout.ToSharedRef(), TSharedPtr<SWindow>()).ToSharedRef()
				]
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
}

SProjectCleanerWindowMain::~SProjectCleanerWindowMain()
{
	TabsUnregister();
}

bool SProjectCleanerWindowMain::IsWidgetEnabled()
{
	return !UProjectCleanerLibrary::IsAssetRegistryWorking();
}

int32 SProjectCleanerWindowMain::GetWidgetIndex()
{
	return UProjectCleanerLibrary::IsAssetRegistryWorking() ? WidgetIndexLoading : WidgetIndexNone;
}

void SProjectCleanerWindowMain::TabsRegister()
{
	const auto DummyTab = SNew(SDockTab).TabRole(NomadTab);
	TabManager = FGlobalTabmanager::Get()->NewTabManager(DummyTab);
	TabManager->SetCanDoDragOperation(false);
	TabLayout = FTabManager::NewLayout("ProjectCleanerTabLayout")
		->AddArea
		(
			FTabManager::NewPrimaryArea()
			->SetOrientation(Orient_Vertical)
			->Split
			(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.4f)
				->AddTab(ProjectCleanerConstants::TabUnusedAssets, ETabState::OpenedTab)
				->AddTab(ProjectCleanerConstants::TabIndirectAssets, ETabState::OpenedTab)
				->AddTab(ProjectCleanerConstants::TabCorruptedAssets, ETabState::OpenedTab)
				->AddTab(ProjectCleanerConstants::TabNonEngineFiles, ETabState::OpenedTab)
				->SetForegroundTab(ProjectCleanerConstants::TabUnusedAssets)
			)
		);

	TabManager->RegisterTabSpawner(
		ProjectCleanerConstants::TabUnusedAssets,
		FOnSpawnTab::CreateRaw(
			this,
			&SProjectCleanerWindowMain::OnTabSpawnUnusedAssets)
	);
	TabManager->RegisterTabSpawner(
		ProjectCleanerConstants::TabIndirectAssets,
		FOnSpawnTab::CreateRaw(
			this,
			&SProjectCleanerWindowMain::OnTabSpawnIndirectAssets)
	);
	TabManager->RegisterTabSpawner(
		ProjectCleanerConstants::TabCorruptedAssets,
		FOnSpawnTab::CreateRaw(
			this,
			&SProjectCleanerWindowMain::OnTabSpawnCorruptedAssets)
	);
	TabManager->RegisterTabSpawner(
		ProjectCleanerConstants::TabNonEngineFiles,
		FOnSpawnTab::CreateRaw(
			this,
			&SProjectCleanerWindowMain::OnTabSpawnNonEngineFiles)
	);
}

void SProjectCleanerWindowMain::TabsUnregister()
{
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(ProjectCleanerConstants::TabUnusedAssets);
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(ProjectCleanerConstants::TabIndirectAssets);
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(ProjectCleanerConstants::TabCorruptedAssets);
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(ProjectCleanerConstants::TabNonEngineFiles);
}

TSharedRef<SDockTab> SProjectCleanerWindowMain::OnTabSpawnUnusedAssets(const FSpawnTabArgs& Args) const
{
	return
		SNew(SDockTab)
		.TabRole(NomadTab)
		.Label(FText::FromString(TEXT("Unused Assets")))
		.ShouldAutosize(true)
		.Icon(FProjectCleanerStyles::Get().GetBrush("ProjectCleaner.IconTabUnused16"))
		.OnCanCloseTab_Lambda([]()
		{
			return false;
		})
		[
			SNew(SOverlay)
			+ SOverlay::Slot()
			[
				SNew(SSplitter)
				.Orientation(Orient_Vertical)
				.Style(FEditorStyle::Get(), "ContentBrowser.Splitter")
				.PhysicalSplitterHandleSize(5.0f)
				+ SSplitter::Slot()
				.Value(0.4f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.FillHeight(1.0f)
					[
						SNew(SProjectCleanerTreeView)
						.RootFolder(FPaths::ProjectContentDir())
					]
				]
				+ SSplitter::Slot()
				.Value(0.6f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.FillHeight(1.0f)
					[
						SNew(SProjectCleanerTreeView)
						.RootFolder(FPaths::ProjectContentDir())
					]
				]
			]
		];
}

TSharedRef<SDockTab> SProjectCleanerWindowMain::OnTabSpawnIndirectAssets(const FSpawnTabArgs& Args) const
{
	return
		SNew(SDockTab)
		.TabRole(NomadTab)
		.Label(FText::FromString(TEXT("Indirect Assets")))
		.ShouldAutosize(true)
		.Icon(FProjectCleanerStyles::Get().GetBrush("ProjectCleaner.IconTabIndirect16"))
		.OnCanCloseTab_Lambda([]()
		{
			return false;
		})
		[
			SNew(SProjectCleanerWindowIndirectAssets)
			.ListItems(ProjectCleanerScanner->GetAssetsIndirect())
		];
}

TSharedRef<SDockTab> SProjectCleanerWindowMain::OnTabSpawnCorruptedAssets(const FSpawnTabArgs& Args) const
{
	return
		SNew(SDockTab)
		.TabRole(NomadTab)
		.Label(FText::FromString(TEXT("Corrupted Assets")))
		.ShouldAutosize(true)
		.Icon(FProjectCleanerStyles::Get().GetBrush("ProjectCleaner.IconTabCorrupted16"))
		.OnCanCloseTab_Lambda([]()
		{
			return false;
		})
		[
			SNew(SProjectCleanerFileListView)
			.Title(TEXT("List of possibly corrupted assets, that exist in Content folder, but not loaded by engine"))
			.Description(TEXT("In order to fix, try to reload project and see if its loaded. Otherwise close editor and remove them manually from explorer"))
			.Files(ProjectCleanerScanner->GetFilesCorrupted())
		];
}

TSharedRef<SDockTab> SProjectCleanerWindowMain::OnTabSpawnNonEngineFiles(const FSpawnTabArgs& Args) const
{
	return
		SNew(SDockTab)
		.TabRole(NomadTab)
		.Label(FText::FromString(TEXT("Non Engine Files")))
		.ShouldAutosize(true)
		.Icon(FProjectCleanerStyles::Get().GetBrush("ProjectCleaner.IconTabNonEngine16"))
		.OnCanCloseTab_Lambda([]()
		{
			return false;
		})
		[
			SNew(SProjectCleanerFileListView)
			.Title(TEXT("List of Non Engine files, that are inside Content folder"))
			.Files(ProjectCleanerScanner->GetFilesNonEngine())
		];
}

FReply SProjectCleanerWindowMain::OnBtnScanProjectClicked() const
{
	ProjectCleanerScanner->Scan(ScanSettings);

	return FReply::Handled();
}

FReply SProjectCleanerWindowMain::OnBtnCleanProjectClicked() const
{
	UE_LOG(LogProjectCleaner, Warning, TEXT("Clean btn clicked!"));
	return FReply::Handled();
}

FReply SProjectCleanerWindowMain::OnBtnDeleteEmptyFoldersClicked() const
{
	UE_LOG(LogProjectCleaner, Warning, TEXT("Delete btn clicked!"));
	return FReply::Handled();
}
