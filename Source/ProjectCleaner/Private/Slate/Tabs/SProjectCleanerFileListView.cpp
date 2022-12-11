// // Copyright Ashot Barkhudaryan. All Rights Reserved.
//
// #include "Slate/Tabs/SProjectCleanerFileListView.h"
// #include "ProjectCleanerStyles.h"
// // Engine Headers
// #include "ProjectCleanerConstants.h"
// #include "Widgets/Layout/SScrollBox.h"
//
// void SProjectCleanerFileListView::Construct(const FArguments& InArgs)
// {
// 	Title = InArgs._Title;
// 	Description = InArgs._Description;
// 	
// 	UpdateView(InArgs._Files);
//
// 	ChildSlot
// 	[
// 		SNew(SOverlay)
// 		+ SOverlay::Slot()
// 		.Padding(FMargin{5.0f})
// 		[
// 			SNew(SVerticalBox)
// 			+ SVerticalBox::Slot()
// 			  .Padding(FMargin{0.0f, 0.0f, 0.0f, 10.0f})
// 			  .AutoHeight()
// 			[
// 				SNew(STextBlock)
// 				.AutoWrapText(true)
// 				.Font(FProjectCleanerStyles::GetFont("Light", 15))
// 				.Text(FText::FromString(InArgs._Title))
// 				.Visibility_Lambda([&]()
// 				{
// 					return Title.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible;
// 				})
// 			]
// 			+ SVerticalBox::Slot()
// 			  .Padding(FMargin{0.0f, 0.0f, 0.0f, 10.0f})
// 			  .AutoHeight()
// 			[
// 				SNew(STextBlock)
// 				.AutoWrapText(true)
// 				.Font(FProjectCleanerStyles::GetFont("Light", 10))
// 				.Text(FText::FromString(InArgs._Description))
// 				.Visibility_Lambda([&]()
// 				{
// 					return Description.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible;
// 				})
// 			]
// 			+ SVerticalBox::Slot()
// 			  .Padding(FMargin{0.0f, 10.0f})
// 			  .AutoHeight()
// 			[
// 				SNew(STextBlock)
// 				.AutoWrapText(true)
// 				.Font(FProjectCleanerStyles::GetFont("Light", 8))
// 				.Text(FText::FromString(TEXT("Double click on row to open in Explorer")))
// 			]
// 			+ SVerticalBox::Slot()
// 			  .FillHeight(1.0f)
// 			  .Padding(FMargin{0.0f, 5.0f})
// 			[
// 				SNew(SScrollBox)
// 				.ScrollWhenFocusChanges(EScrollWhenFocusChanges::NoScroll)
// 				.AnimateWheelScrolling(true)
// 				.AllowOverscroll(EAllowOverscroll::No)
// 				+ SScrollBox::Slot()
// 				[
// 					ListView.ToSharedRef()
// 				]
// 			]
// 			+ SVerticalBox::Slot()
// 			.AutoHeight()
// 			[
// 				SNew(STextBlock)
// 				.AutoWrapText(true)
// 				.Font(FProjectCleanerStyles::GetFont("Light", 8))
// 				.Text_Raw(this, &SProjectCleanerFileListView::GetTotalSizeTxt)
// 			]
// 		]
// 	];
// }
//
// void SProjectCleanerFileListView::UpdateView(const TSet<FString>& Files)
// {
// 	if (!ListView)
// 	{
// 		SAssignNew(ListView, SListView<TSharedPtr<FProjectCleanerFileViewItem>>)
// 		.ListItemsSource(&ListItems)
// 		.SelectionMode(ESelectionMode::SingleToggle)
// 		.OnGenerateRow(this, &SProjectCleanerFileListView::OnGenerateRow)
// 		.OnMouseButtonDoubleClick_Raw(this, &SProjectCleanerFileListView::OnListItemDblClick)
// 		.HeaderRow(GetListHeaderRow());
// 	}
// 	
// 	TotalSize = 0;
// 	ListItems.Reset();
// 	
// 	for (const auto& File : Files)
// 	{
// 		const TSharedPtr<FProjectCleanerFileViewItem> NewItem = MakeShareable(new FProjectCleanerFileViewItem);
// 		if (!NewItem) continue;
//
// 		NewItem->FilePath = File;
// 		NewItem->FileName = FPaths::GetPathLeaf(File);
// 		NewItem->FileExt = FPaths::GetExtension(File, true);
// 		NewItem->FileSize = IFileManager::Get().FileSize(*File);
//
// 		TotalSize += NewItem->FileSize;
//
// 		ListItems.Add(NewItem);
// 	}
//
// 	if (ListView.IsValid())
// 	{
// 		ListView->RequestListRefresh();
// 	}
// }
//
// FText SProjectCleanerFileListView::GetTotalSizeTxt() const
// {
// 	const FString TotalSizeStr = FString::Printf(TEXT("%d item%s. Total Size: %s"), ListItems.Num(), ListItems.Num() > 1 ? TEXT("s") : TEXT(""), *FText::AsMemory(TotalSize).ToString());
// 	return FText::FromString(TotalSizeStr);
// }
//
// TSharedPtr<SHeaderRow> SProjectCleanerFileListView::GetListHeaderRow() const
// {
// 	return
// 		SNew(SHeaderRow)
// 		+ SHeaderRow::Column(FName("FileName"))
// 		  .HAlignCell(HAlign_Center)
// 		  .VAlignCell(VAlign_Center)
// 		  .HAlignHeader(HAlign_Center)
// 		  .HeaderContentPadding(FMargin(10.0f))
// 		[
// 			SNew(STextBlock)
// 			.Text(FText::FromString(TEXT("Name")))
// 			.ColorAndOpacity(FProjectCleanerStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
// 			.Font(FProjectCleanerStyles::GetFont("Light", ProjectCleanerConstants::HeaderRowFontSize))
// 		]
// 		+ SHeaderRow::Column(FName("FileExt"))
// 		  .HAlignCell(HAlign_Center)
// 		  .VAlignCell(VAlign_Center)
// 		  .HAlignHeader(HAlign_Center)
// 		  .HeaderContentPadding(FMargin(10.0f))
// 		  .FillWidth(0.2f)
// 		[
// 			SNew(STextBlock)
// 			.Text(FText::FromString(TEXT("Extension")))
// 			.ColorAndOpacity(FProjectCleanerStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
// 			.Font(FProjectCleanerStyles::GetFont("Light", ProjectCleanerConstants::HeaderRowFontSize))
// 		]
// 		+ SHeaderRow::Column(FName("FileSize"))
// 		  .HAlignCell(HAlign_Center)
// 		  .VAlignCell(VAlign_Center)
// 		  .HAlignHeader(HAlign_Center)
// 		  .HeaderContentPadding(FMargin(10.0f))
// 		  .FillWidth(0.2f)
// 		[
// 			SNew(STextBlock)
// 			.Text(FText::FromString(TEXT("Size")))
// 			.ColorAndOpacity(FProjectCleanerStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
// 			.Font(FProjectCleanerStyles::GetFont("Light", ProjectCleanerConstants::HeaderRowFontSize))
// 		]
// 		+ SHeaderRow::Column(FName("FilePath"))
// 		  .HAlignCell(HAlign_Center)
// 		  .VAlignCell(VAlign_Center)
// 		  .HAlignHeader(HAlign_Center)
// 		  .HeaderContentPadding(FMargin(10.0f))
// 		  .FillWidth(1.0f)
// 		[
// 			SNew(STextBlock)
// 			.Text(FText::FromString(TEXT("Path")))
// 			.ColorAndOpacity(FProjectCleanerStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
// 			.Font(FProjectCleanerStyles::GetFont("Light", ProjectCleanerConstants::HeaderRowFontSize))
// 		];
// }
//
// void SProjectCleanerFileListView::OnListItemDblClick(TSharedPtr<FProjectCleanerFileViewItem> Item) const
// {
// 	if (!Item.IsValid()) return;
//
// 	const FString DirPath = FPaths::GetPath(Item->FilePath);
// 	if (!FPaths::DirectoryExists(DirPath)) return;
//
// 	FPlatformProcess::ExploreFolder(*DirPath);
// }
//
// TSharedRef<ITableRow> SProjectCleanerFileListView::OnGenerateRow(TSharedPtr<FProjectCleanerFileViewItem> InItem, const TSharedRef<STableViewBase>& OwnerTable) const
// {
// 	return SNew(SProjectCleanerFileViewItem, OwnerTable).ListItem(InItem);
// }
