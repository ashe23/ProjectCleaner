// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/ProjectCleanerStatTreeItem.h"
#include "ProjectCleanerTypes.h"

void SProjectCleanerStats::Construct(const FArguments& Args)
{
	TreeItems.Reset();

	const auto Root = NewObject<UProjectCleanerStatTreeItem>();
	Root->Path = TEXT("Root");
	Root->Size = TEXT("100 MiB");
	Root->Files = TEXT("123");
	Root->Folders = TEXT("22");
	Root->Unused = TEXT("13");
	Root->Empty = TEXT("2");
	Root->Progress = 0.8f;

	TreeItems.Add(Root);

	const auto Root1 = NewObject<UProjectCleanerStatTreeItem>();
	Root1->Path = TEXT("Test");
	Root1->Size = TEXT("23 MiB");
	Root1->Files = TEXT("123");
	Root1->Folders = TEXT("22");
	Root1->Unused = TEXT("13");
	Root1->Empty = TEXT("2");
	Root1->Progress = 0.7f;

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
			                                                            + SHeaderRow::Column(FName{TEXT("Files")})
																		[
																			SNew(STextBlock)
																			.Text(FText::FromString(TEXT("Assets")))
																		]
																	   + SHeaderRow::Column(FName{TEXT("Unused")})
																	   [
																		   SNew(STextBlock)
																		   .Text(FText::FromString(TEXT("Unused Assets")))
																	   ]
																		+ SHeaderRow::Column(FName{TEXT("Folders")})
																	   [
																		   SNew(STextBlock)
																		   .Text(FText::FromString(TEXT("Folders")))
																	   ]
																	   + SHeaderRow::Column(FName{TEXT("Empty")})
																	   [
																		   SNew(STextBlock)
																		   .Text(FText::FromString(TEXT("Empty Folders")))
																	   ]
																	   + SHeaderRow::Column(FName{TEXT("Progress")})
																	   [
																		   SNew(STextBlock)
																		   .Text(FText::FromString(TEXT("% of Parent (Size)")))
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
	Child_01->Files = TEXT("123");
	Child_01->Folders = TEXT("22");
	Child_01->Unused = TEXT("13");
	Child_01->Empty = TEXT("2");
	Child_01->Progress = 0.5f;

	const auto Child_02 = NewObject<UProjectCleanerStatTreeItem>();
	Child_02->Path = TEXT("Child_02");
	Child_02->Size = TEXT("56.23 MiB");
	Child_02->Files = TEXT("123");
	Child_02->Folders = TEXT("22");
	Child_02->Unused = TEXT("13");
	Child_02->Empty = TEXT("2");
	Child_01->Progress = 0.2f;
	

	OutChildren.Add(Child_01);
	OutChildren.Add(Child_02);
}
