// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#include "UI/ProjectCleanerNonUassetFilesUI.h"
#include "ProjectCleanerStyle.h"
#include "ProjectCleanerUtility.h"
// Engine Headers
#include "Widgets/Layout/SScrollBox.h"

#define LOCTEXT_NAMESPACE "FProjectCleanerModule"

void SProjectCleanerNonUassetFilesUI::Construct(const FArguments& InArgs)
{
	SetNonUassetFiles(InArgs._NonUassetFiles);
}

void SProjectCleanerNonUassetFilesUI::SetNonUassetFiles(const TSet<FName>& NewNonUassetFile)
{
	NonUassetFiles.Reset();
	NonUassetFiles.Reserve(NewNonUassetFile.Num());

	for (const auto& File: NewNonUassetFile)
	{
		const auto& NonUassetFile = NewObject<UNonUassetFile>();
		if(!NonUassetFile) continue;
		NonUassetFile->FileName = FPaths::GetBaseFilename(File.ToString()) + "." + FPaths::GetExtension(File.ToString());
		NonUassetFile->FilePath = ProjectCleanerUtility::ConvertRelativeToAbsPath(File).ToString();
		NonUassetFiles.AddUnique(NonUassetFile);
	}

	RefreshUIContent();
}

void SProjectCleanerNonUassetFilesUI::RefreshUIContent()
{
	WidgetRef =
		SNew(SOverlay)
		+ SOverlay::Slot()
		.Padding(20.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.AutoWrapText(true)
				.Font(FProjectCleanerStyle::Get().GetFontStyle("ProjectCleaner.Font.Light20"))
				.Text(LOCTEXT("non_uasset_files", "Non .uasset files"))
			]
			+ SVerticalBox::Slot()
			.Padding(FMargin{0.0f, 10.0f})
			.AutoHeight()
			[
				SNew(STextBlock)
				.AutoWrapText(true)
				.Font(FProjectCleanerStyle::Get().GetFontStyle("ProjectCleaner.Font.Light10"))
				.Text(LOCTEXT("dblclickonrow", "Double click on row to open in Explorer"))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FMargin{0.0f, 20.0f})
			[
				SNew(SScrollBox)
				+ SScrollBox::Slot()
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
				]
			]
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
	if (!FPaths::DirectoryExists(DirectoryPath)) return;
	
	FPlatformProcess::ExploreFolder(*DirectoryPath);
}

#undef LOCTEXT_NAMESPACE