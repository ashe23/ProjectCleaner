// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

struct FProjectCleanerTreeViewItem;

class SProjectCleanerTreeViewItem final : public SMultiColumnTableRow<TSharedPtr<FProjectCleanerTreeViewItem>>
{
	SLATE_BEGIN_ARGS(SProjectCleanerTreeViewItem)
		{
		}

		SLATE_ARGUMENT(TSharedPtr<FProjectCleanerTreeViewItem>, TreeItem)
		SLATE_ARGUMENT(FString, SearchText)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& OwnerTable);
	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& InColumnName) override;

private:
	const FSlateBrush* GetFolderIcon() const;
	FSlateColor GetFolderColor() const;
	FSlateColor GetProgressBarColor() const;

	FString SearchText;
	TSharedPtr<FProjectCleanerTreeViewItem> TreeItem;
};
