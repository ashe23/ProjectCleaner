// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/SPjcItemUsage.h"
#include "PjcStyles.h"
#include "PjcTypes.h"
// Engine Headers
#include "Misc/PackageName.h"
#include "Styling/SlateIconFinder.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

void SPjcItemUsage::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InTable) {
	Item = InArgs._Item;
	HighlightText = InArgs._HighlightText;

	SMultiColumnTableRow::Construct(SMultiColumnTableRow::FArguments().Padding(FMargin {0.0f, 2.0f, 0.0f, 0.0f}), InTable);
}

TSharedRef<SWidget> SPjcItemUsage::GenerateWidgetForColumn(const FName& InColumnName) {
	if (!Item.IsValid()) return SNew(STextBlock);

	if (InColumnName.IsEqual(TEXT("Name"))) {
		const UClass* AssetClass = Item->Asset.IsValid() ? Item->Asset.GetClass() : nullptr;

		// clang-format off
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(FMargin{5.0f, 0.0f, 3.0f, 0.0f})
			.VAlign(VAlign_Center)
			[
				SNew(SBox).WidthOverride(8.0f).HeightOverride(8.0f)
				[
					SNew(SImage)
					.Image(FPjcStyles::Get().GetBrush("ProjectCleaner.BgWhite"))
					.ColorAndOpacity(GetStatusColor(*Item))
				]
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(FMargin{3.0f, 0.0f})
			.VAlign(VAlign_Center)
			[
				SNew(SBox).WidthOverride(16.0f).HeightOverride(16.0f)
				[
					SNew(SImage).Image(FSlateIconFinder::FindIconBrushForClass(AssetClass))
				]
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.Padding(FMargin{3.0f, 0.0f})
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString(GetItemName(*Item)))
				.HighlightText(HighlightText)
				.ToolTipText(FText::FromName(Item->PackageName))
			];
		// clang-format on
	}

	if (InColumnName.IsEqual(TEXT("Class"))) {
		return SNew(STextBlock)
			.Text(FText::FromString(Item->ClassName))
			.HighlightText(HighlightText)
			.ColorAndOpacity(FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Gray"));
	}

	if (InColumnName.IsEqual(TEXT("Status"))) {
		return SNew(STextBlock).Text(GetStatusText(*Item)).ColorAndOpacity(GetStatusColor(*Item));
	}

	if (InColumnName.IsEqual(TEXT("Why"))) {
		return SNew(STextBlock)
			.Text(FText::FromString(Item->ReasonText))
			.HighlightText(HighlightText)
			.ToolTipText(FText::FromString(Item->ReasonText));
	}

	return SNew(STextBlock);
}

FString SPjcItemUsage::GetItemName(const FPjcUsageItem& InItem) {
	return InItem.Asset.IsValid() ? InItem.Asset.AssetName.ToString() : FPackageName::GetShortName(InItem.PackageName);
}

FText SPjcItemUsage::GetStatusText(const FPjcUsageItem& InItem) {
	if (!InItem.bScanned) return FText::FromString(TEXT("Not scanned"));
	if (InItem.Usage.bUsed) return FText::FromString(TEXT("Used"));
	if (!InItem.Kept.IsEmpty()) return FText::FromString(TEXT("Not deleted"));

	return FText::FromString(TEXT("Unused"));
}

FLinearColor SPjcItemUsage::GetStatusColor(const FPjcUsageItem& InItem) {
	if (!InItem.bScanned) return FPjcStyles::Get().GetColor("ProjectCleaner.Color.Gray");
	if (InItem.Usage.bUsed) return FPjcStyles::Get().GetColor("ProjectCleaner.Color.Green");
	if (!InItem.Kept.IsEmpty()) return FPjcStyles::Get().GetColor("ProjectCleaner.Color.Yellow");

	return FPjcStyles::Get().GetColor("ProjectCleaner.Color.Red");
}

int32 SPjcItemUsage::GetStatusOrder(const FPjcUsageItem& InItem) {
	if (!InItem.bScanned) return 3;
	if (InItem.Usage.bUsed) return 2;
	if (!InItem.Kept.IsEmpty()) return 0;

	return 1;
}
