// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct FPjcTreeViewItem
{
	float UnusedSize = 0.0f;
	float Percentage = 0.0f;
	bool bIsDev = false;
	bool bIsRoot = false;
	bool bIsEmpty = false;
	bool bIsExcluded = false;
	bool bIsExpanded = false;
	FString PathAbs;
	FString PathName;
	FString PathContent;

	TSharedPtr<FPjcTreeViewItem> ParentItem;
	TArray<TSharedPtr<FPjcTreeViewItem>> SubItems;
};

class SPjcTreeViewItem final : public SMultiColumnTableRow<TSharedPtr<FPjcTreeViewItem>>
{
public:
	SLATE_BEGIN_ARGS(SPjcTreeViewItem) {}
		SLATE_ARGUMENT(TSharedPtr<FPjcTreeViewItem>, Item)
		SLATE_ARGUMENT(FString, HighlightText)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InTable);
	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& InColumnName) override;

private:
	const FSlateBrush* GetFolderIcon() const;
	FSlateColor GetFolderColor() const;

	FString HighlightText;
	TSharedPtr<FPjcTreeViewItem> Item;
};
