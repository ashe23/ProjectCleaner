// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/FileBrowser/SPjcFileBrowser.h"
#include "Slate/FileBrowser/SPjcFileBrowserItem.h"
#include "PjcStyles.h"
#include "PjcConstants.h"
#include "PjcSubsystem.h"
#include "Libs/PjcLibPath.h"
#include "Libs/PjcLibEditor.h"
// Engine Headers
#include "Misc/ScopedSlowTask.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SWidgetSwitcher.h"

void SPjcFileBrowser::Construct(const FArguments& InArgs)
{
	if (!GEditor) return;

	SubsystemPtr = GEditor->GetEditorSubsystem<UPjcSubsystem>();
	if (!SubsystemPtr) return;

	SubsystemPtr->OnScanFiles().AddRaw(this, &SPjcFileBrowser::OnScanFiles);

	SAssignNew(ListView, SListView<TSharedPtr<FPjcFileBrowserItem>>)
	.ListItemsSource(&ListItems)
	.OnGenerateRow(this, &SPjcFileBrowser::OnGenerateRow)
	.OnMouseButtonDoubleClick_Raw(this, &SPjcFileBrowser::OnListItemDblClick)
	.SelectionMode(ESelectionMode::Multi)
	.HeaderRow(GetHeaderRow());

	ListItemsUpdate();

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(5.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SButton)
				.ContentPadding(FMargin{3.0f})
				.OnClicked_Raw(this, &SPjcFileBrowser::OnBtnScanFilesClicked)
				.ButtonColorAndOpacity(FPjcStyles::Get().GetColor("ProjectCleaner.Color.Blue"))
				[
					SNew(STextBlock)
					.Justification(ETextJustify::Center)
					.ToolTipText(FText::FromString(TEXT("Scan for external and corrupted asset files inside Content folder")))
					.ColorAndOpacity(FPjcStyles::Get().GetColor("ProjectCleaner.Color.White"))
					.ShadowOffset(FVector2D{1.5f, 1.5f})
					.ShadowColorAndOpacity(FLinearColor::Black)
					.Font(FPjcStyles::GetFont("Bold", 10))
					.Text(FText::FromString(TEXT("Scan Files")))
				]
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin{5.0f, 0.0f, 0.0f, 0.0f})
			[
				SNew(SButton)
				.ContentPadding(FMargin{3.0f})
				.OnClicked_Raw(this, &SPjcFileBrowser::OnBtnDeleteFilesClicked)
				.IsEnabled_Raw(this, &SPjcFileBrowser::IsAnyItemSelected)
				.ButtonColorAndOpacity(FPjcStyles::Get().GetColor("ProjectCleaner.Color.Red"))
				[
					SNew(STextBlock)
					.Justification(ETextJustify::Center)
					.ToolTipText(FText::FromString(TEXT("Delete Selected Files")))
					.ColorAndOpacity(FPjcStyles::Get().GetColor("ProjectCleaner.Color.White"))
					.ShadowOffset(FVector2D{1.5f, 1.5f})
					.ShadowColorAndOpacity(FLinearColor::Black)
					.Font(FPjcStyles::GetFont("Bold", 10))
					.Text(FText::FromString(TEXT("Delete Selected Files")))
				]
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin{5.0f, 0.0f, 0.0f, 0.0f})
			[
				SNew(SButton)
				.ContentPadding(FMargin{3.0f})
				.OnClicked_Raw(this, &SPjcFileBrowser::OnBtnClearSelectionClicked)
				.IsEnabled_Raw(this, &SPjcFileBrowser::IsAnyItemSelected)
				.ButtonColorAndOpacity(FPjcStyles::Get().GetColor("ProjectCleaner.Color.Gray"))
				[
					SNew(STextBlock)
					.Justification(ETextJustify::Center)
					.ToolTipText(FText::FromString(TEXT("Clear view selection")))
					.ColorAndOpacity(FPjcStyles::Get().GetColor("ProjectCleaner.Color.White"))
					.ShadowOffset(FVector2D{1.5f, 1.5f})
					.ShadowColorAndOpacity(FLinearColor::Black)
					.Font(FPjcStyles::GetFont("Bold", 10))
					.Text(FText::FromString(TEXT("Clear Selection")))
				]
			]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(5.0f)
		[
			SNew(SSearchBox)
			.HintText(FText::FromString(TEXT("Search...")))
			.OnTextChanged_Raw(this, &SPjcFileBrowser::OnSearchTextChanged)
			.OnTextCommitted_Raw(this, &SPjcFileBrowser::OnSearchTextCommitted)
		]
		+ SVerticalBox::Slot().FillHeight(1.0f).Padding(5.0f)
		[
			SNew(SWidgetSwitcher)
			.WidgetIndex_Raw(this, &SPjcFileBrowser::GetWidgetIndex)
			+ SWidgetSwitcher::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Justification(ETextJustify::Center)
				.ColorAndOpacity(FPjcStyles::Get().GetColor("ProjectCleaner.Color.Gray"))
				.ShadowOffset(FVector2D{1.5f, 1.5f})
				.ShadowColorAndOpacity(FLinearColor::Black)
				.Font(FPjcStyles::GetFont("Bold", 20))
				.Text(FText::FromString(TEXT("No files to display")))
			]
			+ SWidgetSwitcher::Slot()
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
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(5.0f)
		[
			SNew(STextBlock)
			.Text_Raw(this, &SPjcFileBrowser::GetSummary)
		]
	];
}

