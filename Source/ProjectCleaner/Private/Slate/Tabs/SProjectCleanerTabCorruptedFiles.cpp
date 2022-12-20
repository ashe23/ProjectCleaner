// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/Tabs/SProjectCleanerTabCorruptedFiles.h"
#include "ProjectCleanerStyles.h"
#include "ProjectCleanerConstants.h"
// Engine Headers
#include "Widgets/Layout/SScrollBox.h"

void SProjectCleanerTabCorrupted::Construct(const FArguments& InArgs)
{
	if (!GEditor) return;
	SubsystemPtr = GEditor->GetEditorSubsystem<UProjectCleanerSubsystem>();
	if (!SubsystemPtr) return;

	ListUpdate();

	ChildSlot
	[
		SNew(SOverlay)
		+ SOverlay::Slot()
		.Padding(FMargin{5.0f})
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			  .Padding(FMargin{0.0f, 0.0f, 0.0f, 10.0f})
			  .AutoHeight()
			[
				SNew(STextBlock)
				.AutoWrapText(true)
				.Font(FProjectCleanerStyles::GetFont("Light", 15))
				.Text(FText::FromString(TEXT("List of potentially corrupted assets found in the Content folder but not loaded by the engine.")))
			]
			+ SVerticalBox::Slot()
			  .Padding(FMargin{0.0f, 0.0f, 0.0f, 10.0f})
			  .AutoHeight()
			[
				SNew(STextBlock)
				.AutoWrapText(true)
				.Font(FProjectCleanerStyles::GetFont("Light", 10))
				.Text(FText::FromString(TEXT(
					                "In order to fix it, try to reload the project and see if it's loaded. Check OutputLog for more information. If nothing helps, close the editor and remove them manually from Explorer.")))
			]
			+ SVerticalBox::Slot()
			  .Padding(FMargin{0.0f, 10.0f})
			  .AutoHeight()
			[
				SNew(STextBlock)
				.AutoWrapText(true)
				.Font(FProjectCleanerStyles::GetFont("Light", 8))
				.Text(FText::FromString(TEXT("Double click on row to navigate file in FileExplorer")))
			]
			+ SVerticalBox::Slot()
			  .FillHeight(1.0f)
			  .Padding(FMargin{0.0f, 5.0f})
			[
				SNew(SScrollBox)
				.ScrollWhenFocusChanges(EScrollWhenFocusChanges::NoScroll)
				.AnimateWheelScrolling(true)
				.AllowOverscroll(EAllowOverscroll::No)
				+ SScrollBox::Slot()
				[
					ListView.ToSharedRef()
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.AutoWrapText(true)
				.Font(FProjectCleanerStyles::GetFont("Light", 8))
				.Text_Raw(this, &SProjectCleanerTabCorrupted::GetListTextSummary)
			]
		]
	];

	SubsystemPtr->OnProjectScanned().AddRaw(this, &SProjectCleanerTabCorrupted::OnProjectScanned);
}

SProjectCleanerTabCorrupted::~SProjectCleanerTabCorrupted()
{
	SubsystemPtr->OnProjectScanned().RemoveAll(this);
	SubsystemPtr = nullptr;
}

void SProjectCleanerTabCorrupted::OnProjectScanned()
{
	ListUpdate();
}

void SProjectCleanerTabCorrupted::ListUpdate()
{
	if (!SubsystemPtr) return;

	if (!ListView.IsValid())
	{
		SAssignNew(ListView, SListView<TSharedPtr<FProjectCleanerTabCorruptedListItem>>)
		.ListItemsSource(&ListItems)
		.SelectionMode(ESelectionMode::SingleToggle)
		.OnGenerateRow(this, &SProjectCleanerTabCorrupted::OnListGenerateRow)
		.OnMouseButtonDoubleClick_Raw(this, &SProjectCleanerTabCorrupted::OnListItemDblClick)
		.HeaderRow(GetListHeaderRow());
	}


	ListItems.Reset();
	TotalSize = 0;

	for (const auto& CorruptedFile : SubsystemPtr->GetScanData().FilesCorrupted)
	{
		const TSharedPtr<FProjectCleanerTabCorruptedListItem> NewItem = MakeShareable(new FProjectCleanerTabCorruptedListItem);
		if (!NewItem) continue;

		if (CorruptedFile.IsEmpty() || !FPaths::FileExists(CorruptedFile)) continue;

		NewItem->FileName = FPaths::GetCleanFilename(CorruptedFile);
		NewItem->FileExtension = FPaths::GetExtension(CorruptedFile, true);
		NewItem->FilePathAbs = CorruptedFile;
		NewItem->FileSize = IFileManager::Get().FileSize(*CorruptedFile);
		TotalSize += NewItem->FileSize;

		ListItems.Add(NewItem);
	}

	ListSort();
	ListView->RequestListRefresh();
}

