#include "UI/ProjectCleanerBrowserStatisticsUI.h"

#define LOCTEXT_NAMESPACE "FProjectCleanerModule"

void SProjectCleanerBrowserStatisticsUI::Construct(const FArguments& InArgs)
{
	if (InArgs._UnusedAssets)
	{
		UnusedAssets = InArgs._UnusedAssets;
	}
	if (InArgs._TotalSize)
	{
		TotalSize = InArgs._TotalSize;
	}
	if (InArgs._EmptyFolders)
	{
		EmptyFolders = InArgs._EmptyFolders;
	}


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
	                    .Text_Lambda([this]() -> FText { return FText::AsNumber(UnusedAssets); })
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
                        .Text_Lambda([this]() -> FText { return FText::AsMemory(TotalSize); })
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
                        .Text_Lambda([this]() -> FText { return FText::AsNumber(EmptyFolders); })
					]
				]
				+ SVerticalBox::Slot()
                  .AutoHeight()
                [
                    SNew(SBorder)
					.Padding(FMargin(10))
					.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
                    [
                        SNew(SHorizontalBox)
                        + SHorizontalBox::Slot()
                        .FillWidth(1.0f)
                        [
                            SNew(SButton)
                            .HAlign(HAlign_Center)
                            .VAlign(VAlign_Center)
                            .Text(FText::FromString("Refresh"))
                            // .OnClicked_Raw(this, &FProjectCleanerModule::RefreshBrowser)
                        ]
                        + SHorizontalBox::Slot()
                          .FillWidth(1.0f)
                          .Padding(FMargin{40.0f, 0.0f, 40.0f, 0.0f})
                        [
                            SNew(SButton)
                            .HAlign(HAlign_Center)
                            .VAlign(VAlign_Center)
                            .Text(FText::FromString("Delete Unused Assets"))
                            // .OnClicked_Raw(this, &FProjectCleanerModule::OnDeleteUnusedAssetsBtnClick)
                        ]
                        + SHorizontalBox::Slot()
                        .FillWidth(1.0f)
                        [
                            SNew(SButton)
                            .HAlign(HAlign_Center)
                            .VAlign(VAlign_Center)
                            .Text(FText::FromString("Delete Empty Folders"))
                            // .OnClicked_Raw(this, &FProjectCleanerModule::OnDeleteEmptyFolderClick)
                        ]
                    ]
                ]
			]
		]
	];
}

#undef LOCTEXT_NAMESPACE
