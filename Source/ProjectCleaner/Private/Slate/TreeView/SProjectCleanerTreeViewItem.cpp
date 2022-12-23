// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/TreeView/SProjectCleanerTreeViewItem.h"
#include "ProjectCleanerSubsystem.h"
#include "ProjectCleanerTypes.h"
#include "ProjectCleanerStyles.h"
// Engine Headers
#include "Kismet/KismetMathLibrary.h"
#include "Widgets/Notifications/SProgressBar.h"

void SProjectCleanerTreeViewItem::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& OwnerTable)
{
	TreeItem = InArgs._TreeItem;
	SearchText = InArgs._SearchText;

	SMultiColumnTableRow::Construct(SMultiColumnTableRow::FArguments().ToolTipText(FText::FromString(TreeItem->FolderPathRel)), OwnerTable);
}

TSharedRef<SWidget> SProjectCleanerTreeViewItem::GenerateWidgetForColumn(const FName& InColumnName)
{
	if (InColumnName.IsEqual(TEXT("Name")))
	{
		return
			SNew(SHorizontalBox)
			.ToolTipText(FText::FromString(TreeItem->FolderPathRel))
			+ SHorizontalBox::Slot()
			  .AutoWidth()
			  .Padding(FMargin{2.0f})
			[
				SNew(SExpanderArrow, SharedThis(this))
				.IndentAmount(10)
				.ShouldDrawWires(GetDefault<UProjectCleanerSubsystem>()->bShowTreeViewLines)
			]
			+ SHorizontalBox::Slot()
			  .AutoWidth()
			  .Padding(0, 0, 2, 0)
			  .VAlign(VAlign_Center)
			[
				// Folder Icon
				SNew(SImage)
				.Image(this, &SProjectCleanerTreeViewItem::GetFolderIcon)
				.ColorAndOpacity(this, &SProjectCleanerTreeViewItem::GetFolderColor)
			]
			+ SHorizontalBox::Slot()
			  .AutoWidth()
			  .Padding(FMargin{5.0f})
			[
				SNew(STextBlock).Text(FText::FromString(TreeItem->FolderName)).HighlightText(FText::FromString(SearchText))
			];
	}

	if (InColumnName.IsEqual(TEXT("FoldersTotal")))
	{
		return
			SNew(SHorizontalBox)
			.ToolTipText(FText::FromString(TEXT("Number of folders in this path")))
			+ SHorizontalBox::Slot()
			  .FillWidth(1.0f)
			  .HAlign(HAlign_Center)
			  .VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Justification(ETextJustify::Center)
				.Text(FText::FromString(FString::Printf(TEXT("%d"), TreeItem->FoldersTotal)))
			];
	}

	if (InColumnName.IsEqual(TEXT("FoldersEmpty")))
	{
		return
			SNew(SHorizontalBox)
			.ToolTipText(FText::FromString(TEXT("Number of empty folders in this path")))
			+ SHorizontalBox::Slot()
			  .FillWidth(1.0f)
			  .HAlign(HAlign_Center)
			  .VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Justification(ETextJustify::Center)
				.Text(FText::FromString(FString::Printf(TEXT("%d"), TreeItem->FoldersEmpty)))
			];
	}

	if (InColumnName.IsEqual(TEXT("AssetsTotal")))
	{
		return
			SNew(SHorizontalBox)
			.ToolTipText(FText::FromString(TEXT("Number of assets in this path")))
			+ SHorizontalBox::Slot()
			  .FillWidth(1.0f)
			  .HAlign(HAlign_Center)
			  .VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Justification(ETextJustify::Center)
				.Text(FText::FromString(FString::Printf(TEXT("%d"), TreeItem->AssetsTotal)))
			];
	}

	if (InColumnName.IsEqual(TEXT("AssetsUnused")))
	{
		return
			SNew(SHorizontalBox)
			.ToolTipText(FText::FromString(TEXT("Number of unused assets in this path")))
			+ SHorizontalBox::Slot()
			  .FillWidth(1.0f)
			  .HAlign(HAlign_Center)
			  .VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Justification(ETextJustify::Center)
				.Text(FText::FromString(FString::Printf(TEXT("%d"), TreeItem->AssetsUnused)))
			];
	}

	if (InColumnName.IsEqual(TEXT("SizeTotal")))
	{
		return
			SNew(SHorizontalBox)
			.ToolTipText(FText::FromString(TEXT("Total size of all assets in this path")))
			+ SHorizontalBox::Slot()
			  .FillWidth(1.0f)
			  .HAlign(HAlign_Center)
			  .VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Justification(ETextJustify::Center)
				.Text(FText::AsMemory(TreeItem->SizeTotal))
			];
	}

	if (InColumnName.IsEqual(TEXT("SizeUnused")))
	{
		return
			SNew(SHorizontalBox)
			.ToolTipText(FText::FromString(TEXT("Total size of unused assets in this path")))
			+ SHorizontalBox::Slot()
			  .FillWidth(1.0f)
			  .HAlign(HAlign_Center)
			  .VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Justification(ETextJustify::Center)
				.Text(FText::AsMemory(TreeItem->SizeUnused))
			];
	}

	if (InColumnName.IsEqual(TEXT("Percent")))
	{
		return
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			  .Padding(FMargin{20.0f, 5.0f})
			  .FillWidth(1.0f)
			  .HAlign(HAlign_Fill)
			  .VAlign(VAlign_Fill)
			[
				SNew(SOverlay)
				+ SOverlay::Slot()
				  .HAlign(HAlign_Fill)
				  .VAlign(VAlign_Fill)
				[
					SNew(SProgressBar)
					.Percent(TreeItem->PercentUnusedNormalized)
					.FillColorAndOpacity_Raw(this, &SProjectCleanerTreeViewItem::GetProgressBarColor)
				]
				+ SOverlay::Slot()
				  .HAlign(HAlign_Center)
				  .VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.AutoWrapText(false)
					.ColorAndOpacity(FLinearColor{0.0f, 0.0f, 0.0f, 1.0f})
					.Text(FText::FromString(FString::Printf(TEXT("%.2f %%"), TreeItem->PercentUnused)))
				]
			];
	}

	return SNew(STextBlock).Text(FText::FromString(TEXT("")));
}

