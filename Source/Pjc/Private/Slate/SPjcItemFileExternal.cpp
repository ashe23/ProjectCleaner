// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/SPjcItemFileExternal.h"
#include "PjcTypes.h"
#include "PjcSubsystem.h"
// Engine Headers
#include "Widgets/Input/SHyperlink.h"

void SPjcItemFileExternal::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InTable)
{
	Item = InArgs._Item;
	TextHighlight = InArgs._TextHighlight;

	SMultiColumnTableRow::Construct(SMultiColumnTableRow::FArguments().Padding(FMargin{0.0f, 2.0f, 0.0f, 0.0f}), InTable);
}

TSharedRef<SWidget> SPjcItemFileExternal::GenerateWidgetForColumn(const FName& InColumnName)
{
	const float ItemRenderOpacity = Item->bExcluded ? 0.5f : 1.0f;

	if (InColumnName.IsEqual(TEXT("FilePath")))
	{
		return
			SNew(SHorizontalBox).RenderOpacity(ItemRenderOpacity)
			+ SHorizontalBox::Slot().FillWidth(1.0f).HAlign(HAlign_Left).VAlign(VAlign_Center).Padding(FMargin{5.0f, 0.0f})
			[
				SNew(SHyperlink)
				.Text(FText::FromString(Item->FilePath))
				.OnNavigate_Lambda([&]() { UPjcSubsystem::OpenPathInFileExplorer(Item->FilePath); })
			];
	}

	if (InColumnName.IsEqual(TEXT("FileName")))
	{
		return SNew(STextBlock).Text(FText::FromString(Item->FileName)).RenderOpacity(ItemRenderOpacity).HighlightText(TextHighlight);
	}

	if (InColumnName.IsEqual(TEXT("FileExt")))
	{
		return SNew(STextBlock).Text(FText::FromString(TEXT(".") + Item->FileExt)).RenderOpacity(ItemRenderOpacity);
	}

	if (InColumnName.IsEqual(TEXT("FileSize")))
	{
		return SNew(STextBlock).Text(FText::AsMemory(Item->FileSize, IEC)).RenderOpacity(ItemRenderOpacity);
	}

	return SNew(STextBlock).Text(FText::GetEmpty());
}
