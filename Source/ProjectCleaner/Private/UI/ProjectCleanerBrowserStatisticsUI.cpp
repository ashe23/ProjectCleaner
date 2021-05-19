// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#include "UI/ProjectCleanerBrowserStatisticsUI.h"
#include "ProjectCleanerStyle.h"

#define LOCTEXT_NAMESPACE "FProjectCleanerModule"

void SProjectCleanerBrowserStatisticsUI::Construct(const FArguments& InArgs)
{
	Stats = InArgs._Stats;
	RefreshUIContent();
}

void SProjectCleanerBrowserStatisticsUI::SetStats(const FCleaningStats& NewStats)
{
	Stats = NewStats;
	RefreshUIContent();
}

FCleaningStats SProjectCleanerBrowserStatisticsUI::GetStats() const
{
	return Stats;
}

void SProjectCleanerBrowserStatisticsUI::RefreshUIContent()
{
	WidgetRef = SNew(SBorder)
	.Padding(FMargin(10.0f))
	.HAlign(HAlign_Fill)
	.VAlign(VAlign_Fill)
	.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
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
				.Font(FProjectCleanerStyle::Get().GetFontStyle("ProjectCleaner.Font.Light20"))
				.Text(LOCTEXT("statistics", "Statistics"))
			]
		]
		+ SOverlay::Slot()
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.MaxHeight(20.0f)
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
					.Text(LOCTEXT("unused_assets", "Unused Assets - "))
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(STextBlock)
					.AutoWrapText(true)
					.Text_Lambda([this]() -> FText { return FText::AsNumber(Stats.UnusedAssetsNum); })
				]
			]
			+ SVerticalBox::Slot()
			.MaxHeight(20.0f)
			.Padding(FMargin{0.0, 0.0f, 0.0f, 3.0f})
			.HAlign(HAlign_Center)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(STextBlock)
					.AutoWrapText(true)
					.Text(LOCTEXT("total_size", "Total Size - "))
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(STextBlock)
					.AutoWrapText(true)
					.Text_Lambda([this]() -> FText { return FText::AsMemory(Stats.UnusedAssetsTotalSize); })
				]
			]
			+ SVerticalBox::Slot()
			.MaxHeight(20.0f)
			.Padding(FMargin{0.0, 0.0f, 0.0f, 3.0f})
			.HAlign(HAlign_Center)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(STextBlock)
					.AutoWrapText(true)
					.Text(LOCTEXT("non_uasset_files_", "Non .uasset files - "))
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(STextBlock)
					.AutoWrapText(true)
					.Text_Lambda([this]() -> FText { return FText::AsNumber(Stats.NonUassetFilesNum); })
				]
			]
			+ SVerticalBox::Slot()
			.MaxHeight(20.0f)
			.Padding(FMargin{0.0, 0.0f, 0.0f, 3.0f})
			.HAlign(HAlign_Center)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(STextBlock)
					.AutoWrapText(true)
					.Text(LOCTEXT("sourcecode_assets", "Assets Used In Source Code Files - "))
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(STextBlock)
					.AutoWrapText(true)
					.Text_Lambda([this]() -> FText { return FText::AsNumber(Stats.SourceCodeAssetsNum); })
				]
			]
			+ SVerticalBox::Slot()
			.MaxHeight(20.0f)
			.Padding(FMargin{0.0, 0.0f, 0.0f, 3.0f})
			.HAlign(HAlign_Center)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(STextBlock)
					.AutoWrapText(true)
					.Text(LOCTEXT("empty_folders", "Empty Folders - "))
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(STextBlock)
					.AutoWrapText(true)
					.Text_Lambda([this]() -> FText { return FText::AsNumber(Stats.EmptyFolders); })
				]
			]
		]
	];

	ChildSlot
	[
		WidgetRef
	];
}

#undef LOCTEXT_NAMESPACE