// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/SPjcTabFilesExternal.h"
#include "Slate/SPjcItemFileExternal.h"
#include "Slate/SPjcItemStat.h"
#include "PjcCmds.h"
#include "PjcStyles.h"
#include "PjcSubsystem.h"
// Engine Headers
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"

void SPjcTabFilesExternal::Construct(const FArguments& InArgs)
{
	Cmds = MakeShareable(new FUICommandList);

	Cmds->MapAction(
		FPjcCmds::Get().FilesScan,
		FExecuteAction::CreateLambda([&]()
		{
			ListUpdate();
		})
	);
	Cmds->MapAction(
		FPjcCmds::Get().FilesDelete,
		FExecuteAction::CreateLambda([]() {})
	);
	Cmds->MapAction(
		FPjcCmds::Get().FilesExclude,
		FExecuteAction::CreateLambda([]() {})
	);
	Cmds->MapAction(
		FPjcCmds::Get().FilesExcludeByExt,
		FExecuteAction::CreateLambda([]() {})
	);
	Cmds->MapAction(
		FPjcCmds::Get().FilesExcludeByPath,
		FExecuteAction::CreateLambda([]() {})
	);
	Cmds->MapAction(
		FPjcCmds::Get().ClearSelection,
		FExecuteAction::CreateLambda([]() {})
	);


	SAssignNew(StatView, SListView<TSharedPtr<FPjcStatItem>>)
	.ListItemsSource(&StatItems)
	.OnGenerateRow(this, &SPjcTabFilesExternal::OnStatGenerateRow)
	.SelectionMode(ESelectionMode::None)
	.IsFocusable(false)
	.HeaderRow(GetStatHeaderRow());

	SAssignNew(ListView, SListView<TSharedPtr<FPjcFileExternalItem>>)
	.ListItemsSource(&ListItems)
	.OnGenerateRow(this, &SPjcTabFilesExternal::OnListGenerateRow)
	.SelectionMode(ESelectionMode::Multi)
	.HeaderRow(GetListHeaderRow());

	StatsInit();
	ListUpdate();

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
				SNew(SBox)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().Padding(3.0f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(1.0f).HAlign(HAlign_Center).VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Justification(ETextJustify::Center)
							.ColorAndOpacity(FPjcStyles::Get().GetColor("ProjectCleaner.Color.Gray"))
							.ShadowOffset(FVector2D{1.5f, 1.5f})
							.ShadowColorAndOpacity(FLinearColor::Black)
							.Font(FPjcStyles::GetFont("Bold", 15))
							.Text(FText::FromString(TEXT("Summary")))
						]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(3.0f)
					[
						StatView.ToSharedRef()
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(3.0f)
					[
						SNew(SSeparator).Thickness(3.0f)
					]
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
			]
			+ SSplitter::Slot().Value(0.7f)
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
	];
}

void SPjcTabFilesExternal::StatsInit()
{
	StatItems.Reset();

	const int32 NumFilesTotal = 0;
	const int32 NumFilesUndetermined = 0;
	const int32 NumFilesExcluded = 0;

	const int64 SizeFilesTotal = 0;
	const int64 SizeFilesUndetermined = 0;
	const int64 SizeFilesExcluded = 0;

	StatItems.Emplace(
		MakeShareable(
			new FPjcStatItem{
				FText::FromString(TEXT("Undetermined")),
				FText::AsNumber(NumFilesUndetermined),
				FText::AsMemory(SizeFilesUndetermined, IEC),
				FText::FromString(TEXT("Undetermined Files that will be considered external if you user explicitly did not exclude it from scanning")),
				FText::FromString(TEXT("Total number of undetermined files")),
				FText::FromString(TEXT("Total size of undetermined files")),
			}
		)
	);

	StatItems.Emplace(
		MakeShareable(
			new FPjcStatItem{
				FText::FromString(TEXT("Excluded")),
				FText::AsNumber(NumFilesExcluded),
				FText::AsMemory(SizeFilesExcluded, IEC),
				FText::FromString(TEXT("Excluded Files")),
				FText::FromString(TEXT("Total number of excluded files")),
				FText::FromString(TEXT("Total size of excluded files")),
			}
		)
	);

	StatItems.Emplace(
		MakeShareable(
			new FPjcStatItem{
				FText::FromString(TEXT("Total")),
				FText::AsNumber(NumFilesExcluded),
				FText::AsMemory(SizeFilesExcluded, IEC),
				FText::FromString(TEXT("All external files in project")),
				FText::FromString(TEXT("Total number of external files")),
				FText::FromString(TEXT("Total size of external files")),
			}
		)
	);

	if (StatView.IsValid())
	{
		StatView->RebuildList();
	}
}

