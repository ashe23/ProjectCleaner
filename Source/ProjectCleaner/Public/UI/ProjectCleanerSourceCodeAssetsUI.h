// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "ProjectCleanerSourceCodeAssetsUI.generated.h"


UCLASS(Transient)
class USourceCodeAsset : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY(DisplayName = "AssetName", VisibleAnywhere, Category="AssetsUsedInSourceCode")
	FString AssetName;

	UPROPERTY(DisplayName = "AssetPath", VisibleAnywhere, Category="AssetsUsedInSourceCode")
	FString AssetPath;

	UPROPERTY(DisplayName = "SourceCodePath", VisibleAnywhere, Category="AssetsUsedInSourceCode")
	FString SourceCodePath;
};


class SSourceCodeAssetsUISelectionRow : public SMultiColumnTableRow<TWeakObjectPtr<USourceCodeAsset>>
{
public:
	
	SLATE_BEGIN_ARGS(SSourceCodeAssetsUISelectionRow){}
		SLATE_ARGUMENT(TWeakObjectPtr<USourceCodeAsset>, SelectedRowItem)
	SLATE_END_ARGS()

void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView)
	{
		SelectedRowItem = InArgs._SelectedRowItem;

		SMultiColumnTableRow<TWeakObjectPtr<USourceCodeAsset>>::Construct(
		SMultiColumnTableRow<TWeakObjectPtr<USourceCodeAsset>>::FArguments()
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
	TWeakObjectPtr<USourceCodeAsset> SelectedRowItem;
};

class SProjectCleanerSourceCodeAssetsUI : public SCompoundWidget
{
public:
	
	SLATE_BEGIN_ARGS(SProjectCleanerSourceCodeAssetsUI) {}
		SLATE_ARGUMENT(TArray<TWeakObjectPtr<USourceCodeAsset>>, SourceCodeAssets)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	void SetSourceCodeAssets(const TArray<TWeakObjectPtr<USourceCodeAsset>>& NewSourceCodeAssets);
private:
	void RefreshUIContent();
	TSharedRef<ITableRow> OnGenerateRow(
		TWeakObjectPtr<USourceCodeAsset> InItem,
		const TSharedRef<STableViewBase>& OwnerTable
	) const;
	void OnMouseDoubleClick(TWeakObjectPtr<USourceCodeAsset> Item) const;
	TArray<TWeakObjectPtr<USourceCodeAsset>> SourceCodeAssets;
	TSharedRef<SWidget> WidgetRef =  SNullWidget::NullWidget;
};