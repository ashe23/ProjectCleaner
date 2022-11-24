// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/ProjectCleanerStatListItem.h"
#include "ProjectCleanerStyles.h"
#include "ProjectCleanerTypes.h"

void SProjectCleanerStatListItem::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView)
{
	ListItem = InArgs._ListItem;

	SMultiColumnTableRow<TWeakObjectPtr<UProjectCleanerStatListItem>>::Construct(
		SMultiColumnTableRow<TWeakObjectPtr<UProjectCleanerStatListItem>>::FArguments()
		.Padding(FMargin{0.0f, 2.0f, 0.0f, 0.0f}),
		InOwnerTableView
	);
}

TSharedRef<SWidget> SProjectCleanerStatListItem::GenerateWidgetForColumn(const FName& InColumnName)
{
	if (InColumnName.IsEqual(TEXT("Name")))
	{
		return
			SNew(STextBlock)
				.AutoWrapText(false)
				.Justification(ETextJustify::Center)
				.Text(FText::FromString(ListItem->Name))
				.ColorAndOpacity(FLinearColor{FColor::FromHex(TEXT("#00a6fb"))})
				.Font(FProjectCleanerStyles::Get().GetFontStyle("ProjectCleaner.Font.Light13"));
	}

	if (InColumnName.IsEqual(TEXT("Category")))
	{
		return SNew(STextBlock).Text(FText::FromString(ListItem->Category)).ColorAndOpacity(ListItem->Color);
	}

	if (InColumnName.IsEqual(TEXT("Count")))
	{
		return SNew(STextBlock).Text(FText::FromString(ListItem->Count)).ColorAndOpacity(ListItem->Color);
	}

	if (InColumnName.IsEqual(TEXT("Size")))
	{
		return SNew(STextBlock).Text(FText::FromString(ListItem->Size)).ColorAndOpacity(ListItem->Color);
	}

	return SNew(STextBlock).Text(FText::FromString(TEXT("")));
}
