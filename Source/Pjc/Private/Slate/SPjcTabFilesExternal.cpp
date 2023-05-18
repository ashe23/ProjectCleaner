// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/SPjcTabFilesExternal.h"
#include "Slate/SPjcItemFileExternal.h"
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

	SAssignNew(ListView, SListView<TSharedPtr<FPjcFileExternalItem>>)
	.ListItemsSource(&ListItems)
	.OnGenerateRow(this, &SPjcTabFilesExternal::OnListGenerateRow)
	.SelectionMode(ESelectionMode::Multi)
	.HeaderRow(GetListHeaderRow());

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
				+ SVerticalBox::Slot().AutoHeight().Padding(3.0f)
				[
					SNew(STextBlock)
					.Justification(ETextJustify::Center)
					.ColorAndOpacity(FPjcStyles::Get().GetColor("ProjectCleaner.Color.Gray"))
					.ShadowOffset(FVector2D{0.5f, 0.5f})
					.ShadowColorAndOpacity(FLinearColor::Black)
					.Font(FPjcStyles::GetFont("Bold", 15))
					.Text(FText::FromString(TEXT("List of external files inside Content folder.")))
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(3.0f)
				[
					SNew(STextBlock)
					.Justification(ETextJustify::Center)
					.ColorAndOpacity(FPjcStyles::Get().GetColor("ProjectCleaner.Color.Gray"))
					.ShadowOffset(FVector2D{0.5f, 0.5f})
					.ShadowColorAndOpacity(FLinearColor::Black)
					.Font(FPjcStyles::GetFont("Bold", 10))
					.Text(FText::FromString(TEXT(
						                "These files won't be visible in the ContentBrowser. Therefore, you can choose to include only the files necessary for your project, and clean up by excluding the rest.")))
				]
				+ SVerticalBox::Slot().FillHeight(1.0f).Padding(3.0f)
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
				+ SVerticalBox::Slot().AutoHeight().Padding(3.0f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.0f).HAlign(HAlign_Left).VAlign(VAlign_Center).Padding(3.0f, 0.0f, 0.0f, 0.0f)
					[
						SNew(STextBlock).Text(FText::FromString(TEXT("Total 12 files (123.44 MiB)")))
					]
					+ SHorizontalBox::Slot().FillWidth(1.0f).HAlign(HAlign_Center).VAlign(VAlign_Center)
					[
						SNew(STextBlock).Text(FText::FromString(TEXT("Selected 1 of 12 files")))
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

TSharedRef<SWidget> SPjcTabFilesExternal::GetBtnOptionsContent()
{
	const TSharedPtr<FExtender> Extender;
	FMenuBuilder MenuBuilder(true, Cmds, Extender, true);

	MenuBuilder.AddMenuEntry(
		FText::FromString(TEXT("Show Files Excluded")),
		FText::FromString(TEXT("Show excluded files in list view")),
		FSlateIcon(),
		FUIAction
		(
			FExecuteAction::CreateLambda([&] { })
		),
		NAME_None,
		EUserInterfaceActionType::ToggleButton
	);

	return MenuBuilder.MakeWidget();
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

TSharedRef<ITableRow> SPjcTabFilesExternal::OnListGenerateRow(TSharedPtr<FPjcFileExternalItem> Item, const TSharedRef<STableViewBase>& OwnerTable) const
{
	return SNew(SPjcItemFileExternal, OwnerTable).Item(Item);
}

FSlateColor SPjcTabFilesExternal::GetOptionsBtnForegroundColor() const
{
	static const FName InvertedForegroundName("InvertedForeground");
	static const FName DefaultForegroundName("DefaultForeground");

	if (!OptionBtn.IsValid()) return FEditorStyle::GetSlateColor(DefaultForegroundName);

	return OptionBtn->IsHovered() ? FEditorStyle::GetSlateColor(InvertedForegroundName) : FEditorStyle::GetSlateColor(DefaultForegroundName);
}
