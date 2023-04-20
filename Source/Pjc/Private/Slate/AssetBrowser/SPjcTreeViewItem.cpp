﻿// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/AssetBrowser/SPjcTreeViewItem.h"

#include "PjcStyles.h"
#include "Widgets/Notifications/SProgressBar.h"

void SPjcTreeViewItem::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InTable)
{
	Item = InArgs._Item;
	HighlightText = InArgs._HighlightText;

	SMultiColumnTableRow::Construct(SMultiColumnTableRow::FArguments().Padding(FMargin{0.0f, 2.0f, 0.0f, 0.0f}), InTable);
}

TSharedRef<SWidget> SPjcTreeViewItem::GenerateWidgetForColumn(const FName& InColumnName)
{
	if (InColumnName.IsEqual(TEXT("Path")))
	{
		return
			SNew(SHorizontalBox).ToolTipText(FText::FromString(Item->PathContent))
			+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin{2.0f})
			[
				SNew(SExpanderArrow, SharedThis(this)).IndentAmount(10).ShouldDrawWires(false)
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 2, 0).VAlign(VAlign_Center)
			[
				SNew(SImage).Image(GetFolderIcon()).ColorAndOpacity(GetFolderColor())
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin{2.0f})
			[
				SNew(STextBlock).Text(FText::FromString(Item->PathName)).HighlightText(FText::FromString(HighlightText))
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

const FSlateBrush* SPjcTreeViewItem::GetFolderIcon() const
{
	if (Item->bIsDev)
	{
		return FEditorStyle::GetBrush(TEXT("ContentBrowser.AssetTreeFolderDeveloper"));
	}

	return FEditorStyle::GetBrush(Item->bIsExpanded ? TEXT("ContentBrowser.AssetTreeFolderOpen") : TEXT("ContentBrowser.AssetTreeFolderClosed"));
}

FSlateColor SPjcTreeViewItem::GetFolderColor() const
{
	if (Item->bIsExcluded)
	{
		return FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Yellow");
	}

	if (Item->bIsEmpty)
	{
		return FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Red");
	}

	return FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Gray");
}
