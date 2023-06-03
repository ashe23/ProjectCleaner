// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

struct FPjcStatItem;

class SPjcItemStat final : public SMultiColumnTableRow<TSharedPtr<FPjcStatItem>>
{
public:
	SLATE_BEGIN_ARGS(SPjcItemStat) {}
		SLATE_ARGUMENT(TSharedPtr<FPjcStatItem>, Item)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InTable);
	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& InColumnName) override;

private:
	TSharedPtr<FPjcStatItem> Item;
};
