// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/SPjcFileBrowser.h"
#include "PjcConstants.h"
#include "PjcStyles.h"
#include "Libs/PjcLibPath.h"
#include "Libs/PjcLibAsset.h"
#include "Libs/PjcLibEditor.h"
// Engine Headers
#include "Pjc.h"
#include "Misc/ScopedSlowTask.h"
#include "Widgets/Input/SHyperlink.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Layout/SScrollBox.h"
// #include "Widgets/Layout/SSeparator.h"

void SPjcFileBrowserItem::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InTable)
{
	Item = InArgs._Item;
	SearchText = InArgs._SearchText;

	SMultiColumnTableRow::Construct(SMultiColumnTableRow::FArguments().Padding(FMargin{0.0f, 2.0f, 0.0f, 0.0f}), InTable);
}

TSharedRef<SWidget> SPjcFileBrowserItem::GenerateWidgetForColumn(const FName& InColumnName)
{
	if (InColumnName.IsEqual(TEXT("FileName")))
	{
		return
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin{5.0f, 0.0f})
			[
				SNew(STextBlock).Text(FText::FromString(Item->FileName)).HighlightText(FText::FromString(SearchText))
			];
	}

	if (InColumnName.IsEqual(TEXT("FileExt")))
	{
		return SNew(STextBlock).Text(FText::FromString(Item->FileExtension)).HighlightText(FText::FromString(SearchText));
	}

	if (InColumnName.IsEqual(TEXT("FileSize")))
	{
		return SNew(STextBlock).Text(FText::AsMemory(Item->FileSize, IEC));
	}

	if (InColumnName.IsEqual(TEXT("FileType")))
	{
		const FString ColorSpecifier = Item->FileType == EPjcFileType::External ? TEXT("ProjectCleaner.Color.Blue") : TEXT("ProjectCleaner.Color.Red");
		return
			SNew(SBorder)
			.BorderImage(FPjcStyles::Get().GetBrush(TEXT("ProjectCleaner.BgWhite")))
			.BorderBackgroundColor(FPjcStyles::Get().GetSlateColor(*ColorSpecifier))
			.ColorAndOpacity(FLinearColor::White)
			[
				SNew(STextBlock)
				.Font(FPjcStyles::GetFont("Bold", 9))
				.Text(FText::FromString(Item->FileType == EPjcFileType::External ? TEXT("External") : TEXT("Corrupted Asset File")))
			];
	}

	if (InColumnName.IsEqual(TEXT("FilePath")))
	{
		return
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin{5.0f, 0.0f})
			[
				SNew(SHyperlink).Text(FText::FromString(Item->FilePathAbs)).OnNavigate_Lambda([&]()
				{
					FPjcLibEditor::NavigateToPathInFileExplorer(Item->FilePathAbs);
				})
			];
	}

	return SNew(STextBlock).Text(FText::FromString(""));
}

void SPjcFileBrowser::Construct(const FArguments& InArgs)
{
	// FPropertyEditorModule& PropertyEditor = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	// FDetailsViewArgs DetailsViewArgs;
	// DetailsViewArgs.bUpdatesFromSelection = false;
	// DetailsViewArgs.bLockable = false;
	// DetailsViewArgs.bAllowSearch = false;
	// DetailsViewArgs.bShowOptions = true;
	// DetailsViewArgs.bAllowFavoriteSystem = false;
	// DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	// DetailsViewArgs.ViewIdentifier = "PjcFileExcludeSettings";
	//
	// const auto SettingsProperty = PropertyEditor.CreateDetailView(DetailsViewArgs);
	// SettingsProperty->SetObject(GetMutableDefault<UPjcFileExcludeSettings>());

	SAssignNew(ListView, SListView<TSharedPtr<FPjcFileBrowserItem>>)
	.ListItemsSource(&ListItems)
	.OnGenerateRow(this, &SPjcFileBrowser::OnGenerateRow)
	.OnMouseButtonDoubleClick_Raw(this, &SPjcFileBrowser::OnListItemDblClick)
	.SelectionMode(ESelectionMode::Multi)
	.HeaderRow(GetHeaderRow());

	NumberFormattingOptions.SetUseGrouping(true);
	NumberFormattingOptions.SetMinimumFractionalDigits(0);

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin{5.0f})
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SButton)
				.ContentPadding(FMargin{5.0f})
				.OnClicked_Raw(this, &SPjcFileBrowser::OnBtnScanFilesClick)
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
				.ContentPadding(FMargin{5.0f})
				.OnClicked_Raw(this, &SPjcFileBrowser::OnBtnDeleteFilesClick)
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
					.Text(FText::FromString(TEXT("Delete Files")))
				]
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin{5.0f, 0.0f, 0.0f, 0.0f})
			[
				SNew(SButton)
				.ContentPadding(FMargin{5.0f})
				.OnClicked_Raw(this, &SPjcFileBrowser::OnBtnClearSelectionClick)
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
		+ SVerticalBox::Slot().FillHeight(1.0f).Padding(FMargin{5.0f})
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
		+ SVerticalBox::Slot().Padding(FMargin{5.0f}).AutoHeight()
		[
			SNew(SSearchBox)
			.HintText(FText::FromString(TEXT("Search...")))
			.OnTextChanged(this, &SPjcFileBrowser::OnSearchTextChanged)
			.OnTextCommitted(this, &SPjcFileBrowser::OnSearchTextCommitted)
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin{5.0f})
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.0f).HAlign(HAlign_Left)
			[
				SNew(STextBlock)
				.AutoWrapText(false)
				.Font(FPjcStyles::GetFont("Light", 10))
				.Text_Raw(this, &SPjcFileBrowser::GetListSummaryText)
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f).HAlign(HAlign_Right)
			[
				SNew(SComboButton)
				.ContentPadding(0)
				.ButtonStyle(FEditorStyle::Get(), "ToggleButton")
				.OnGetMenuContent_Raw(this, &SPjcFileBrowser::GetViewOptionsBtnContent)
				.ForegroundColor_Raw(this, &SPjcFileBrowser::GetViewOptionsForegroundColor)
				.ButtonContent()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(SImage).Image(FEditorStyle::GetBrush("GenericViewButton"))
					]
					+ SHorizontalBox::Slot().AutoWidth().Padding(2.0f, 0.0f, 0.0f, 0.0f).VAlign(VAlign_Center)
					[
						SNew(STextBlock).Text(FText::FromString(TEXT("View Options")))
					]
				]
			]
		]
	];
}