SPjcFileBrowser::~SPjcFileBrowser()
{
	if (!SubsystemPtr) return;

	SubsystemPtr->OnScanFiles().RemoveAll(this);
}

TSharedPtr<FPjcFileBrowserItem> SPjcFileBrowser::CreateListItem(const FString& InFilePath) const
{
	const FString FileName = FPjcLibPath::GetFileName(InFilePath);
	const FString FileExtension = FPjcLibPath::GetFileExtension(InFilePath, false).ToLower();
	const int64 FileSize = FPjcLibPath::GetFileSize(InFilePath);
	const EPjcBrowserItemFileType FileType = PjcConstants::EngineFileExtensions.Contains(FileExtension) ? EPjcBrowserItemFileType::Corrupted : EPjcBrowserItemFileType::External;

	return MakeShareable(new FPjcFileBrowserItem{FileSize, FileType, FileName, InFilePath, FileExtension});
}

TSharedRef<SHeaderRow> SPjcFileBrowser::GetHeaderRow()
{
	return
		SNew(SHeaderRow)
		+ SHeaderRow::Column("FileName")
		  .FillWidth(0.4f)
		  .HAlignCell(HAlign_Left)
		  .VAlignCell(VAlign_Center)
		  .HAlignHeader(HAlign_Center)
		  .OnSort_Raw(this, &SPjcFileBrowser::OnListSort)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("File Name")))
			.ColorAndOpacity(FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
			.Font(FPjcStyles::GetFont("Light", 10.0f))
		]
		+ SHeaderRow::Column("FileExt")
		  .FillWidth(0.15f)
		  .HAlignCell(HAlign_Center)
		  .VAlignCell(VAlign_Center)
		  .HAlignHeader(HAlign_Center)
		  .OnSort_Raw(this, &SPjcFileBrowser::OnListSort)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("File Extension")))
			.ColorAndOpacity(FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
			.Font(FPjcStyles::GetFont("Light", 10.0f))
		]
		+ SHeaderRow::Column("FileType")
		  .FillWidth(0.15f)
		  .HAlignCell(HAlign_Center)
		  .VAlignCell(VAlign_Center)
		  .HAlignHeader(HAlign_Center)
		  .OnSort_Raw(this, &SPjcFileBrowser::OnListSort)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("File Type")))
			.ColorAndOpacity(FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
			.Font(FPjcStyles::GetFont("Light", 10.0f))
		]
		+ SHeaderRow::Column("FileSize")
		  .FillWidth(0.15f)
		  .HAlignCell(HAlign_Center)
		  .VAlignCell(VAlign_Center)
		  .HAlignHeader(HAlign_Center)
		  .OnSort_Raw(this, &SPjcFileBrowser::OnListSort)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("File Size")))
			.ColorAndOpacity(FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
			.Font(FPjcStyles::GetFont("Light", 10.0f))
		]
		+ SHeaderRow::Column("FilePath")
		  .HAlignCell(HAlign_Left)
		  .VAlignCell(VAlign_Center)
		  .HAlignHeader(HAlign_Center)
		  .OnSort_Raw(this, &SPjcFileBrowser::OnListSort)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("File Path")))
			.ColorAndOpacity(FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
			.Font(FPjcStyles::GetFont("Light", 10.0f))
		];
}

TSharedRef<ITableRow> SPjcFileBrowser::OnGenerateRow(TSharedPtr<FPjcFileBrowserItem> Item, const TSharedRef<STableViewBase>& OwnerTable) const
{
	return SNew(SPjcFileBrowserItem, OwnerTable).Item(Item).HighlightText(SearchText);
}

FReply SPjcFileBrowser::OnBtnScanFilesClicked() const
{
	if (SubsystemPtr)
	{
		SubsystemPtr->ScanProjectFilesAndFolders();
	}

	return FReply::Handled();
}

