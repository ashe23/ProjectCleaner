// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#include "UI/ProjectCleanerCorruptedFilesUI.h"
#include "ProjectCleanerUtility.h"
#include "ProjectCleanerHelper.h"
#include "ProjectCleanerStyle.h"
// Engine Headers
#include "Widgets/Layout/SScrollBox.h"

#define LOCTEXT_NAMESPACE "FProjectCleanerModule"

void SProjectCleanerCorruptedFilesUI::Construct(const FArguments& InArgs)
{
	SetCorruptedFiles(InArgs._CorruptedFiles);

	RefreshUIContent();
}

void SProjectCleanerCorruptedFilesUI::SetCorruptedFiles(const TSet<FString>& NewCorruptedFiles)
{
	CorruptedFiles.Reset();
	CorruptedFiles.Reserve(NewCorruptedFiles.Num());

	for (const auto File : NewCorruptedFiles)
	{
		const auto& CorruptedFile = NewObject<UCorruptedFile>();
		CorruptedFile->Name = FPaths::GetBaseFilename(File);
		CorruptedFile->AbsolutePath = File;
		CorruptedFiles.AddUnique(CorruptedFile);
	}

	if (ListView.IsValid())
	{
		ListView->SetListItemsSource(CorruptedFiles);
		ListView->RebuildList();
	}
}

void SProjectCleanerCorruptedFilesUI::RefreshUIContent()
{
	ListView = SNew(SListView<TWeakObjectPtr<UCorruptedFile>>)
		.ListItemsSource(&CorruptedFiles)
		.SelectionMode(ESelectionMode::SingleToggle)
		.OnGenerateRow(this, &SProjectCleanerCorruptedFilesUI::OnGenerateRow)
		.OnMouseButtonDoubleClick_Raw(this, &SProjectCleanerCorruptedFilesUI::OnMouseDoubleClick)
		.HeaderRow
		(
			SNew(SHeaderRow)
			+ SHeaderRow::Column(FName("Name"))
			.HAlignCell(HAlign_Center)
			.VAlignCell(VAlign_Center)
			.HAlignHeader(HAlign_Center)
			.HeaderContentPadding(FMargin(10.0f))
			.FillWidth(0.3f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("NameColumn", "Name"))
			]
			+ SHeaderRow::Column(FName("AbsolutePath"))
			.HAlignCell(HAlign_Center)
			.VAlignCell(VAlign_Center)
			.HAlignHeader(HAlign_Center)
			.HeaderContentPadding(FMargin(10.0f))
			.FillWidth(0.7f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("PathColumn", "AbsolutePath"))
			]
	);

	WidgetRef =
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
					SNew(SVerticalBox)
					+SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(STextBlock)
						.AutoWrapText(true)
						.Font(FProjectCleanerStyle::Get().GetFontStyle("ProjectCleaner.Font.Light20"))
						.Text(LOCTEXT("corrupted_files", "Corrupted Files"))
					]
					+SVerticalBox::Slot()
					.AutoHeight()
					.Padding(FMargin{200.0f, 10.0f})
					[
						SNew(SBorder)
						.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
						.BorderBackgroundColor(FSlateColor{FLinearColor{1.0f, 0.8f, 0.0f ,1.0f}})
						.Padding(FMargin{20.0f})
						.VAlign(VAlign_Center)
						.HAlign(HAlign_Center)
						[
							SNew(STextBlock)
							.AutoWrapText(true)
							.Font(FProjectCleanerStyle::Get().GetFontStyle("ProjectCleaner.Font.Light15"))
							.Text(LOCTEXT("corrupted_files_tip_text", "Sometimes AssetRegistry not updated correctly and some assets may be shown as corrupted.\nThis Happens for example when you adding content from marketplace using Launcher(Add to Project).\nSo to be sure restart Engine and refresh list, see if assets remain in corrupted list."))
						]
					]
					+SVerticalBox::Slot()
					.AutoHeight()
					.Padding(FMargin{5.0f, 10.0f})
					[
						SNew(STextBlock)
						.AutoWrapText(true)
						.Font(FProjectCleanerStyle::Get().GetFontStyle("ProjectCleaner.Font.Light10"))
						.Text(LOCTEXT("corrupted_files_fix_text", "How to fix?:\n\t1.Close Editor\n\t2.Delete files manually from Windows explorer"))
					]
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(FMargin{ 0.0f, 20.0f })
				[
					ListView.ToSharedRef()
				]
			]
		];
	
	ChildSlot
	[
		WidgetRef
	];
}

TSharedRef<ITableRow> SProjectCleanerCorruptedFilesUI::OnGenerateRow(TWeakObjectPtr<UCorruptedFile> InItem,
	const TSharedRef<STableViewBase>& OwnerTable) const
{
	return SNew(SCorruptedFileUISelectionRow, OwnerTable).SelectedRowItem(InItem);
}

void SProjectCleanerCorruptedFilesUI::OnMouseDoubleClick(TWeakObjectPtr<UCorruptedFile> Item) const
{
	if (!Item.IsValid()) return;

	const auto DirectoryPath = FPaths::GetPath(Item.Get()->AbsolutePath);
	if (!FPaths::DirectoryExists(DirectoryPath)) return;
	
	FPlatformProcess::ExploreFolder(*DirectoryPath);
}

#undef LOCTEXT_NAMESPACE