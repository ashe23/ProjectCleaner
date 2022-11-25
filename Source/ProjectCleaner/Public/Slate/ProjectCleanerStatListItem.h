// // Copyright Ashot Barkhudaryan. All Rights Reserved.
//
// #pragma once
//
// #include "CoreMinimal.h"
//
// class UProjectCleanerStatListItem;
//
// class SProjectCleanerStatListItem final : public SMultiColumnTableRow<TWeakObjectPtr<UProjectCleanerStatListItem>>
// {
// public:
// 	SLATE_BEGIN_ARGS(SProjectCleanerStatListItem)
// 		{
// 		}
//
// 		SLATE_ARGUMENT(TWeakObjectPtr<UProjectCleanerStatListItem>, ListItem)
// 	SLATE_END_ARGS()
//
// 	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView);
// 	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& InColumnName) override;
// private:
// 	TWeakObjectPtr<UProjectCleanerStatListItem> ListItem;
// };