FReply SPjcFileBrowser::OnBtnDeleteFilesClicked()
{
	if (!ListView.IsValid()) return FReply::Handled();

	const FText Title = FText::FromString(TEXT("Delete Files"));
	const FText Msg = FText::FromString(TEXT("Are you sure you want to delete selected files?"));
	const EAppReturnType::Type Result = FMessageDialog::Open(EAppMsgType::YesNo, Msg, &Title);

	if (Result == EAppReturnType::No || Result == EAppReturnType::Cancel)
	{
		return FReply::Handled();
	}

	const auto SelectedItems = ListView.Get()->GetSelectedItems();

	FScopedSlowTask SlowTask{
		static_cast<float>(SelectedItems.Num()),
		FText::FromString(TEXT("Deleting Files...")),
		GIsEditor && !IsRunningCommandlet()
	};
	SlowTask.MakeDialog(false, false);

	const int32 FilesTotal = SelectedItems.Num();
	int32 FilesDeleted = 0;

	for (const auto& Item : SelectedItems)
	{
		if (!Item.IsValid()) continue;

		SlowTask.EnterProgressFrame(1.0f, FText::FromString(Item->FilePath));

		if (!DeleteFile(Item->FilePath)) continue;

		++FilesDeleted;
	}

	SubsystemPtr->ScanProjectFilesAndFolders();

	const FString DeleteStatMsg = FString::Printf(TEXT("Deleted %d of %d files."), FilesDeleted, FilesTotal);

	if (FilesDeleted == FilesTotal)
	{
		FPjcLibEditor::ShowNotification(DeleteStatMsg, SNotificationItem::CS_Success, 5.0f);
	}
	else
	{
		FPjcLibEditor::ShowNotificationWithOutputLog(DeleteStatMsg, SNotificationItem::CS_Fail, 5.0f);
	}

	return FReply::Handled();
}

FReply SPjcFileBrowser::OnBtnClearSelectionClicked() const
{
	ListClearSelection();

	return FReply::Handled();
}

void SPjcFileBrowser::OnScanFiles()
{
	ListItemsUpdate();
}

void SPjcFileBrowser::OnSearchTextChanged(const FText& InSearchText)
{
	SearchText = InSearchText.ToString();

	ListViewUpdate();
}

void SPjcFileBrowser::OnSearchTextCommitted(const FText& InSearchText, ETextCommit::Type Type)
{
	SearchText = InSearchText.ToString();

	ListViewUpdate();
}

void SPjcFileBrowser::OnListItemDblClick(TSharedPtr<FPjcFileBrowserItem> Item) const
{
	if (!Item.IsValid()) return;

	if (Item->FileType == EPjcBrowserItemFileType::Corrupted)
	{
		FPjcLibEditor::ShowNotification(TEXT("Cant open corrupted asset file"), SNotificationItem::CS_Fail, 3.0f);
		return;
	}

	FPjcLibEditor::OpenFileInFileExplorer(Item->FilePath);
}

void SPjcFileBrowser::OnListSort(EColumnSortPriority::Type SortPriority, const FName& ColumnName, EColumnSortMode::Type SortMode)
{
	if (ColumnName.IsEqual(TEXT("FileName")))
	{
		ColumnFileNameSortMode = ColumnFileNameSortMode == EColumnSortMode::Ascending ? EColumnSortMode::Descending : EColumnSortMode::Ascending;

		ListItems.Sort([&](const TSharedPtr<FPjcFileBrowserItem>& Item1, const TSharedPtr<FPjcFileBrowserItem>& Item2)
		{
			return ColumnFileNameSortMode == EColumnSortMode::Ascending
				       ? Item1->FileName < Item2->FileName
				       : Item1->FileName > Item2->FileName;
		});
	}

	if (ColumnName.IsEqual(TEXT("FileExt")))
	{
		ColumnFileExtSortMode = ColumnFileExtSortMode == EColumnSortMode::Ascending ? EColumnSortMode::Descending : EColumnSortMode::Ascending;

		ListItems.Sort([&](const TSharedPtr<FPjcFileBrowserItem>& Item1, const TSharedPtr<FPjcFileBrowserItem>& Item2)
		{
			return ColumnFileExtSortMode == EColumnSortMode::Ascending
				       ? Item1->FileExtension < Item2->FileExtension
				       : Item1->FileExtension > Item2->FileExtension;
		});
	}

	if (ColumnName.IsEqual(TEXT("FileSize")))
	{
		ColumnFileSizeSortMode = ColumnFileSizeSortMode == EColumnSortMode::Ascending ? EColumnSortMode::Descending : EColumnSortMode::Ascending;

		ListItems.Sort([&](const TSharedPtr<FPjcFileBrowserItem>& Item1, const TSharedPtr<FPjcFileBrowserItem>& Item2)
		{
			return ColumnFileSizeSortMode == EColumnSortMode::Ascending
				       ? Item1->FileSize < Item2->FileSize
				       : Item1->FileSize > Item2->FileSize;
		});
	}

	if (ColumnName.IsEqual(TEXT("FileType")))
	{
		ColumnFileTypeSortMode = ColumnFileTypeSortMode == EColumnSortMode::Ascending ? EColumnSortMode::Descending : EColumnSortMode::Ascending;

		ListItems.Sort([&](const TSharedPtr<FPjcFileBrowserItem>& Item1, const TSharedPtr<FPjcFileBrowserItem>& Item2)
		{
			return ColumnFileTypeSortMode == EColumnSortMode::Ascending
				       ? Item1->FileType < Item2->FileType
				       : Item1->FileType > Item2->FileType;
		});
	}

	if (ColumnName.IsEqual(TEXT("FilePath")))
	{
		ColumnFilePathSortMode = ColumnFilePathSortMode == EColumnSortMode::Ascending ? EColumnSortMode::Descending : EColumnSortMode::Ascending;

		ListItems.Sort([&](const TSharedPtr<FPjcFileBrowserItem>& Item1, const TSharedPtr<FPjcFileBrowserItem>& Item2)
		{
			return ColumnFilePathSortMode == EColumnSortMode::Ascending
				       ? Item1->FilePath < Item2->FilePath
				       : Item1->FilePath > Item2->FilePath;
		});
	}

	if (ListView.IsValid())
	{
		ListView->RequestListRefresh();
	}
}

