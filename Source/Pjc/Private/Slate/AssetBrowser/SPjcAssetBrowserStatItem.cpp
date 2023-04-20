// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/AssetBrowser/SPjcAssetBrowserStatItem.h"
#include "PjcStyles.h"

void SPjcAssetBrowserStatItem::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InTable)
{
	Item = InArgs._Item;

	// todo:ashe23 change row style color for 'total'
	SMultiColumnTableRow::Construct(SMultiColumnTableRow::FArguments().Padding(FMargin{0.0f, 2.0f, 0.0f, 0.0f}), InTable);
}

TSharedRef<SWidget> SPjcAssetBrowserStatItem::GenerateWidgetForColumn(const FName& InColumnName)
{
	if (InColumnName.IsEqual(TEXT("Category")))
	{
		return
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(Item->CategoryPadding)
			[
				SNew(STextBlock)
				.Font(FPjcStyles::GetFont("Bold", 12))
				.Text(FText::FromString(Item->Category))
				.ColorAndOpacity(Item->TextColor)
				.ToolTipText(FText::FromString(Item->CategoryToolTip))
			];
	}

	if (InColumnName.IsEqual(TEXT("Num")))
	{
		return SNew(STextBlock).Text(FText::AsNumber(Item->Num)).ColorAndOpacity(Item->TextColor);
	}

	if (InColumnName.IsEqual(TEXT("Size")))
	{
		return SNew(STextBlock).Text(FText::AsMemory(Item->Size, IEC)).ColorAndOpacity(Item->TextColor);
	}

	return SNew(STextBlock).Text(FText::FromString(TEXT("")));
}
