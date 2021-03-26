#include "UI/ProjectCleanerBrowserStatisticsUI.h"

#define LOCTEXT_NAMESPACE "FProjectCleanerModule"

void SProjectCleanerBrowserStatisticsUI::Construct(const FArguments& InArgs)
{
	Stats = InArgs._Stats;
	

	const FSlateFontInfo FontInfo = FSlateFontInfo(
		FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Light.ttf"),
		20
	);

	ChildSlot
	[
		SNew(SBorder)
		.Padding(FMargin(40.0f, 20.0f, 40.0f, 20.0f))
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
                    .Font(FontInfo)
                    .Text(LOCTEXT("Statistics", "Statistics"))
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
	                    .Text(LOCTEXT("Unused Assets", "Unused Assets - "))
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
					// Unused Assets
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(STextBlock)
                        .AutoWrapText(true)
                        .Text(LOCTEXT("Total Size", "Total Size - "))
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
					// Unused Assets
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(STextBlock)
                        .AutoWrapText(true)
                        .Text(LOCTEXT("Empty Folders", "Empty Folders - "))
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(STextBlock)
                        .AutoWrapText(true)
                        .Text_Lambda([this]() -> FText { return FText::AsNumber(Stats.EmptyFolders); })
					]
				]
				+ SVerticalBox::Slot()
                .MaxHeight(20.0f)
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
                        .Text(LOCTEXT("NonProjectFilesNum", "Non Project Files Count - "))
                    ]
                    + SHorizontalBox::Slot()
                    .AutoWidth()
                    [
                        SNew(STextBlock)
                        .AutoWrapText(true)
                        .Text_Lambda([this]() -> FText { return FText::AsNumber(Stats.NonProjectFilesNum); })
                    ]
                ]
			]
		]
	];
}

void SProjectCleanerBrowserStatisticsUI::SetStats(const FCleaningStats& NewStats)
{
	Stats = NewStats;
}

FCleaningStats SProjectCleanerBrowserStatisticsUI::GetStats() const
{
	return Stats;
}

#undef LOCTEXT_NAMESPACE
