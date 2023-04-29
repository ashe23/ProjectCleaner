// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/SPjcTabAssetsUnused.h"
#include "Slate/Items/SPjcItemStat.h"
#include "PjcCmds.h"
#include "PjcTypes.h"
#include "PjcStyles.h"
// Engine Headers
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "Settings/ContentBrowserSettings.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"

void SPjcTabAssetsUnused::Construct(const FArguments& InArgs)
{
	Cmds = MakeShareable(new FUICommandList);

	Cmds->MapAction(
		FPjcCmds::Get().TabAssetsUnusedBtnScan,
		FExecuteAction::CreateLambda([&]()
		{
			UE_LOG(LogTemp, Warning, TEXT("Scanning assets"));
		})
	);

	Cmds->MapAction(
		FPjcCmds::Get().TabAssetsUnusedBtnClean,
		FExecuteAction::CreateLambda([]()
		{
			UE_LOG(LogTemp, Warning, TEXT("Cleaning project"));

			// const TSharedRef<SWindow> Window = SNew(SWindow).Title(FText::FromString(TEXT("Some Title"))).ClientSize(FVector2D{600, 400});
			// const TSharedRef<SWidget> Content =
			// 	SNew(SVerticalBox)
			// 	+ SVerticalBox::Slot()
			// 	[
			// 		SNew(STextBlock).Text(FText::FromString(TEXT("Some Content")))
			// 	];
			//
			// Window->SetContent(Content);
			//
			// if (GEditor)
			// {
			// 	GEditor->EditorAddModalWindow(Window);
			// }
		}),
		FCanExecuteAction::CreateLambda([]()
		{
			return true;
		})
	);

	FPropertyEditorModule& PropertyEditor = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.bUpdatesFromSelection = false;
	DetailsViewArgs.bLockable = false;
	DetailsViewArgs.bAllowSearch = false;
	DetailsViewArgs.bShowOptions = true;
	DetailsViewArgs.bAllowFavoriteSystem = false;
	DetailsViewArgs.bShowPropertyMatrixButton = false;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	DetailsViewArgs.ViewIdentifier = "PjcEditorAssetExcludeSettings";

	const auto SettingsProperty = PropertyEditor.CreateDetailView(DetailsViewArgs);
	SettingsProperty->SetObject(GetMutableDefault<UPjcEditorAssetExcludeSettings>());


	const FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");

	UContentBrowserSettings* ContentBrowserSettings = GetMutableDefault<UContentBrowserSettings>();
	if (ContentBrowserSettings)
	{
		ContentBrowserSettings->SetDisplayEngineFolder(false);
		ContentBrowserSettings->SetDisplayCppFolders(false);
		ContentBrowserSettings->SetDisplayPluginFolders(false);
		ContentBrowserSettings->PostEditChange();
	}


	FPathPickerConfig PathPickerConfig;
	PathPickerConfig.bAllowClassesFolder = false;
	PathPickerConfig.bAllowContextMenu = false;
	PathPickerConfig.bFocusSearchBoxWhenOpened = false;


	FAssetPickerConfig AssetPickerConfig;
	AssetPickerConfig.bAllowDragging = false;
	AssetPickerConfig.bCanShowClasses = false;
	AssetPickerConfig.bCanShowFolders = false;
	AssetPickerConfig.bForceShowEngineContent = false;
	AssetPickerConfig.bForceShowPluginContent = false;

	SAssignNew(StatView, SListView<TSharedPtr<FPjcStatItem>>)
	.ListItemsSource(&StatItems)
	.OnGenerateRow(this, &SPjcTabAssetsUnused::OnStatGenerateRow)
	.SelectionMode(ESelectionMode::None)
	.IsFocusable(false)
	.HeaderRow(GetStatHeaderRow());

	StatItemsInit();

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(5.0f)
		[
			CreateToolbar()
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(5.0f)
		[
			SNew(SSeparator).Thickness(5.0f)
		]
		+ SVerticalBox::Slot().FillHeight(1.0f).Padding(5.0f)
		[
			SNew(SSplitter)
			.PhysicalSplitterHandleSize(3.0f)
			.Style(FEditorStyle::Get(), "DetailsView.Splitter")
			+ SSplitter::Slot().Value(0.2f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(3.0f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.0f).HAlign(HAlign_Center).VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Justification(ETextJustify::Center)
						.ColorAndOpacity(FPjcStyles::Get().GetColor("ProjectCleaner.Color.Gray"))
						.ShadowOffset(FVector2D{1.5f, 1.5f})
						.ShadowColorAndOpacity(FLinearColor::Black)
						.Font(FPjcStyles::GetFont("Bold", 15))
						.Text(FText::FromString(TEXT("Assets Statistics")))
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(3.0f)
				[
					StatView.ToSharedRef()
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(3.0f)
				[
					SNew(SSeparator).Thickness(5.0f)
				]
				+ SVerticalBox::Slot().FillHeight(1.0f).Padding(3.0f)
				[
					SNew(SBox)
					[
						SNew(SScrollBox)
						.ScrollWhenFocusChanges(EScrollWhenFocusChanges::NoScroll)
						.AnimateWheelScrolling(true)
						.AllowOverscroll(EAllowOverscroll::No)
						+ SScrollBox::Slot()
						[
							SettingsProperty
						]
					]
				]
			]
			+ SSplitter::Slot().Value(0.3f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().FillHeight(1.0f).Padding(5.0f)
				[
					ContentBrowserModule.Get().CreatePathPicker(PathPickerConfig)
				]
			]
			+ SSplitter::Slot().Value(0.5f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().FillHeight(1.0f).Padding(5.0f)
				[
					ContentBrowserModule.Get().CreateAssetPicker(AssetPickerConfig)
				]
			]
		]
	];
}

void SPjcTabAssetsUnused::StatItemsInit()
{
	StatItems.Reset();
	
	const FMargin FirstLvl{5.0f, 0.0f, 0.0f, 0.0f};
	const FMargin SecondLvl{20.0f, 0.0f, 0.0f, 0.0f};

	StatItems.Emplace(
		MakeShareable(
			new FPjcStatItem{
				FText::FromString(TEXT("Unused")),
				FText::FromString(TEXT("0")),
				FText::FromString(TEXT("0")),
				FText::FromString(TEXT("Unused Assets")),
				FText::FromString(TEXT("Total number of unused assets")),
				FText::FromString(TEXT("Total size of unused assets")),
				FLinearColor::White,
				FirstLvl
			}
		)
	);

	StatItems.Emplace(
		MakeShareable(
			new FPjcStatItem{
				FText::FromString(TEXT("Used")),
				FText::FromString(TEXT("0")),
				FText::FromString(TEXT("0")),
				FText::FromString(TEXT("Used Assets")),
				FText::FromString(TEXT("Total number of used assets")),
				FText::FromString(TEXT("Total size of used assets")),
				FLinearColor::White,
				FirstLvl
			}
		)
	);

	StatItems.Emplace(
		MakeShareable(
			new FPjcStatItem{
				FText::FromString(TEXT("Primary")),
				FText::FromString(TEXT("0")),
				FText::FromString(TEXT("0")),
				FText::FromString(TEXT("Primary Assets")),
				FText::FromString(TEXT("Total number of primary assets")),
				FText::FromString(TEXT("Total size of primary assets")),
				FLinearColor::White,
				SecondLvl
			}
		)
	);

	StatItems.Emplace(
		MakeShareable(
			new FPjcStatItem{
				FText::FromString(TEXT("Editor")),
				FText::FromString(TEXT("0")),
				FText::FromString(TEXT("0")),
				FText::FromString(TEXT("Editor Assets")),
				FText::FromString(TEXT("Total number of Editor assets")),
				FText::FromString(TEXT("Total size of Editor assets")),
				FLinearColor::White,
				SecondLvl
			}
		)
	);

	StatItems.Emplace(
		MakeShareable(
			new FPjcStatItem{
				FText::FromString(TEXT("Indirect")),
				FText::FromString(TEXT("0")),
				FText::FromString(TEXT("0")),
				FText::FromString(TEXT("Indirect Assets")),
				FText::FromString(TEXT("Total number of Indirect assets")),
				FText::FromString(TEXT("Total size of Indirect assets")),
				FLinearColor::White,
				SecondLvl
			}
		)
	);

	StatItems.Emplace(
		MakeShareable(
			new FPjcStatItem{
				FText::FromString(TEXT("ExtReferenced")),
				FText::FromString(TEXT("0")),
				FText::FromString(TEXT("0")),
				FText::FromString(TEXT("ExtReferenced Assets")),
				FText::FromString(TEXT("Total number of ExtReferenced assets")),
				FText::FromString(TEXT("Total size of ExtReferenced assets")),
				FLinearColor::White,
				SecondLvl
			}
		)
	);

	StatItems.Emplace(
		MakeShareable(
			new FPjcStatItem{
				FText::FromString(TEXT("Excluded")),
				FText::FromString(TEXT("0")),
				FText::FromString(TEXT("0")),
				FText::FromString(TEXT("Excluded Assets")),
				FText::FromString(TEXT("Total number of Excluded assets")),
				FText::FromString(TEXT("Total size of Excluded assets")),
				FLinearColor::White,
				SecondLvl
			}
		)
	);

	StatItems.Emplace(
		MakeShareable(
			new FPjcStatItem{
				FText::FromString(TEXT("Total")),
				FText::FromString(TEXT("0")),
				FText::FromString(TEXT("0")),
				FText::FromString(TEXT("All Assets")),
				FText::FromString(TEXT("Total number of assets")),
				FText::FromString(TEXT("Total size of assets")),
				FLinearColor::White,
				FirstLvl
			}
		)
	);
}

TSharedRef<SWidget> SPjcTabAssetsUnused::CreateToolbar() const
{
	FToolBarBuilder ToolBarBuilder{Cmds, FMultiBoxCustomization::None};
	ToolBarBuilder.BeginSection("PjcTabAssetUnusedScanActions");
	{
		ToolBarBuilder.AddToolBarButton(FPjcCmds::Get().TabAssetsUnusedBtnScan);
		ToolBarBuilder.AddToolBarButton(FPjcCmds::Get().TabAssetsUnusedBtnClean);
	}
	ToolBarBuilder.EndSection();

	return ToolBarBuilder.MakeWidget();
}

TSharedRef<ITableRow> SPjcTabAssetsUnused::OnStatGenerateRow(TSharedPtr<FPjcStatItem> Item, const TSharedRef<STableViewBase>& OwnerTable) const
{
	return SNew(SPjcItemStat, OwnerTable).Item(Item);
}

TSharedRef<SHeaderRow> SPjcTabAssetsUnused::GetStatHeaderRow() const
{
	const FMargin HeaderContentPadding{5.0f};

	return
		SNew(SHeaderRow)
		+ SHeaderRow::Column("Name").FillWidth(0.4f).HAlignCell(HAlign_Left).VAlignCell(VAlign_Center).HAlignHeader(HAlign_Center).HeaderContentPadding(HeaderContentPadding)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Category")))
			.ColorAndOpacity(FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
			.Font(FPjcStyles::GetFont("Light", 10.0f))
		]
		+ SHeaderRow::Column("Num").FillWidth(0.3f).HAlignCell(HAlign_Center).VAlignCell(VAlign_Center).HAlignHeader(HAlign_Center).HeaderContentPadding(HeaderContentPadding)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Num")))
			.ColorAndOpacity(FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
			.Font(FPjcStyles::GetFont("Light", 10.0f))
		]
		+ SHeaderRow::Column("Size").FillWidth(0.3f).HAlignCell(HAlign_Center).VAlignCell(VAlign_Center).HAlignHeader(HAlign_Center).HeaderContentPadding(HeaderContentPadding)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Size")))
			.ColorAndOpacity(FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
			.Font(FPjcStyles::GetFont("Light", 10.0f))
		];
}
