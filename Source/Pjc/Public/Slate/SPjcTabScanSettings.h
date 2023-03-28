// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

enum class EPjcScanResult : uint8;

struct FPjcStatItem
{
	int64 Size = 0;
	int32 Num = 0;
	FString Category;
	FString ToolTip;
};

class SPjcSlice : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SPjcSlice) {}
		SLATE_ARGUMENT(const FSlateBrush*, Brush)
		SLATE_ARGUMENT(float, Angle)
		SLATE_ARGUMENT(float, ArcSize)
		SLATE_ARGUMENT(TAttribute<FLinearColor>, Color)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	                      FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle,
	                      bool bParentEnabled) const override;

	void SetBrush(const FSlateBrush* InBrush);
	void SetAngle(const float InAngle);
	void SetArcSize(const float InArcSize);

protected:
	FInvalidatableBrushAttribute Brush;
	float Angle = 0.0f;
	float ArcSize = 0.0f;
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

private:
	FReply OnBtnScanProjectClick() const;
	FReply OnBtnCleanProjectClick() const;
	bool BtnCleanProjectEnabled() const;

	void StatsUpdate();

	TSharedRef<ITableRow> OnStatsGenerateRow(TSharedPtr<FPjcStatItem> Item, const TSharedRef<STableViewBase>& OwnerTable) const;
	TSharedRef<SHeaderRow> GetStatsHeaderRow() const;

	TArray<TSharedPtr<FPjcStatItem>> StatItems;
	TSharedPtr<SListView<TSharedPtr<FPjcStatItem>>> StatView;
};
