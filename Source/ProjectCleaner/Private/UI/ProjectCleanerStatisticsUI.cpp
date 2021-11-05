// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#include "UI/ProjectCleanerStatisticsUI.h"
#include "UI/ProjectCleanerStyle.h"
#include "Core/ProjectCleanerManager.h"
#include "Core/ProjectCleanerUtility.h"
// Engine Headers
#include "Widgets/Notifications/SProgressBar.h"

#define LOCTEXT_NAMESPACE "FProjectCleanerModule"

void SProjectCleanerStatisticsUI::Construct(const FArguments& InArgs)
{
	if (InArgs._CleanerManager)
	{
		SetCleanerManager(InArgs._CleanerManager);
	}

	constexpr float MaxHeight = 40.0f;

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
					.Text(LOCTEXT("stat_excluded_assets_num", "Excluded Assets - "))
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(STextBlock)
					.AutoWrapText(true)
					.Font(FProjectCleanerStyle::Get().GetFontStyle("ProjectCleaner.Font.Light20"))
					.Text_Raw(this, &SProjectCleanerStatisticsUI::GetExcludedAssetsNum)
				]
			]
			+ SVerticalBox::Slot()
			.MaxHeight(MaxHeight)
			.Padding(FMargin{0.0, 10.0f, 0.0f, 3.0f})
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			.FillHeight(1.0f)
			[
				SNew(SHorizontalBox)
				.ToolTipText(LOCTEXT("stat_unused_assets_percent_ratio_text", "Percentage of unused assets in project. (unused asssets num / all assets num ratio)"))
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Fill)
				[
					SNew(SOverlay)
					+ SOverlay::Slot()
					.HAlign(HAlign_Fill)
					.VAlign(VAlign_Fill)
					[
						SNew(SProgressBar)
						.Percent_Raw(this, &SProjectCleanerStatisticsUI::GetPercentRatio)
						.FillColorAndOpacity_Raw(this, &SProjectCleanerStatisticsUI::GetProgressBarColor)
					]
					+ SOverlay::Slot()
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.AutoWrapText(false)
						.Font(FProjectCleanerStyle::Get().GetFontStyle("ProjectCleaner.Font.Light15"))
						.ColorAndOpacity(FLinearColor{ 0.0f, 0.0f, 0.0f, 1.0f })
						.Text_Raw(this, &SProjectCleanerStatisticsUI::GetProgressBarText )
					]
				]
			]
		]
	];
}

void SProjectCleanerStatisticsUI::SetCleanerManager(FProjectCleanerManager* CleanerManagerPtr)
{
	if (!CleanerManagerPtr) return;
	CleanerManager = CleanerManagerPtr;
}

FText SProjectCleanerStatisticsUI::GetAllAssetsNum() const
{
	return FText::AsNumber(CleanerManager->GetAllAssets().Num());
}

FText SProjectCleanerStatisticsUI::GetUnusedAssetsNum() const
{
	return FText::AsNumber(CleanerManager->GetUnusedAssets().Num());
}

FText SProjectCleanerStatisticsUI::GetTotalProjectSize() const
{
	const int64 TotalProjectSize = ProjectCleanerUtility::GetTotalSize(CleanerManager->GetAllAssets());
	return FText::AsMemory(TotalProjectSize);
}

FText SProjectCleanerStatisticsUI::GetTotalUnusedAssetsSize() const
{
	const int64 UnusedSize = ProjectCleanerUtility::GetTotalSize(CleanerManager->GetUnusedAssets());
	return FText::AsMemory(UnusedSize);
}

FText SProjectCleanerStatisticsUI::GetNonEngineFilesNum() const
{
	return FText::AsNumber(CleanerManager->GetNonEngineFiles().Num());
}

FText SProjectCleanerStatisticsUI::GetIndirectAssetsNum() const
{
	return FText::AsNumber(CleanerManager->GetIndirectAssets().Num());
}

FText SProjectCleanerStatisticsUI::GetEmptyFoldersNum() const
{
	return FText::AsNumber(CleanerManager->GetEmptyFolders().Num());
}

FText SProjectCleanerStatisticsUI::GetCorruptedAssetsNum() const
{
	return FText::AsNumber(CleanerManager->GetCorruptedAssets().Num());
}

FText SProjectCleanerStatisticsUI::GetExcludedAssetsNum() const
{
	return FText::AsNumber(CleanerManager->GetExcludedAssets().Num());
}

TOptional<float> SProjectCleanerStatisticsUI::GetPercentRatio() const
{
	const float Percent = CleanerManager->GetUnusedAssetsPercent();
	return FMath::GetMappedRangeValueClamped(FVector2D{0.0f, 100.0f}, FVector2D{0.0f, 1.0f}, Percent);
}

FSlateColor SProjectCleanerStatisticsUI::GetProgressBarColor() const
{
	const float Percent = CleanerManager->GetUnusedAssetsPercent();

	if (Percent > 0.0f && Percent < 10.0f)
	{
		return FSlateColor{ FLinearColor{0.0f, 0.901f , 0.462f ,1.0f} }; // light green
	}
	if (Percent > 10.0f && Percent < 50.0f)
	{
		return FSlateColor{ FLinearColor{1.0f, 0.933f , 0.345f ,1.0f} }; // light yellow
	}
	if (Percent > 50.0f && Percent < 80.0f)
	{
		return FSlateColor{ FLinearColor{0.898f, 0.450f , 0.450f ,1.0f} }; // light red
	}

	return FSlateColor{ FLinearColor{0.766f, 0.156f , 0.156f ,1.0f} }; // bright red
}

FText SProjectCleanerStatisticsUI::GetProgressBarText() const
{
	return FText::FromString(
		FString::Printf(
			TEXT("%.2f %% (%d of %d) unused assets"),
			CleanerManager->GetUnusedAssetsPercent(),
			CleanerManager->GetUnusedAssets().Num(),
			CleanerManager->GetAllAssets().Num()
		)
	);
}

#undef LOCTEXT_NAMESPACE
