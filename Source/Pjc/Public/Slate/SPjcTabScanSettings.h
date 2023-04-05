// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

struct FPjcScanResult;
enum class EPjcScanResult : uint8;

struct FPjcStatItem
{
	int64 Size = 0;
	int32 Num = 0;
	FString Category;
	FString ToolTip;
	FMargin Padding;
	FLinearColor TextColor{FLinearColor::White};
};

class SPjcStatItem final : public SMultiColumnTableRow<TSharedPtr<FPjcStatItem>>
{
public:
	SLATE_BEGIN_ARGS(SPjcStatItem) {}
		SLATE_ARGUMENT(TSharedPtr<FPjcStatItem>, StatItem)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InTable);
	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& InColumnName) override;

private:
	TSharedPtr<FPjcStatItem> StatItem;
};

class SPjcTabScanSettings final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SPjcTabScanSettings) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~SPjcTabScanSettings() override;

private:
	FReply OnBtnScanProjectClick() const;
	FReply OnBtnCleanProjectClick() const;
	bool BtnCleanProjectEnabled() const;
	void StatsUpdate(const FPjcScanResult& InScanResult);

	TSharedRef<ITableRow> OnStatsGenerateRow(TSharedPtr<FPjcStatItem> Item, const TSharedRef<STableViewBase>& OwnerTable) const;
	TSharedRef<SHeaderRow> GetStatsHeaderRow() const;

	TArray<TSharedPtr<FPjcStatItem>> StatItems;
	TSharedPtr<SListView<TSharedPtr<FPjcStatItem>>> StatView;
};
