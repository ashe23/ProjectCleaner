// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/ProjectCleanerWindowMain.h"
#include "ProjectCleanerTypes.h"
#include "ProjectCleanerStyles.h"
#include "ProjectCleanerConstants.h"
#include "Libs/ProjectCleanerAssetLibrary.h"
// Engine Headers
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "Widgets/Input/SHyperlink.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SWidgetSwitcher.h"

static constexpr int32 WidgetIndexNone = 0;
static constexpr int32 WidgetIndexLoading = 1;
static const FName TabUnusedAssets{TEXT("TabUnusedAssets")};

void SProjectCleanerStatListItem::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView)
{
	ListItem = InArgs._ListItem;

	SMultiColumnTableRow<TWeakObjectPtr<UProjectCleanerStatListItem>>::Construct(
		SMultiColumnTableRow<TWeakObjectPtr<UProjectCleanerStatListItem>>::FArguments()
		.Padding(FMargin{0.0f, 2.0f, 0.0f, 0.0f}),
		InOwnerTableView
	);
}

TSharedRef<SWidget> SProjectCleanerStatListItem::GenerateWidgetForColumn(const FName& InColumnName)
{
	if (InColumnName.IsEqual(TEXT("Name")))
	{
		return
			SNew(STextBlock)
				.AutoWrapText(false)
				.Justification(ETextJustify::Center)
				.Text(FText::FromString(ListItem->Name))
				.ColorAndOpacity(FLinearColor{FColor::FromHex(TEXT("#00a6fb"))})
				.Font(FProjectCleanerStyles::Get().GetFontStyle("ProjectCleaner.Font.Light13"));
	}

	if (InColumnName.IsEqual(TEXT("Category")))
	{
		return SNew(STextBlock).Text(FText::FromString(ListItem->Category)).ColorAndOpacity(ListItem->Color);
	}

	if (InColumnName.IsEqual(TEXT("Count")))
	{
		return SNew(STextBlock).Text(FText::FromString(ListItem->Count)).ColorAndOpacity(ListItem->Color);
	}

	if (InColumnName.IsEqual(TEXT("Size")))
	{
		return SNew(STextBlock).Text(FText::FromString(ListItem->Size)).ColorAndOpacity(ListItem->Color);
	}

	return SNew(STextBlock).Text(FText::FromString(TEXT("")));
}

