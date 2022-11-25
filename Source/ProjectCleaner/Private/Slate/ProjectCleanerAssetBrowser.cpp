// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/ProjectCleanerAssetBrowser.h"

#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "ProjectCleanerStyles.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Views/STileView.h"

void SProjectCleanerAssetBrowser::Construct(const FArguments& InArgs)
{
	const FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	const FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");

	FAssetPickerConfig AssetPickerConfig;
	AssetPickerConfig.InitialAssetViewType = EAssetViewType::Tile;
	AssetPickerConfig.bCanShowFolders = true;
	AssetPickerConfig.bAddFilterUI = true;
	AssetPickerConfig.bPreloadAssetsForContextMenu = false;
	AssetPickerConfig.bSortByPathInColumnView = true;
	AssetPickerConfig.bShowPathInColumnView = true;
	AssetPickerConfig.bShowBottomToolbar = true;
	AssetPickerConfig.bCanShowDevelopersFolder = true;
	AssetPickerConfig.bCanShowClasses = false;
	AssetPickerConfig.bAllowDragging = false;
	AssetPickerConfig.bForceShowEngineContent = false;
	AssetPickerConfig.bCanShowRealTimeThumbnails = false;
	AssetPickerConfig.AssetShowWarningText = FText::FromName("No assets");

	FARFilter Filter;
	Filter.PackagePaths.Add(TEXT("/Game"));
	Filter.bRecursivePaths = true;

	TArray<FAssetData> AssetData;
	AssetRegistryModule.Get().GetAssets(Filter, AssetData);


	for (const auto& Asset : AssetData)
	{
		Items.Add(MakeShareable(new FTestData(Asset)));
	}

	ChildSlot
	[
		SNew(SSplitter)
		.PhysicalSplitterHandleSize(5.0f)
		.Style(FEditorStyle::Get(), "ContentBrowser.Splitter")
		+ SSplitter::Slot()
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("Excluded Assets")))
				.Font(FProjectCleanerStyles::Get().GetFontStyle("ProjectCleaner.Font.Light20"))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SScrollBox)
				.ScrollWhenFocusChanges(EScrollWhenFocusChanges::NoScroll)
				.AnimateWheelScrolling(true)
				.AllowOverscroll(EAllowOverscroll::No)
				+ SScrollBox::Slot()
				[
					ContentBrowserModule.Get().CreateAssetPicker(AssetPickerConfig)
				]
			]
		]
		+ SSplitter::Slot()
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("Unused Assets")))
				.Font(FProjectCleanerStyles::Get().GetFontStyle("ProjectCleaner.Font.Light20"))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SScrollBox)
				.ScrollWhenFocusChanges(EScrollWhenFocusChanges::NoScroll)
				.AnimateWheelScrolling(true)
				.AllowOverscroll(EAllowOverscroll::No)
				+ SScrollBox::Slot()
				[
					ContentBrowserModule.Get().CreateAssetPicker(AssetPickerConfig)
				]
			]
		]
		// [
		// 	SNew(STileView< TSharedPtr<FTestData>>)
		// 	.ItemWidth(109)
		// 	.ItemHeight(128)
		// 	.ListItemsSource(&Items)
		// 	.SelectionMode(ESelectionMode::Multi)
		// 	.OnGenerateTile(this, &SProjectCleanerAssetBrowser::OnGenerateWidgetForTileView)
		// ]
	];
}

TSharedRef<ITableRow> SProjectCleanerAssetBrowser::OnGenerateWidgetForTileView(TSharedPtr<FTestData> InItem, const TSharedRef<STableViewBase>& OwnerTable)
{
	const TSharedPtr<FAssetThumbnail> AssetThumbnail = MakeShareable(new FAssetThumbnail(InItem->AssetData, 108, 108, nullptr));
	const FAssetThumbnailConfig ThumbnailConfig;

	const bool RandBool = FMath::RandBool();
	return SNew(STableRow< TSharedPtr<FTestData> >, OwnerTable)
	[
		SNew(SBorder)
		.Padding(2)
		.BorderImage(FEditorStyle::GetBrush("NoBorder"))
		.ColorAndOpacity(FLinearColor{1.0f, 1.0f, 1.0f, RandBool ? 0.2f : 1.0f})
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			  .AutoHeight()
			  .HAlign(HAlign_Center)
			[
				SNew(SBox)
				.Padding(0)
				.WidthOverride(108)
				.HeightOverride(108)
				[
					AssetThumbnail->MakeThumbnailWidget(ThumbnailConfig)
				]
				// [
				// 	// Drop shadow border
				// 	// SNew(SBorder)
				// 	// .Padding(4.f)
				// 	// .BorderBackgroundColor(RandBool ? FLinearColor::White : FLinearColor::Red)
				// 	// [
				// 	// 	SNew(SOverlay)
				// 	// 	+ SOverlay::Slot()
				// 	// 	.HAlign(HAlign_Fill)
				// 	// 	.VAlign(VAlign_Fill)
				// 	// 	[
				// 	// 	]
				// 	// ]
				// ]
			]
			+ SVerticalBox::Slot()
			  .FillHeight(1.0f)
			  .HAlign(HAlign_Center)
			  .VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.AutoWrapText(true)
				.WrapTextAt(0.7f)
				.Text(FText::FromString(InItem->AssetData.AssetName.ToString()))
			]
		]
	];
}
