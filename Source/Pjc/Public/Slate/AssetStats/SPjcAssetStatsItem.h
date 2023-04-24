// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

struct FPjcAssetStatsItem
{
	int64 Size = 0;
	int64 Num = 0;
	FString Category;
	FMargin CategoryPadding{FMargin{0}};
	FLinearColor TextColorDefault{FLinearColor::White};
	FLinearColor TextColorActive{FLinearColor::White};
	FString ToolTipCategory;
	FString ToolTipNum;
	FString ToolTipSize;
};

class SPjcAssetStatsItem final : public SMultiColumnTableRow<TSharedPtr<FPjcAssetStatsItem>>
{
public:
	SLATE_BEGIN_ARGS(SPjcAssetStatsItem) {}
		SLATE_ARGUMENT(TSharedPtr<FPjcAssetStatsItem>, Item)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InTable);
	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& InColumnName) override;

private:
	TSharedPtr<FPjcAssetStatsItem> Item;
};