void SProjectCleanerWindowMain::Construct(const FArguments& InArgs)
{
	ScanSettings = GetMutableDefault<UProjectCleanerScanSettings>();
	check(ScanSettings.Get())

	FPropertyEditorModule& PropertyEditor = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	const FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");

	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.bUpdatesFromSelection = false;
	DetailsViewArgs.bLockable = false;
	DetailsViewArgs.bShowScrollBar = true;
	DetailsViewArgs.bAllowSearch = false;
	DetailsViewArgs.bShowOptions = false;
	DetailsViewArgs.bAllowFavoriteSystem = false;
	DetailsViewArgs.bShowPropertyMatrixButton = false;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	DetailsViewArgs.ViewIdentifier = "ProjectCleanerStatSettings";

	const TSharedPtr<IDetailsView> ScanSettingsProperty = PropertyEditor.CreateDetailView(DetailsViewArgs);
	ScanSettingsProperty->SetObject(ScanSettings.Get());

	FAssetPickerConfig AssetPickerConfig;
	AssetPickerConfig.InitialAssetViewType = EAssetViewType::Tile;
	AssetPickerConfig.bCanShowFolders = true;
	AssetPickerConfig.bAddFilterUI = true;
	AssetPickerConfig.bPreloadAssetsForContextMenu = false;
	AssetPickerConfig.bSortByPathInColumnView = true;
	AssetPickerConfig.bShowPathInColumnView = true;
	AssetPickerConfig.bShowBottomToolbar = true;
	AssetPickerConfig.bCanShowDevelopersFolder = true; // todo:ashe23
	AssetPickerConfig.bCanShowClasses = false;
	AssetPickerConfig.bAllowDragging = false;
	AssetPickerConfig.bForceShowEngineContent = false;
	AssetPickerConfig.bCanShowRealTimeThumbnails = false;
	AssetPickerConfig.AssetShowWarningText = FText::FromName("No assets");

	FPathPickerConfig PathPickerConfig;
	PathPickerConfig.bAllowContextMenu = true;
	PathPickerConfig.bAllowClassesFolder = false;
	PathPickerConfig.bFocusSearchBoxWhenOpened = false;
	PathPickerConfig.bAddDefaultPath = true;
	PathPickerConfig.DefaultPath = TEXT("/Game");

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
				->AddTab(TabUnusedAssets, ETabState::OpenedTab)
				->SetForegroundTab(TabUnusedAssets)
			)
		);

	TabManager->RegisterTabSpawner(
		TabUnusedAssets,
		FOnSpawnTab::CreateLambda([&](const FSpawnTabArgs& SpawnTabArgs) -> TSharedRef<SDockTab>
		{
			return
				SNew(SDockTab)
				.TabRole(NomadTab)
				.Label(FText::FromString(TEXT("Unused Assets")))
				.ShouldAutosize(true)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					  .Padding(20.0f)
					  .AutoHeight()
					[
						SNew(SSplitter)
						.Style(FEditorStyle::Get(), "ContentBrowser.Splitter")
						.PhysicalSplitterHandleSize(2.0f)
						+ SSplitter::Slot()
						.Value(0.3f)
						[
							ContentBrowserModule.Get().CreatePathPicker(PathPickerConfig)
						]
						+ SSplitter::Slot()
						[
							ContentBrowserModule.Get().CreateAssetPicker(AssetPickerConfig)
						]
					]
				];
		})
	);

	const TSharedRef<SWidget> TabContents = TabManager->RestoreFrom(TabLayout.ToSharedRef(), TSharedPtr<SWindow>()).ToSharedRef();

	const TWeakObjectPtr<UProjectCleanerStatListItem> ListItemAssets = NewObject<UProjectCleanerStatListItem>();
	ListItemAssets->Name = TEXT("Assets");
	ListItems.Add(ListItemAssets);

	const TWeakObjectPtr<UProjectCleanerStatListItem> ListItemUsed = NewObject<UProjectCleanerStatListItem>();
	ListItemUsed->Category = TEXT("Used");
	ListItemUsed->Count = TEXT("100");
	ListItemUsed->Size = TEXT("100.23 MiB");
	ListItems.Add(ListItemUsed);

	const TWeakObjectPtr<UProjectCleanerStatListItem> ListItemExcluded = NewObject<UProjectCleanerStatListItem>();
	ListItemExcluded->Category = TEXT("Excluded");
	ListItemExcluded->Count = TEXT("0");
	ListItemExcluded->Size = TEXT("0.00 MiB");
	ListItemExcluded->Color = FLinearColor{FColor::FromHex(TEXT("#f9c74f"))};
	ListItems.Add(ListItemExcluded);

	const TWeakObjectPtr<UProjectCleanerStatListItem> ListItemUnused = NewObject<UProjectCleanerStatListItem>();
	ListItemUnused->Category = TEXT("Unused");
	ListItemUnused->Count = TEXT("20");
	ListItemUnused->Size = TEXT("20.08 MiB");
	ListItemUnused->Color = FLinearColor{FColor::FromHex(TEXT("#f94144"))};
	ListItems.Add(ListItemUnused);

	const TWeakObjectPtr<UProjectCleanerStatListItem> ListItemTotal = NewObject<UProjectCleanerStatListItem>();
	ListItemTotal->Category = TEXT("Total");
	ListItemTotal->Count = TEXT("120");
	ListItemTotal->Size = TEXT("120.23 MiB");
	ListItemTotal->Color = FLinearColor{FColor::FromHex(TEXT("#43aa8b"))};
	ListItems.Add(ListItemTotal);

	const TWeakObjectPtr<UProjectCleanerStatListItem> ListItemCategoryFiles = NewObject<UProjectCleanerStatListItem>();
	ListItemCategoryFiles->Name = TEXT("Files");
	ListItems.Add(ListItemCategoryFiles);

	const TWeakObjectPtr<UProjectCleanerStatListItem> ListItemNonEngineFiles = NewObject<UProjectCleanerStatListItem>();
	ListItemNonEngineFiles->Category = TEXT("Non Engine Files");
	ListItemNonEngineFiles->Count = TEXT("23");
	ListItemNonEngineFiles->Size = TEXT("23.45 MiB");
	ListItems.Add(ListItemNonEngineFiles);

	const TWeakObjectPtr<UProjectCleanerStatListItem> ListItemCorruptedFiles = NewObject<UProjectCleanerStatListItem>();
	ListItemCorruptedFiles->Category = TEXT("Corrupted Files");
	ListItemCorruptedFiles->Count = TEXT("2");
	ListItemCorruptedFiles->Size = TEXT("2.45 MiB");
	ListItemCorruptedFiles->Color = FLinearColor{FColor::FromHex(TEXT("#f94144"))};
	ListItems.Add(ListItemCorruptedFiles);

	const TWeakObjectPtr<UProjectCleanerStatListItem> ListItemCategoryFolders = NewObject<UProjectCleanerStatListItem>();
	ListItemCategoryFolders->Name = TEXT("Folders");
	ListItems.Add(ListItemCategoryFolders);

	const TWeakObjectPtr<UProjectCleanerStatListItem> ListItemFoldersEmpty = NewObject<UProjectCleanerStatListItem>();
	ListItemFoldersEmpty->Category = TEXT("Empty");
	ListItemFoldersEmpty->Count = TEXT("23");
	ListItemFoldersEmpty->Size = TEXT("");
	ListItemFoldersEmpty->Color = FLinearColor{FColor::FromHex(TEXT("#f94144"))};
	ListItems.Add(ListItemFoldersEmpty);

	const TWeakObjectPtr<UProjectCleanerStatListItem> ListItemFoldersTotal = NewObject<UProjectCleanerStatListItem>();
	ListItemFoldersTotal->Category = TEXT("Total");
	ListItemFoldersTotal->Count = TEXT("120");
	ListItemFoldersTotal->Size = TEXT("");
	ListItemFoldersTotal->Color = FLinearColor{FColor::FromHex(TEXT("#43aa8b"))};
	ListItems.Add(ListItemFoldersTotal);


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
			  .Padding(FMargin{10.0f})
			  .AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				  .FillWidth(1.0f)
				  .HAlign(HAlign_Right)
				  .VAlign(VAlign_Center)
				[
					SNew(SHyperlink)
            		.Text(FText::FromString(TEXT("Wiki")))
            		.OnNavigate_Static(OnNavigateWiki)
				]
			]
			+ SVerticalBox::Slot()
			  .Padding(FMargin{10.0f})
			  .FillHeight(1.0f)
			[
				SNew(SSplitter)
				.Style(FEditorStyle::Get(), "ContentBrowser.Splitter")
				.PhysicalSplitterHandleSize(5.0f)
				+ SSplitter::Slot()
				.Value(0.35f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					  .AutoHeight()
					  .Padding(FMargin{0.0f, 0.0f, 10.0f, 0.0f})
					[
						SNew(SBorder)
						.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
						.Padding(FMargin{10.0f})
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot()
							  .AutoHeight()
							  .Padding(FMargin{15.0f})
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot()
								  .FillWidth(1.0f)
								  .VAlign(VAlign_Center)
								  .HAlign(HAlign_Center)
								[
									SNew(STextBlock)
									.AutoWrapText(false)
									.Justification(ETextJustify::Center)
									.Text(FText::FromString(TEXT("Project Content Stats")))
									.Font(FProjectCleanerStyles::Get().GetFontStyle("ProjectCleaner.Font.Light23"))
								]
							]
							+ SVerticalBox::Slot()
							  .Padding(FMargin{10.0f, 0.0f})
							  .AutoHeight()
							[
								SAssignNew(ListView, SListView<TWeakObjectPtr<UProjectCleanerStatListItem>>)
							  	.ListItemsSource(&ListItems)
							  	.SelectionMode(ESelectionMode::None)
							  	.OnGenerateRow(this, &SProjectCleanerWindowMain::OnGenerateRow)
							  	.HeaderRow(GetHeaderRow())
							]
							+ SVerticalBox::Slot()
							  .Padding(FMargin{10.0f, 20.0f})
							  .AutoHeight()
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot()
								  .FillWidth(1.0f)
								  .Padding(FMargin{0.0f, 0.0f, 5.0f, 0.0f})
								[
									SNew(SButton)
									.ContentPadding(FMargin{5})
									.ButtonColorAndOpacity(FLinearColor{FColor::FromHex(TEXT("#3a86ff"))})
									[
										SNew(STextBlock)
										.Justification(ETextJustify::Center)
										.ColorAndOpacity(FLinearColor{FColor::White})
										.Text(FText::FromString(TEXT("Scan Project")))
									]
								]
								+ SHorizontalBox::Slot()
								  .FillWidth(1.0f)
								  .Padding(FMargin{0.0f, 0.0f, 5.0f, 0.0f})
								[
									SNew(SButton)
									.ContentPadding(FMargin{5})
									.ButtonColorAndOpacity(FLinearColor{FColor::FromHex(TEXT("#e63946"))})
									[
										SNew(STextBlock)
										.Justification(ETextJustify::Center)
										.ColorAndOpacity(FLinearColor{FColor::White})
										.Text(FText::FromString(TEXT("Clean Project")))
									]
								]
								+ SHorizontalBox::Slot()
								.FillWidth(1.0f)
								[
									SNew(SButton)
									.ContentPadding(FMargin{5})
									.ButtonColorAndOpacity(FLinearColor{FColor::FromHex(TEXT("#e63946"))})
									[
										SNew(STextBlock)
										.Justification(ETextJustify::Center)
										.ColorAndOpacity(FLinearColor{FColor::White})
										.Text(FText::FromString(TEXT("Delete Empty Folders")))
									]
								]
							]
						]
					]
					+ SVerticalBox::Slot()
					  .FillHeight(1.0f)
					  .Padding(FMargin{0.0f, 10.0f, 10.0f, 0.0f})
					[
						SNew(SScrollBox)
						.ScrollWhenFocusChanges(EScrollWhenFocusChanges::AnimatedScroll)
						.AnimateWheelScrolling(true)
						.AllowOverscroll(EAllowOverscroll::No)
						+ SScrollBox::Slot()
						[
							ScanSettingsProperty.ToSharedRef()
						]
					]
				]
				+ SSplitter::Slot()
				[
					SNew(SScrollBox)
					.ScrollWhenFocusChanges(EScrollWhenFocusChanges::NoScroll)
					.AnimateWheelScrolling(true)
					.AllowOverscroll(EAllowOverscroll::No)
					+ SScrollBox::Slot()
					[
						TabContents
					]
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
				.Font(FProjectCleanerStyles::Get().GetFontStyle("ProjectCleaner.Font.Light30"))
				.Text(FText::FromString(ProjectCleanerConstants::MsgAssetRegistryStillWorking))
			]
		]
	];
}

