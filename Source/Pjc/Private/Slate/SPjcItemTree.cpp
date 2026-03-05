// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/SPjcItemTree.h"
#include "PjcStyles.h"
#include "PjcShim.h"
#include "PjcTypes.h"
// Engine Headers
#include "Widgets/Notifications/SProgressBar.h"

void SPjcItemTree::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InTable) {
	Item = InArgs._Item;
	HighlightText = InArgs._HightlightText;

	SMultiColumnTableRow::Construct(SMultiColumnTableRow::FArguments().Padding(FMargin {0.0f, 2.0f, 0.0f, 0.0f}), InTable);
}

TSharedRef<SWidget> SPjcItemTree::GenerateWidgetForColumn(const FName& InColumnName) {
	if (InColumnName.IsEqual(TEXT("Path"))) {
		// clang-format off
		return SNew(SHorizontalBox).ToolTipText(FText::FromString(Item->FolderPath))
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(FMargin{2.0f})
			[
				SNew(SExpanderArrow, SharedThis(this)).IndentAmount(10).ShouldDrawWires(false)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0, 0, 2, 0)
			.VAlign(VAlign_Center)
			[
				SNew(SImage).Image(GetFolderIcon()).ColorAndOpacity(GetFolderColor())
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(FMargin{2.0f})
			[
				SNew(STextBlock).Text(FText::FromString(Item->FolderName)).HighlightText(HighlightText)
			];
		// clang-format on
	}

	if (InColumnName.IsEqual(TEXT("NumAssetsTotal"))) {
		// clang-format off
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.AutoWrapText(false)
				.Justification(ETextJustify::Center)
				.ColorAndOpacity(FLinearColor::White)
				.Text(FText::AsNumber(Item->NumAssetsTotal))
			];
		// clang-format on
	}

	if (InColumnName.IsEqual(TEXT("NumAssetsUsed"))) {
		// clang-format off
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.AutoWrapText(false)
				.Justification(ETextJustify::Center)
				.ColorAndOpacity(FLinearColor::White)
				.Text(FText::AsNumber(Item->NumAssetsUsed))
			];
		// clang-format on
	}

	if (InColumnName.IsEqual(TEXT("NumAssetsUnused"))) {
		// clang-format off
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.AutoWrapText(false)
				.Justification(ETextJustify::Center)
				.ColorAndOpacity(FLinearColor::White)
				.Text(FText::AsNumber(Item->NumAssetsUnused))
			];
		// clang-format on
	}

	if (InColumnName.IsEqual(TEXT("UnusedPercent"))) {
		FNumberFormattingOptions FormattingOptions;
		FormattingOptions.UseGrouping = true;
		FormattingOptions.MinimumFractionalDigits = 2;
		FormattingOptions.MaximumFractionalDigits = 2;

		const FString StrPercent = FText::AsNumber(Item->PercentageUnused, &FormattingOptions).ToString() + TEXT(" %");

		// clang-format off
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.Padding(FMargin{5.0f, 1.0f})
			.FillWidth(1.0f)
			[
				SNew(SOverlay)
				+ SOverlay::Slot()
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Fill)
				[
					SNew(SProgressBar)
					.BorderPadding(FVector2D{0.0f, 0.0f})
					.Percent(Item->PercentageUnusedNormalized)
					.BackgroundImage(FPjcStyles::Get().GetBrush("ProjectCleaner.BgProgressbar"))
					.FillColorAndOpacity(FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Red"))
				]
				+ SOverlay::Slot()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.AutoWrapText(false)
					.ColorAndOpacity(FLinearColor::White)
					.Text(FText::FromString(StrPercent))
				]
			];
		// clang-format on
	}

	if (InColumnName.IsEqual(TEXT("UnusedSize"))) {
		// clang-format off
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.Padding(FMargin{5.0f, 1.0f})
			.FillWidth(1.0f)
			[
				SNew(STextBlock)
				.AutoWrapText(false)
				.ColorAndOpacity(FLinearColor::White)
				.Justification(ETextJustify::Center)
				.ColorAndOpacity(FLinearColor::White)
				.Text(FText::AsMemory(Item->SizeAssetsUnused, IEC))
			];
		// clang-format on
	}

	return SNew(STextBlock).Text(FText::FromString(TEXT("")));
}

const FSlateBrush* SPjcItemTree::GetFolderIcon() const {
	return PjcShim::GetBrush(Item->bIsExpanded ? TEXT("ContentBrowser.AssetTreeFolderOpen") : TEXT("ContentBrowser.AssetTreeFolderClosed"));
}

FSlateColor SPjcItemTree::GetFolderColor() const {
	if (Item->bIsExcluded) {
		return FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Yellow");
	}

	if (Item->bIsDev) {
		return FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Blue");
	}

	if (Item->bIsEmpty) {
		return FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Red");
	}

	return FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Gray");
}
