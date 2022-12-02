// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectCleanerTypes.h"
#include "Widgets/SCompoundWidget.h"

struct FProjectCleanerIndirectAsset;

class SProjectCleanerTabIndirectItem final : public SMultiColumnTableRow<TSharedPtr<FProjectCleanerIndirectAsset>>
{
public:
	SLATE_BEGIN_ARGS(SProjectCleanerTabIndirectItem)
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
			const TSharedPtr<FAssetThumbnail> AssetThumbnail = MakeShareable(new FAssetThumbnail(ListItem->AssetData, 16, 16, nullptr));
			const FAssetThumbnailConfig ThumbnailConfig;

			return
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.Padding(FMargin{0.0f, 0.0f, 5.0f, 0.0f})
				.AutoWidth()
				[
					SNew(SBox)
					.WidthOverride(16)
					.HeightOverride(16)
					[
						AssetThumbnail->MakeThumbnailWidget(ThumbnailConfig)
					]
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Justification(ETextJustify::Center)
					.Text(FText::FromString(ListItem->AssetData.AssetName.ToString()))
				];
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


class SProjectCleanerTabIndirect final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SProjectCleanerTabIndirect)
		{
		}

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	void ListUpdate();
private:
	void CmdsRegister();
	
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