void SPjcTabFilesExternal::StatsUpdate()
{
	// todo:ashe23 implement logic later here
}

void SPjcTabFilesExternal::ListUpdate()
{
	TSet<FString> FilesExternal;
	UPjcSubsystem::GetFilesExternal(FilesExternal);

	ListItems.Reset();
	ListItems.Reserve(FilesExternal.Num());

	for (const auto& File : FilesExternal)
	{
		const int64 FileSize = IFileManager::Get().FileSize(*File);
		const FString FileName = FPaths::GetBaseFilename(File);
		const FString FileExt = FPaths::GetExtension(File, false).ToLower();
		ListItems.Emplace(MakeShareable(new FPjcFileExternalItem{FileSize, FileName, FileExt, File}));
	}

	if (ListView.IsValid())
	{
		ListView->RebuildList();
	}
}

TSharedRef<SWidget> SPjcTabFilesExternal::CreateToolbar() const
{
	FToolBarBuilder ToolBarBuilder{Cmds, FMultiBoxCustomization::None};
	ToolBarBuilder.BeginSection("PjcSectionActionsFilesExternal");
	{
		ToolBarBuilder.AddToolBarButton(FPjcCmds::Get().FilesScan);
		ToolBarBuilder.AddToolBarButton(FPjcCmds::Get().FilesDelete);
		ToolBarBuilder.AddSeparator();
		ToolBarBuilder.AddToolBarButton(FPjcCmds::Get().FilesExclude);
		ToolBarBuilder.AddToolBarButton(FPjcCmds::Get().FilesExcludeByExt);
		ToolBarBuilder.AddToolBarButton(FPjcCmds::Get().FilesExcludeByPath);
		ToolBarBuilder.AddSeparator();
		ToolBarBuilder.AddToolBarButton(FPjcCmds::Get().ClearSelection);
	}
	ToolBarBuilder.EndSection();

	return ToolBarBuilder.MakeWidget();
}

TSharedRef<SHeaderRow> SPjcTabFilesExternal::GetStatHeaderRow() const
{
	const FMargin HeaderMargin{5.0f};

	return
		SNew(SHeaderRow)
		+ SHeaderRow::Column("Name")
		  .FillWidth(0.4f)
		  .HAlignCell(HAlign_Left)
		  .VAlignCell(VAlign_Center)
		  .HAlignHeader(HAlign_Center)
		  .HeaderContentPadding(HeaderMargin)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Category")))
			.Font(FPjcStyles::GetFont("Light", 10.0f))
			.ColorAndOpacity(FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
		]
		+ SHeaderRow::Column("Num")
		  .FillWidth(0.3f)
		  .HAlignCell(HAlign_Center)
		  .VAlignCell(VAlign_Center)
		  .HAlignHeader(HAlign_Center)
		  .HeaderContentPadding(HeaderMargin)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Num")))
			.Font(FPjcStyles::GetFont("Light", 10.0f))
			.ColorAndOpacity(FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
		]
		+ SHeaderRow::Column("Size")
		  .FillWidth(0.3f)
		  .HAlignCell(HAlign_Center)
		  .VAlignCell(VAlign_Center)
		  .HAlignHeader(HAlign_Center)
		  .HeaderContentPadding(HeaderMargin)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Size")))
			.Font(FPjcStyles::GetFont("Light", 10.0f))
			.ColorAndOpacity(FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
		];
}

TSharedRef<SHeaderRow> SPjcTabFilesExternal::GetListHeaderRow() const
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
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("FileSize")))
			.Font(FPjcStyles::GetFont("Light", 10.0f))
			.ColorAndOpacity(FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
		];
}

TSharedRef<ITableRow> SPjcTabFilesExternal::OnStatGenerateRow(TSharedPtr<FPjcStatItem> Item, const TSharedRef<STableViewBase>& OwnerTable) const
{
	return SNew(SPjcItemStat, OwnerTable).Item(Item);
}

TSharedRef<ITableRow> SPjcTabFilesExternal::OnListGenerateRow(TSharedPtr<FPjcFileExternalItem> Item, const TSharedRef<STableViewBase>& OwnerTable) const
{
	return SNew(SPjcItemFileExternal, OwnerTable).Item(Item);
}
