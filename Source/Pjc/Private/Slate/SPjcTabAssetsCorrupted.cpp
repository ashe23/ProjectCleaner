// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/SPjcTabAssetsCorrupted.h"
#include "Slate/SPjcItemAssetCorrupted.h"
#include "PjcCmds.h"
#include "PjcStyles.h"
#include "PjcConstants.h"
#include "PjcSubsystem.h"
// Engine Headers
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SWidgetSwitcher.h"

void SPjcTabAssetsCorrupted::Construct(const FArguments& InArgs)
{
	Cmds = MakeShareable(new FUICommandList);

	Cmds->MapAction(
		FPjcCmds::Get().Refresh,
		FExecuteAction::CreateRaw(this, &SPjcTabAssetsCorrupted::OnRefresh)
	);

	Cmds->MapAction(
		FPjcCmds::Get().Delete,
		FExecuteAction::CreateRaw(this, &SPjcTabAssetsCorrupted::OnDelete),
		FCanExecuteAction::CreateRaw(this, &SPjcTabAssetsCorrupted::AnyAssetSelected)
	);

	Cmds->MapAction(
		FPjcCmds::Get().ClearSelection,
		FExecuteAction::CreateRaw(this, &SPjcTabAssetsCorrupted::OnClearSelection),
		FCanExecuteAction::CreateRaw(this, &SPjcTabAssetsCorrupted::AnyAssetSelected)
	);

	SAssignNew(ListView, SListView<TSharedPtr<FPjcCorruptedAssetItem>>)
	.ListItemsSource(&ItemsFiltered)
	.OnGenerateRow(this, &SPjcTabAssetsCorrupted::OnListGenerateRow)
	.OnContextMenuOpening_Raw(this, &SPjcTabAssetsCorrupted::OnContextMenuOpening)
	.SelectionMode(ESelectionMode::Multi)
	.HeaderRow(GetListHeaderRow());

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(5.0f)
		[
			CreateToolbar()
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(5.0f)
		[
			SNew(SSeparator).Thickness(5.0f)
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(5.0f)
		[
			SNew(STextBlock)
			.Justification(ETextJustify::Center)
			.ColorAndOpacity(FPjcStyles::Get().GetColor("ProjectCleaner.Color.Gray"))
			.ShadowOffset(FVector2D{0.5f, 0.5f})
			.ShadowColorAndOpacity(FLinearColor::Black)
			.Font(FPjcStyles::GetFont("Bold", 15))
			.Text(FText::FromString(TEXT("List of corrupted asset files inside Content folder.")))
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(5.0f, 0.0f)
		[
			SNew(STextBlock)
			.Justification(ETextJustify::Center)
			.ColorAndOpacity(FPjcStyles::Get().GetColor("ProjectCleaner.Color.Gray"))
			.ShadowOffset(FVector2D{0.5f, 0.5f})
			.ShadowColorAndOpacity(FLinearColor::Black)
			.Font(FPjcStyles::GetFont("Bold", 10))
			.Text(FText::FromString(TEXT("These are the assets that are not being loaded by the AssetRegistry. To identify these problematic assets, you can view the OutputLog, which should provide information regarding the reasons why these assets aren't loading.")))
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(5.0f, 0.0f)
		[
			SNew(STextBlock)
			.Justification(ETextJustify::Center)
			.ColorAndOpacity(FPjcStyles::Get().GetColor("ProjectCleaner.Color.Gray"))
			.ShadowOffset(FVector2D{0.5f, 0.5f})
			.ShadowColorAndOpacity(FLinearColor::Black)
			.Font(FPjcStyles::GetFont("Bold", 10))
			.Text(FText::FromString(TEXT("Often, these issues arise when an asset has been migrated from a different project that uses a different engine version, or if there were problems during the asset saving process, among other reasons.")))
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(5.0f, 0.0f)
		[
			SNew(STextBlock)
			.Justification(ETextJustify::Center)
			.ColorAndOpacity(FPjcStyles::Get().GetColor("ProjectCleaner.Color.Gray"))
			.ShadowOffset(FVector2D{0.5f, 0.5f})
			.ShadowColorAndOpacity(FLinearColor::Black)
			.Font(FPjcStyles::GetFont("Bold", 10))
			.Text(FText::FromString(TEXT("You can attempt to manually load these assets again, by restarting editor, to see if the problem persists. If the issue remains unresolved, you may consider deleting the problematic asset file directly from the file explorer.")))
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(5.0f)
		[
			SNew(SSeparator).Thickness(5.0f)
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(5.0f)
		[
			SNew(SSearchBox)
			.HintText(FText::FromString(TEXT("Search files...")))
			.OnTextChanged_Raw(this, &SPjcTabAssetsCorrupted::OnSearchTextChanged)
			.OnTextCommitted_Raw(this, &SPjcTabAssetsCorrupted::OnSearchTextCommitted)
		]
		+ SVerticalBox::Slot().FillHeight(1.0f).Padding(5.0f)
		[
			SNew(SWidgetSwitcher)
			.WidgetIndex_Raw(this, &SPjcTabAssetsCorrupted::GetWidgetIndex)
			+ SWidgetSwitcher::Slot()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.0f).HAlign(HAlign_Center).VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Justification(ETextJustify::Center)
					.ColorAndOpacity(FPjcStyles::Get().GetColor("ProjectCleaner.Color.Gray"))
					.ShadowOffset(FVector2D{0.5f, 0.5f})
					.ShadowColorAndOpacity(FLinearColor::Black)
					.Font(FPjcStyles::GetFont("Bold", 15))
					.Text(FText::FromString(TEXT("No corrupted asset files were found.")))
				]
			]
			+ SWidgetSwitcher::Slot()
			[
				SNew(SScrollBox)
				.ScrollWhenFocusChanges(EScrollWhenFocusChanges::NoScroll)
				.AnimateWheelScrolling(true)
				.AllowOverscroll(EAllowOverscroll::No)
				+ SScrollBox::Slot().Padding(5.0f)
				[
					ListView.ToSharedRef()
				]
			]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(5.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.0f).HAlign(HAlign_Left).VAlign(VAlign_Center).Padding(3.0f, 0.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock).Text_Raw(this, &SPjcTabAssetsCorrupted::GetTxtSummary)
			]
		]
	];
}

void SPjcTabAssetsCorrupted::ListUpdateData()
{
	TArray<FString> FilesCorrupted;
	UPjcSubsystem::GetFilesCorrupted(FilesCorrupted, true);

	ItemsAll.Reset(FilesCorrupted.Num());

	NumFilesTotal = FilesCorrupted.Num();
	SizeFilesTotal = 0;

	for (const auto& File : FilesCorrupted)
	{
		const int64 FileSize = IFileManager::Get().FileSize(*File);
		const FString FileName = FPaths::GetBaseFilename(File);
		const FString FileExt = FPaths::GetExtension(File, false).ToLower();
		SizeFilesTotal += FileSize;

		ItemsAll.Emplace(
			MakeShareable(
				new FPjcCorruptedAssetItem{
					FileSize,
					FileName,
					FileExt,
					File
				}
			)
		);
	}
}

void SPjcTabAssetsCorrupted::ListUpdateView()
{
	if (!ListView.IsValid()) return;

	ItemsFiltered.Reset();
	ItemsFiltered.Reserve(ItemsAll.Num());

	const FString SearchString = SearchText.ToString();

	for (const auto& Item : ItemsAll)
	{
		if (
			!Item.IsValid() ||
			(
				!SearchText.IsEmpty() &&
				!Item->FilePath.Contains(SearchString) &&
				!Item->FileName.Contains(SearchString)
			)
		)
		{
			continue;
		}

		ItemsFiltered.Emplace(
			MakeShareable(
				new FPjcCorruptedAssetItem{
					Item->FileSize,
					Item->FileName,
					Item->FileExt,
					Item->FilePath
				}
			)
		);
	}

	ListView->ClearSelection();
	ListView->ClearHighlightedItems();
	ListView->RebuildList();
}

void SPjcTabAssetsCorrupted::OnListSort(EColumnSortPriority::Type SortPriority, const FName& ColumnName, EColumnSortMode::Type InSortMode)
{
	if (!ListView.IsValid()) return;

	auto SortListItems = [&](auto& SortMode, auto SortFunc)
	{
		SortMode = SortMode == EColumnSortMode::Ascending ? EColumnSortMode::Descending : EColumnSortMode::Ascending;

		ItemsFiltered.Sort(SortFunc);
	};

	if (ColumnName.IsEqual(TEXT("FilePath")))
	{
		SortListItems(ColumnSortModeFilePath, [&](const TSharedPtr<FPjcCorruptedAssetItem>& Item1, const TSharedPtr<FPjcCorruptedAssetItem>& Item2)
		{
			return ColumnSortModeFilePath == EColumnSortMode::Ascending ? Item1->FilePath < Item2->FilePath : Item1->FilePath > Item2->FilePath;
		});
	}

	if (ColumnName.IsEqual(TEXT("FileName")))
	{
		SortListItems(ColumnSortModeFileName, [&](const TSharedPtr<FPjcCorruptedAssetItem>& Item1, const TSharedPtr<FPjcCorruptedAssetItem>& Item2)
		{
			return ColumnSortModeFileName == EColumnSortMode::Ascending ? Item1->FileName < Item2->FileName : Item1->FileName > Item2->FileName;
		});
	}

	if (ColumnName.IsEqual(TEXT("FileExt")))
	{
		SortListItems(ColumnSortModeFileExt, [&](const TSharedPtr<FPjcCorruptedAssetItem>& Item1, const TSharedPtr<FPjcCorruptedAssetItem>& Item2)
		{
			return ColumnSortModeFileExt == EColumnSortMode::Ascending ? Item1->FileExt < Item2->FileExt : Item1->FileExt > Item2->FileExt;
		});
	}

	if (ColumnName.IsEqual(TEXT("FileSize")))
	{
		SortListItems(ColumnSortModeFileSize, [&](const TSharedPtr<FPjcCorruptedAssetItem>& Item1, const TSharedPtr<FPjcCorruptedAssetItem>& Item2)
		{
			return ColumnSortModeFileSize == EColumnSortMode::Ascending ? Item1->FileSize < Item2->FileSize : Item1->FileSize > Item2->FileSize;
		});
	}

	ListView->RebuildList();
}

void SPjcTabAssetsCorrupted::OnSearchTextChanged(const FText& InSearchText)
{
	SearchText = InSearchText;
	ListUpdateView();
}

void SPjcTabAssetsCorrupted::OnSearchTextCommitted(const FText& InSearchText, ETextCommit::Type)
{
	SearchText = InSearchText;
	ListUpdateView();
}

TSharedRef<SHeaderRow> SPjcTabAssetsCorrupted::GetListHeaderRow()
{
	const FMargin HeaderMargin{5.0f};

	return
		SNew(SHeaderRow)
		+ SHeaderRow::Column("FilePath")
		.FillWidth(0.6f)
		.HAlignCell(HAlign_Left)
		.VAlignCell(VAlign_Center)
		.HAlignHeader(HAlign_Center)
		.HeaderContentPadding(HeaderMargin)
		.OnSort_Raw(this, &SPjcTabAssetsCorrupted::OnListSort)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("FilePath")))
			.Font(FPjcStyles::GetFont("Light", 10.0f))
			.ColorAndOpacity(FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
		]
		+ SHeaderRow::Column("FileName")
		.FillWidth(0.2f)
		.HAlignCell(HAlign_Center)
		.VAlignCell(VAlign_Center)
		.HAlignHeader(HAlign_Center)
		.HeaderContentPadding(HeaderMargin)
		.OnSort_Raw(this, &SPjcTabAssetsCorrupted::OnListSort)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("FileName")))
			.Font(FPjcStyles::GetFont("Light", 10.0f))
			.ColorAndOpacity(FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
		]
		+ SHeaderRow::Column("FileExt")
		.FillWidth(0.1f)
		.HAlignCell(HAlign_Center)
		.VAlignCell(VAlign_Center)
		.HAlignHeader(HAlign_Center)
		.HeaderContentPadding(HeaderMargin)
		.OnSort_Raw(this, &SPjcTabAssetsCorrupted::OnListSort)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("FileExtension")))
			.Font(FPjcStyles::GetFont("Light", 10.0f))
			.ColorAndOpacity(FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
		]
		+ SHeaderRow::Column("FileSize")
		.FillWidth(0.1f)
		.HAlignCell(HAlign_Center)
		.VAlignCell(VAlign_Center)
		.HAlignHeader(HAlign_Center)
		.HeaderContentPadding(HeaderMargin)
		.OnSort_Raw(this, &SPjcTabAssetsCorrupted::OnListSort)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("FileSize")))
			.Font(FPjcStyles::GetFont("Light", 10.0f))
			.ColorAndOpacity(FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
		];
}

