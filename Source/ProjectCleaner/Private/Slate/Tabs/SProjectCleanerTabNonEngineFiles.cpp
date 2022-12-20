// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/Tabs/SProjectCleanerTabNonEngineFiles.h"
#include "ProjectCleanerStyles.h"
#include "ProjectCleanerConstants.h"
#include "ProjectCleanerCmds.h"
#include "ProjectCleanerSubsystem.h"
// Engine Headers
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Framework/Notifications/NotificationManager.h"

void SProjectCleanerTabNonEngine::Construct(const FArguments& InArgs)
{
	if (!GEditor) return;
	SubsystemPtr = GEditor->GetEditorSubsystem<UProjectCleanerSubsystem>();
	if (!SubsystemPtr) return;

	Cmds = MakeShareable(new FUICommandList);

	Cmds->MapAction(
		FProjectCleanerCmds::Get().TabNonEngineTryOpenFile,
		FUIAction(
			FExecuteAction::CreateLambda([&]()
			{
				if (!GEditor) return;
				if (!ListView.IsValid()) return;

				const auto& SelectedItems = ListView->GetSelectedItems();
				if (SelectedItems.Num() == 0) return;

				const FString FilePath = SelectedItems[0]->FilePathAbs;
				if (!FPaths::FileExists(FilePath)) return;

				FPlatformProcess::LaunchFileInDefaultExternalApplication(*FilePath);
			})
		)
	);

	Cmds->MapAction(
		FProjectCleanerCmds::Get().TabNonEngineDeleteFile,
		FUIAction(
			FExecuteAction::CreateLambda([&]()
			{
				if (!GEditor) return;
				if (!ListView.IsValid()) return;

				const auto& SelectedItems = ListView->GetSelectedItems();
				if (SelectedItems.Num() == 0) return;

				const FString FilePath = SelectedItems[0]->FilePathAbs;
				if (!FPaths::FileExists(FilePath)) return;


				if (IFileManager::Get().Delete(*FilePath))
				{
					FNotificationInfo Info{FText::FromString(TEXT("File deleted successfully"))};
					Info.ExpireDuration = 3.0f;

					const auto NotificationPtr = FSlateNotificationManager::Get().AddNotification(Info);
					if (!NotificationPtr.IsValid()) return;

					NotificationPtr.Get()->SetCompletionState(SNotificationItem::CS_Success);
				}
				else
				{
					FNotificationInfo Info{FText::FromString(TEXT("Failed to delete file"))};
					Info.ExpireDuration = 3.0f;
					Info.Hyperlink = FSimpleDelegate::CreateLambda([]()
					{
						FGlobalTabmanager::Get()->TryInvokeTab(FName{TEXT("OutputLog")});
					});
					Info.HyperlinkText = FText::FromString(TEXT("Show OutputLog..."));

					const auto NotificationPtr = FSlateNotificationManager::Get().AddNotification(Info);
					if (!NotificationPtr.IsValid()) return;

					NotificationPtr.Get()->SetCompletionState(SNotificationItem::CS_Fail);
				}

				GEditor->GetEditorSubsystem<UProjectCleanerSubsystem>()->ProjectScan();
			})
		)
	);

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
				.Text(FText::FromString(TEXT("List of NonEngine files inside Content folder.")))
			]
			+ SVerticalBox::Slot()
			  .Padding(FMargin{0.0f, 0.0f, 0.0f, 10.0f})
			  .AutoHeight()
			[
				SNew(STextBlock)
				.AutoWrapText(true)
				.Font(FProjectCleanerStyles::GetFont("Light", 10))
				.Text(FText::FromString(TEXT("Make sure you delete all unnecessary files in order to remove empty folders. These files won't be visible in ContentBrowser.")))
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
			.AutoHeight()
			[
				SNew(STextBlock)
				.AutoWrapText(true)
				.Font(FProjectCleanerStyles::GetFont("Light", 8))
				.Text(FText::FromString(TEXT("Right click on row to open context menu")))
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
				.Text_Raw(this, &SProjectCleanerTabNonEngine::GetListTextSummary)
			]
		]
	];

	SubsystemPtr->OnProjectScanned().AddRaw(this, &SProjectCleanerTabNonEngine::OnProjectScanned);
}

SProjectCleanerTabNonEngine::~SProjectCleanerTabNonEngine()
{
	SubsystemPtr->OnProjectScanned().RemoveAll(this);
	SubsystemPtr = nullptr;
}

void SProjectCleanerTabNonEngine::OnProjectScanned()
{
	ListUpdate();
}

