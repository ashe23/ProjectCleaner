// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/AssetStats/SPjcAssetStatsItem.h"
#include "PjcStyles.h"

void SPjcAssetStatsItem::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InTable)
{
	Item = InArgs._Item;

	SMultiColumnTableRow::Construct(SMultiColumnTableRow::FArguments().Padding(FMargin{0.0f, 2.0f, 0.0f, 0.0f}), InTable);
}

TSharedRef<SWidget> SPjcAssetStatsItem::GenerateWidgetForColumn(const FName& InColumnName)
{
	const FLinearColor TextColor = Item->Num > 0 ? Item->TextColorActive : Item->TextColorDefault;

	if (InColumnName.IsEqual(TEXT("Category")))
	{
		return
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(Item->CategoryPadding)
			[
				SNew(STextBlock)
				.Font(FPjcStyles::GetFont("Bold", 12))
				.Text(FText::FromString(Item->Category))
				.ColorAndOpacity(TextColor)
				.ToolTipText(FText::FromString(Item->ToolTipCategory))
			];
	}

	if (InColumnName.IsEqual(TEXT("Num")))
	{
		return
			SNew(STextBlock)
			.Justification(ETextJustify::Center)
			.Text(FText::AsNumber(Item->Num)).ColorAndOpacity(TextColor)
			.ToolTipText(FText::FromString(Item->ToolTipNum));
	}

	if (InColumnName.IsEqual(TEXT("Size")))
	{
		return
			SNew(STextBlock)
			.Justification(ETextJustify::Center)
			.Text(FText::AsMemory(Item->Size, IEC))
			.ColorAndOpacity(TextColor)
			.ToolTipText(FText::FromString(Item->ToolTipSize));
	}

	return SNew(STextBlock).Text(FText::FromString(TEXT("")));
}
