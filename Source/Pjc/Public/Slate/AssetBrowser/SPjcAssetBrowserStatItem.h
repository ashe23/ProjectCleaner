// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

struct FPjcAssetBrowserStatItem
{
	int64 Size = 0;
	int64 Num = 0;
	FString Category;
	FMargin CategoryPadding;
	FLinearColor TextColor{FLinearColor::White};
	FString CategoryToolTip;
};

class SPjcAssetBrowserStatItem final : public SMultiColumnTableRow<TSharedPtr<FPjcAssetBrowserStatItem>>
{
public:
	SLATE_BEGIN_ARGS(SPjcAssetBrowserStatItem) {}
		SLATE_ARGUMENT(TSharedPtr<FPjcAssetBrowserStatItem>, Item)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InTable);
	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& InColumnName) override;

private:
	TSharedPtr<FPjcAssetBrowserStatItem> Item;
};
