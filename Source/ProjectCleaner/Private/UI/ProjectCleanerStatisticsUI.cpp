// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#include "UI/ProjectCleanerStatisticsUI.h"
#include "Core/ProjectCleanerDataManager.h"
#include "UI/ProjectCleanerStyle.h"

#define LOCTEXT_NAMESPACE "FProjectCleanerModule"

void SProjectCleanerStatisticsUI::Construct(const FArguments& InArgs)
{
	if (InArgs._DataManager)
	{
		SetDataManager(InArgs._DataManager);
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
					.Text(LOCTEXT("stat_all_assets_num", "All Assets - "))
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(STextBlock)
					.AutoWrapText(true)
					.Font(FProjectCleanerStyle::Get().GetFontStyle("ProjectCleaner.Font.Light20"))
					.Text_Raw(this, &SProjectCleanerStatisticsUI::GetAllAssetsNum)
				]
			]
			+ SVerticalBox::Slot()
			.MaxHeight(MaxHeight)
			.Padding(FMargin{0.0, 0.0f, 0.0f, 3.0f})
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
					.Text_Raw(this, &SProjectCleanerStatisticsUI::GetUnusedAssetsNum)
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
					.Text(LOCTEXT("stat_total_project_size", "Project Size - "))
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(STextBlock)
					.AutoWrapText(false)
					.Font(FProjectCleanerStyle::Get().GetFontStyle("ProjectCleaner.Font.Light20"))
					.Text_Raw(this, &SProjectCleanerStatisticsUI::GetTotalProjectSize)
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
					.Text(LOCTEXT("stat_total_unused_assets_size", "Unused Assets Size - "))
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(STextBlock)
					.AutoWrapText(false)
					.Font(FProjectCleanerStyle::Get().GetFontStyle("ProjectCleaner.Font.Light20"))
					.Text_Raw(this, &SProjectCleanerStatisticsUI::GetTotalUnusedAssetsSize)
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
					.Text_Raw(this, &SProjectCleanerStatisticsUI::GetNonEngineFilesNum)
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
					.Text(this, &SProjectCleanerStatisticsUI::GetIndirectAssetsNum)
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
					.Text_Raw(this, &SProjectCleanerStatisticsUI::GetEmptyFoldersNum)
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
					.Text(LOCTEXT("stat_corrupted_assets_num", "Corrupted Assets - "))
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(STextBlock)
					.AutoWrapText(true)
					.Font(FProjectCleanerStyle::Get().GetFontStyle("ProjectCleaner.Font.Light20"))
					.Text_Raw(this, &SProjectCleanerStatisticsUI::GetCorruptedAssetsNum)
				]
			]
		]
	];
}

void SProjectCleanerStatisticsUI::SetDataManager(ProjectCleanerDataManager* DataManagerPtr)
{
	if (!DataManagerPtr) return;
	DataManager = DataManagerPtr;
}

FText SProjectCleanerStatisticsUI::GetAllAssetsNum() const
{
	return FText::AsNumber(DataManager->GetAllAssets().Num());
}

FText SProjectCleanerStatisticsUI::GetUnusedAssetsNum() const
{
	return FText::AsNumber(DataManager->GetUnusedAssets().Num());
}

FText SProjectCleanerStatisticsUI::GetTotalProjectSize() const
{
	return FText::AsMemory(DataManager->GetTotalProjectSize());
}

FText SProjectCleanerStatisticsUI::GetTotalUnusedAssetsSize() const
{
	return FText::AsMemory(DataManager->GetTotalUnusedAssetsSize());
}

FText SProjectCleanerStatisticsUI::GetNonEngineFilesNum() const
{
	return FText::AsNumber(DataManager->GetNonEngineFiles().Num());
}

FText SProjectCleanerStatisticsUI::GetIndirectAssetsNum() const
{
	return FText::AsNumber(DataManager->GetIndirectlyUsedAssets().Num());
}

FText SProjectCleanerStatisticsUI::GetEmptyFoldersNum() const
{
	return FText::AsNumber(DataManager->GetEmptyFolders().Num());
}

FText SProjectCleanerStatisticsUI::GetCorruptedAssetsNum() const
{
	return FText::AsNumber(DataManager->GetCorruptedAssets().Num());
}


#undef LOCTEXT_NAMESPACE
