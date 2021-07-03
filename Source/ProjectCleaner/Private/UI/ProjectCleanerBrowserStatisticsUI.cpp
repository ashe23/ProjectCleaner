// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#include "UI/ProjectCleanerBrowserStatisticsUI.h"
#include "UI/ProjectCleanerStyle.h"
#include "Core/ProjectCleanerUtility.h"

#define LOCTEXT_NAMESPACE "FProjectCleanerModule"

void SProjectCleanerBrowserStatisticsUI::Construct(const FArguments& InArgs)
{
	if (InArgs._CleanerData)
	{
		SetData(*InArgs._CleanerData);
	}

	const float MaxHeight = 40.0f;

	ChildSlot
	[
		SNew(SOverlay)
		+ SOverlay::Slot()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Top)
			[
				SNew(STextBlock)
				.AutoWrapText(true)
				.Font(FProjectCleanerStyle::Get().GetFontStyle("ProjectCleaner.Font.Light30"))
				.Text(LOCTEXT("stat_title", "Statistics"))
			]
		]
		+ SOverlay::Slot()
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.MaxHeight(MaxHeight)
			.Padding(FMargin{0.0, 40.0f, 0.0f, 3.0f})
			.HAlign(HAlign_Center)
			[
				// Unused Assets
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(STextBlock)
					.AutoWrapText(true)
					.Font(FProjectCleanerStyle::Get().GetFontStyle("ProjectCleaner.Font.Light20"))
					.Text(LOCTEXT("stat_unused_assets_num", "Unused Assets - "))
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(STextBlock)
					.AutoWrapText(true)
					.Font(FProjectCleanerStyle::Get().GetFontStyle("ProjectCleaner.Font.Light20"))
					.Text_Raw(this, &SProjectCleanerBrowserStatisticsUI::GetUnusedAssetsNum)
				]
			]
			+ SVerticalBox::Slot()
			.MaxHeight(MaxHeight)
			.Padding(FMargin{0.0, 0.0f, 0.0f, 3.0f})
			.HAlign(HAlign_Center)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(STextBlock)
					.AutoWrapText(true)
					.Font(FProjectCleanerStyle::Get().GetFontStyle("ProjectCleaner.Font.Light20"))
					.Text(LOCTEXT("stat_total_size", "Total Size - "))
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(STextBlock)
					.AutoWrapText(false)
					.Font(FProjectCleanerStyle::Get().GetFontStyle("ProjectCleaner.Font.Light20"))
					.Text_Raw(this, &SProjectCleanerBrowserStatisticsUI::GetTotalSize)
				]
			]
			+ SVerticalBox::Slot()
			.MaxHeight(MaxHeight)
			.Padding(FMargin{0.0, 0.0f, 0.0f, 3.0f})
			.HAlign(HAlign_Center)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(STextBlock)
					.AutoWrapText(true)
					.Font(FProjectCleanerStyle::Get().GetFontStyle("ProjectCleaner.Font.Light20"))
					.Text(LOCTEXT("stat_non_engine_files_num", "Non Engine files - "))
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(STextBlock)
					.AutoWrapText(true)
					.Font(FProjectCleanerStyle::Get().GetFontStyle("ProjectCleaner.Font.Light20"))
					.Text_Raw(this, &SProjectCleanerBrowserStatisticsUI::GetNonEngineFilesNum)
				]
			]
			+ SVerticalBox::Slot()
			.MaxHeight(MaxHeight)
			.Padding(FMargin{0.0, 0.0f, 0.0f, 3.0f})
			.HAlign(HAlign_Center)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(STextBlock)
					.AutoWrapText(true)
					.Font(FProjectCleanerStyle::Get().GetFontStyle("ProjectCleaner.Font.Light20"))
					.Text(LOCTEXT("stat_indirect_files_num", "Indirect Assets - "))
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(STextBlock)
					.AutoWrapText(true)
					.Font(FProjectCleanerStyle::Get().GetFontStyle("ProjectCleaner.Font.Light20"))
					.Text(this, &SProjectCleanerBrowserStatisticsUI::GetIndirectAssetsNum)
				]
			]
			+ SVerticalBox::Slot()
			.MaxHeight(MaxHeight)
			.Padding(FMargin{0.0, 0.0f, 0.0f, 3.0f})
			.HAlign(HAlign_Center)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(STextBlock)
					.AutoWrapText(true)
					.Font(FProjectCleanerStyle::Get().GetFontStyle("ProjectCleaner.Font.Light20"))
					.Text(LOCTEXT("stat_empty_folders_num", "Empty Folders - "))
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(STextBlock)
					.AutoWrapText(true)
					.Font(FProjectCleanerStyle::Get().GetFontStyle("ProjectCleaner.Font.Light20"))
					.Text_Raw(this, &SProjectCleanerBrowserStatisticsUI::GetEmptyFoldersNum)
				]
			]
			+ SVerticalBox::Slot()
			.MaxHeight(MaxHeight)
			.Padding(FMargin{0.0, 0.0f, 0.0f, 3.0f})
			.HAlign(HAlign_Center)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(STextBlock)
					.AutoWrapText(true)
					.Font(FProjectCleanerStyle::Get().GetFontStyle("ProjectCleaner.Font.Light20"))
					.Text(LOCTEXT("stat_corrupted_files_num", "Corrupted Files - "))
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(STextBlock)
					.AutoWrapText(true)
					.Font(FProjectCleanerStyle::Get().GetFontStyle("ProjectCleaner.Font.Light20"))
					.Text_Raw(this, &SProjectCleanerBrowserStatisticsUI::GetCorruptedFilesNum)
				]
			]
		]
	];
}

void SProjectCleanerBrowserStatisticsUI::SetData(FProjectCleanerData& Data)
{
	CleanerData = &Data;
}

FText SProjectCleanerBrowserStatisticsUI::GetUnusedAssetsNum() const
{
	return FText::AsNumber(CleanerData ? CleanerData->UnusedAssets.Num() : 0);
}

FText SProjectCleanerBrowserStatisticsUI::GetTotalSize() const
{
	return FText::AsMemory(CleanerData ? CleanerData->TotalSize : 0);
}

FText SProjectCleanerBrowserStatisticsUI::GetNonEngineFilesNum() const
{
	return FText::AsNumber(CleanerData ? CleanerData->NonEngineFiles.Num() : 0);
}

FText SProjectCleanerBrowserStatisticsUI::GetIndirectAssetsNum() const
{
	return FText::AsNumber(CleanerData ? CleanerData->IndirectFileInfos.Num() : 0);
}

FText SProjectCleanerBrowserStatisticsUI::GetEmptyFoldersNum() const
{
	return FText::AsNumber(CleanerData ? CleanerData->EmptyFolders.Num() : 0);
}

FText SProjectCleanerBrowserStatisticsUI::GetCorruptedFilesNum() const
{
	return FText::AsNumber(CleanerData ? CleanerData->CorruptedFiles.Num() : 0);
}


#undef LOCTEXT_NAMESPACE
