// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "StructsContainer.h"

class FProjectCleanerManager;

class SIndirectAssetsUISelectionRow : public SMultiColumnTableRow<TWeakObjectPtr<UIndirectAsset>>
{
public:
	
	SLATE_BEGIN_ARGS(SIndirectAssetsUISelectionRow){}
		SLATE_ARGUMENT(TWeakObjectPtr<UIndirectAsset>, SelectedRowItem)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView)
	{
		SelectedRowItem = InArgs._SelectedRowItem;

		SMultiColumnTableRow<TWeakObjectPtr<UIndirectAsset>>::Construct(
		SMultiColumnTableRow<TWeakObjectPtr<UIndirectAsset>>::FArguments()
		.Padding(
			FMargin(0.f, 2.f, 0.f, 0.f)),
			InOwnerTableView
		);
	}

	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& InColumnName) override
	{
		TSharedPtr<SWidget> ColumnWidget;

		if (InColumnName == TEXT("AssetName"))
		{
			ColumnWidget = SNew(STextBlock).Text(FText::FromString(SelectedRowItem->AssetName));
		}
		else if (InColumnName == TEXT("AssetPath"))
		{
			ColumnWidget = SNew(STextBlock).Text(FText::FromString(SelectedRowItem->AssetPath));
		}
		else if (InColumnName == TEXT("FilePath"))
		{
			ColumnWidget = SNew(STextBlock).Text(FText::FromString(SelectedRowItem->FilePath));
		}
		else if (InColumnName == TEXT("LineNum"))
		{
			ColumnWidget = SNew(STextBlock).Text(FText::FromString(FString::FromInt(SelectedRowItem->LineNum)));
		}
		else
		{
			ColumnWidget = SNew(STextBlock).Text(FText::FromString("No Data"));	
		}
		
		return ColumnWidget.ToSharedRef();
	}

private:
	TWeakObjectPtr<UIndirectAsset> SelectedRowItem;
};

class SProjectCleanerIndirectAssetsUI : public SCompoundWidget
{
public:
	
	SLATE_BEGIN_ARGS(SProjectCleanerIndirectAssetsUI) {}
		SLATE_ARGUMENT(FProjectCleanerManager* , CleanerManager)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	void SetCleanerManager(FProjectCleanerManager* CleanerManagerPtr);
	void UpdateUI();
private:
	TSharedRef<ITableRow> OnGenerateRow(
		TWeakObjectPtr<UIndirectAsset> InItem,
		const TSharedRef<STableViewBase>& OwnerTable
	) const;
	void OnMouseDoubleClick(TWeakObjectPtr<UIndirectAsset> Item) const;
	TArray<TWeakObjectPtr<UIndirectAsset>> IndirectAssets;
	TSharedPtr<SListView<TWeakObjectPtr<UIndirectAsset>>> ListView;
	FProjectCleanerManager* CleanerManager = nullptr;
};