void SPjcFileBrowser::ListClearSelection() const
{
	if (!ListView.IsValid()) return;

	ListView->ClearSelection();
	ListView->ClearHighlightedItems();
}

void SPjcFileBrowser::ListItemsUpdate()
{
	if (!SubsystemPtr) return;

	const int32 NumFilesTotal = SubsystemPtr->GetFilesExternal().Num() + SubsystemPtr->GetFilesCorrupted().Num();

	ListItems.Reset(NumFilesTotal);
	ListItemsCached.Reset(NumFilesTotal);

	TotalSize = 0;

	for (const FString& File : SubsystemPtr->GetFilesExternal())
	{
		TotalSize += FPjcLibPath::GetFileSize(File);
		ListItems.Emplace(CreateListItem(File));
		ListItemsCached.Emplace(CreateListItem(File));
	}

	for (const FString& File : SubsystemPtr->GetFilesCorrupted())
	{
		TotalSize += FPjcLibPath::GetFileSize(File);
		ListItems.Emplace(CreateListItem(File));
		ListItemsCached.Emplace(CreateListItem(File));
	}

	ListItems.Shrink();
	ListItemsCached.Shrink();

	ListViewUpdate();
}

void SPjcFileBrowser::ListViewUpdate()
{
	if (!ListView.IsValid()) return;

	ListClearSelection();

	ListItems.Reset();
	TotalSize = 0;

	for (const auto& Item : ListItemsCached)
	{
		if (!Item.IsValid()) continue;

		if (SearchText.IsEmpty() || Item->FileName.Contains(SearchText) || Item->FileExtension.Contains(SearchText))
		{
			ListItems.Emplace(Item);

			TotalSize += Item->FileSize;
		}
	}

	ListView->RebuildList();
}

bool SPjcFileBrowser::IsAnyItemSelected() const
{
	return ListView.IsValid() && ListView.Get()->GetSelectedItems().Num() > 0;
}

bool SPjcFileBrowser::DeleteFile(const FString& InFilePath)
{
	if (!FPjcLibPath::IsValid(InFilePath)) return false;

	if (!IFileManager::Get().Delete(*InFilePath, true))
	{
		return false;
	}

	ListItemsCached.RemoveAllSwap([InFilePath](const TSharedPtr<FPjcFileBrowserItem>& Item)
	{
		return Item->FilePath.Equals(InFilePath);
	}, false);

	return true;
}

int32 SPjcFileBrowser::GetWidgetIndex() const
{
	return ListItems.Num() == 0 ? PjcConstants::WidgetIndexIdle : PjcConstants::WidgetIndexWorking;
}

FText SPjcFileBrowser::GetSummary() const
{
	if (ListView.IsValid() && ListView->GetSelectedItems().Num() > 0)
	{
		return FText::FromString(
			FString::Printf(
				TEXT("%d file%s (%d selected). Total Size: %s"),
				ListItems.Num(),
				ListItems.Num() == 1 ? TEXT("") : TEXT("s"),
				ListView->GetSelectedItems().Num(),
				*FText::AsMemory(TotalSize, IEC).ToString()
			)
		);
	}

	return FText::FromString(
		FString::Printf(
			TEXT("%d file%s. Total Size: %s"),
			ListItems.Num(),
			ListItems.Num() == 1 ? TEXT("") : TEXT("s"),
			*FText::AsMemory(TotalSize, IEC).ToString()
		)
	);
}
