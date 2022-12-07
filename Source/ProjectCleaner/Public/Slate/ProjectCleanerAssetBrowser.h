// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

struct FTestData
{
	FTestData(const FAssetData& Data) : AssetData(Data)
	{
	}

	FAssetData AssetData;
};

class SProjectCleanerAssetBrowser : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SProjectCleanerAssetBrowser)
		{
		}

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;
private:
	TSharedRef<ITableRow> OnGenerateWidgetForTileView(TSharedPtr<FTestData> InItem, const TSharedRef<STableViewBase>& OwnerTable) const;
	TSharedPtr<FAssetThumbnailPool> AssetThumbnailPool;
	TArray<TSharedPtr<FTestData>> Items;
};