TSharedRef<ITableRow> SPjcTabAssetsCorrupted::OnListGenerateRow(TSharedPtr<FPjcCorruptedAssetItem> Item, const TSharedRef<STableViewBase>& OwnerTable) const
{
	return SNew(SPjcItemAssetCorrupted, OwnerTable).Item(Item).TextHighlight(SearchText);
}

TSharedPtr<SWidget> SPjcTabAssetsCorrupted::OnContextMenuOpening() const
{
	FMenuBuilder MenuBuilder{true, Cmds};
	MenuBuilder.BeginSection("PjcSectionFilesExternalCtxMenu");
	{
		MenuBuilder.AddMenuEntry(FPjcCmds::Get().Delete, NAME_None, FText::FromString(TEXT("Delete")), FText::FromString(TEXT("Delete Selected Files")));
	}
	MenuBuilder.EndSection();

	return MenuBuilder.MakeWidget();
}

TSharedRef<SWidget> SPjcTabAssetsCorrupted::CreateToolbar() const
{
	FToolBarBuilder ToolBarBuilder{Cmds, FMultiBoxCustomization::None};
	ToolBarBuilder.BeginSection("PjcSectionActionsFilesCorrupted");
	{
		ToolBarBuilder.AddToolBarButton(FPjcCmds::Get().Refresh, NAME_None, FText::FromString(TEXT("Scan")), FText::FromString(TEXT("Scan For Corrupted Assets")));
		ToolBarBuilder.AddToolBarButton(FPjcCmds::Get().Delete, NAME_None, FText::FromString(TEXT("Delete")), FText::FromString(TEXT("Delete Selected Files")));
		ToolBarBuilder.AddSeparator();
		ToolBarBuilder.AddToolBarButton(FPjcCmds::Get().ClearSelection);
	}
	ToolBarBuilder.EndSection();

	return ToolBarBuilder.MakeWidget();
}

