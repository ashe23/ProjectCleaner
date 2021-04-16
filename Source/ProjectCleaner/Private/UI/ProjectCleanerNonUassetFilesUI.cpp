#include "UI/ProjectCleanerNonUassetFilesUI.h"

#define LOCTEXT_NAMESPACE "FProjectCleanerModule"

void SProjectCleanerNonUassetFilesUI::Construct(const FArguments& InArgs)
{
	SetNonUassetFiles(InArgs._NonUassetFiles);
	RefreshUIContent();
}

void SProjectCleanerNonUassetFilesUI::SetNonUassetFiles(const TSet<FName>& NewNonUassetFile)
{
	NonUassetFiles.Reset();
	NonUassetFiles.Reserve(NewNonUassetFile.Num());

	for(const auto& File :NewNonUassetFile)
	{
		const auto& NonUassetFile = NewObject<UNonUassetFile>();
		if(!NonUassetFile) continue;
		NonUassetFile->FileName = FPaths::GetBaseFilename(File.ToString()) + "." + FPaths::GetExtension(File.ToString());
		NonUassetFile->FilePath = File.ToString();
		NonUassetFiles.AddUnique(NonUassetFile);
	}	

	RefreshUIContent();
}

void SProjectCleanerNonUassetFilesUI::RefreshUIContent()
{
	const auto FontInfo = FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Light.ttf"), 20);

	WidgetRef =
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(STextBlock)
			.AutoWrapText(true)
			.Font(FontInfo)
			.Text(LOCTEXT("nonuassetfiles", "Non .uasset files"))
		]
		+ SVerticalBox::Slot()
		  .Padding(FMargin{0.0f, 10.0f})
		  .AutoHeight()
		[
			SNew(STextBlock)
			.AutoWrapText(true)
			.Font(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Light.ttf"), 8))
			.Text(LOCTEXT("dblclickonrow", "Double click on row to open in Explorer"))
		]
		+ SVerticalBox::Slot()
		  .AutoHeight()
		  .Padding(FMargin{0.0f, 20.0f})
		[
			SNew(SListView<TWeakObjectPtr<UNonUassetFile>>)
			.ListItemsSource(&NonUassetFiles)
			.SelectionMode(ESelectionMode::SingleToggle)
			.OnGenerateRow(this, &SProjectCleanerNonUassetFilesUI::OnGenerateRow)
			.OnMouseButtonDoubleClick_Raw(this, &SProjectCleanerNonUassetFilesUI::OnMouseDoubleClick)
			.HeaderRow
			(
				SNew(SHeaderRow)
				+ SHeaderRow::Column(FName("FileName"))
				.HAlignCell(HAlign_Center)
				.VAlignCell(VAlign_Center)
				.HAlignHeader(HAlign_Center)
				.HeaderContentPadding(FMargin(10.0f))
				.FillWidth(0.3f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("NameColumn", "FileName"))
				]
				+ SHeaderRow::Column(FName("FilePath"))
				.HAlignCell(HAlign_Center)
				.VAlignCell(VAlign_Center)
				.HAlignHeader(HAlign_Center)
				.HeaderContentPadding(FMargin(10.0f))
				.FillWidth(0.7f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("PathColumn", "FilePath"))
				]
			)
		];

	ChildSlot
	[
		WidgetRef
	];
}

TSharedRef<ITableRow> SProjectCleanerNonUassetFilesUI::OnGenerateRow(
	TWeakObjectPtr<UNonUassetFile> InItem,
	const TSharedRef<STableViewBase>& OwnerTable
) const
{
	return SNew(SNonUassetFileUISelectionRow, OwnerTable).SelectedRowItem(InItem);
}

void SProjectCleanerNonUassetFilesUI::OnMouseDoubleClick(TWeakObjectPtr<UNonUassetFile> Item) const
{
	if (!Item.IsValid()) return;

	const auto DirectoryPath = FPaths::GetPath(Item.Get()->FilePath);
	if (FPaths::DirectoryExists(DirectoryPath))
	{
		FPlatformProcess::ExploreFolder(*DirectoryPath);
	}
}

#undef LOCTEXT_NAMESPACE
