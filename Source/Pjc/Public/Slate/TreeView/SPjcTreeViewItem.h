// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct FPjcTreeViewItem
{
	int64 UnusedSize = 0;
	int32 NumAssetsTotal = 0;
	int32 NumAssetsUsed = 0;
	int32 NumAssetsUnused = 0;
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

	bool operator==(const FPjcTreeViewItem& Other) const
	{
		return PathAbs.Equals(Other.PathAbs);
	}

	bool operator!=(const FPjcTreeViewItem& Other) const
	{
		return !PathAbs.Equals(Other.PathAbs);
	}
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
