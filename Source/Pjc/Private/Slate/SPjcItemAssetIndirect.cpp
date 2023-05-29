// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/SPjcItemAssetIndirect.h"
#include "PjcSubsystem.h"
#include "PjcTypes.h"
// Engine Headers
#include "Widgets/Input/SHyperlink.h"

void SPjcItemAssetIndirect::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InTable)
{
	Item = InArgs._Item;
	HighlightText = InArgs._HighlightText;

	SMultiColumnTableRow::Construct(SMultiColumnTableRow::FArguments().Padding(FMargin{0.0f, 2.0f, 0.0f, 0.0f}), InTable);
}

TSharedRef<SWidget> SPjcItemAssetIndirect::GenerateWidgetForColumn(const FName& InColumnName)
{
	if (InColumnName.IsEqual(TEXT("AssetName")))
	{
		const TSharedPtr<FAssetThumbnail> AssetThumbnail = MakeShareable(new FAssetThumbnail(Item->Asset, 16, 16, nullptr));
		const FAssetThumbnailConfig ThumbnailConfig;

		return
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(5.0, 0.0f).HAlign(HAlign_Left).VAlign(VAlign_Center)
			[
				SNew(SBox)
				.WidthOverride(16)
				.HeightOverride(16)
				[
					AssetThumbnail->MakeThumbnailWidget(ThumbnailConfig)
				]
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(5.0f, 0.0f).HAlign(HAlign_Left).VAlign(VAlign_Center)
			[
				SNew(STextBlock).Justification(ETextJustify::Center).Text(FText::FromName(Item->Asset.AssetName)).HighlightText(HighlightText)
			];
	}

	if (InColumnName.IsEqual(TEXT("AssetPath")))
	{
		return
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(5.0, 0.0f).HAlign(HAlign_Left).VAlign(VAlign_Center)
			[
				SNew(STextBlock).Justification(ETextJustify::Center).Text(FText::FromName(Item->Asset.PackagePath)).HighlightText(HighlightText)
			];
	}

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
