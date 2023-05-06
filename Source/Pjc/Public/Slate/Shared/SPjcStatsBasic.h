// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class UPjcSubsystem;
struct FPjcStatItem;

class SPjcStatsBasic final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SPjcStatsBasic) {}
		SLATE_ARGUMENT(FText, Title)
		SLATE_ARGUMENT(FMargin, HeaderMargin)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

protected:
	void StatItemsUpdate();
	TSharedRef<SHeaderRow> GetStatHeaderRow() const;
	TSharedRef<ITableRow> OnStatGenerateRow(TSharedPtr<FPjcStatItem> Item, const TSharedRef<STableViewBase>& OwnerTable) const;

private:
	FText Title;
	FMargin HeaderMargin;
	UPjcSubsystem* SubsystemPtr = nullptr;
	TArray<TSharedPtr<FPjcStatItem>> StatItems;
	TSharedPtr<SListView<TSharedPtr<FPjcStatItem>>> StatView;
};
