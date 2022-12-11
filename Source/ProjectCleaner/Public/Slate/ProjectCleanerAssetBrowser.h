// // Copyright Ashot Barkhudaryan. All Rights Reserved.
//
// #pragma once
//
// #include "CoreMinimal.h"
// #include "Widgets/SCompoundWidget.h"
// #include "Widgets/Views/STileView.h"
//
// // class STileView;
// struct FProjectCleanerScanner;
//
// struct FTestData
// {
// 	FTestData(const FAssetData& Data) : AssetData(Data)
// 	{
// 	}
//
// 	FAssetData AssetData;
// };
//
// class SProjectCleanerAssetBrowser : public SCompoundWidget
// {
// public:
// 	SLATE_BEGIN_ARGS(SProjectCleanerAssetBrowser)
// 		{
// 		}
//
// 		SLATE_ARGUMENT(TSharedPtr<FProjectCleanerScanner>, Scanner)
// 	SLATE_END_ARGS()
//
// 	void Construct(const FArguments& InArgs);
// 	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;
// 	void UpdateView();
// private:
// 	TSharedRef<ITableRow> OnGenerateWidgetForTileView(TSharedPtr<FTestData> InItem, const TSharedRef<STableViewBase>& OwnerTable) const;
// 	TSharedPtr<FAssetThumbnailPool> AssetThumbnailPool;
// 	TSharedPtr<FProjectCleanerScanner> Scanner;
// 	TArray<TSharedPtr<FTestData>> Items;
// 	TSharedPtr<STileView<TSharedPtr<FTestData>>> ListView;
// };
