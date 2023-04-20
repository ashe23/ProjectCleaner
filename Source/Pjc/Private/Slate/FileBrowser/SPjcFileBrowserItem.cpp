// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/FileBrowser/SPjcFileBrowserItem.h"
#include "PjcStyles.h"
#include "Libs/PjcLibEditor.h"
// Engine Headers
#include "Widgets/Input/SHyperlink.h"

void SPjcFileBrowserItem::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InTable)
{
	Item = InArgs._Item;
	HighlightText = InArgs._HighlightText;

	SMultiColumnTableRow::Construct(SMultiColumnTableRow::FArguments().Padding(FMargin{0.0f, 2.0f, 0.0f, 0.0f}), InTable);
}

TSharedRef<SWidget> SPjcFileBrowserItem::GenerateWidgetForColumn(const FName& InColumnName)
{
	if (InColumnName.IsEqual(TEXT("FileName")))
	{
		return
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin{5.0f, 0.0f})
			[
				SNew(STextBlock).Text(FText::FromString(Item->FileName)).HighlightText(FText::FromString(HighlightText))
			];
	}

	if (InColumnName.IsEqual(TEXT("FileExt")))
	{
		return SNew(STextBlock).Text(FText::FromString(TEXT(".") + Item->FileExtension)).HighlightText(FText::FromString(HighlightText));
	}

	if (InColumnName.IsEqual(TEXT("FileSize")))
	{
		return SNew(STextBlock).Text(FText::AsMemory(Item->FileSize, IEC));
	}

	if (InColumnName.IsEqual(TEXT("FileType")))
	{
		const bool bIsCorruptedAssetFile = Item->FileType == EPjcBrowserItemFileType::Corrupted;
		const FString ColorSpecifier = bIsCorruptedAssetFile ? TEXT("ProjectCleaner.Color.Red") : TEXT("ProjectCleaner.Color.Blue");
		const FString Title = bIsCorruptedAssetFile ? TEXT("Corrupted Asset File") : TEXT("External");

		return
			SNew(SBorder)
			.BorderImage(FPjcStyles::Get().GetBrush(TEXT("ProjectCleaner.BgWhite")))
			.BorderBackgroundColor(FPjcStyles::Get().GetSlateColor(*ColorSpecifier))
			.ColorAndOpacity(FLinearColor::White)
			[
				SNew(STextBlock)
				.Font(FPjcStyles::GetFont("Bold", 9))
				.Text(FText::FromString(Title))
				.ColorAndOpacity(FLinearColor::White)
			];
	}

	if (InColumnName.IsEqual(TEXT("FilePath")))
	{
		return
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin{5.0f, 0.0f})
			[
				SNew(SHyperlink).Text(FText::FromString(Item->FilePath)).OnNavigate_Lambda([&]()
				{
					FPjcLibEditor::NavigateToPathInFileExplorer(Item->FilePath);
				})
			];
	}

	return SNew(STextBlock).Text(FText::FromString(""));
}
