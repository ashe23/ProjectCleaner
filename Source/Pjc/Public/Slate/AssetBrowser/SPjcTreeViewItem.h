// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct FPjcTreeViewItem
{
	float UnusedSize = 0.0f;
	float Percentage = 0.0f;
	FString PathAbs;
	FString PathName;
	FString PathContent;
};

class SPjcTreeViewItem final : public SMultiColumnTableRow<TSharedPtr<FPjcTreeViewItem>>
{
public:
	SLATE_BEGIN_ARGS(SPjcTreeViewItem) {}
		SLATE_ARGUMENT(TSharedPtr<FPjcTreeViewItem>, Item)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InTable);
	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& InColumnName) override;

private:
	TSharedPtr<FPjcTreeViewItem> Item;
};
