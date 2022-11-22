// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/ProjectCleanerWindowMain.h"
#include "ProjectCleanerStyles.h"
#include "ProjectCleanerConstants.h"
#include "Libs/ProjectCleanerAssetLibrary.h"
// Engine Headers
#include "Widgets/Input/SHyperlink.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/Notifications/SProgressBar.h"

static constexpr int32 WidgetIndexNone = 0;
static constexpr int32 WidgetIndexLoading = 1;

void SProjectCleanerWindowMain::Construct(const FArguments& InArgs)
{
	ChildSlot
	[
		SNew(SWidgetSwitcher)
		.IsEnabled_Static(IsWidgetEnabled)
		.WidgetIndex_Static(GetWidgetIndex)
		+ SWidgetSwitcher::Slot()
		  .HAlign(HAlign_Fill)
		  .VAlign(VAlign_Fill)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			  .Padding(FMargin{10.0f})
			  .AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				  .FillWidth(1.0f)
				  .HAlign(HAlign_Right)
				  .VAlign(VAlign_Center)
				[
					SNew(SHyperlink)
            		.Text(FText::FromString(TEXT("Wiki")))
            		.OnNavigate_Static(OnNavigateWiki)
				]
			]
			+ SVerticalBox::Slot()
			  .Padding(FMargin{10.0f})
			  .FillHeight(1.0f)
			[
				SNew(SSplitter)
				.Style(FEditorStyle::Get(), "ContentBrowser.Splitter")
				.PhysicalSplitterHandleSize(5.0f)
				+ SSplitter::Slot()
				.Value(0.35f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(FMargin{0.0f, 0.0f, 10.0f, 0.0f})
					[
						SNew(SScrollBox)
						.ScrollWhenFocusChanges(EScrollWhenFocusChanges::AnimatedScroll)
						.AnimateWheelScrolling(true)
						.AllowOverscroll(EAllowOverscroll::No)
						+ SScrollBox::Slot()
						[
							SNew(SBorder)
							.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
							.Padding(FMargin{10.0f})
							[
								SNew(SVerticalBox)
								// + SVerticalBox::Slot()
								// .AutoHeight()
								// [
								// 	SNew(SHorizontalBox)
								// 	+ SHorizontalBox::Slot()
								// 	  .FillWidth(1.0f)
								// 	  .VAlign(VAlign_Center)
								// 	  .HAlign(HAlign_Center)
								// 	[
								// 		SNew(STextBlock)
								// 		.AutoWrapText(true)
								// 		.Justification(ETextJustify::Center)
								// 		.Text(FText::FromString(TEXT("Stats")))
								// 		.ColorAndOpacity(FLinearColor{FColor::FromHex(TEXT("#3498DB"))})
								// 		.ToolTipText(FText::FromString(TEXT("Total number of files in Content folder")))
								// 		.Font(FProjectCleanerStyles::Get().GetFontStyle("ProjectCleaner.Font.Light30"))
								// 	]
								// ]
								+ SVerticalBox::Slot()
								  .Padding(FMargin{10.0f, 0.0f})
								  .AutoHeight()
								[
									SNew(SHorizontalBox)
									+ SHorizontalBox::Slot()
									  .AutoWidth()
									  .HAlign(HAlign_Left)
									[
										SNew(STextBlock)
										.AutoWrapText(true)
										.Justification(ETextJustify::Center)
										.Text(FText::FromString(TEXT("Assets - 271 (234.55 MiB)")))
										.ColorAndOpacity(FLinearColor{FColor::FromHex(TEXT("#3498DB"))})
										.Font(FProjectCleanerStyles::Get().GetFontStyle("ProjectCleaner.Font.Light20"))
									]
								]
								+ SVerticalBox::Slot()
								  .Padding(FMargin{40.0f, 0.0f, 0.0f, 0.0f})
								  .AutoHeight()
								[
									SNew(SHorizontalBox)
									+ SHorizontalBox::Slot()
									  .AutoWidth()
									  .HAlign(HAlign_Left)
									[
										SNew(STextBlock)
										.AutoWrapText(true)
										.Justification(ETextJustify::Center)
										.ColorAndOpacity(FLinearColor{FColor::FromHex(TEXT("#27AE60"))})
										.Text(FText::FromString(TEXT("Used - 70 (150.55 MiB)")))
										.Font(FProjectCleanerStyles::Get().GetFontStyle("ProjectCleaner.Font.Light15"))
									]
								]
								+ SVerticalBox::Slot()
								  .Padding(FMargin{60.0f, 0.0f, 0.0f, 0.0f})
								  .AutoHeight()
								[
									SNew(SHorizontalBox)
									+ SHorizontalBox::Slot()
									  .AutoWidth()
									  .HAlign(HAlign_Left)
									[
										SNew(STextBlock)
										.AutoWrapText(true)
										.Justification(ETextJustify::Center)
										.ColorAndOpacity(FLinearColor{FColor::FromHex(TEXT("#7F8C8D"))})
										.Text(FText::FromString(TEXT("Primary assets - 20")))
										.Font(FProjectCleanerStyles::Get().GetFontStyle("ProjectCleaner.Font.Light15"))
									]
								]
								+ SVerticalBox::Slot()
								  .Padding(FMargin{60.0f, 0.0f, 0.0f, 0.0f})
								  .AutoHeight()
								[
									SNew(SHorizontalBox)
									+ SHorizontalBox::Slot()
									  .AutoWidth()
									  .HAlign(HAlign_Left)
									[
										SNew(STextBlock)
										.AutoWrapText(true)
										.Justification(ETextJustify::Center)
										.ColorAndOpacity(FLinearColor{FColor::FromHex(TEXT("#7F8C8D"))})
										.Text(FText::FromString(TEXT("Used by primary assets - 20")))
										.Font(FProjectCleanerStyles::Get().GetFontStyle("ProjectCleaner.Font.Light15"))
									]
								]
								+ SVerticalBox::Slot()
								  .Padding(FMargin{60.0f, 0.0f, 0.0f, 0.0f})
								  .AutoHeight()
								[
									SNew(SHorizontalBox)
									+ SHorizontalBox::Slot()
									  .AutoWidth()
									  .HAlign(HAlign_Left)
									[
										SNew(STextBlock)
										.AutoWrapText(true)
										.Justification(ETextJustify::Center)
										.ColorAndOpacity(FLinearColor{FColor::FromHex(TEXT("#7F8C8D"))})
										.Text(FText::FromString(TEXT("Used indirectly - 30")))
										.Font(FProjectCleanerStyles::Get().GetFontStyle("ProjectCleaner.Font.Light15"))
									]
								]
								+ SVerticalBox::Slot()
								  .Padding(FMargin{60.0f, 0.0f, 0.0f, 0.0f})
								  .AutoHeight()
								[
									SNew(SHorizontalBox)
									+ SHorizontalBox::Slot()
									  .AutoWidth()
									  .HAlign(HAlign_Left)
									[
										SNew(STextBlock)
										.AutoWrapText(true)
										.Justification(ETextJustify::Center)
										.ColorAndOpacity(FLinearColor{FColor::FromHex(TEXT("#7F8C8D"))})
										.Text(FText::FromString(TEXT("Excluded - 16")))
										.Font(FProjectCleanerStyles::Get().GetFontStyle("ProjectCleaner.Font.Light15"))
									]
								]
								+ SVerticalBox::Slot()
								  .Padding(FMargin{60.0f, 0.0f, 0.0f, 0.0f})
								  .AutoHeight()
								[
									SNew(SHorizontalBox)
									+ SHorizontalBox::Slot()
									  .AutoWidth()
									  .HAlign(HAlign_Left)
									[
										SNew(STextBlock)
										.AutoWrapText(true)
										.Justification(ETextJustify::Center)
										.ColorAndOpacity(FLinearColor{FColor::FromHex(TEXT("#7F8C8D"))})
										.Text(FText::FromString(TEXT("Ignored - 16")))
										.Font(FProjectCleanerStyles::Get().GetFontStyle("ProjectCleaner.Font.Light15"))
									]
								]
								+ SVerticalBox::Slot()
								  .Padding(FMargin{40.0f, 0.0f, 0.0f, 0.0f})
								  .AutoHeight()
								[
									SNew(SHorizontalBox)
									+ SHorizontalBox::Slot()
									  .AutoWidth()
									  .HAlign(HAlign_Left)
									[
										SNew(STextBlock)
										.AutoWrapText(true)
										.Justification(ETextJustify::Center)
										.ColorAndOpacity(FLinearColor{FColor::FromHex(TEXT("#E74C3C"))})
										.Text(FText::FromString(TEXT("Unused - 23 (44.55 MiB)")))
										.Font(FProjectCleanerStyles::Get().GetFontStyle("ProjectCleaner.Font.Light15"))
									]
								]
								// + SVerticalBox::Slot()
								// .MaxHeight(40.0f)
								// .Padding(FMargin{20.0f})
								// .HAlign(HAlign_Fill)
								// .VAlign(VAlign_Fill)
								// .AutoHeight()
								// [
								// 	SNew(SHorizontalBox)
								// 	+ SHorizontalBox::Slot()
								// 	.FillWidth(1.0f)
								// 	.HAlign(HAlign_Fill)
								// 	.VAlign(VAlign_Fill)
								// 	[
								// 		SNew(SOverlay)
								// 		+ SOverlay::Slot()
								// 		.HAlign(HAlign_Fill)
								// 		.VAlign(VAlign_Fill)
								// 		[
								// 			SNew(SProgressBar)
								// 			.FillColorAndOpacity(FLinearColor{FColor::Red})
								// 			.Percent(0.5f)
								// 		]
								// 		+ SOverlay::Slot()
								// 		.HAlign(HAlign_Center)
								// 		.VAlign(VAlign_Center)
								// 		[
								// 			SNew(STextBlock)
								// 			.AutoWrapText(false)
								// 			.Font(FProjectCleanerStyles::Get().GetFontStyle("ProjectCleaner.Font.Light10"))
								// 			.ColorAndOpacity(FLinearColor{ 0.0f, 0.0f, 0.0f, 1.0f })
								// 			.Text(FText::FromString(TEXT("38.32% (123 of 240) unused assets")))
								// 		]
								// 	]
								// ]
								+ SVerticalBox::Slot()
								  .Padding(FMargin{10.0f, 0.0f})
								  .AutoHeight()
								[
									SNew(SHorizontalBox)
									+ SHorizontalBox::Slot()
									  .AutoWidth()
									  .HAlign(HAlign_Left)
									[
										SNew(STextBlock)
										.AutoWrapText(true)
										.Justification(ETextJustify::Center)
										.Text(FText::FromString(TEXT("Files - 25 (44.55 MiB)")))
										.ColorAndOpacity(FLinearColor{FColor::FromHex(TEXT("#3498DB"))})
										.Font(FProjectCleanerStyles::Get().GetFontStyle("ProjectCleaner.Font.Light20"))
									]
								]
								+ SVerticalBox::Slot()
								  .Padding(FMargin{40.0f, 0.0f, 0.0f, 0.0f})
								  .AutoHeight()
								[
									SNew(SHorizontalBox)
									+ SHorizontalBox::Slot()
									  .AutoWidth()
									  .HAlign(HAlign_Left)
									[
										SNew(STextBlock)
										.AutoWrapText(true)
										.Justification(ETextJustify::Center)
										.ColorAndOpacity(FLinearColor{FColor::FromHex(TEXT("#BDC3C7"))})
										.Text(FText::FromString(TEXT("Non engine files - 21 (44.55 MiB)")))
										.Font(FProjectCleanerStyles::Get().GetFontStyle("ProjectCleaner.Font.Light15"))
									]
								]
								+ SVerticalBox::Slot()
								  .Padding(FMargin{40.0f, 0.0f, 0.0f, 0.0f})
								  .AutoHeight()
								[
									SNew(SHorizontalBox)
									+ SHorizontalBox::Slot()
									  .AutoWidth()
									  .HAlign(HAlign_Left)
									[
										SNew(STextBlock)
										.AutoWrapText(true)
										.Justification(ETextJustify::Center)
										.ColorAndOpacity(FLinearColor{FColor::FromHex(TEXT("#E74C3C"))})
										.Text(FText::FromString(TEXT("Corrupted engine files - 3 (44.55 MiB)")))
										.Font(FProjectCleanerStyles::Get().GetFontStyle("ProjectCleaner.Font.Light15"))
									]
								]
								+ SVerticalBox::Slot()
								  .Padding(FMargin{10.0f, 0.0f})
								  .AutoHeight()
								[
									SNew(SHorizontalBox)
									+ SHorizontalBox::Slot()
									  .AutoWidth()
									  .HAlign(HAlign_Left)
									[
										SNew(STextBlock)
										.AutoWrapText(true)
										.Justification(ETextJustify::Center)
										.Text(FText::FromString(TEXT("Folders - 100")))
										.ColorAndOpacity(FLinearColor{FColor::FromHex(TEXT("#3498DB"))})
										.Font(FProjectCleanerStyles::Get().GetFontStyle("ProjectCleaner.Font.Light20"))
									]
								]
								+ SVerticalBox::Slot()
								  .Padding(FMargin{40.0f, 0.0f, 0.0f, 0.0f})
								  .AutoHeight()
								[
									SNew(SHorizontalBox)
									+ SHorizontalBox::Slot()
									  .AutoWidth()
									  .HAlign(HAlign_Left)
									[
										SNew(STextBlock)
										.AutoWrapText(true)
										.Justification(ETextJustify::Center)
										.ColorAndOpacity(FLinearColor{FColor::FromHex(TEXT("#E74C3C"))})
										.Text(FText::FromString(TEXT("Empty - 5")))
										.Font(FProjectCleanerStyles::Get().GetFontStyle("ProjectCleaner.Font.Light15"))
									]
								]
							]
						]
					]
				]
				+ SSplitter::Slot()
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("Content Browser TODO")))
				]
			]
		]
		+ SWidgetSwitcher::Slot()
		  .HAlign(HAlign_Fill)
		  .VAlign(VAlign_Fill)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			  .FillWidth(1.0f)
			  .HAlign(HAlign_Center)
			  .VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Justification(ETextJustify::Center)
				.Font(FProjectCleanerStyles::Get().GetFontStyle("ProjectCleaner.Font.Light30"))
				.Text(FText::FromString(ProjectCleanerConstants::MsgAssetRegistryStillWorking))
			]
		]
	];
}

