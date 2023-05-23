// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/SPjcTabFilesExternal.h"
#include "Slate/SPjcItemFileExternal.h"
#include "PjcCmds.h"
#include "PjcStyles.h"
#include "PjcSubsystem.h"
#include "PjcConstants.h"
// Engine Headers
#include "Misc/ScopedSlowTask.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SWidgetSwitcher.h"

void SPjcTabFilesExternal::Construct(const FArguments& InArgs)
{
	SubsystemPtr = GEditor->GetEditorSubsystem<UPjcSubsystem>();

	Cmds = MakeShareable(new FUICommandList);

	Cmds->MapAction(
		FPjcCmds::Get().Refresh,
		FExecuteAction::CreateLambda([&]()
		{
			ListUpdateData();
			ListUpdateView();
		})
	);
	Cmds->MapAction(
		FPjcCmds::Get().Delete,
		FExecuteAction::CreateLambda([&]()
		{
			UPjcFileExcludeSettings* FileExcludeSettings = GetMutableDefault<UPjcFileExcludeSettings>();
			if (!FileExcludeSettings) return;

			const auto ItemsSelected = ListView->GetSelectedItems();
			const int32 NumTotal = ItemsSelected.Num();
			int32 NumDeleted = 0;
			const FText Title = FText::FromString(TEXT("Delete External Files"));
			const FText Context = FText::FromString(TEXT("Are you sure you want to delete selected files?"));

			const EAppReturnType::Type ReturnType = FMessageDialog::Open(EAppMsgType::YesNo, Context, &Title);
			if (ReturnType == EAppReturnType::Cancel || ReturnType == EAppReturnType::No) return;

			for (const auto& Item : ItemsSelected)
			{
				if (!Item.IsValid() || !FPaths::FileExists(Item->FilePath)) continue;
				if (!IFileManager::Get().Delete(*Item->FilePath, true)) continue;

				if (Item->bExcluded)
				{
					FileExcludeSettings->ExcludedFiles.RemoveAllSwap([&](const FFilePath& InFile)
					{
						FString Path = Item->FilePath;
						Path.RemoveFromStart(FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()));

						return InFile.FilePath.Equals(Path);
					}, false);
				}

				++NumDeleted;
			}

			FileExcludeSettings->PostEditChange();

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
		}),
		FCanExecuteAction::CreateLambda([&]()
		{
			return ListView.IsValid() && ListView->GetSelectedItems().Num() > 0;
		})
	);
	Cmds->MapAction(
		FPjcCmds::Get().Exclude,
		FExecuteAction::CreateLambda([&]()
		{
			const auto ItemsSelected = ListView->GetSelectedItems();

			UPjcFileExcludeSettings* FileExcludeSettings = GetMutableDefault<UPjcFileExcludeSettings>();
			if (!FileExcludeSettings) return;

			const FString ProjectDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());

			for (const auto& ItemSelected : ItemsSelected)
			{
				if (!ItemSelected.IsValid()) continue;

				FString Path = ItemSelected->FilePath;
				Path.RemoveFromStart(ProjectDir);

				const bool bAlreadyInList = FileExcludeSettings->ExcludedFiles.ContainsByPredicate([&Path](const FFilePath& InFilePath)
				{
					return InFilePath.FilePath.Equals(Path);
				});

				if (bAlreadyInList) continue;

				FileExcludeSettings->ExcludedFiles.Emplace(FFilePath{Path});
			}
			FileExcludeSettings->PostEditChange();

			ListUpdateData();
			ListUpdateView();
		}),
		FCanExecuteAction::CreateLambda([&]()
		{
			return ListView.IsValid() && ListView->GetSelectedItems().Num() > 0;
		})
	);
	Cmds->MapAction(
		FPjcCmds::Get().ExcludeByExt,
		FExecuteAction::CreateLambda([&]()
		{
			const auto ItemsSelected = ListView->GetSelectedItems();

			UPjcFileExcludeSettings* FileExcludeSettings = GetMutableDefault<UPjcFileExcludeSettings>();
			if (!FileExcludeSettings) return;

			for (const auto& ItemSelected : ItemsSelected)
			{
				if (!ItemSelected.IsValid()) continue;

				FileExcludeSettings->ExcludedExtensions.AddUnique(ItemSelected->FileExt);
			}

			FileExcludeSettings->PostEditChange();

			ListUpdateData();
			ListUpdateView();
		}),
		FCanExecuteAction::CreateLambda([&]()
		{
			return ListView.IsValid() && ListView->GetSelectedItems().Num() > 0;
		})
	);

	Cmds->MapAction(
		FPjcCmds::Get().ClearSelection,
		FExecuteAction::CreateLambda([&]()
		{
			ListView->ClearSelection();
			ListView->ClearHighlightedItems();
		}),
		FCanExecuteAction::CreateLambda([&]()
		{
			return ListView.IsValid() && ListView->GetSelectedItems().Num() > 0;
		})
	);

	SAssignNew(ListView, SListView<TSharedPtr<FPjcFileExternalItem>>)
	.ListItemsSource(&ItemsFiltered)
	.OnGenerateRow(this, &SPjcTabFilesExternal::OnListGenerateRow)
	.OnContextMenuOpening_Raw(this, &SPjcTabFilesExternal::OnContextMenuOpening)
	.SelectionMode(ESelectionMode::Multi)
	.HeaderRow(GetListHeaderRow());

	FPropertyEditorModule& PropertyEditor = UPjcSubsystem::GetModulePropertyEditor();
	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.bUpdatesFromSelection = false;
	DetailsViewArgs.bLockable = false;
	DetailsViewArgs.bAllowSearch = false;
	DetailsViewArgs.bShowOptions = true;
	DetailsViewArgs.bAllowFavoriteSystem = false;
	DetailsViewArgs.bShowPropertyMatrixButton = false;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	DetailsViewArgs.ViewIdentifier = "PjcEditorFileExcludeSettings";

	const auto SettingsProperty = PropertyEditor.CreateDetailView(DetailsViewArgs);
	SettingsProperty->SetObject(GetMutableDefault<UPjcFileExcludeSettings>());

	const FText DescShort = FText::FromString(TEXT("List of external files inside Content folder."));
	const FText DescLong = FText::FromString(TEXT("These files won't be visible in the ContentBrowser. Therefore, you can choose to include only the files necessary for your project, and clean up by excluding the rest."));

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
		+ SVerticalBox::Slot().FillHeight(1.0f).Padding(5.0f)
		[
			SNew(SSplitter)
			.PhysicalSplitterHandleSize(3.0f)
			.Style(FEditorStyle::Get(), "DetailsView.Splitter")
			+ SSplitter::Slot().Value(0.3f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().FillHeight(1.0f).Padding(3.0f)
				[
					SNew(SScrollBox)
					.ScrollWhenFocusChanges(EScrollWhenFocusChanges::NoScroll)
					.AnimateWheelScrolling(true)
					.AllowOverscroll(EAllowOverscroll::No)
					+ SScrollBox::Slot()
					[
						SettingsProperty
					]
				]
			]
			+ SSplitter::Slot().Value(0.7f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(5.0f)
				[
					SNew(STextBlock)
					.Justification(ETextJustify::Center)
					.ColorAndOpacity(FPjcStyles::Get().GetColor("ProjectCleaner.Color.Gray"))
					.ShadowOffset(FVector2D{0.5f, 0.5f})
					.ShadowColorAndOpacity(FLinearColor::Black)
					.Font(FPjcStyles::GetFont("Bold", 15))
					.Text(DescShort)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(10.0f, 5.0f)
				[
					SNew(STextBlock)
					.Justification(ETextJustify::Center)
					.ColorAndOpacity(FPjcStyles::Get().GetColor("ProjectCleaner.Color.Gray"))
					.ShadowOffset(FVector2D{0.5f, 0.5f})
					.ShadowColorAndOpacity(FLinearColor::Black)
					.Font(FPjcStyles::GetFont("Bold", 10))
					.Text(DescLong)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(10.0f, 5.0f)
				[
					SNew(SSeparator).Thickness(5.0f)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(10.0f, 5.0f)
				[
					SNew(SSearchBox)
					.HintText(FText::FromString(TEXT("Search files...")))
					.OnTextChanged_Raw(this, &SPjcTabFilesExternal::OnSearchTextChanged)
					.OnTextCommitted_Raw(this, &SPjcTabFilesExternal::OnSearchTextCommitted)
				]
				+ SVerticalBox::Slot().FillHeight(1.0f).Padding(5.0f)
				[
					SNew(SWidgetSwitcher)
					.WidgetIndex_Raw(this, &SPjcTabFilesExternal::GetWidgetIndex)
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
							.Text(FText::FromString(TEXT("No external files found")))
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
						SNew(STextBlock).Text_Raw(this, &SPjcTabFilesExternal::GetTxtSummary)
					]
					+ SHorizontalBox::Slot().FillWidth(1.0f).HAlign(HAlign_Center).VAlign(VAlign_Center)
					[
						SNew(STextBlock).Text_Raw(this, &SPjcTabFilesExternal::GetTxtSelection)
					]
					+ SHorizontalBox::Slot().FillWidth(1.0f).HAlign(HAlign_Right).VAlign(VAlign_Center)
					[
						SNew(SComboButton)
						.ContentPadding(0)
						.ForegroundColor_Raw(this, &SPjcTabFilesExternal::GetOptionsBtnForegroundColor)
						.ButtonStyle(FEditorStyle::Get(), "ToggleButton")
						.OnGetMenuContent(this, &SPjcTabFilesExternal::GetBtnOptionsContent)
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
			]
		]
	];
}

void SPjcTabFilesExternal::ListUpdateData()
{
	if (!ListView.IsValid() || !SubsystemPtr) return;

	const UPjcFileExcludeSettings* FileExcludeSettings = GetDefault<UPjcFileExcludeSettings>();
	if (!FileExcludeSettings) return;

	FScopedSlowTask SlowTaskMain{
		2.0f,
		FText::FromString(TEXT("Scanning for external files...")),
		GIsEditor && !IsRunningCommandlet()
	};
	SlowTaskMain.MakeDialog(false, false);
	SlowTaskMain.EnterProgressFrame(1.0f);

	// getting all external files in project
	TSet<FString> FilesExternalAll;
	UPjcSubsystem::GetFilesInPathByExt(FPaths::ProjectContentDir(), true, true, PjcConstants::EngineFileExtensions, FilesExternalAll);

	// validate exclude settings
	TSet<FString> ExcludedFiles;
	TSet<FString> ExcludedExtensions;

	const FString ProjectDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());

	for (const auto& ExcludedFile : FileExcludeSettings->ExcludedFiles)
	{
		if (!ExcludedFile.FilePath.StartsWith(TEXT("Content"))) continue;

		const FString PathAbs = UPjcSubsystem::PathNormalize(ProjectDir / ExcludedFile.FilePath);

		if (!FPaths::FileExists(PathAbs)) continue;

		ExcludedFiles.Emplace(PathAbs);
	}

	for (const auto& ExcludedExt : FileExcludeSettings->ExcludedExtensions)
	{
		if (ExcludedExt.IsEmpty()) continue;

		ExcludedExtensions.Emplace(ExcludedExt.Replace(TEXT("."), TEXT("")).ToLower());
	}

	SlowTaskMain.EnterProgressFrame(1.0f);

	FScopedSlowTask SlowTask{
		static_cast<float>(FilesExternalAll.Num()),
		FText::FromString(TEXT("Scanning for external files...")),
		GIsEditor && !IsRunningCommandlet()
	};
	SlowTask.MakeDialog(false, false);

	ItemsAll.Reset();
	ItemsAll.Reserve(FilesExternalAll.Num());

	NumFilesTotal = FilesExternalAll.Num();
	NumFilesExcluded = 0;
	SizeFilesTotal = 0;
	SizeFilesExcluded = 0;

	for (const auto& File : FilesExternalAll)
	{
		SlowTask.EnterProgressFrame(1.0f, FText::FromString(File));

		const FString FileName = FPaths::GetBaseFilename(File);
		const FString FileExt = FPaths::GetExtension(File, false).ToLower();
		const int64 FileSize = IFileManager::Get().FileSize(*File);
		const bool bExcluded = ExcludedFiles.Contains(File) || ExcludedExtensions.Contains(FileExt);

		SizeFilesTotal += FileSize;

		if (bExcluded)
		{
			++NumFilesExcluded;
			SizeFilesExcluded += FileSize;
		}

		ItemsAll.Emplace(MakeShareable(new FPjcFileExternalItem{FileSize, bExcluded, FileName, FileExt, File}));
	}
}

void SPjcTabFilesExternal::ListUpdateView()
{
	if (!ListView.IsValid() || !SubsystemPtr) return;

	ListView->ClearSelection();
	ListView->ClearHighlightedItems();

	ItemsFiltered.Reset();
	ItemsFiltered.Reserve(ItemsAll.Num());

	const FString SearchString = SearchText.ToString();

	for (const auto& Item : ItemsAll)
	{
		if (!Item.IsValid()) continue;

		if (
			(!SearchText.IsEmpty() && !Item->FilePath.Contains(SearchString) && !Item->FileName.Contains(SearchString)) ||
			(!SubsystemPtr->bShowFilesExternal && Item->bExcluded))
		{
			continue;
		}

		ItemsFiltered.Emplace(
			MakeShareable(
				new FPjcFileExternalItem{
					Item->FileSize,
					Item->bExcluded,
					Item->FileName,
					Item->FileExt,
					Item->FilePath
				}
			)
		);
	}

	ListView->RebuildList();
}

void SPjcTabFilesExternal::OnListSort(EColumnSortPriority::Type SortPriority, const FName& ColumnName, EColumnSortMode::Type InSortMode)
{
	if (!ListView.IsValid()) return;

	auto SortListItems = [&](auto& SortMode, auto SortFunc)
	{
		SortMode = SortMode == EColumnSortMode::Ascending ? EColumnSortMode::Descending : EColumnSortMode::Ascending;

		ItemsFiltered.Sort(SortFunc);
	};

	if (ColumnName.IsEqual(TEXT("FilePath")))
	{
		SortListItems(ColumnSortModeFilePath, [&](const TSharedPtr<FPjcFileExternalItem>& Item1, const TSharedPtr<FPjcFileExternalItem>& Item2)
		{
			return ColumnSortModeFilePath == EColumnSortMode::Ascending ? Item1->FilePath < Item2->FilePath : Item1->FilePath > Item2->FilePath;
		});
	}

	if (ColumnName.IsEqual(TEXT("FileName")))
	{
		SortListItems(ColumnSortModeFileName, [&](const TSharedPtr<FPjcFileExternalItem>& Item1, const TSharedPtr<FPjcFileExternalItem>& Item2)
		{
			return ColumnSortModeFileName == EColumnSortMode::Ascending ? Item1->FileName < Item2->FileName : Item1->FileName > Item2->FileName;
		});
	}

	if (ColumnName.IsEqual(TEXT("FileExt")))
	{
		SortListItems(ColumnSortModeFileExt, [&](const TSharedPtr<FPjcFileExternalItem>& Item1, const TSharedPtr<FPjcFileExternalItem>& Item2)
		{
			return ColumnSortModeFileExt == EColumnSortMode::Ascending ? Item1->FileExt < Item2->FileExt : Item1->FileExt > Item2->FileExt;
		});
	}

	if (ColumnName.IsEqual(TEXT("FileSize")))
	{
		SortListItems(ColumnSortModeFileSize, [&](const TSharedPtr<FPjcFileExternalItem>& Item1, const TSharedPtr<FPjcFileExternalItem>& Item2)
		{
			return ColumnSortModeFileSize == EColumnSortMode::Ascending ? Item1->FileSize < Item2->FileSize : Item1->FileSize > Item2->FileSize;
		});
	}

	ListView->RebuildList();
}

void SPjcTabFilesExternal::OnSearchTextChanged(const FText& InText)
{
	SearchText = InText;
	ListUpdateView();
}

void SPjcTabFilesExternal::OnSearchTextCommitted(const FText& InText, ETextCommit::Type)
{
	SearchText = InText;
	ListUpdateView();
}

TSharedRef<SWidget> SPjcTabFilesExternal::CreateToolbar() const
{
	FToolBarBuilder ToolBarBuilder{Cmds, FMultiBoxCustomization::None};
	ToolBarBuilder.BeginSection("PjcSectionFilesExternalToolbar");
	{
		ToolBarBuilder.AddToolBarButton(FPjcCmds::Get().Refresh, NAME_None, FText::FromString(TEXT("Scan")), FText::FromString(TEXT("Scan For External Files")));
		ToolBarBuilder.AddToolBarButton(FPjcCmds::Get().Delete, NAME_None, FText::FromString(TEXT("Delete")), FText::FromString(TEXT("Delete Selected Files")));
		ToolBarBuilder.AddSeparator();
		ToolBarBuilder.AddToolBarButton(FPjcCmds::Get().Exclude, NAME_None, FText::FromString(TEXT("Exclude")), FText::FromString(TEXT("Exclude Selected Files")));
		ToolBarBuilder.AddToolBarButton(FPjcCmds::Get().ExcludeByExt, NAME_None, FText::FromString(TEXT("Exclude By Ext")), FText::FromString(TEXT("Exclude Selected Files By Their Extension")));
		ToolBarBuilder.AddSeparator();
		ToolBarBuilder.AddToolBarButton(FPjcCmds::Get().ClearSelection);
	}
	ToolBarBuilder.EndSection();

	return ToolBarBuilder.MakeWidget();
}

TSharedPtr<SWidget> SPjcTabFilesExternal::OnContextMenuOpening() const
{
	FMenuBuilder MenuBuilder{true, Cmds};
	MenuBuilder.BeginSection("PjcSectionFilesExternalCtxMenu");
	{
		MenuBuilder.AddMenuEntry(FPjcCmds::Get().Exclude, NAME_None, FText::FromString(TEXT("Exclude")), FText::FromString(TEXT("Exclude Selected Files")));
		MenuBuilder.AddMenuEntry(FPjcCmds::Get().ExcludeByExt, NAME_None, FText::FromString(TEXT("Exclude By Ext")), FText::FromString(TEXT("Exclude Selected Files By Their Extension")));
		MenuBuilder.AddSeparator();
		MenuBuilder.AddMenuEntry(FPjcCmds::Get().Delete, NAME_None, FText::FromString(TEXT("Delete")), FText::FromString(TEXT("Delete Selected Files")));
	}
	MenuBuilder.EndSection();

	return MenuBuilder.MakeWidget();
}

TSharedRef<SWidget> SPjcTabFilesExternal::GetBtnOptionsContent()
{
	const TSharedPtr<FExtender> Extender;
	FMenuBuilder MenuBuilder(true, Cmds, Extender, true);

	MenuBuilder.AddMenuEntry(
		FText::FromString(TEXT("Show Files Excluded")),
		FText::FromString(TEXT("Show Excluded Files In List View")),
		FSlateIcon(),
		FUIAction
		(
			FExecuteAction::CreateLambda([&]
			{
				SubsystemPtr->bShowFilesExternal = !SubsystemPtr->bShowFilesExternal;
				SubsystemPtr->PostEditChange();

				ListUpdateView();
			}),
			FCanExecuteAction::CreateLambda([&]()
			{
				return SubsystemPtr != nullptr;
			}),
			FGetActionCheckState::CreateLambda([&]()
			{
				return SubsystemPtr && SubsystemPtr->bShowFilesExternal ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
			})
		),
		NAME_None,
		EUserInterfaceActionType::ToggleButton
	);

	return MenuBuilder.MakeWidget();
}

TSharedRef<SHeaderRow> SPjcTabFilesExternal::GetListHeaderRow()
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
		  .OnSort_Raw(this, &SPjcTabFilesExternal::OnListSort)
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
		  .OnSort_Raw(this, &SPjcTabFilesExternal::OnListSort)
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
		  .OnSort_Raw(this, &SPjcTabFilesExternal::OnListSort)
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
		  .OnSort_Raw(this, &SPjcTabFilesExternal::OnListSort)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("FileSize")))
			.Font(FPjcStyles::GetFont("Light", 10.0f))
			.ColorAndOpacity(FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
		];
}

