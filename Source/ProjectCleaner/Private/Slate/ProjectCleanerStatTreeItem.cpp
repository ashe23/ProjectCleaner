// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/ProjectCleanerStatTreeItem.h"
#include "ProjectCleanerTypes.h"

void SProjectCleanerStats::Construct(const FArguments& Args)
{
	TreeItems.Reset();

	const auto Root = NewObject<UProjectCleanerStatTreeItem>();
	Root->Path = TEXT("Root");
	Root->Size = TEXT("100 MiB");

	TreeItems.Add(Root);

	const auto Root1 = NewObject<UProjectCleanerStatTreeItem>();
	Root1->Path = TEXT("Test");
	Root1->Size = TEXT("23 MiB");

	TreeItems.Add(Root1);

	ChildSlot
	[
		SNew(STreeView<TWeakObjectPtr<UProjectCleanerStatTreeItem>>)
		.TreeItemsSource(&TreeItems)
		.OnGenerateRow(this, &SProjectCleanerStats::OnGenerateRow)
		.OnGetChildren(this, &SProjectCleanerStats::OnGetChildren)
		.HeaderRow(
			                                                            SNew(SHeaderRow)
			                                                            + SHeaderRow::Column(FName{TEXT("Path")})
			                                                            [
				                                                            SNew(STextBlock)
				                                                            .Text(FText::FromString(TEXT("Path")))
			                                                            ]
			                                                            + SHeaderRow::Column(FName{TEXT("Size")})
			                                                            [
				                                                            SNew(STextBlock)
				                                                            .Text(FText::FromString(TEXT("Size")))
			                                                            ]
		                                                            )
	];
}

TSharedRef<ITableRow> SProjectCleanerStats::OnGenerateRow(TWeakObjectPtr<UProjectCleanerStatTreeItem> Item, const TSharedRef<STableViewBase>& OwnerTable) const
{
	return SNew(SProjectCleanerStatTreeItem, OwnerTable).ListItem(Item);
}

void SProjectCleanerStats::OnGetChildren(TWeakObjectPtr<UProjectCleanerStatTreeItem> Item, TArray<TWeakObjectPtr<UProjectCleanerStatTreeItem>>& OutChildren)
{
	const auto Child_01 = NewObject<UProjectCleanerStatTreeItem>();
	Child_01->Path = TEXT("Child_01");
	Child_01->Size = TEXT("56.23 MiB");

	const auto Child_02 = NewObject<UProjectCleanerStatTreeItem>();
	Child_02->Path = TEXT("Child_02");
	Child_02->Size = TEXT("56.23 MiB");

	OutChildren.Add(Child_01);
	OutChildren.Add(Child_02);
}
