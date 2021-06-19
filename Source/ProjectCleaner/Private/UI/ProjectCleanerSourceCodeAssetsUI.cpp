// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#include "UI/ProjectCleanerSourceCodeAssetsUI.h"
#include "ProjectCleanerStyle.h"
// Engine Headers
#include "Widgets/Layout/SScrollBox.h"
#include "IContentBrowserSingleton.h"
#include "Editor/ContentBrowser/Public/ContentBrowserModule.h"

#define LOCTEXT_NAMESPACE "FProjectCleanerModule"

void SProjectCleanerSourceCodeAssetsUI::Construct(const FArguments& InArgs)
{
	SetSourceCodeAssets(InArgs._SourceCodeAssets);

	InitUI();
}

void SProjectCleanerSourceCodeAssetsUI::SetSourceCodeAssets(const TArray<TWeakObjectPtr<USourceCodeAsset>>& NewSourceCodeAssets)
{
	SourceCodeAssets.Reset();
	SourceCodeAssets.Reserve(NewSourceCodeAssets.Num());
	SourceCodeAssets = NewSourceCodeAssets;

	if (ListView.IsValid())
	{
		ListView->SetListItemsSource(SourceCodeAssets);
		ListView->RebuildList();
	}
}

void SProjectCleanerSourceCodeAssetsUI::InitUI()
{
	ListView = SNew(SListView<TWeakObjectPtr<USourceCodeAsset>>)
		.ListItemsSource(&SourceCodeAssets)
		.SelectionMode(ESelectionMode::SingleToggle)
		.OnGenerateRow(this, &SProjectCleanerSourceCodeAssetsUI::OnGenerateRow)
		.OnMouseButtonDoubleClick_Raw(this, &SProjectCleanerSourceCodeAssetsUI::OnMouseDoubleClick)
		.HeaderRow
		(
			SNew(SHeaderRow)
			+ SHeaderRow::Column(FName("AssetName"))
			.HAlignCell(HAlign_Center)
			.VAlignCell(VAlign_Center)
			.HAlignHeader(HAlign_Center)
			.HeaderContentPadding(FMargin(10.0f))
			.FillWidth(0.2f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("AssetName", "Asset Name"))
			]
			+ SHeaderRow::Column(FName("AssetPath"))
			.HAlignCell(HAlign_Center)
			.VAlignCell(VAlign_Center)
			.HAlignHeader(HAlign_Center)
			.HeaderContentPadding(FMargin(10.0f))
			.FillWidth(0.2f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("AssetPath", "Asset Path"))
			]
			+ SHeaderRow::Column(FName("SourceCodePath"))
			.HAlignCell(HAlign_Center)
			.VAlignCell(VAlign_Center)
			.HAlignHeader(HAlign_Center)
			.HeaderContentPadding(FMargin(10.0f))
			.FillWidth(0.6f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("SourceCodePath", "Source file path"))
			]
		);

	WidgetRef =
		SNew(SOverlay)
		+ SOverlay::Slot()
		.Padding(20.0f)
		[
			SNew(SScrollBox)
			+ SScrollBox::Slot()
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(STextBlock)
					.AutoWrapText(true)
					.Font(FProjectCleanerStyle::Get().GetFontStyle("ProjectCleaner.Font.Light20"))
					.Text(LOCTEXT("source_code_assets_title", "Assets Used Indirectly"))
				]
				+ SVerticalBox::Slot()
				.Padding(FMargin{ 0.0f, 10.0f })
				.AutoHeight()
				[
					SNew(STextBlock)
					.AutoWrapText(true)
					.Font(FProjectCleanerStyle::Get().GetFontStyle("ProjectCleaner.Font.Light15"))
					.Text(LOCTEXT("source_code_assets_info", "List of assets that are used in source code, config or other files indirectly.\n"))
				]
				+ SVerticalBox::Slot()
				.Padding(FMargin{0.0f, 10.0f})
				.AutoHeight()
				[
					SNew(STextBlock)
					.AutoWrapText(true)
					.Font(FProjectCleanerStyle::Get().GetFontStyle("ProjectCleaner.Font.Light10"))
					.Text(LOCTEXT("source_code_assets_dbl_click_on_row", "Double click on row to navigate in content browser"))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(FMargin{0.0f, 20.0f})
				[
					ListView.ToSharedRef()
				]
			]
		];

	ChildSlot
	[
		WidgetRef
	];
}

TSharedRef<ITableRow> SProjectCleanerSourceCodeAssetsUI::OnGenerateRow(
	TWeakObjectPtr<USourceCodeAsset> InItem,
	const TSharedRef<STableViewBase>& OwnerTable) const
{
	return SNew(SSourceCodeAssetsUISelectionRow, OwnerTable).SelectedRowItem(InItem);
}

void SProjectCleanerSourceCodeAssetsUI::OnMouseDoubleClick(TWeakObjectPtr<USourceCodeAsset> Item) const
{
	if (!Item.IsValid()) return;

	TArray<FAssetData> SelectedAssets;
	SelectedAssets.Add(Item->AssetData);

	FContentBrowserModule& CBModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	CBModule.Get().SyncBrowserToAssets(SelectedAssets);
}

#undef LOCTEXT_NAMESPACE