SProjectCleanerWindowMain::~SProjectCleanerWindowMain()
{
	TabManager->UnregisterTabSpawner(TabUnusedAssets);
}

FText SProjectCleanerWindowMain::GetNumAllAssets() const
{
	const FString SizeTotal = FText::AsMemory(10516565).ToString();
	return FText::FromString(FString::Printf(TEXT("Total: 250"))); // todo:ashe23
}

FText SProjectCleanerWindowMain::GetNumUsedAssets() const
{
	return FText::FromString(FString::Printf(TEXT("Used - 23 (53%%)"))); // todo:ashe23
}

FText SProjectCleanerWindowMain::GetNumUnusedAssets() const
{
	return FText::FromString(FString::Printf(TEXT("Unused - 10 (15%%)"))); // todo:ashe23
}

FText SProjectCleanerWindowMain::GetNumExcludedAssets() const
{
	return FText::FromString(FString::Printf(TEXT("Excluded - 10 (15%%)"))); // todo:ashe23
}

FText SProjectCleanerWindowMain::GetNumIndirectAssets() const
{
	return FText::FromString(FString::Printf(TEXT("Indirect - 10 (15%%)"))); // todo:ashe23
}

FText SProjectCleanerWindowMain::GetNumCorruptedAssets() const
{
	return FText::FromString(FString::Printf(TEXT("Corrupted assets: 10"))); // todo:ashe23
}

