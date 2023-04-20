// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/AssetBrowser/SPjcTreeViewItem.h"

#include "PjcStyles.h"
#include "Widgets/Notifications/SProgressBar.h"

void SPjcTreeViewItem::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InTable)
{
	Item = InArgs._Item;

	SMultiColumnTableRow::Construct(SMultiColumnTableRow::FArguments().Padding(FMargin{0.0f, 2.0f, 0.0f, 0.0f}), InTable);
}

TSharedRef<SWidget> SPjcTreeViewItem::GenerateWidgetForColumn(const FName& InColumnName)
{
	if (InColumnName.IsEqual(TEXT("Path")))
	{
		return
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().HAlign(HAlign_Left).VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Font(FPjcStyles::GetFont("Bold", 12))
				.Text(FText::FromString(Item->PathName))
				.ToolTipText(FText::FromString(Item->PathContent))
			];
	}

	if (InColumnName.IsEqual(TEXT("UnusedSize")))
	{
		return
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().Padding(FMargin{20.0f, 1.0f}).FillWidth(1.0f)
			[
				SNew(SOverlay)
				+ SOverlay::Slot().HAlign(HAlign_Fill).VAlign(VAlign_Fill)
				[
					SNew(SProgressBar)
					.BorderPadding(FVector2D{0.0f, 0.0f})
					.Percent(Item->Percentage)
					.BackgroundImage(FPjcStyles::Get().GetBrush("ProjectCleaner.Progressbar"))
					.FillColorAndOpacity(FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Red"))
				]
				+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.AutoWrapText(false)
					.ColorAndOpacity(FLinearColor::White)
					.Text(FText::AsMemory(Item->UnusedSize, IEC))
				]
			];
	}

	return SNew(STextBlock).Text(FText::FromString(TEXT("")));
}
