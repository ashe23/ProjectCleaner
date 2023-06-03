// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

struct FPjcFileExternalItem;

class SPjcItemFileExternal final : public SMultiColumnTableRow<TSharedPtr<FPjcFileExternalItem>>
{
public:
	SLATE_BEGIN_ARGS(SPjcItemFileExternal) {}
		SLATE_ARGUMENT(TSharedPtr<FPjcFileExternalItem>, Item)
		SLATE_ARGUMENT(FText, TextHighlight)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InTable);
	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& InColumnName) override;

private:
	FText TextHighlight;
	TSharedPtr<FPjcFileExternalItem> Item;
};
