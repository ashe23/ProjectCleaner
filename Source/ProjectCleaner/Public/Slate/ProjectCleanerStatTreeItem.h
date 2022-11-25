// // Copyright Ashot Barkhudaryan. All Rights Reserved.
//
// #pragma once
//
// #include "CoreMinimal.h"
// #include "ProjectCleanerTypes.h"
// #include "Components/ProgressBar.h"
// #include "Widgets/SCompoundWidget.h"
//
// class UProjectCleanerStatTreeItem;
//
// class SProjectCleanerStatTreeItem : public SMultiColumnTableRow<TWeakObjectPtr<UProjectCleanerStatTreeItem>>
// {
// public:
// 	SLATE_BEGIN_ARGS(SProjectCleanerStatTreeItem)
// 		{
// 		}
//
// 		SLATE_ARGUMENT(TWeakObjectPtr<UProjectCleanerStatTreeItem>, ListItem)
// 	SLATE_END_ARGS()
//
// 	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView)
// 	{
// 		ListItem = InArgs._ListItem;
//
// 		SMultiColumnTableRow<TWeakObjectPtr<UProjectCleanerStatTreeItem>>::Construct(
// 			SMultiColumnTableRow<TWeakObjectPtr<UProjectCleanerStatTreeItem>>::FArguments(),
// 			InOwnerTableView
// 		);
// 	}
//
// 	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& InColumnName) override
// 	{
// 		if (InColumnName.IsEqual(TEXT("Path")))
// 		{
// 			return
// 				SNew(SHorizontalBox)
// 				+ SHorizontalBox::Slot()
// 				.AutoWidth()
// 				[
// 					SNew(SExpanderArrow, SharedThis(this))
// 					.IndentAmount(20)
// 					.ShouldDrawWires(true)
// 				]
// 				+ SHorizontalBox::Slot()
// 				.AutoWidth()
// 				[
// 					SNew(STextBlock).Text(FText::FromString(ListItem->Path))
// 				];
// 		}
//
// 		if (InColumnName.IsEqual(TEXT("Size")))
// 		{
// 			return SNew(STextBlock).Text(FText::FromString(ListItem->Size));
// 		}
//
// 		if (InColumnName.IsEqual(TEXT("Files")))
// 		{
// 			return SNew(STextBlock).Text(FText::FromString(ListItem->Files));
// 		}
//
// 		if (InColumnName.IsEqual(TEXT("Folders")))
// 		{
// 			return SNew(STextBlock).Text(FText::FromString(ListItem->Folders));
// 		}
//
// 		if (InColumnName.IsEqual(TEXT("Unused")))
// 		{
// 			return SNew(STextBlock).Text(FText::FromString(ListItem->Unused));
// 		}
//
// 		if (InColumnName.IsEqual(TEXT("Empty")))
// 		{
// 			return SNew(STextBlock).Text(FText::FromString(ListItem->Empty));
// 		}
//
// 		if (InColumnName.IsEqual(TEXT("Progress")))
// 		{
// 			return
// 					SNew(SBorder)
// 					.BorderImage( FEditorStyle::GetBrush("ErrorReporting.Box") )
// 					.HAlign(HAlign_Fill)
// 					.VAlign(VAlign_Center)
// 					.Padding( FMargin(3,0) )
// 					.BorderBackgroundColor( FSlateColor( FLinearColor( 1.0f, 0.0f, 1.0f, 0.0f ) ) )
// 					[
// 						//progress bar for percent of enabled children completed
// 						SNew(SProgressBar)
// 						.Percent(ListItem->Progress)
// 					];
// 		}
//
// 		return SNew(STextBlock).Text(FText::FromString(""));
// 	}
//
// private:
// 	TWeakObjectPtr<UProjectCleanerStatTreeItem> ListItem;
// };
//
// class SProjectCleanerStats : public SCompoundWidget
// {
// public:
// 	SLATE_BEGIN_ARGS(SProjectCleanerStats)
// 		{
// 		}
//
// 	SLATE_END_ARGS()
//
// 	void Construct(const FArguments& Args);
//
// private:
// 	void UpdateTreeItems();
// 	
// 	TSharedRef<ITableRow> OnGenerateRow(TWeakObjectPtr<UProjectCleanerStatTreeItem> Item, const TSharedRef<STableViewBase>& OwnerTable) const;
// 	void OnGetChildren(TWeakObjectPtr<UProjectCleanerStatTreeItem> Item, TArray<TWeakObjectPtr<UProjectCleanerStatTreeItem>>& OutChildren);
//
// 	TArray<TWeakObjectPtr<UProjectCleanerStatTreeItem>> TreeItems;
// 	TSharedPtr<STreeView<TSharedPtr<UProjectCleanerStatTreeItem>>> TreeView;
// };
