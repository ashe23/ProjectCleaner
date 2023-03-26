// // Copyright Ashot Barkhudaryan. All Rights Reserved.
//
// #include "Slate/SPjcFileListView.h"
// #include "PjcCmds.h"
// #include "PjcStyles.h"
// #include "PjcConstants.h"
// #include "PjcSubsystem.h"
// // Engine Headers
// #include "Widgets/Input/SHyperlink.h"
// #include "Widgets/Layout/SScrollBox.h"
//
// void SPjcFileListViewItem::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InTable)
// {
// 	Item = InArgs._Item;
//
// 	SMultiColumnTableRow::Construct(SMultiColumnTableRow::FArguments(), InTable);
// }
//
// TSharedRef<SWidget> SPjcFileListViewItem::GenerateWidgetForColumn(const FName& InColumnName)
// {
// 	if (InColumnName.IsEqual(TEXT("FileName")))
// 	{
// 		return
// 			SNew(SHorizontalBox)
// 			+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin{5.0f, 0.0f})
// 			[
// 				SNew(STextBlock).Text(FText::FromString(Item->FileName))
// 			];
// 	}
//
// 	if (InColumnName.IsEqual(TEXT("FileExt")))
// 	{
// 		return SNew(STextBlock).Text(FText::FromString(Item->FileExt));
// 	}
//
// 	if (InColumnName.IsEqual(TEXT("FileSize")))
// 	{
// 		return SNew(STextBlock).Text(FText::AsMemory(Item->FileSize));
// 	}
//
// 	if (InColumnName.IsEqual(TEXT("FilePath")))
// 	{
// 		return
// 			SNew(SHorizontalBox)
// 			+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin{5.0f, 0.0f})
// 			[
// 				SNew(SHyperlink).Text(FText::FromString(Item->FilePath)).OnNavigate_Lambda([&]()
// 				{
// 					if (!FPaths::FileExists(Item->FilePath)) return;
//
// 					FPlatformProcess::ExploreFolder(*Item->FilePath);
// 				})
// 			];
// 	}
//
// 	return SNew(STextBlock).Text(FText::FromString(""));
// }
//
// void SPjcFileListView::Construct(const FArguments& InArgs)
// {
// 	bOpenFileOnDblClick = InArgs._OpenFileOnDblClick;
//
// 	CmdsRegister();
//
// 	ChildSlot
// 	[
// 		SNew(SOverlay)
// 		+ SOverlay::Slot().Padding(FMargin{5.0f})
// 		[
// 			SNew(SVerticalBox)
// 			+ SVerticalBox::Slot().FillHeight(1.0f)
// 			[
// 				SNew(SScrollBox)
// 				.ScrollWhenFocusChanges(EScrollWhenFocusChanges::NoScroll)
// 				.AnimateWheelScrolling(true)
// 				.AllowOverscroll(EAllowOverscroll::No)
// 				+ SScrollBox::Slot()
// 				[
// 					SAssignNew(ListView, SListView<TSharedPtr<FPjcFileListViewItem>>)
// 					.ListItemsSource(&ListItems)
// 					.OnGenerateRow(this, &SPjcFileListView::OnGenerateRow)
// 					.OnMouseButtonDoubleClick_Raw(this, &SPjcFileListView::OnListItemDblClick)
// 					.OnContextMenuOpening_Raw(this, &SPjcFileListView::OnListContextMenu)
// 					.SelectionMode(ESelectionMode::Multi)
// 					.HeaderRow(GetHeaderRow())
// 				]
// 			]
// 			+ SVerticalBox::Slot().AutoHeight()
// 			[
// 				SNew(STextBlock)
// 				.AutoWrapText(true)
// 				.Font(FPjcStyles::GetFont("Light", 8))
// 				.Text_Raw(this, &SPjcFileListView::GetListSummaryText)
// 			]
// 		]
// 	];
// }
//
// void SPjcFileListView::UpdateData(const TArray<FString>& InFiles)
// {
// 	ListItems.Empty();
// 	ListItems.Reserve(InFiles.Num());
//
// 	TotalSize = 0;
//
// 	for (const auto& File : InFiles)
// 	{
// 		const TSharedPtr<FPjcFileListViewItem> Item = MakeShareable(new FPjcFileListViewItem);
// 		if (!Item.IsValid()) continue;
//
// 		Item->FilePath = File;
// 		Item->FileName = FPaths::GetBaseFilename(File);
// 		Item->FileExt = FPaths::GetExtension(File, true).ToLower();
// 		Item->FileSize = UPjcSubsystem::GetFileSize(File);
//
// 		TotalSize += Item->FileSize;
//
// 		ListItems.Add(Item);
// 	}
//
// 	if (ListView.IsValid())
// 	{
// 		ListView->RequestListRefresh();
// 	}
// }
//
// FPjcDelegateRequestedFilesDelete& SPjcFileListView::OnFilesDeleteRequest()
// {
// 	return DelegateRequestedFilesDelete;
// }
//
// void SPjcFileListView::CmdsRegister()
// {
// 	Cmds = MakeShareable(new FUICommandList);
// 	Cmds->MapAction(
// 		FPjcCmds::Get().DeleteFiles,
// 		FUIAction(
// 			FExecuteAction::CreateLambda([&]()
// 			{
// 				if (!ListView.IsValid()) return;
//
// 				const auto SelectedItem = ListView->GetSelectedItems();
// 				if (SelectedItem.Num() == 0) return;
//
// 				TArray<FString> Files;
// 				Files.Reserve(SelectedItem.Num());
//
// 				for (const auto& Item : SelectedItem)
// 				{
// 					Files.Add(Item->FilePath);
// 				}
//
// 				if (DelegateRequestedFilesDelete.IsBound())
// 				{
// 					DelegateRequestedFilesDelete.Broadcast(Files);
// 				}
// 			})
// 		)
// 	);
// }
//
// void SPjcFileListView::OnListItemDblClick(TSharedPtr<FPjcFileListViewItem> Item) const
// {
// 	if (!Item.IsValid()) return;
// 	if (!FPaths::FileExists(Item->FilePath)) return;
//
// 	if (bOpenFileOnDblClick)
// 	{
// 		FPlatformProcess::LaunchFileInDefaultExternalApplication(*Item->FilePath);
// 	}
// 	else
// 	{
// 		FPlatformProcess::ExploreFolder(*Item->FilePath);
// 	}
// }
//
// void SPjcFileListView::OnListSort(EColumnSortPriority::Type SortPriority, const FName& ColumnName, EColumnSortMode::Type SortMode)
// {
// 	if (ColumnName.IsEqual(TEXT("FileName")))
// 	{
// 		ColumnFileNameSortMode = ColumnFileNameSortMode == EColumnSortMode::Ascending ? EColumnSortMode::Descending : EColumnSortMode::Ascending;
//
// 		ListItems.Sort([&](const TSharedPtr<FPjcFileListViewItem>& Item1, const TSharedPtr<FPjcFileListViewItem>& Item2)
// 		{
// 			return ColumnFileNameSortMode == EColumnSortMode::Ascending
// 				       ? Item1->FileName < Item2->FileName
// 				       : Item1->FileName > Item2->FileName;
// 		});
// 	}
//
// 	if (ColumnName.IsEqual(TEXT("FileExt")))
// 	{
// 		ColumnFileExtSortMode = ColumnFileExtSortMode == EColumnSortMode::Ascending ? EColumnSortMode::Descending : EColumnSortMode::Ascending;
//
// 		ListItems.Sort([&](const TSharedPtr<FPjcFileListViewItem>& Item1, const TSharedPtr<FPjcFileListViewItem>& Item2)
// 		{
// 			return ColumnFileExtSortMode == EColumnSortMode::Ascending
// 				       ? Item1->FileExt < Item2->FileExt
// 				       : Item1->FileExt > Item2->FileExt;
// 		});
// 	}
//
// 	if (ColumnName.IsEqual(TEXT("FileSize")))
// 	{
// 		ColumnFileSizeSortMode = ColumnFileSizeSortMode == EColumnSortMode::Ascending ? EColumnSortMode::Descending : EColumnSortMode::Ascending;
//
// 		ListItems.Sort([&](const TSharedPtr<FPjcFileListViewItem>& Item1, const TSharedPtr<FPjcFileListViewItem>& Item2)
// 		{
// 			return ColumnFileSizeSortMode == EColumnSortMode::Ascending
// 				       ? Item1->FileSize < Item2->FileSize
// 				       : Item1->FileSize > Item2->FileSize;
// 		});
// 	}
//
// 	if (ColumnName.IsEqual(TEXT("FilePath")))
// 	{
// 		ColumnFilePathSortMode = ColumnFilePathSortMode == EColumnSortMode::Ascending ? EColumnSortMode::Descending : EColumnSortMode::Ascending;
//
// 		ListItems.Sort([&](const TSharedPtr<FPjcFileListViewItem>& Item1, const TSharedPtr<FPjcFileListViewItem>& Item2)
// 		{
// 			return ColumnFilePathSortMode == EColumnSortMode::Ascending
// 				       ? Item1->FilePath < Item2->FilePath
// 				       : Item1->FilePath > Item2->FilePath;
// 		});
// 	}
//
// 	if (ListView.IsValid())
// 	{
// 		ListView->RequestListRefresh();
// 	}
// }
//
// FText SPjcFileListView::GetListSummaryText() const
// {
// 	if (ListView.IsValid() && ListView->GetSelectedItems().Num() > 0)
// 	{
// 		return FText::FromString(
// 			FString::Printf(
// 				TEXT("%d file%s (%d selected). Total Size: %s"),
// 				ListItems.Num(),
// 				ListItems.Num() > 1 ? TEXT("s") : TEXT(""),
// 				ListView->GetSelectedItems().Num(),
// 				*FText::AsMemory(TotalSize).ToString()
// 			)
// 		);
// 	}
//
// 	return FText::FromString(
// 		FString::Printf(
// 			TEXT("%d file%s. Total Size: %s"),
// 			ListItems.Num(),
// 			ListItems.Num() > 1 ? TEXT("s") : TEXT(""),
// 			*FText::AsMemory(TotalSize).ToString()
// 		)
// 	);
// }
//
// TSharedRef<SHeaderRow> SPjcFileListView::GetHeaderRow()
// {
// 	return
// 		SNew(SHeaderRow)
// 		+ SHeaderRow::Column("FileName")
// 		  .FillWidth(0.2f)
// 		  .HAlignCell(HAlign_Left)
// 		  .VAlignCell(VAlign_Center)
// 		  .HAlignHeader(HAlign_Center)
// 		  .HeaderContentPadding(PjcConstants::HeaderRowMargin)
// 		  .OnSort_Raw(this, &SPjcFileListView::OnListSort)
// 		[
// 			SNew(STextBlock)
// 			.Text(FText::FromString(TEXT("File Name")))
// 			.ColorAndOpacity(FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
// 			.Font(FPjcStyles::GetFont("Light", PjcConstants::HeaderRowFontSize))
// 		]
// 		+ SHeaderRow::Column("FileExt")
// 		  .FillWidth(0.2f)
// 		  .HAlignCell(HAlign_Center)
// 		  .VAlignCell(VAlign_Center)
// 		  .HAlignHeader(HAlign_Center)
// 		  .HeaderContentPadding(PjcConstants::HeaderRowMargin)
// 		  .OnSort_Raw(this, &SPjcFileListView::OnListSort)
// 		[
// 			SNew(STextBlock)
// 			.Text(FText::FromString(TEXT("File Extension")))
// 			.ColorAndOpacity(FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
// 			.Font(FPjcStyles::GetFont("Light", PjcConstants::HeaderRowFontSize))
// 		]
// 		+ SHeaderRow::Column("FileSize")
// 		  .FillWidth(0.2f)
// 		  .HAlignCell(HAlign_Center)
// 		  .VAlignCell(VAlign_Center)
// 		  .HAlignHeader(HAlign_Center)
// 		  .HeaderContentPadding(PjcConstants::HeaderRowMargin)
// 		  .OnSort_Raw(this, &SPjcFileListView::OnListSort)
// 		[
// 			SNew(STextBlock)
// 			.Text(FText::FromString(TEXT("File Size")))
// 			.ColorAndOpacity(FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
// 			.Font(FPjcStyles::GetFont("Light", PjcConstants::HeaderRowFontSize))
// 		]
// 		+ SHeaderRow::Column("FilePath")
// 		  .FillWidth(0.5f)
// 		  .HAlignCell(HAlign_Left)
// 		  .VAlignCell(VAlign_Center)
// 		  .HAlignHeader(HAlign_Center)
// 		  .HeaderContentPadding(PjcConstants::HeaderRowMargin)
// 		  .OnSort_Raw(this, &SPjcFileListView::OnListSort)
// 		[
// 			SNew(STextBlock)
// 			.Text(FText::FromString(TEXT("File Path")))
// 			.ColorAndOpacity(FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
// 			.Font(FPjcStyles::GetFont("Light", PjcConstants::HeaderRowFontSize))
// 		];
// }
//
// TSharedPtr<SWidget> SPjcFileListView::OnListContextMenu() const
// {
// 	FMenuBuilder MenuBuilder{true, Cmds};
// 	MenuBuilder.BeginSection(TEXT("Actions"), FText::FromString(TEXT("File Actions")));
// 	{
// 		MenuBuilder.AddMenuEntry(FPjcCmds::Get().DeleteFiles);
// 	}
// 	MenuBuilder.EndSection();
//
// 	return MenuBuilder.MakeWidget();
// }
//
// TSharedRef<ITableRow> SPjcFileListView::OnGenerateRow(TSharedPtr<FPjcFileListViewItem> Item, const TSharedRef<STableViewBase>& OwnerTable) const
// {
// 	return SNew(SPjcFileListViewItem, OwnerTable).Item(Item);
// }