FText SProjectCleanerWindowMain::GetNumExternalFiles() const
{
	return FText::FromString(FString::Printf(TEXT("External files: 10"))); // todo:ashe23
}

FText SProjectCleanerWindowMain::GetNumAllFolders() const
{
	return FText::FromString(FString::Printf(TEXT("Folders: 10"))); // todo:ashe23
}

FText SProjectCleanerWindowMain::GetNumEmptyFolders() const
{
	return FText::FromString(FString::Printf(TEXT("Empty - 10 (15%%)"))); // todo:ashe23
}

FText SProjectCleanerWindowMain::GetSizeTotal() const
{
	const FString SizeTotal = FText::AsMemory(10516565).ToString();
	return FText::FromString(FString::Printf(TEXT("Sizes: %s"), *SizeTotal));
}

FText SProjectCleanerWindowMain::GetSizeUsed() const
{
	const FString SizeUsed = FText::AsMemory(10516565).ToString();
	return FText::FromString(FString::Printf(TEXT("Used - %s"), *SizeUsed));
}

FText SProjectCleanerWindowMain::GetSizeUnused() const
{
	const FString SizeUnused = FText::AsMemory(10516565).ToString();
	return FText::FromString(FString::Printf(TEXT("Unused - %s"), *SizeUnused));
}

