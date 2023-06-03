// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/SPjcItemStat.h"
#include "PjcStyles.h"
#include "PjcTypes.h"

void SPjcItemStat::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InTable)
{
	Item = InArgs._Item;

	SMultiColumnTableRow::Construct(SMultiColumnTableRow::FArguments().Padding(FMargin{0.0f, 2.0f, 0.0f, 0.0f}), InTable);
}

TSharedRef<SWidget> SPjcItemStat::GenerateWidgetForColumn(const FName& InColumnName)
{
	if (InColumnName.IsEqual(TEXT("Name")))
	{
		return
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(Item->NamePadding)
			[
				SNew(STextBlock)
				.Font(FPjcStyles::GetFont("Bold", 12))
				.Text(Item->Name)
				.ColorAndOpacity(Item->TextColor)
				.ToolTipText(Item->TooltipName)
			];
	}

	if (InColumnName.IsEqual(TEXT("Num")))
	{
		return
			SNew(STextBlock)
			.Justification(ETextJustify::Center)
			.Text(Item->Num)
			.ColorAndOpacity(Item->TextColor)
			.ToolTipText(Item->ToolTipNum);
	}

	if (InColumnName.IsEqual(TEXT("Size")))
	{
		return
			SNew(STextBlock)
			.Justification(ETextJustify::Center)
			.Text(Item->Size)
			.ColorAndOpacity(Item->TextColor)
			.ToolTipText(Item->ToolTipSize);
	}

	return SNew(STextBlock).Text(FText::FromString(TEXT("")));
}
