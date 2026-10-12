// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Views/STableViewBase.h"

struct FPjcUsageItem;

/** Row of Asset Usage tab lists, columns: Name, Class, Status, Why */
class SPjcItemUsage final : public SMultiColumnTableRow<TSharedPtr<FPjcUsageItem>>
{
  public:

	SLATE_BEGIN_ARGS(SPjcItemUsage) {}
	SLATE_ARGUMENT(TSharedPtr<FPjcUsageItem>, Item)
	SLATE_ARGUMENT(FText, HighlightText)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InTable);
	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& InColumnName) override;

	static FString GetItemName(const FPjcUsageItem& InItem);
	static FText GetStatusText(const FPjcUsageItem& InItem);
	static FLinearColor GetStatusColor(const FPjcUsageItem& InItem);
	/** Sort order of statuses: not deleted, unused, used, not scanned */
	static int32 GetStatusOrder(const FPjcUsageItem& InItem);

  private:

	FText HighlightText;
	TSharedPtr<FPjcUsageItem> Item;
};
