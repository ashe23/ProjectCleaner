#pragma once

// Engine Headers
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "ProjectCleanerAssetsUsedInSourceCodeUI.generated.h"


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


class SAssetUsedInSourceCodeUISelectionRow : public SMultiColumnTableRow<TWeakObjectPtr<USourceCodeAsset>>
{
public:
	SLATE_BEGIN_ARGS(SAssetUsedInSourceCodeUISelectionRow){}
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

		if(InColumnName == TEXT("AssetName"))
		{
			ColumnWidget = SNew(STextBlock).Text(FText::FromString(SelectedRowItem->AssetName));
		}
		else if(InColumnName == TEXT("AssetPath"))
		{
			ColumnWidget = SNew(STextBlock).Text(FText::FromString(SelectedRowItem->AssetPath));
		}
		else if(InColumnName == TEXT("SourceCodePath"))
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

class SProjectCleanerAssetsUsedInSourceCodeUI : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SProjectCleanerAssetsUsedInSourceCodeUI) {}
		SLATE_ARGUMENT(TArray<TWeakObjectPtr<USourceCodeAsset>>, AssetsUsedInSourceCode)
	SLATE_END_ARGS()

	
	void Construct(const FArguments& InArgs);
	void RefreshUIContent();
	void SetAssetsUsedInSourceCode(TArray<TWeakObjectPtr<USourceCodeAsset>>& NewAssetsUsedInSourceCode);
private:
	TSharedRef<ITableRow> OnGenerateRow(TWeakObjectPtr<USourceCodeAsset> InItem, const TSharedRef<STableViewBase>& OwnerTable);
	void OnMouseDoubleClick(TWeakObjectPtr<USourceCodeAsset> Item);
	TSharedRef<SWidget> WidgetRef =  SNullWidget::NullWidget;
	
	TArray<TWeakObjectPtr<USourceCodeAsset>> AssetsUsedInSourceCode;
};
