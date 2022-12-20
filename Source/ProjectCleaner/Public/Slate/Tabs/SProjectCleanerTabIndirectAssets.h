// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectCleanerTypes.h"
#include "Widgets/SCompoundWidget.h"

class UProjectCleanerSubsystem;

class SProjectCleanerTabIndirectItem final : public SMultiColumnTableRow<TSharedPtr<FProjectCleanerIndirectAssetInfo>>
{
public:
	SLATE_BEGIN_ARGS(SProjectCleanerTabIndirectItem)
		{
		}

		SLATE_ARGUMENT(TSharedPtr<FProjectCleanerIndirectAssetInfo>, ListItem)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InTable)
	{
		ListItem = InArgs._ListItem;

		SMultiColumnTableRow::Construct(SMultiColumnTableRow::FArguments(), InTable);
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
	TSharedPtr<FProjectCleanerIndirectAssetInfo> ListItem;
};

class SProjectCleanerTabIndirect final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SProjectCleanerTabIndirect)
		{
		}

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~SProjectCleanerTabIndirect() override;
private:
	void OnProjectScanned();
	void ListUpdate();
	void ListSort();
	void OnListSort(EColumnSortPriority::Type SortPriority, const FName& Name, EColumnSortMode::Type SortMode);
	void OnListItemDblClick(TSharedPtr<FProjectCleanerIndirectAssetInfo> Item) const;
	TSharedPtr<SHeaderRow> GetListHeaderRow();
	TSharedPtr<SWidget> OnListContextMenu() const;
	TSharedRef<ITableRow> OnListGenerateRow(TSharedPtr<FProjectCleanerIndirectAssetInfo> InItem, const TSharedRef<STableViewBase>& OwnerTable) const;
	FText GetListTextSummary() const;

	int64 TotalSize = 0;
	FName ListSortColumn{TEXT("AssetPath")};
	TEnumAsByte<EColumnSortMode::Type> ListSortMode = EColumnSortMode::Descending;
	TSharedPtr<FUICommandList> Cmds;
	TArray<TSharedPtr<FProjectCleanerIndirectAssetInfo>> ListItems;
	TSharedPtr<SListView<TSharedPtr<FProjectCleanerIndirectAssetInfo>>> ListView;
	UProjectCleanerSubsystem* SubsystemPtr = nullptr;
};
