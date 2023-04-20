// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "SLate/AssetBrowser/SPjcAssetBrowser.h"
#include "Slate/AssetBrowser/SPjcAssetBrowserStatItem.h"
#include "Slate/AssetBrowser/SPjcContentBrowser.h"
#include "Slate/AssetBrowser/SPjcTreeView.h"
#include "PjcTypes.h"
#include "PjcStyles.h"
// #include "PjcConstants.h"
// #include "Libs/PjcLibAsset.h"
// Engine Headers
#include "EditorWidgetsModule.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"
// #include "Widgets/Layout/SWidgetSwitcher.h"

void SPjcAssetBrowser::Construct(const FArguments& InArgs)
{
	FEditorWidgetsModule& EditorWidgetsModule = FModuleManager::LoadModuleChecked<FEditorWidgetsModule>("EditorWidgets");
	const TSharedRef<SWidget> AssetDiscoveryIndicator = EditorWidgetsModule.CreateAssetDiscoveryIndicator(EAssetDiscoveryIndicatorScaleMode::Scale_None, FMargin(16, 8), false);

	const FMargin FirstLvl{5.0f, 0.0f, 0.0f, 0.0f};
	const FMargin SecondLvl{20.0f, 0.0f, 0.0f, 0.0f};

	StatItems.Emplace(MakeShareable(new FPjcAssetBrowserStatItem{123, 1233, TEXT("Unused"), FirstLvl, FPjcStyles::Get().GetSlateColor(TEXT("ProjectCleaner.Color.Red")).GetSpecifiedColor()}));
	StatItems.Emplace(MakeShareable(new FPjcAssetBrowserStatItem{123, 1233, TEXT("Used"), FirstLvl}));
	StatItems.Emplace(MakeShareable(new FPjcAssetBrowserStatItem{123, 1233, TEXT("Primary"), SecondLvl}));
	StatItems.Emplace(MakeShareable(new FPjcAssetBrowserStatItem{123, 1233, TEXT("Indirect"), SecondLvl}));
	StatItems.Emplace(MakeShareable(new FPjcAssetBrowserStatItem{123, 1233, TEXT("Editor"), SecondLvl}));
	StatItems.Emplace(MakeShareable(new FPjcAssetBrowserStatItem{123, 1233, TEXT("Excluded"), SecondLvl, FPjcStyles::Get().GetSlateColor(TEXT("ProjectCleaner.Color.Yellow")).GetSpecifiedColor()}));
	StatItems.Emplace(MakeShareable(new FPjcAssetBrowserStatItem{123, 1233, TEXT("ExtReferenced"), SecondLvl}));
	StatItems.Emplace(MakeShareable(new FPjcAssetBrowserStatItem{
		123, 1233, TEXT("Total"), FirstLvl, FPjcStyles::Get().GetSlateColor(TEXT("ProjectCleaner.Color.White")).GetSpecifiedColor(), TEXT("Total Number of asset in project")
	}));

	FPropertyEditorModule& PropertyEditor = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.bUpdatesFromSelection = false;
	DetailsViewArgs.bLockable = false;
	DetailsViewArgs.bAllowSearch = false;
	DetailsViewArgs.bShowOptions = true;
	DetailsViewArgs.bAllowFavoriteSystem = false;
	DetailsViewArgs.bShowPropertyMatrixButton = false;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	DetailsViewArgs.ViewIdentifier = "PjcScaSettingsEditor";

	const auto SettingsProperty = PropertyEditor.CreateDetailView(DetailsViewArgs);
	SettingsProperty->SetObject(GetMutableDefault<UPjcScanSettingsEditor>());

	ChildSlot
	[
		SNew(SSplitter)
		.Style(FEditorStyle::Get(), "DetailsView.Splitter")
		.PhysicalSplitterHandleSize(3.0f)
		+ SSplitter::Slot()
		.Value(0.3f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(5.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SButton)
					.ContentPadding(FMargin{3.0f})
					.OnClicked_Raw(this, &SPjcAssetBrowser::OnBtnScanAssetsClick)
					.ButtonColorAndOpacity(FPjcStyles::Get().GetColor("ProjectCleaner.Color.Blue"))
					[
						SNew(STextBlock)
						.Justification(ETextJustify::Center)
						.ToolTipText(FText::FromString(TEXT("Scan for unused assets in project")))
						.ColorAndOpacity(FPjcStyles::Get().GetColor("ProjectCleaner.Color.White"))
						.ShadowOffset(FVector2D{1.5f, 1.5f})
						.ShadowColorAndOpacity(FLinearColor::Black)
						.Font(FPjcStyles::GetFont("Bold", 10))
						.Text(FText::FromString(TEXT("Scan Project")))
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin{5.0f, 0.0f, 0.0f, 0.0f})
				[
					SNew(SButton)
					.ContentPadding(FMargin{3.0f})
					.ButtonColorAndOpacity(FPjcStyles::Get().GetColor("ProjectCleaner.Color.Red"))
					[
						SNew(STextBlock)
						.Justification(ETextJustify::Center)
						.ToolTipText(FText::FromString(TEXT("Scan for unused assets in project")))
						.ColorAndOpacity(FPjcStyles::Get().GetColor("ProjectCleaner.Color.White"))
						.ShadowOffset(FVector2D{1.5f, 1.5f})
						.ShadowColorAndOpacity(FLinearColor::Black)
						.Font(FPjcStyles::GetFont("Bold", 10))
						.Text(FText::FromString(TEXT("Clean Project")))
					]
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(5.0f)
			[
				SNew(SSeparator).Thickness(5.0f)
			]

			+ SVerticalBox::Slot().AutoHeight().Padding(5.0f)
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
					.Text(FText::FromString(TEXT("Project Assets Statistics")))
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(5.0f)
			[
				SAssignNew(StatListView, SListView<TSharedPtr<FPjcAssetBrowserStatItem>>)
				.ListItemsSource(&StatItems)
				.OnGenerateRow(this, &SPjcAssetBrowser::OnStatGenerateRow)
				.SelectionMode(ESelectionMode::None)
				.HeaderRow(GetStatHeaderRow())
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(5.0f)
			[
				SNew(SSeparator).Thickness(5.0f)
			]
			+ SVerticalBox::Slot().FillHeight(1.0f).Padding(5.0f)
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
		+ SSplitter::Slot()
		.Value(0.7f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().FillHeight(1.0f)
			[
				SNew(SOverlay)
				+ SOverlay::Slot().Padding(5.0f)
				[
					SNew(SSplitter)
					.Style(FEditorStyle::Get(), "DetailsView.Splitter")
					.PhysicalSplitterHandleSize(3.0f)
					+ SSplitter::Slot().Value(0.3f)
					[
						SAssignNew(TreeView, SPjcTreeView)
					]
					+ SSplitter::Slot().Value(0.7f)
					[
						SAssignNew(ContentBrowser, SPjcContentBrowser)
					]
				]
				+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(5.0f)
				[
					AssetDiscoveryIndicator
				]
			]
		]
	];
}

TSharedRef<ITableRow> SPjcAssetBrowser::OnStatGenerateRow(TSharedPtr<FPjcAssetBrowserStatItem> Item, const TSharedRef<STableViewBase>& OwnerTable) const
{
	return SNew(SPjcAssetBrowserStatItem, OwnerTable).Item(Item);
}

TSharedRef<SHeaderRow> SPjcAssetBrowser::GetStatHeaderRow() const
{
	const FMargin HeaderContentPadding{5.0f};

	return
		SNew(SHeaderRow)
		+ SHeaderRow::Column("Category").FillWidth(0.4f).HAlignCell(HAlign_Left).VAlignCell(VAlign_Center).HAlignHeader(HAlign_Center).HeaderContentPadding(HeaderContentPadding)
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

FReply SPjcAssetBrowser::OnBtnScanAssetsClick() const
{
	if (TreeView.IsValid())
	{
		TreeView->TreeViewListUpdate();
	}

	return FReply::Handled();
}