void SPjcFileBrowser::ListUpdate()
{
	if (!ListView.IsValid()) return;

	FScopedSlowTask SlowTaskMain{
		1.0f,
		FText::FromString(TEXT("Scanning Files...")),
		GIsEditor && !IsRunningCommandlet()
	};
	SlowTaskMain.MakeDialog(false, false);
	SlowTaskMain.EnterProgressFrame(1.0f);

	TSet<FString> FilesAll;
	FPjcLibPath::GetFilesInPath(FPjcLibPath::ContentDir(), true, FilesAll);

	Files.Reset(FilesAll.Num());
	ListItems.Reset(FilesAll.Num());

	FScopedSlowTask SlowTask{
		static_cast<float>(FilesAll.Num()),
		FText::FromString(TEXT("Processing Files...")),
		GIsEditor && !IsRunningCommandlet()
	};
	SlowTask.MakeDialog(false, false);

	for (const auto& File : FilesAll)
	{
		SlowTask.EnterProgressFrame(1.0f, FText::FromString(File));

		const FString FileName = FPjcLibPath::GetFileName(File);
		const FString FileExtension = FPjcLibPath::GetFileExtension(File, false).ToLower();
		const int64 FileSize = FPjcLibPath::GetFileSize(File);

		if (PjcConstants::EngineFileExtensions.Contains(FileExtension))
		{
			if (!FPjcLibAsset::GetAssetByObjectPath(FPjcLibPath::ToObjectPath(File)).IsValid())
			{
				ListItems.Emplace(
					MakeShareable(
						new FPjcFileBrowserItem{
							FileSize,
							FileName,
							File,
							TEXT(".") + FileExtension,
							EPjcFileType::Corrupted
						}
					)
				);

				Files.Emplace(File);
			}
		}
		else
		{
			ListItems.Emplace(
				MakeShareable(
					new FPjcFileBrowserItem{
						FileSize,
						FileName,
						File,
						TEXT(".") + FileExtension,
						EPjcFileType::External
					}
				)
			);

			Files.Emplace(File);
		}
	}

	TotalSize = FPjcLibPath::GetFilesSize(Files);

	if (!SearchText.IsEmpty())
	{
		ListSearch();

		return;
	}

	ListView->RebuildList();
}

void SPjcFileBrowser::ListSearch()
{
	if (!ListView.IsValid()) return;

	ListItems.Reset(Files.Num());

	for (const auto& File : Files)
	{
		const FString FileName = FPjcLibPath::GetFileName(File);
		const FString FileExtension = FPjcLibPath::GetFileExtension(File, false).ToLower();
		const int64 FileSize = FPjcLibPath::GetFileSize(File);
		const EPjcFileType FileType = PjcConstants::EngineFileExtensions.Contains(FileExtension) ? EPjcFileType::Corrupted : EPjcFileType::External;

		if (SearchText.IsEmpty() || FileName.Contains(SearchText) || FileExtension.Contains(SearchText))
		{
			ListItems.Emplace(
				MakeShareable(
					new FPjcFileBrowserItem{
						FileSize,
						FileName,
						File,
						TEXT(".") + FileExtension,
						FileType
					}
				)
			);
		}
	}

	ListView->RebuildList();
}

