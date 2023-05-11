// // Copyright Ashot Barkhudaryan. All Rights Reserved.
//
// #pragma once
//
// #include "CoreMinimal.h"
// #include "Widgets/SCompoundWidget.h"
//
// struct FPjcStatItem;
//
// class SPjcStatsBasic final : public SCompoundWidget
// {
// public:
// 	SLATE_BEGIN_ARGS(SPjcStatsBasic) {}
// 		SLATE_ARGUMENT(FText, Title)
// 		SLATE_ARGUMENT(FMargin, HeaderMargin)
// 		SLATE_ARGUMENT(TArray<TSharedPtr<FPjcStatItem>>*, InitialItems)
// 	SLATE_END_ARGS()
//
// 	void Construct(const FArguments& InArgs);
// 	void StatItemsUpdate(const TArray<TSharedPtr<FPjcStatItem>>& InItems);
//
// protected:
// 	TSharedRef<SHeaderRow> GetStatHeaderRow() const;
// 	TSharedRef<ITableRow> OnStatGenerateRow(TSharedPtr<FPjcStatItem> Item, const TSharedRef<STableViewBase>& OwnerTable) const;
//
// private:
// 	FText Title;
// 	FMargin HeaderMargin;
// 	TArray<TSharedPtr<FPjcStatItem>> StatItems;
// 	TSharedPtr<SListView<TSharedPtr<FPjcStatItem>>> StatView;
// };
