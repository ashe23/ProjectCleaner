// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/SPjcItemFileInfo.h"

#include "PjcSubsystem.h"
#include "PjcTypes.h"
#include "Widgets/Input/SHyperlink.h"

void SPjcItemFileInfo::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InTable)
{
	Item = InArgs._Item;
	SMultiColumnTableRow::Construct(SMultiColumnTableRow::FArguments().Padding(FMargin{0.0f, 2.0f, 0.0f, 0.0f}), InTable);
}

TSharedRef<SWidget> SPjcItemFileInfo::GenerateWidgetForColumn(const FName& InColumnName)
{
	if (InColumnName.IsEqual(TEXT("FilePath")))
	{
		return
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.0f).HAlign(HAlign_Left).VAlign(VAlign_Center).Padding(FMargin{5.0f, 0.0f})
			[
				SNew(SHyperlink)
				.Text(FText::FromString(Item->FilePath))
				.OnNavigate_Lambda([&]() { UPjcSubsystem::OpenPathInFileExplorer(Item->FilePath); })
			];
	}

	if (InColumnName.IsEqual(TEXT("FileLine")))
	{
		return SNew(STextBlock).Justification(ETextJustify::Center).Text(FText::AsNumber(Item->FileNum));
	}

	return SNew(STextBlock).Text(FText::FromString(TEXT("")));
}
