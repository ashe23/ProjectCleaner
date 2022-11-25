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
private:
	TSharedRef<ITableRow> OnGenerateWidgetForTileView(TSharedPtr<FTestData> InItem, const TSharedRef<STableViewBase>& OwnerTable);

	TArray<TSharedPtr<FTestData>> Items;
};