void SProjectCleanerTabCorrupted::ListSort()
{
	if (ListSortMode == EColumnSortMode::Ascending)
	{
		ListItems.Sort([&]
		(
			const TSharedPtr<FProjectCleanerTabCorruptedListItem>& Data1,
			const TSharedPtr<FProjectCleanerTabCorruptedListItem>& Data2)
			{
				if (ListSortColumn.IsEqual(TEXT("FilePath")))
				{
					return Data1->FilePathAbs.Compare(Data2->FilePathAbs) < 0;
				}

				if (ListSortColumn.IsEqual(TEXT("FileName")))
				{
					return Data1->FileName.Compare(Data2->FileName) < 0;
				}

				if (ListSortColumn.IsEqual(TEXT("FileExt")))
				{
					return Data1->FileExtension.Compare(Data2->FileExtension) < 0;
				}

				if (ListSortColumn.IsEqual(TEXT("FileSize")))
				{
					return Data1->FileSize < Data2->FileSize;
				}

				return Data1->FileSize < Data2->FileSize;
			}
		);
	}

	if (ListSortMode == EColumnSortMode::Descending)
	{
		ListItems.Sort([&]
		(
			const TSharedPtr<FProjectCleanerTabCorruptedListItem>& Data1,
			const TSharedPtr<FProjectCleanerTabCorruptedListItem>& Data2)
			{
				if (ListSortColumn.IsEqual(TEXT("FilePath")))
				{
					return Data1->FilePathAbs.Compare(Data2->FilePathAbs) > 0;
				}

				if (ListSortColumn.IsEqual(TEXT("FileName")))
				{
					return Data1->FileName.Compare(Data2->FileName) > 0;
				}

				if (ListSortColumn.IsEqual(TEXT("FileExt")))
				{
					return Data1->FileExtension.Compare(Data2->FileExtension) > 0;
				}

				if (ListSortColumn.IsEqual(TEXT("FileSize")))
				{
					return Data1->FileSize > Data2->FileSize;
				}

				return Data1->FileSize > Data2->FileSize;
			}
		);
	}
}

void SProjectCleanerTabCorrupted::OnListSort(EColumnSortPriority::Type SortPriority, const FName& Name, EColumnSortMode::Type SortMode)
{
	switch (ListSortMode)
	{
		case EColumnSortMode::Ascending:
			ListSortMode = EColumnSortMode::Descending;
			break;
		case EColumnSortMode::Descending:
			ListSortMode = EColumnSortMode::Ascending;
			break;
		case EColumnSortMode::None:
			ListSortMode = EColumnSortMode::Ascending;
			break;
		default:
			ListSortMode = EColumnSortMode::Descending;
	}

	ListSortColumn = Name;

	ListSort();

	if (ListView.IsValid())
	{
		ListView->RequestListRefresh();
	}
}

void SProjectCleanerTabCorrupted::OnListItemDblClick(TSharedPtr<FProjectCleanerTabCorruptedListItem> Item) const
{
	if (!Item.IsValid()) return;

	const FString DirPath = FPaths::GetPath(Item->FilePathAbs);
	if (DirPath.IsEmpty() || !FPaths::DirectoryExists(DirPath)) return;

	FPlatformProcess::ExploreFolder(*DirPath);
}

TSharedPtr<SHeaderRow> SProjectCleanerTabCorrupted::GetListHeaderRow()
{
	return
		SNew(SHeaderRow)
		+ SHeaderRow::Column(FName("FilePath"))
		  .HAlignCell(HAlign_Left)
		  .VAlignCell(VAlign_Center)
		  .HAlignHeader(HAlign_Center)
		  .HeaderContentPadding(FMargin(10.0f))
		  .FillWidth(1.0f)
		  .OnSort_Raw(this, &SProjectCleanerTabCorrupted::OnListSort)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Path")))
			.ColorAndOpacity(FProjectCleanerStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
			.Font(FProjectCleanerStyles::GetFont("Light", ProjectCleanerConstants::HeaderRowFontSize))
		]
		+ SHeaderRow::Column(FName("FileName"))
		  .HAlignCell(HAlign_Center)
		  .VAlignCell(VAlign_Center)
		  .HAlignHeader(HAlign_Center)
		  .HeaderContentPadding(FMargin(10.0f))
		  .FillWidth(0.5f)
		  .OnSort_Raw(this, &SProjectCleanerTabCorrupted::OnListSort)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Name")))
			.ColorAndOpacity(FProjectCleanerStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
			.Font(FProjectCleanerStyles::GetFont("Light", ProjectCleanerConstants::HeaderRowFontSize))
		]
		+ SHeaderRow::Column(FName("FileExt"))
		  .HAlignCell(HAlign_Center)
		  .VAlignCell(VAlign_Center)
		  .HAlignHeader(HAlign_Center)
		  .HeaderContentPadding(FMargin(10.0f))
		  .FillWidth(0.2f)
		  .OnSort_Raw(this, &SProjectCleanerTabCorrupted::OnListSort)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Extension")))
			.ColorAndOpacity(FProjectCleanerStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
			.Font(FProjectCleanerStyles::GetFont("Light", ProjectCleanerConstants::HeaderRowFontSize))
		]
		+ SHeaderRow::Column(FName("FileSize"))
		  .HAlignCell(HAlign_Center)
		  .VAlignCell(VAlign_Center)
		  .HAlignHeader(HAlign_Center)
		  .HeaderContentPadding(FMargin(10.0f))
		  .FillWidth(0.2f)
		  .OnSort_Raw(this, &SProjectCleanerTabCorrupted::OnListSort)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Size")))
			.ColorAndOpacity(FProjectCleanerStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
			.Font(FProjectCleanerStyles::GetFont("Light", ProjectCleanerConstants::HeaderRowFontSize))
		];
}

TSharedRef<ITableRow> SProjectCleanerTabCorrupted::OnListGenerateRow(TSharedPtr<FProjectCleanerTabCorruptedListItem> InItem, const TSharedRef<STableViewBase>& OwnerTable) const
{
	return SNew(SProjectCleanerTabCorruptedListItem, OwnerTable).ListItem(InItem);
}

FText SProjectCleanerTabCorrupted::GetListTextSummary() const
{
	return FText::FromString(
		FString::Printf(
			TEXT("%d item%s. Total Size: %s"),
			ListItems.Num(),
			ListItems.Num() > 1 ? TEXT("s") : TEXT(""),
			*FText::AsMemory(TotalSize).ToString()
		)
	);
}