void SProjectCleanerTabNonEngine::ListUpdate()
{
	if (!SubsystemPtr) return;

	if (!ListView.IsValid())
	{
		SAssignNew(ListView, SListView<TSharedPtr<FProjectCleanerTabNonEngineListItem>>)
		.ListItemsSource(&ListItems)
		.SelectionMode(ESelectionMode::SingleToggle)
		.OnGenerateRow(this, &SProjectCleanerTabNonEngine::OnListGenerateRow)
		.OnMouseButtonDoubleClick_Raw(this, &SProjectCleanerTabNonEngine::OnListItemDblClick)
		.OnContextMenuOpening_Raw(this, &SProjectCleanerTabNonEngine::OnListContextMenu)
		.HeaderRow(GetListHeaderRow());
	}

	ListItems.Reset();
	TotalSize = 0;

	for (const auto& NonEngineFile : SubsystemPtr->GetScanData().FilesNonEngine)
	{
		const TSharedPtr<FProjectCleanerTabNonEngineListItem> NewItem = MakeShareable(new FProjectCleanerTabNonEngineListItem);
		if (!NewItem) continue;

		if (NonEngineFile.IsEmpty() || !FPaths::FileExists(NonEngineFile)) continue;

		NewItem->FileName = FPaths::GetCleanFilename(NonEngineFile);
		NewItem->FileExtension = FPaths::GetExtension(NonEngineFile, true);
		NewItem->FilePathAbs = NonEngineFile;
		NewItem->FileSize = IFileManager::Get().FileSize(*NonEngineFile);
		TotalSize += NewItem->FileSize;

		ListItems.Add(NewItem);
	}

	ListSort();
	ListView->RequestListRefresh();
}

void SProjectCleanerTabNonEngine::ListSort()
{
	if (ListSortMode == EColumnSortMode::Ascending)
	{
		ListItems.Sort([&]
		(
			const TSharedPtr<FProjectCleanerTabNonEngineListItem>& Data1,
			const TSharedPtr<FProjectCleanerTabNonEngineListItem>& Data2)
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
			const TSharedPtr<FProjectCleanerTabNonEngineListItem>& Data1,
			const TSharedPtr<FProjectCleanerTabNonEngineListItem>& Data2)
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

void SProjectCleanerTabNonEngine::OnListSort(EColumnSortPriority::Type SortPriority, const FName& Name, EColumnSortMode::Type SortMode)
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

void SProjectCleanerTabNonEngine::OnListItemDblClick(TSharedPtr<FProjectCleanerTabNonEngineListItem> Item) const
{
	if (!Item.IsValid()) return;

	const FString DirPath = FPaths::GetPath(Item->FilePathAbs);
	if (DirPath.IsEmpty() || !FPaths::DirectoryExists(DirPath)) return;

	FPlatformProcess::ExploreFolder(*DirPath);
}

TSharedPtr<SHeaderRow> SProjectCleanerTabNonEngine::GetListHeaderRow()
{
	return
		SNew(SHeaderRow)
		+ SHeaderRow::Column(FName("FilePath"))
		  .HAlignCell(HAlign_Left)
		  .VAlignCell(VAlign_Center)
		  .HAlignHeader(HAlign_Center)
		  .HeaderContentPadding(FMargin(10.0f))
		  .FillWidth(1.0f)
		  .OnSort_Raw(this, &SProjectCleanerTabNonEngine::OnListSort)
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
		  .OnSort_Raw(this, &SProjectCleanerTabNonEngine::OnListSort)
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
		  .OnSort_Raw(this, &SProjectCleanerTabNonEngine::OnListSort)
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
		  .OnSort_Raw(this, &SProjectCleanerTabNonEngine::OnListSort)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Size")))
			.ColorAndOpacity(FProjectCleanerStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
			.Font(FProjectCleanerStyles::GetFont("Light", ProjectCleanerConstants::HeaderRowFontSize))
		];
}

TSharedPtr<SWidget> SProjectCleanerTabNonEngine::OnListContextMenu() const
{
	FMenuBuilder MenuBuilder{true, Cmds};
	MenuBuilder.BeginSection(TEXT("Actions"), FText::FromString(TEXT("File Actions")));
	{
		MenuBuilder.AddMenuEntry(FProjectCleanerCmds::Get().TabNonEngineTryOpenFile);
		MenuBuilder.AddMenuEntry(FProjectCleanerCmds::Get().TabNonEngineDeleteFile);
	}
	MenuBuilder.EndSection();

	return MenuBuilder.MakeWidget();
}

TSharedRef<ITableRow> SProjectCleanerTabNonEngine::OnListGenerateRow(TSharedPtr<FProjectCleanerTabNonEngineListItem> InItem, const TSharedRef<STableViewBase>& OwnerTable) const
{
	return SNew(SProjectCleanerTabNonEngineListItem, OwnerTable).ListItem(InItem);
}

FText SProjectCleanerTabNonEngine::GetListTextSummary() const
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
