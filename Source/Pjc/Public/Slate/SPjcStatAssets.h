// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

enum class EPjcAssetCategory:uint8;
struct FPjcStatItem;

class SPjcStatAssets final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SPjcStatAssets) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	void StatsUpdateData(TMap<EPjcAssetCategory, TSet<FAssetData>>& AssetsCategoryMapping);

protected:
	void StatsInit();
	TSharedRef<SHeaderRow> GetHeaderRow() const;
	TSharedRef<ITableRow> OnGenerateRow(TSharedPtr<FPjcStatItem> Item, const TSharedRef<STableViewBase>& OwnerTable) const;

private:
	TArray<TSharedPtr<FPjcStatItem>> Items;
	TSharedPtr<SListView<TSharedPtr<FPjcStatItem>>> ListView;
};