void SPjcFileBrowser::OnSearchTextChanged(const FText& InText)
{
	SearchText = InText.ToString();

	ListSearch();
}

void SPjcFileBrowser::OnSearchTextCommitted(const FText& InText, ETextCommit::Type)
{
	SearchText = InText.ToString();

	ListSearch();
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
				       ? Item1->FilePathAbs < Item2->FilePathAbs
				       : Item1->FilePathAbs > Item2->FilePathAbs;
		});
	}

	if (ListView.IsValid())
	{
		ListView->RequestListRefresh();
	}
}

void SPjcFileBrowser::OnListItemDblClick(TSharedPtr<FPjcFileBrowserItem> Item) const
{
	if (!Item.IsValid()) return;

	if (Item->FileType == EPjcFileType::Corrupted)
	{
		FPjcLibEditor::ShowNotification(TEXT("Cant open corrupted asset file"), SNotificationItem::CS_Fail, 3.0f);
		return;
	}

	FPjcLibEditor::OpenFileInFileExplorer(Item->FilePathAbs);
}

FText SPjcFileBrowser::GetListSummaryText() const
{
	if (ListView.IsValid() && ListView->GetSelectedItems().Num() > 0)
	{
		return FText::FromString(
			FString::Printf(
				TEXT("%d file%s (%d selected). Total Size: %s"),
				ListItems.Num(),
				ListItems.Num() > 1 ? TEXT("s") : TEXT(""),
				ListView->GetSelectedItems().Num(),
				*FText::AsMemory(TotalSize, IEC).ToString()
			)
		);
	}

	return FText::FromString(
		FString::Printf(
			TEXT("%d file%s. Total Size: %s"),
			ListItems.Num(),
			ListItems.Num() > 1 ? TEXT("s") : TEXT(""),
			*FText::AsMemory(TotalSize, IEC).ToString()
		)
	);
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
		  .HeaderContentPadding(PjcConstants::HeaderRowMargin)
		  .OnSort_Raw(this, &SPjcFileBrowser::OnListSort)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("File Name")))
			.ColorAndOpacity(FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
			.Font(FPjcStyles::GetFont("Light", PjcConstants::HeaderRowFontSize))
		]
		+ SHeaderRow::Column("FileExt")
		  .FillWidth(0.15f)
		  .HAlignCell(HAlign_Center)
		  .VAlignCell(VAlign_Center)
		  .HAlignHeader(HAlign_Center)
		  .HeaderContentPadding(PjcConstants::HeaderRowMargin)
		  .OnSort_Raw(this, &SPjcFileBrowser::OnListSort)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("File Extension")))
			.ColorAndOpacity(FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
			.Font(FPjcStyles::GetFont("Light", PjcConstants::HeaderRowFontSize))
		]
		+ SHeaderRow::Column("FileType")
		  .FillWidth(0.15f)
		  .HAlignCell(HAlign_Center)
		  .VAlignCell(VAlign_Center)
		  .HAlignHeader(HAlign_Center)
		  .HeaderContentPadding(PjcConstants::HeaderRowMargin)
		  .OnSort_Raw(this, &SPjcFileBrowser::OnListSort)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("File Type")))
			.ColorAndOpacity(FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
			.Font(FPjcStyles::GetFont("Light", PjcConstants::HeaderRowFontSize))
		]
		+ SHeaderRow::Column("FileSize")
		  .FillWidth(0.15f)
		  .HAlignCell(HAlign_Center)
		  .VAlignCell(VAlign_Center)
		  .HAlignHeader(HAlign_Center)
		  .HeaderContentPadding(PjcConstants::HeaderRowMargin)
		  .OnSort_Raw(this, &SPjcFileBrowser::OnListSort)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("File Size")))
			.ColorAndOpacity(FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
			.Font(FPjcStyles::GetFont("Light", PjcConstants::HeaderRowFontSize))
		]
		+ SHeaderRow::Column("FilePath")
		  .HAlignCell(HAlign_Left)
		  .VAlignCell(VAlign_Center)
		  .HAlignHeader(HAlign_Center)
		  .HeaderContentPadding(PjcConstants::HeaderRowMargin)
		  .OnSort_Raw(this, &SPjcFileBrowser::OnListSort)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("File Path")))
			.ColorAndOpacity(FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
			.Font(FPjcStyles::GetFont("Light", PjcConstants::HeaderRowFontSize))
		];
}

TSharedRef<ITableRow> SPjcFileBrowser::OnGenerateRow(TSharedPtr<FPjcFileBrowserItem> Item, const TSharedRef<STableViewBase>& OwnerTable) const
{
	return SNew(SPjcFileBrowserItem, OwnerTable).Item(Item).SearchText(SearchText);
}