FText SProjectCleanerWindowMain::GetNumAllAssets() const
{
	const FString SizeTotal = FText::AsMemory(10516565).ToString();
	return FText::FromString(FString::Printf(TEXT("Total: 250"))); // todo:ashe23
}

FText SProjectCleanerWindowMain::GetNumUsedAssets() const
{
	return FText::FromString(FString::Printf(TEXT("Used - 23 (53%%)"))); // todo:ashe23
}

FText SProjectCleanerWindowMain::GetNumUnusedAssets() const
{
	return FText::FromString(FString::Printf(TEXT("Unused - 10 (15%%)"))); // todo:ashe23
}

FText SProjectCleanerWindowMain::GetNumExcludedAssets() const
{
	return FText::FromString(FString::Printf(TEXT("Excluded - 10 (15%%)"))); // todo:ashe23
}

FText SProjectCleanerWindowMain::GetNumIndirectAssets() const
{
	return FText::FromString(FString::Printf(TEXT("Indirect - 10 (15%%)"))); // todo:ashe23
}

FText SProjectCleanerWindowMain::GetNumCorruptedAssets() const
{
	return FText::FromString(FString::Printf(TEXT("Corrupted assets: 10"))); // todo:ashe23
}

FText SProjectCleanerWindowMain::GetNumExternalFiles() const
{
	return FText::FromString(FString::Printf(TEXT("External files: 10"))); // todo:ashe23
}