FText SPjcTabAssetsCorrupted::GetTxtSummary() const
{
	if (ListView.IsValid())
	{
		const int32 NumFilesSelected = ListView->GetSelectedItems().Num();

		return FText::FromString(FString::Printf(TEXT("Total - %d (%s). Selected %d"), NumFilesTotal, *FText::AsMemory(SizeFilesTotal, IEC).ToString(), NumFilesSelected));
	}

	return FText::FromString(FString::Printf(TEXT("Total - %d (%s)"), NumFilesTotal, *FText::AsMemory(SizeFilesTotal, IEC).ToString()));
}

int32 SPjcTabAssetsCorrupted::GetWidgetIndex() const
{
	return ItemsAll.Num() == 0 ? PjcConstants::WidgetIndexIdle : PjcConstants::WidgetIndexWorking;
}

void SPjcTabAssetsCorrupted::OnRefresh()
{
	ListUpdateData();
	ListUpdateView();
}

void SPjcTabAssetsCorrupted::OnDelete()
{
	const FText Title = FText::FromString(TEXT("Delete Corrupted Asset Files"));
	const FText Context = FText::FromString(TEXT("Are you sure you want to delete selected files?"));

	const EAppReturnType::Type ReturnType = FMessageDialog::Open(EAppMsgCategory::Warning, EAppMsgType::YesNo, Context, Title);
	if (ReturnType == EAppReturnType::Cancel || ReturnType == EAppReturnType::No) return;

	const auto ItemsSelected = ListView->GetSelectedItems();
	const int32 NumTotal = ItemsSelected.Num();
	int32 NumDeleted = 0;

	for (const auto& Item : ItemsSelected)
	{
		if (!Item.IsValid()) continue;
		if (!IFileManager::Get().Delete(*Item->FilePath)) continue;

		++NumDeleted;
	}

	const FString Msg = FString::Printf(TEXT("Deleted %d of %d files"), NumDeleted, NumTotal);

	if (NumDeleted == NumTotal)
	{
		UPjcSubsystem::ShowNotification(Msg, SNotificationItem::CS_Success, 5.0f);
	}
	else
	{
		UPjcSubsystem::ShowNotificationWithOutputLog(Msg, SNotificationItem::CS_Fail, 5.0f);
	}

	ListUpdateData();
	ListUpdateView();
}

void SPjcTabAssetsCorrupted::OnClearSelection() const
{
	ListView->ClearSelection();
	ListView->ClearHighlightedItems();
}

bool SPjcTabAssetsCorrupted::AnyAssetSelected() const
{
	return ListView.IsValid() && ListView->GetSelectedItems().Num() > 0;
}