TSharedRef<ITableRow> SPjcTabFilesExternal::OnListGenerateRow(TSharedPtr<FPjcFileExternalItem> Item, const TSharedRef<STableViewBase>& OwnerTable) const
{
	return SNew(SPjcItemFileExternal, OwnerTable).Item(Item).TextHighlight(SearchText);
}

FSlateColor SPjcTabFilesExternal::GetOptionsBtnForegroundColor() const
{
	static const FName InvertedForegroundName("InvertedForeground");
	static const FName DefaultForegroundName("DefaultForeground");

	if (!OptionBtn.IsValid()) return FEditorStyle::GetSlateColor(DefaultForegroundName);

	return OptionBtn->IsHovered() ? FEditorStyle::GetSlateColor(InvertedForegroundName) : FEditorStyle::GetSlateColor(DefaultForegroundName);
}

FText SPjcTabFilesExternal::GetTxtSummary() const
{
	if (NumFilesExcluded > 0)
	{
		return FText::FromString(
			FString::Printf(
				TEXT("Total - %d (%s). Excluded - %d (%s)"),
				NumFilesTotal,
				*FText::AsMemory(SizeFilesTotal, IEC).ToString(),
				NumFilesExcluded,
				*FText::AsMemory(SizeFilesExcluded, IEC).ToString()
			)
		);
	}

	return FText::FromString(FString::Printf(TEXT("Total - %d (%s)"), NumFilesTotal, *FText::AsMemory(SizeFilesTotal, IEC).ToString()));
}

FText SPjcTabFilesExternal::GetTxtSelection() const
{
	if (ListView.IsValid())
	{
		const int32 NumFilesSelected = ListView->GetSelectedItems().Num();

		return FText::FromString(FString::Printf(TEXT("Selected %d of %d"), NumFilesSelected, ItemsFiltered.Num()));
	}

	return FText::GetEmpty();
}

int32 SPjcTabFilesExternal::GetWidgetIndex() const
{
	return ItemsAll.Num() == 0 ? PjcConstants::WidgetIndexIdle : PjcConstants::WidgetIndexWorking;
}
