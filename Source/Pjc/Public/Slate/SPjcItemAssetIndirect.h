// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

struct FPjcAssetIndirectInfo;

class SPjcItemAssetIndirect final : public SMultiColumnTableRow<TSharedPtr<FPjcAssetIndirectInfo>>
{
public:
	SLATE_BEGIN_ARGS(SPjcItemAssetIndirect) {}
		SLATE_ARGUMENT(TSharedPtr<FPjcAssetIndirectInfo>, Item)
		SLATE_ARGUMENT(FText, HighlightText)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InTable);
	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& InColumnName) override;

private:
	FText HighlightText;
	TSharedPtr<FPjcAssetIndirectInfo> Item;
};
