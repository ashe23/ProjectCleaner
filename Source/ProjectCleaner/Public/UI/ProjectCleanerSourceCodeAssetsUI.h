﻿// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "ProjectCleanerSourceCodeAssetsUI.generated.h"


UCLASS(Transient)
class UIndirectAsset : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY(DisplayName = "AssetName", VisibleAnywhere, Category="AssetUsedIndirectly")
	FString AssetName;

	UPROPERTY(DisplayName = "AssetPath", VisibleAnywhere, Category="AssetUsedIndirectly")
	FString AssetPath;

	UPROPERTY(DisplayName = "SourceCodePath", VisibleAnywhere, Category="AssetUsedIndirectly")
	FString SourceCodePath;

	UPROPERTY(DisplayName = "AssetData", VisibleAnywhere, Category="AssetUsedIndirectly")
	FAssetData AssetData;
};


class SSourceCodeAssetsUISelectionRow : public SMultiColumnTableRow<TWeakObjectPtr<UIndirectAsset>>
{
public:
	
	SLATE_BEGIN_ARGS(SSourceCodeAssetsUISelectionRow){}
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
		else if (InColumnName == TEXT("SourceCodePath"))
		{
			ColumnWidget = SNew(STextBlock).Text(FText::FromString(SelectedRowItem->SourceCodePath));
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

class SProjectCleanerSourceCodeAssetsUI : public SCompoundWidget
{
public:
	
	SLATE_BEGIN_ARGS(SProjectCleanerSourceCodeAssetsUI) {}
		SLATE_ARGUMENT(TArray<TWeakObjectPtr<UIndirectAsset>>*, SourceCodeAssets)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	void SetSourceCodeAssets(const TArray<TWeakObjectPtr<UIndirectAsset>>& NewSourceCodeAssets);
private:
	void InitUI();
	TSharedRef<ITableRow> OnGenerateRow(TWeakObjectPtr<UIndirectAsset> InItem, const TSharedRef<STableViewBase>& OwnerTable) const;
	void OnMouseDoubleClick(TWeakObjectPtr<UIndirectAsset> Item) const;
	TArray<TWeakObjectPtr<UIndirectAsset>> SourceCodeAssets;
	TSharedPtr<SListView<TWeakObjectPtr<UIndirectAsset>>> ListView;
};