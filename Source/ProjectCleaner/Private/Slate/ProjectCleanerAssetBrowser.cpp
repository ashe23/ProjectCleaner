// // Copyright Ashot Barkhudaryan. All Rights Reserved.
//
// #include "Slate/ProjectCleanerAssetBrowser.h"
// #include "ProjectCleanerConstants.h"
// #include "ProjectCleanerStyles.h"
// // Engine Headers
// #include "ProjectCleanerScanner.h"
// #include "AssetRegistry/AssetRegistryModule.h"
// #include "Internationalization/BreakIterator.h"
// #include "Widgets/Input/SSearchBox.h"
// #include "Widgets/Layout/SScrollBox.h"
// #include "Widgets/Layout/SSeparator.h"
//
//
// void SProjectCleanerAssetBrowser::Construct(const FArguments& InArgs)
// {
// 	Scanner = InArgs._Scanner;
// 	if (!Scanner.IsValid()) return;
//
// 	Scanner.Get()->OnScanFinished().AddLambda([&]()
// 	{
// 		UpdateView();
// 	});
//
// 	AssetThumbnailPool = MakeShareable(new FAssetThumbnailPool(1024, false));
//
// 	UpdateView();
//
// 	ChildSlot
// 	[
// 		SNew(SVerticalBox)
// 		+ SVerticalBox::Slot()
// 		  .AutoHeight()
// 		  .Padding(FMargin{0.0f, 0.0f, 0.0f, 5.0f})
// 		[
// 			SNew(SSearchBox)
// 			.HintText(FText::FromString(TEXT("Search Assets...")))
// 			// .OnTextChanged(this, &SProjectCleanerTreeView::OnTreeViewSearchBoxTextChanged)
// 			// .OnTextCommitted(this, &SProjectCleanerTreeView::OnTreeViewSearchBoxTextCommitted)
// 		]
// 		+ SVerticalBox::Slot()
// 		.AutoHeight()
// 		[
// 			SNew(SSeparator)
// 			.Thickness(5.0f)
// 		]
// 		// + SVerticalBox::Slot()
// 		//   .AutoHeight()
// 		//   .Padding(FMargin{0.0f, 0.0f, 0.0f, 5.0f})
// 		// [
// 		// 	// SNew(SHorizontalBox)
// 		// 	// + SHorizontalBox::Slot()
// 		// 	// .AutoWidth()
// 		// 	// [
// 		// 	// 	SNew(SBox)
// 		// 	// 	.WidthOverride(16.0f)
// 		// 	// 	.HeightOverride(16.0f)
// 		// 	// 	[
// 		// 	// 		SNew(SImage)
// 		// 	// 		.Image(FProjectCleanerStyles::Get().GetBrush("ProjectCleaner.IconCircle16"))
// 		// 	// 		.ColorAndOpacity(FProjectCleanerStyles::Get().GetColor("ProjectCleaner.Color.Yellow"))
// 		// 	// 	]
// 		// 	// ]
// 		// 	// + SHorizontalBox::Slot()
// 		// 	//   .Padding(FMargin{0.0f, 2.0f, 0.0f, 0.0f})
// 		// 	//   .AutoWidth()
// 		// 	// [
// 		// 	// 	SNew(STextBlock)
// 		// 	// 	.Text(FText::FromString(TEXT(" - Excluded Asset")))
// 		// 	// ]
// 		// 	// + SHorizontalBox::Slot()
// 		// 	//   .AutoWidth()
// 		// 	//   .Padding(FMargin{5.0f, 0.0f, 0.0f, 0.0f})
// 		// 	// [
// 		// 	// 	SNew(SBox)
// 		// 	// 	.WidthOverride(16.0f)
// 		// 	// 	.HeightOverride(16.0f)
// 		// 	// 	[
// 		// 	// 		SNew(SImage)
// 		// 	// 		.Image(FProjectCleanerStyles::Get().GetBrush("ProjectCleaner.IconCircle16"))
// 		// 	// 		.ColorAndOpacity(FProjectCleanerStyles::Get().GetColor("ProjectCleaner.Color.Blue"))
// 		// 	// 	]
// 		// 	// ]
// 		// 	// + SHorizontalBox::Slot()
// 		// 	//   .Padding(FMargin{0.0f, 2.0f, 0.0f, 0.0f})
// 		// 	//   .AutoWidth()
// 		// 	// [
// 		// 	// 	SNew(STextBlock)
// 		// 	// 	.Text(FText::FromString(TEXT(" - Used Indirectly")))
// 		// 	// ]
// 		// 	// + SHorizontalBox::Slot()
// 		// 	//   .AutoWidth()
// 		// 	//   .Padding(FMargin{5.0f, 0.0f, 0.0f, 0.0f})
// 		// 	// [
// 		// 	// 	SNew(SBox)
// 		// 	// 	.WidthOverride(16.0f)
// 		// 	// 	.HeightOverride(16.0f)
// 		// 	// 	[
// 		// 	// 		SNew(SImage)
// 		// 	// 		.Image(FProjectCleanerStyles::Get().GetBrush("ProjectCleaner.IconCircle16"))
// 		// 	// 		.ColorAndOpacity(FProjectCleanerStyles::Get().GetColor("ProjectCleaner.Color.Red"))
// 		// 	// 	]
// 		// 	// ]
// 		// 	// + SHorizontalBox::Slot()
// 		// 	//   .Padding(FMargin{0.0f, 2.0f, 0.0f, 0.0f})
// 		// 	//   .AutoWidth()
// 		// 	// [
// 		// 	// 	SNew(STextBlock)
// 		// 	// 	.Text(FText::FromString(TEXT(" - In Developers Folder")))
// 		// 	// ]
// 		// ]
// 		+ SVerticalBox::Slot()
// 		  .FillHeight(1.0f)
// 		  .Padding(FMargin{0.0f, 5.0f})
// 		[
// 			SNew(SScrollBox)
// 			.ScrollWhenFocusChanges(EScrollWhenFocusChanges::NoScroll)
// 			.AnimateWheelScrolling(true)
// 			.AllowOverscroll(EAllowOverscroll::No)
// 			+ SScrollBox::Slot()
// 			[
// 				SAssignNew(ListView, STileView<TSharedPtr<FTestData>>)
// 				.ItemWidth(100)
// 				.ItemHeight(166)
// 				.ListItemsSource(&Items)
// 				.SelectionMode(ESelectionMode::Multi)
// 				.OnGenerateTile(this, &SProjectCleanerAssetBrowser::OnGenerateWidgetForTileView)
// 			]
// 		]
// 	];
// }
//
// void SProjectCleanerAssetBrowser::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
// {
// 	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
//
// 	if (!AssetThumbnailPool.IsValid()) return;
//
// 	AssetThumbnailPool->Tick(InDeltaTime);
// }
//
// void SProjectCleanerAssetBrowser::UpdateView()
// {
// 	if (!Scanner.IsValid()) return;
// 	if (Scanner->GetAssetsUnused().Num() == 0) return;
//
// 	Items.Reset();
// 	Items.Reserve(Scanner->GetAssetsUnused().Num());
//
// 	for (const auto& Asset : Scanner->GetAssetsUnused())
// 	{
// 		Items.Add(MakeShareable(new FTestData(Asset)));
// 	}
//
// 	if (ListView.IsValid())
// 	{
// 		ListView->RequestListRefresh();
// 	}
// }
//
// TSharedRef<ITableRow> SProjectCleanerAssetBrowser::OnGenerateWidgetForTileView(TSharedPtr<FTestData> InItem, const TSharedRef<STableViewBase>& OwnerTable) const
// {
// 	const TSharedPtr<FAssetThumbnail> AssetThumbnail = MakeShareable(new FAssetThumbnail(InItem->AssetData, 90, 90, AssetThumbnailPool));
// 	FAssetThumbnailConfig ThumbnailConfig;
// 	ThumbnailConfig.bAllowFadeIn = true;
//
// 	return SNew(STableRow< TSharedPtr<FTestData> >, OwnerTable)
// 		.Padding(FMargin{5.0f})
// 		[
// 			SNew(SBorder)
// 			.Padding(0.0f)
// 			// .BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
// 			.ColorAndOpacity(FLinearColor{1.0f, 1.0f, 1.0f, 1.0f})
// 			[
// 				SNew(SVerticalBox)
// 				+ SVerticalBox::Slot()
// 				  .AutoHeight()
// 				  .HAlign(HAlign_Center)
// 				[
// 					SNew(SBox)
// 					.Padding(0)
// 					.WidthOverride(90)
// 					.HeightOverride(90)
// 					[
// 						AssetThumbnail->MakeThumbnailWidget(ThumbnailConfig)
// 					]
// 				]
// 				+ SVerticalBox::Slot()
// 				  .FillHeight(1.0f)
// 				  .HAlign(HAlign_Fill)
// 				  .VAlign(VAlign_Fill)
// 				[
// 					SNew(SHorizontalBox)
// 					+ SHorizontalBox::Slot()
// 					  .FillWidth(1.0f)
// 					  .HAlign(HAlign_Fill)
// 					  .VAlign(VAlign_Fill)
// 					  .Padding(FMargin{3.0f, 2.0f})
// 					[
// 						SNew(STextBlock)
// 						.WrapTextAt(100.0f)
// 						.LineBreakPolicy(FBreakIterator::CreateCamelCaseBreakIterator())
// 						.Font(FProjectCleanerStyles::GetFont("Light", 10))
// 						.Text(FText::FromString(InItem->AssetData.AssetName.ToString()))
// 					]
// 				]
// 				+ SVerticalBox::Slot()
// 				  .FillHeight(1.0f)
// 				  .HAlign(HAlign_Fill)
// 				  .VAlign(VAlign_Bottom)
// 				[
// 					SNew(SHorizontalBox)
// 					+ SHorizontalBox::Slot()
// 					  .FillWidth(1.0f)
// 					  .HAlign(HAlign_Left)
// 					  .VAlign(VAlign_Center)
// 					  .Padding(FMargin{3.0f, 2.0f})
// 					[
// 						SNew(STextBlock)
// 						.Font(FProjectCleanerStyles::GetFont("Light", 8))
// 						.Text(FText::FromString(InItem->AssetData.AssetClass.ToString()))
// 					]
// 					// + SHorizontalBox::Slot()
// 					//   .AutoWidth()
// 					//   .HAlign(HAlign_Center)
// 					//   .VAlign(VAlign_Center)
// 					//   .Padding(FMargin{3.0f, 2.0f})
// 					// [
// 					// 	SNew(SBox)
// 					// 	.WidthOverride(16.0f)
// 					// 	.HeightOverride(16.0f)
// 					// 	[
// 					// 		SNew(SImage)
// 					// 		.Image(FProjectCleanerStyles::Get().GetBrush("ProjectCleaner.IconCircle16"))
// 					// 		.ColorAndOpacity(FProjectCleanerStyles::Get().GetColor("ProjectCleaner.Color.Yellow"))
// 					// 	]
// 					// ]
// 				]
// 			]
// 		];
// }
