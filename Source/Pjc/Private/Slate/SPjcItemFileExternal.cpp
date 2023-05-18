// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/SPjcItemFileExternal.h"
#include "PjcTypes.h"
// Engine Headers
#include "PjcSubsystem.h"
#include "Widgets/Input/SHyperlink.h"

void SPjcItemFileExternal::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InTable)
{
	Item = InArgs._Item;
	SMultiColumnTableRow::Construct(SMultiColumnTableRow::FArguments().Padding(FMargin{0.0f, 2.0f, 0.0f, 0.0f}), InTable);
}

TSharedRef<SWidget> SPjcItemFileExternal::GenerateWidgetForColumn(const FName& InColumnName)
{
	if (InColumnName.IsEqual(TEXT("FilePath")))
	{
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.0f).HAlign(HAlign_Left).VAlign(VAlign_Center)
			[
				SNew(SHyperlink)
				.Text(FText::FromString(Item->FilePath))
				.OnNavigate_Lambda([&]()
				                {
					                UPjcSubsystem::OpenPathInFileExplorer(Item->FilePath);
				                })
			];
	}

	if (InColumnName.IsEqual(TEXT("FileName")))
	{
		return SNew(STextBlock).Text(FText::FromString(Item->FileName));
	}

	if (InColumnName.IsEqual(TEXT("FileExt")))
	{
		return SNew(STextBlock).Text(FText::FromString(Item->FileExt));
	}

	if (InColumnName.IsEqual(TEXT("FileSize")))
	{
		return SNew(STextBlock).Text(FText::AsMemory(Item->FileSize, IEC));
	}

	return SNew(STextBlock).Text(FText::GetEmpty());
}
