// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/SPjcItemTree.h"
#include "PjcStyles.h"
#include "PjcTypes.h"
// Engine Headers
#include "Widgets/Notifications/SProgressBar.h"

void SPjcItemTree::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InTable)
{
	Item = InArgs._Item;
	HighlightText = InArgs._HightlightText;

	SMultiColumnTableRow::Construct(SMultiColumnTableRow::FArguments().Padding(FMargin{0.0f, 2.0f, 0.0f, 0.0f}), InTable);
}

TSharedRef<SWidget> SPjcItemTree::GenerateWidgetForColumn(const FName& InColumnName)
{
	if (InColumnName.IsEqual(TEXT("Path")))
	{
		return
			SNew(SHorizontalBox).ToolTipText(FText::FromString(Item->FolderPath))
			+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin{2.0f})
			[
				SNew(SExpanderArrow, SharedThis(this)).IndentAmount(10).ShouldDrawWires(false)
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 2, 0).VAlign(VAlign_Center)
			[
				SNew(SImage).Image(GetFolderIcon()).ColorAndOpacity(GetFolderColor())
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin{2.0f})
			[
				SNew(STextBlock).Text(FText::FromString(Item->FolderName)).HighlightText(HighlightText)
			];
	}

	if (InColumnName.IsEqual(TEXT("NumAssetsTotal")))
	{
		return
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.0f).HAlign(HAlign_Center).VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.AutoWrapText(false)
				.Justification(ETextJustify::Center)
				.ColorAndOpacity(FLinearColor::White)
				.Text(FText::AsNumber(Item->NumAssetsTotal))
			];
	}

	if (InColumnName.IsEqual(TEXT("NumAssetsUsed")))
	{
		return
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.0f).HAlign(HAlign_Center).VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.AutoWrapText(false)
				.Justification(ETextJustify::Center)
				.ColorAndOpacity(FLinearColor::White)
				.Text(FText::AsNumber(Item->NumAssetsUsed))
			];
	}

	if (InColumnName.IsEqual(TEXT("NumAssetsUnused")))
	{
		return
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.0f).HAlign(HAlign_Center).VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.AutoWrapText(false)
				.Justification(ETextJustify::Center)
				.ColorAndOpacity(FLinearColor::White)
				.Text(FText::AsNumber(Item->NumAssetsUnused))
			];
	}

	if (InColumnName.IsEqual(TEXT("UnusedPercent")))
	{
		FNumberFormattingOptions FormattingOptions;
		FormattingOptions.UseGrouping = true;
		FormattingOptions.MinimumFractionalDigits = 2;
		FormattingOptions.MaximumFractionalDigits = 2;

		const FString StrPercent = FText::AsNumber(Item->PercentageUnused, &FormattingOptions).ToString() + TEXT(" %");

		return
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().Padding(FMargin{5.0f, 1.0f}).FillWidth(1.0f)
			[
				SNew(SOverlay)
				+ SOverlay::Slot().HAlign(HAlign_Fill).VAlign(VAlign_Fill)
				[
					SNew(SProgressBar)
					.BorderPadding(FVector2D{0.0f, 0.0f})
					.Percent(Item->PercentageUnusedNormalized)
					.BackgroundImage(FPjcStyles::Get().GetBrush("ProjectCleaner.BgProgressbar"))
					.FillImage(FPjcStyles::Get().GetBrush("ProjectCleaner.BgWhite"))
					.FillColorAndOpacity(FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Red"))
				]
				+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.AutoWrapText(false)
					.ColorAndOpacity(FLinearColor::White)
					.Text(FText::FromString(StrPercent))
				]
			];
	}

	if (InColumnName.IsEqual(TEXT("UnusedSize")))
	{
		return
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().Padding(FMargin{5.0f, 1.0f}).FillWidth(1.0f)
			[
				SNew(STextBlock)
				.AutoWrapText(false)
				.ColorAndOpacity(FLinearColor::White)
				.Justification(ETextJustify::Center)
				.ColorAndOpacity(FLinearColor::White)
				.Text(FText::AsMemory(Item->SizeAssetsUnused, IEC))
			];
	}

	return SNew(STextBlock).Text(FText::FromString(TEXT("")));
}

const FSlateBrush* SPjcItemTree::GetFolderIcon() const
{
	return FAppStyle::GetBrush(Item->bIsExpanded ? TEXT("ContentBrowser.AssetTreeFolderOpen") : TEXT("ContentBrowser.AssetTreeFolderClosed"));
}

FSlateColor SPjcItemTree::GetFolderColor() const
{
	if (Item->bIsExcluded)
	{
		return FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Yellow");
	}

	if (Item->bIsDev)
	{
		return FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Blue");
	}

	if (Item->bIsEmpty)
	{
		return FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Red");
	}

	return FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Gray");
}
