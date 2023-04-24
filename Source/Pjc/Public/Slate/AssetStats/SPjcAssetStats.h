// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

struct FPjcAssetStatsItem;
struct FPjcScanDataAssets;
class UPjcSubsystem;

class SPjcAssetStats final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SPjcAssetStats) {}
		SLATE_ARGUMENT(FMargin, Padding)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~SPjcAssetStats() override;
private:
	void OnScanAssets(const FPjcScanDataAssets& InScanDataAssets);
	TSharedRef<ITableRow> OnGenerateRow(TSharedPtr<FPjcAssetStatsItem> Item, const TSharedRef<STableViewBase>& OwnerTable) const;
	TSharedRef<SHeaderRow> GetHeaderRow() const;

	UPjcSubsystem* SubsystemPtr = nullptr;
	FMargin Padding;
	TArray<TSharedPtr<FPjcAssetStatsItem>> ListItems;
	TSharedPtr<SListView<TSharedPtr<FPjcAssetStatsItem>>> ListView;
};
