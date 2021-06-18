// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "ProjectCleanerNonUassetFilesUI.generated.h"


UCLASS(Transient)
class UNonUassetFile : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY(DisplayName = "FileName", VisibleAnywhere, Category = "NonUassetFile")
	FString FileName;

	UPROPERTY(DisplayName = "FilePath", VisibleAnywhere, Category = "NonUassetFile")
	FString FilePath;
};

class SNonUassetFileUISelectionRow : public SMultiColumnTableRow<TWeakObjectPtr<UNonUassetFile>>
{
public:
	
	SLATE_BEGIN_ARGS(SNonUassetFileUISelectionRow) {}
		SLATE_ARGUMENT(TWeakObjectPtr<UNonUassetFile>, SelectedRowItem)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView)
	{
		SelectedRowItem = InArgs._SelectedRowItem;

		SMultiColumnTableRow<TWeakObjectPtr<UNonUassetFile>>::Construct(
			SMultiColumnTableRow<TWeakObjectPtr<UNonUassetFile>>::FArguments()
			.Padding(
				FMargin(0.f, 2.f, 0.f, 0.f)),
			InOwnerTableView
		);
	}

	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& InColumnName) override
	{
		TSharedPtr<SWidget> ColumnWidget;

		if (InColumnName == TEXT("FileName"))
		{
			ColumnWidget = SNew(STextBlock).Text(FText::FromString(SelectedRowItem->FileName));
		}
		else if (InColumnName == TEXT("FilePath"))
		{
			ColumnWidget = SNew(STextBlock).Text(FText::FromString(SelectedRowItem->FilePath));
		}
		else
		{
			ColumnWidget = SNew(STextBlock).Text(FText::FromString("No Data"));
		}

		return ColumnWidget.ToSharedRef();
	}

private:
	TWeakObjectPtr<UNonUassetFile> SelectedRowItem;
};


class SProjectCleanerNonUassetFilesUI : public SCompoundWidget
{
public:
	
	SLATE_BEGIN_ARGS(SProjectCleanerNonUassetFilesUI) {}
		SLATE_ARGUMENT(TSet<FString>, NonUassetFiles)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	void SetNonUassetFiles(const TSet<FString>& NewNonUassetFile);
private:
	void RefreshUIContent();
	TSharedRef<ITableRow> OnGenerateRow(
		TWeakObjectPtr<UNonUassetFile> InItem,
		const TSharedRef<STableViewBase>& OwnerTable
	) const;
	void OnMouseDoubleClick(TWeakObjectPtr<UNonUassetFile> Item) const;

	/** Data **/
	TArray<TWeakObjectPtr<UNonUassetFile>> NonUassetFiles;
	TSharedPtr<SListView<TWeakObjectPtr<UNonUassetFile>>> ListView;
	TSharedRef<SWidget> WidgetRef = SNullWidget::NullWidget;
};