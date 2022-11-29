// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectCleanerTypes.h"
#include "Widgets/SCompoundWidget.h"

class SProjectCleanerIndirectAssetListItem final : public SMultiColumnTableRow<TSharedPtr<FProjectCleanerIndirectAsset>>
{
public:
	SLATE_BEGIN_ARGS(SProjectCleanerIndirectAssetListItem)
		{
		}

		SLATE_ARGUMENT(TSharedPtr<FProjectCleanerIndirectAsset>, ListItem)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InTable)
	{
		ListItem = InArgs._ListItem;

		SMultiColumnTableRow<TSharedPtr<FProjectCleanerIndirectAsset>>::Construct(
			SMultiColumnTableRow<TSharedPtr<FProjectCleanerIndirectAsset>>::FArguments(),
			InTable
		);
	}

	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& InColumnName) override
	{
		if (InColumnName.IsEqual(TEXT("AssetName")))
		{
			return SNew(STextBlock).Text(FText::FromString(ListItem->AssetData.AssetName.ToString()));
		}

		if (InColumnName.IsEqual(TEXT("AssetPath")))
		{
			return SNew(STextBlock).Text(FText::FromString(ListItem->AssetData.PackagePath.ToString()));
		}

		if (InColumnName.IsEqual(TEXT("FilePath")))
		{
			return SNew(STextBlock).Text(FText::FromString(ListItem->FilePath));
		}

		if (InColumnName.IsEqual(TEXT("Line")))
		{
			return SNew(STextBlock).Text(FText::FromString(FString::Printf(TEXT("%d"), ListItem->LineNum)));
		}

		return SNew(STextBlock).Text(FText::FromString("No Data"));
	}

private:
	TSharedPtr<FProjectCleanerIndirectAsset> ListItem;
};

class SProjectCleanerWindowIndirectAssets final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SProjectCleanerWindowIndirectAssets)
		{
		}

		SLATE_ARGUMENT(TArray<FProjectCleanerIndirectAsset>, ListItems)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
private:
	TSharedPtr<SHeaderRow> GetListHeaderRow() const;
	TSharedPtr<SWidget> OnListContextMenu() const;
	void OnListItemDblClick(TSharedPtr<FProjectCleanerIndirectAsset> Item) const;
	TSharedRef<ITableRow> OnGenerateRow(
		TSharedPtr<FProjectCleanerIndirectAsset> InItem,
		const TSharedRef<STableViewBase>& OwnerTable
	) const;

	TSharedPtr<FUICommandList> Cmds;
	TArray<TSharedPtr<FProjectCleanerIndirectAsset>> ListItems;
	TSharedPtr<SListView<TSharedPtr<FProjectCleanerIndirectAsset>>> ListView;
};