FSlateColor SPjcFileBrowser::GetViewOptionsForegroundColor() const
{
	static const FName InvertedForegroundName("InvertedForeground");
	static const FName DefaultForegroundName("DefaultForeground");

	if (!ViewOptionsBtn.IsValid()) return FEditorStyle::GetSlateColor(DefaultForegroundName);

	return ViewOptionsBtn->IsHovered() ? FEditorStyle::GetSlateColor(InvertedForegroundName) : FEditorStyle::GetSlateColor(DefaultForegroundName);
}

TSharedRef<SWidget> SPjcFileBrowser::GetViewOptionsBtnContent() const
{
	const TSharedPtr<FExtender> Extender;
	FMenuBuilder MenuBuilder(true, nullptr, Extender, true);

	MenuBuilder.AddMenuSeparator(TEXT("View"));

	MenuBuilder.AddMenuEntry(
		FText::FromString(TEXT("Show Files External")),
		FText::FromString(TEXT("Show external files in view")),
		FSlateIcon(),
		FUIAction
		(
			FExecuteAction::CreateLambda([&]
			{
				// SubsystemPtr->bShowPathsExcluded = !SubsystemPtr->bShowPathsExcluded;
				// SubsystemPtr->PostEditChange();
				//
				// TreeViewItemsUpdate();
			})
			// FCanExecuteAction::CreateLambda([&]()
			// {
			// 	return SubsystemPtr && SubsystemPtr->bShowPathsUnusedOnly == false;
			// }),
			// FGetActionCheckState::CreateLambda([&]()
			// {
			// 	return SubsystemPtr->bShowPathsExcluded ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
			// })
		),
		NAME_None,
		EUserInterfaceActionType::ToggleButton
	);

	MenuBuilder.AddMenuEntry(
		FText::FromString(TEXT("Show Files Corrupted")),
		FText::FromString(TEXT("Show corrupted asset files in view")),
		FSlateIcon(),
		FUIAction
		(
			FExecuteAction::CreateLambda([&]
			{
				// SubsystemPtr->bShowPathsEngineGenerated = !SubsystemPtr->bShowPathsEngineGenerated;
				// SubsystemPtr->PostEditChange();
				//
				// TreeViewItemsUpdate();
			})
			// FCanExecuteAction::CreateLambda([&]()
			// {
			// 	return SubsystemPtr && SubsystemPtr->bShowPathsUnusedOnly == false;
			// }),
			// FGetActionCheckState::CreateLambda([&]()
			// {
			// 	return SubsystemPtr->bShowPathsEngineGenerated ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
			// })
		),
		NAME_None,
		EUserInterfaceActionType::ToggleButton
	);

	return MenuBuilder.MakeWidget();
}

FReply SPjcFileBrowser::OnBtnScanFilesClick()
{
	ListUpdate();

	return FReply::Handled();
}

FReply SPjcFileBrowser::OnBtnDeleteFilesClick()
{
	if (!ListView.IsValid()) return FReply::Handled();

	const FText Title = FText::FromString(TEXT("Delete Files"));
	const FText Msg = FText::FromString(TEXT("Are you sure you want to delete selected files?"));
	const EAppReturnType::Type Result = FMessageDialog::Open(EAppMsgType::YesNo, Msg, &Title);

	if (Result == EAppReturnType::No || Result == EAppReturnType::Cancel)
	{
		ListView->ClearSelection();
		ListView->ClearHighlightedItems();
		
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

		SlowTask.EnterProgressFrame(1.0f, FText::FromString(Item->FilePathAbs));

		if (!IFileManager::Get().Delete(*Item->FilePathAbs, true))
		{
			UE_LOG(LogProjectCleaner, Error, TEXT("Failed to delete file: %s"), *Item->FilePathAbs);
			continue;
		}

		++FilesDeleted;
	}

	ListUpdate();

	const FString DeleteMsg = FString::Printf(TEXT("Deleted %d of %d files"), FilesDeleted, FilesTotal);
	const auto Status = FilesDeleted == FilesTotal ? SNotificationItem::ECompletionState::CS_Success : SNotificationItem::ECompletionState::CS_Fail;
	FPjcLibEditor::ShowNotification(DeleteMsg, Status, 5.0f);

	return FReply::Handled();
}

FReply SPjcFileBrowser::OnBtnClearSelectionClick() const
{
	if (!ListView.IsValid()) return FReply::Handled();

	ListView->ClearSelection();
	ListView->ClearHighlightedItems();

	return FReply::Handled();
}

bool SPjcFileBrowser::IsAnyItemSelected() const
{
	return ListView.IsValid() && ListView.Get()->GetSelectedItems().Num() > 0;
}