const FSlateBrush* SProjectCleanerTreeViewItem::GetFolderIcon() const
{
	if (TreeItem->bDevFolder)
	{
		return FEditorStyle::GetBrush(TEXT("ContentBrowser.AssetTreeFolderDeveloper"));
	}

	return FEditorStyle::GetBrush(TreeItem->bExpanded ? TEXT("ContentBrowser.AssetTreeFolderOpen") : TEXT("ContentBrowser.AssetTreeFolderClosed"));
}

FSlateColor SProjectCleanerTreeViewItem::GetFolderColor() const
{
	if (TreeItem->bExcluded)
	{
		return FProjectCleanerStyles::Get().GetSlateColor("ProjectCleaner.Color.Yellow");
	}

	if (TreeItem->bEmpty)
	{
		return FProjectCleanerStyles::Get().GetSlateColor("ProjectCleaner.Color.Red");
	}

	return FSlateColor{FLinearColor::Gray};
}

FSlateColor SProjectCleanerTreeViewItem::GetProgressBarColor() const
{
	const FLinearColor Green = FProjectCleanerStyles::Get().GetSlateColor("ProjectCleaner.Color.Green").GetSpecifiedColor();
	// const FLinearColor Yellow = FProjectCleanerStyles::Get().GetSlateColor("ProjectCleaner.Color.Yellow").GetSpecifiedColor();
	// const FLinearColor Red = FProjectCleanerStyles::Get().GetSlateColor("ProjectCleaner.Color.Red").GetSpecifiedColor();

	const FLinearColor Color1 = TreeItem->PercentUnusedNormalized < 0.5f ? FLinearColor::Green : FLinearColor::Yellow;
	const FLinearColor Color2 = TreeItem->PercentUnusedNormalized >= 0.5f ? FLinearColor::Yellow : FLinearColor::Red;
	const FLinearColor CurrentColor = UKismetMathLibrary::LinearColorLerp(FLinearColor::Green, FLinearColor::Red, TreeItem->PercentUnusedNormalized);
	const FLinearColor TargetColor = UKismetMathLibrary::LinearColorLerp(Color1, Color2, TreeItem->PercentUnusedNormalized);

	return FSlateColor{CurrentColor};
}
