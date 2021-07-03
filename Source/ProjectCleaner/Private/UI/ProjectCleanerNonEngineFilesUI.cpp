// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#include "UI/ProjectCleanerNonEngineFilesUI.h"
#include "UI/ProjectCleanerStyle.h"
// Engine Headers
#include "Widgets/Layout/SScrollBox.h"

#define LOCTEXT_NAMESPACE "FProjectCleanerModule"

void SProjectCleanerNonEngineFilesUI::Construct(const FArguments& InArgs)
{
	if (InArgs._NonEngineFiles)
	{
		SetNonEngineFiles(*InArgs._NonEngineFiles);
	}

	ChildSlot
	[
		SNew(SOverlay)
		+ SOverlay::Slot()
		.Padding(20.0f)
		[
			SNew(SScrollBox)
			+ SScrollBox::Slot()
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(STextBlock)
					.AutoWrapText(true)
					.Font(FProjectCleanerStyle::Get().GetFontStyle("ProjectCleaner.Font.Light20"))
					.Text(LOCTEXT("non_engine_files", "Non Engine files"))
				]
				+ SVerticalBox::Slot()
				.Padding(FMargin{0.0f, 10.0f})
				.AutoHeight()
				[
					SNew(STextBlock)
					.AutoWrapText(true)
					.Font(FProjectCleanerStyle::Get().GetFontStyle("ProjectCleaner.Font.Light10"))
					.Text(LOCTEXT("non_engine_files_dblclickonrow", "Double click on row to open in Explorer"))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(FMargin{0.0f, 20.0f})
				[
					SAssignNew(ListView, SListView<TWeakObjectPtr<UNonEngineFile>>)
					.ListItemsSource(&NonEngineFiles)
					.SelectionMode(ESelectionMode::SingleToggle)
					.OnGenerateRow(this, &SProjectCleanerNonEngineFilesUI::OnGenerateRow)
					.OnMouseButtonDoubleClick_Raw(this, &SProjectCleanerNonEngineFilesUI::OnMouseDoubleClick)
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
		]
	];
}

void SProjectCleanerNonEngineFilesUI::SetNonEngineFiles(const TSet<FString>& NewNonEngineFile)
{
	NonEngineFiles.Reset();
	NonEngineFiles.Reserve(NewNonEngineFile.Num());

	for (const auto& File: NewNonEngineFile)
	{
		const auto& NonUassetFile = NewObject<UNonEngineFile>();
		if(!NonUassetFile) continue;
		NonUassetFile->FileName = FPaths::GetBaseFilename(File) + "." + FPaths::GetExtension(File);
		NonUassetFile->FilePath = File;
		NonEngineFiles.AddUnique(NonUassetFile);
	}

	if (ListView.IsValid())
	{
		ListView->RebuildList();
	}
}

TSharedRef<ITableRow> SProjectCleanerNonEngineFilesUI::OnGenerateRow(
	TWeakObjectPtr<UNonEngineFile> InItem,
	const TSharedRef<STableViewBase>& OwnerTable) const
{
	return SNew(SNonEngineFilesUISelectionRow, OwnerTable).SelectedRowItem(InItem);
}

void SProjectCleanerNonEngineFilesUI::OnMouseDoubleClick(TWeakObjectPtr<UNonEngineFile> Item) const
{
	if (!Item.IsValid()) return;

	const auto DirectoryPath = FPaths::GetPath(Item.Get()->FilePath);
	if (!FPaths::DirectoryExists(DirectoryPath)) return;

	FPlatformProcess::ExploreFolder(*DirectoryPath);
}

#undef LOCTEXT_NAMESPACE