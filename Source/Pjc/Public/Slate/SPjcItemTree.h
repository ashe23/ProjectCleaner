// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

struct FPjcTreeItem;

class SPjcItemTree final : public SMultiColumnTableRow<TSharedPtr<FPjcTreeItem>>
{
public:
	SLATE_BEGIN_ARGS(SPjcItemTree) {}
		SLATE_ARGUMENT(TSharedPtr<FPjcTreeItem>, Item)
		SLATE_ARGUMENT(FText, HightlightText)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InTable);
	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& InColumnName) override;

private:
	const FSlateBrush* GetFolderIcon() const;
	FSlateColor GetFolderColor() const;

	FText HighlightText;
	TSharedPtr<FPjcTreeItem> Item;
};
