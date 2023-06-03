// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

struct FPjcCorruptedAssetItem;

class SPjcItemAssetCorrupted final : public SMultiColumnTableRow<TSharedPtr<FPjcCorruptedAssetItem>>
{
public:
	SLATE_BEGIN_ARGS(SPjcItemAssetCorrupted) {}
		SLATE_ARGUMENT(TSharedPtr<FPjcCorruptedAssetItem>, Item)
		SLATE_ARGUMENT(FText, TextHighlight)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InTable);
	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& InColumnName) override;

private:
	FText TextHighlight;
	TSharedPtr<FPjcCorruptedAssetItem> Item;
};