FText SProjectCleanerWindowMain::GetSizeExcluded() const
{
	const FString SizeExcluded = FText::AsMemory(10516565).ToString();
	return FText::FromString(FString::Printf(TEXT("Excluded - %s"), *SizeExcluded));
}

FText SProjectCleanerWindowMain::GetSizeIndirect() const
{
	const FString SizeIndirect = FText::AsMemory(10516565).ToString();
	return FText::FromString(FString::Printf(TEXT("Indirect - %s"), *SizeIndirect));
}

void SProjectCleanerWindowMain::OnNavigateWiki()
{
	if (FPlatformProcess::CanLaunchURL(*ProjectCleanerConstants::UrlWiki))
	{
		FPlatformProcess::LaunchURL(*ProjectCleanerConstants::UrlWiki, nullptr, nullptr);
	}
}

bool SProjectCleanerWindowMain::IsWidgetEnabled()
{
	return !UProjectCleanerAssetLibrary::AssetRegistryIsLoadingAssets();
}

int32 SProjectCleanerWindowMain::GetWidgetIndex()
{
	return UProjectCleanerAssetLibrary::AssetRegistryIsLoadingAssets() ? WidgetIndexLoading : WidgetIndexNone;
}

TSharedRef<ITableRow> SProjectCleanerWindowMain::OnGenerateRow(const TWeakObjectPtr<UProjectCleanerStatListItem> InItem, const TSharedRef<STableViewBase>& OwnerTable) const
{
	return SNew(SProjectCleanerStatListItem, OwnerTable).ListItem(InItem);
}

TSharedPtr<SHeaderRow> SProjectCleanerWindowMain::GetHeaderRow() const
{
	return
		SNew(SHeaderRow)
		+ SHeaderRow::Column(FName{TEXT("Name")})
		  .HAlignCell(HAlign_Center)
		  .VAlignCell(VAlign_Center)
		  .HAlignHeader(HAlign_Center)
		  .HeaderContentPadding(FMargin(10.0f))
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Name")))
		]
		+ SHeaderRow::Column(FName{TEXT("Category")})
		  .HAlignCell(HAlign_Center)
		  .VAlignCell(VAlign_Center)
		  .HAlignHeader(HAlign_Center)
		  .HeaderContentPadding(FMargin(10.0f))
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Category")))
		]
		+ SHeaderRow::Column(FName{TEXT("Count")})
		  .HAlignCell(HAlign_Center)
		  .VAlignCell(VAlign_Center)
		  .HAlignHeader(HAlign_Center)
		  .HeaderContentPadding(FMargin(10.0f))
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Count")))
		]
		+ SHeaderRow::Column(FName{TEXT("Size")})
		  .HAlignCell(HAlign_Center)
		  .VAlignCell(VAlign_Center)
		  .HAlignHeader(HAlign_Center)
		  .HeaderContentPadding(FMargin(10.0f))
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Size")))
		];
}