FText SProjectCleanerWindowMain::GetNumAllFolders() const
{
	return FText::FromString(FString::Printf(TEXT("Folders: 10"))); // todo:ashe23
}

FText SProjectCleanerWindowMain::GetNumEmptyFolders() const
{
	return FText::FromString(FString::Printf(TEXT("Empty - 10 (15%%)"))); // todo:ashe23
}

FText SProjectCleanerWindowMain::GetSizeTotal() const
{
	const FString SizeTotal = FText::AsMemory(10516565).ToString();
	return FText::FromString(FString::Printf(TEXT("Sizes: %s"), *SizeTotal));
}

FText SProjectCleanerWindowMain::GetSizeUsed() const
{
	const FString SizeUsed = FText::AsMemory(10516565).ToString();
	return FText::FromString(FString::Printf(TEXT("Used - %s"), *SizeUsed));
}

FText SProjectCleanerWindowMain::GetSizeUnused() const
{
	const FString SizeUnused = FText::AsMemory(10516565).ToString();
	return FText::FromString(FString::Printf(TEXT("Unused - %s"), *SizeUnused));
}

FText SProjectCleanerWindowMain::GetSizeExcluded() const
{
	const FString SizeExcluded = FText::AsMemory(10516565).ToString();
	return FText::FromString(FString::Printf(TEXT("Excluded - %s"), *SizeExcluded));
}

FText SProjectCleanerWindowMain::GetSizeIndirect() const
{
	const FString SizeIndirect = FText::AsMemory(10516565).ToString();
	return FText::FromString(FString::Printf(TEXT("Indirect - %s"), *SizeIndirect));
}

void SProjectCleanerWindowMain::OnNavigateWiki()
{
	if (FPlatformProcess::CanLaunchURL(*ProjectCleanerConstants::UrlWiki))
	{
		FPlatformProcess::LaunchURL(*ProjectCleanerConstants::UrlWiki, nullptr, nullptr);
	}
}

bool SProjectCleanerWindowMain::IsWidgetEnabled()
{
	return !UProjectCleanerAssetLibrary::AssetRegistryIsLoadingAssets();
}

int32 SProjectCleanerWindowMain::GetWidgetIndex()
{
	return UProjectCleanerAssetLibrary::AssetRegistryIsLoadingAssets() ? WidgetIndexLoading : WidgetIndexNone;
}
