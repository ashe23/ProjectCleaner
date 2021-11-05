// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#include "UI/ProjectCleanerIndirectAssetsUI.h"
#include "UI/ProjectCleanerStyle.h"
// Engine Headers
#include "Widgets/Layout/SScrollBox.h"
#include "IContentBrowserSingleton.h"
#include "Core/ProjectCleanerManager.h"
#include "Editor/ContentBrowser/Public/ContentBrowserModule.h"

#define LOCTEXT_NAMESPACE "FProjectCleanerModule"

void SProjectCleanerIndirectAssetsUI::Construct(const FArguments& InArgs)
{
	if (InArgs._CleanerManager)
	{
		SetCleanerManager(InArgs._CleanerManager);
	}

	ChildSlot
	[
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
					.Text(LOCTEXT("indirect_assets_title", "Assets Used Indirectly"))
				]
				+ SVerticalBox::Slot()
				.Padding(FMargin{ 0.0f, 10.0f })
				.AutoHeight()
				[
					SNew(STextBlock)
					.AutoWrapText(true)
					.Font(FProjectCleanerStyle::Get().GetFontStyle("ProjectCleaner.Font.Light15"))
					.Text(LOCTEXT("indirect_assets_info", "List of assets that are used in source code, config or other files indirectly.\n"))
				]
				+ SVerticalBox::Slot()
				.Padding(FMargin{0.0f, 10.0f})
				.AutoHeight()
				[
					SNew(STextBlock)
					.AutoWrapText(true)
					.Font(FProjectCleanerStyle::Get().GetFontStyle("ProjectCleaner.Font.Light10"))
					.Text(LOCTEXT("indirect_assets_dbl_click_on_row", "Double click on row to open in Explorer"))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(FMargin{0.0f, 20.0f})
				[
					SAssignNew(ListView, SListView<TWeakObjectPtr<UIndirectAsset>>)
					.ListItemsSource(&IndirectAssets)
					.SelectionMode(ESelectionMode::SingleToggle)
					.OnGenerateRow(this, &SProjectCleanerIndirectAssetsUI::OnGenerateRow)
					.OnMouseButtonDoubleClick_Raw(this, &SProjectCleanerIndirectAssetsUI::OnMouseDoubleClick)
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
						+ SHeaderRow::Column(FName("FilePath"))
						.HAlignCell(HAlign_Center)
						.VAlignCell(VAlign_Center)
						.HAlignHeader(HAlign_Center)
						.HeaderContentPadding(FMargin(10.0f))
						.FillWidth(0.5f)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("FilePath", "File path"))
						]
						+ SHeaderRow::Column(FName("LineNum"))
						.HAlignCell(HAlign_Center)
						.VAlignCell(VAlign_Center)
						.HAlignHeader(HAlign_Center)
						.HeaderContentPadding(FMargin(10.0f))
						.FillWidth(0.1f)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("LineNum", "Line Number"))
						]
					)
				]
			]
		]
	];
}

void SProjectCleanerIndirectAssetsUI::SetCleanerManager(FProjectCleanerManager* CleanerManagerPtr)
{
	if (!CleanerManagerPtr) return;
	CleanerManager = CleanerManagerPtr;
	
	UpdateUI();
}

void SProjectCleanerIndirectAssetsUI::UpdateUI()
{
	if (!CleanerManager) return;

	IndirectAssets.Reset();
	IndirectAssets.Reserve(CleanerManager->GetIndirectAssets().Num());

	for (const auto& IndirectFile : CleanerManager->GetIndirectAssets())
	{
		const auto IndirectAsset = NewObject<UIndirectAsset>();
		IndirectAsset->AssetName = IndirectFile.Key.AssetName.ToString();
		IndirectAsset->AssetPath = IndirectFile.Value.RelativePath.ToString();
		IndirectAsset->FilePath = IndirectFile.Value.File;
		IndirectAsset->LineNum = IndirectFile.Value.Line;
		
		IndirectAssets.Add(IndirectAsset);
	}
	
	if (ListView.IsValid())
	{
		ListView->RebuildList();
	}
}

TSharedRef<ITableRow> SProjectCleanerIndirectAssetsUI::OnGenerateRow(
	TWeakObjectPtr<UIndirectAsset> InItem,
	const TSharedRef<STableViewBase>& OwnerTable) const
{
	return SNew(SIndirectAssetsUISelectionRow, OwnerTable).SelectedRowItem(InItem);
}

void SProjectCleanerIndirectAssetsUI::OnMouseDoubleClick(TWeakObjectPtr<UIndirectAsset> Item) const
{
	if (!Item.IsValid()) return;

	const auto DirectoryPath = FPaths::GetPath(Item.Get()->FilePath);
	if (!FPaths::DirectoryExists(DirectoryPath)) return;

	FPlatformProcess::ExploreFolder(*DirectoryPath);
}

#undef LOCTEXT_NAMESPACE