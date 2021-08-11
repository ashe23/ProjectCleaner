// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "StructsContainer.h"

class FProjectCleanerManager;

class SNonEngineFilesUISelectionRow : public SMultiColumnTableRow<TWeakObjectPtr<UNonEngineFile>>
{
public:
	
	SLATE_BEGIN_ARGS(SNonEngineFilesUISelectionRow) {}
		SLATE_ARGUMENT(TWeakObjectPtr<UNonEngineFile>, SelectedRowItem)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView)
	{
		SelectedRowItem = InArgs._SelectedRowItem;

		SMultiColumnTableRow<TWeakObjectPtr<UNonEngineFile>>::Construct(
			SMultiColumnTableRow<TWeakObjectPtr<UNonEngineFile>>::FArguments()
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
	TWeakObjectPtr<UNonEngineFile> SelectedRowItem;
};


class SProjectCleanerNonEngineFilesUI : public SCompoundWidget
{
public:
	
	SLATE_BEGIN_ARGS(SProjectCleanerNonEngineFilesUI) {}
		SLATE_ARGUMENT(FProjectCleanerManager*, CleanerManager)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	void SetCleanerManager(FProjectCleanerManager* CleanerManagerPtr);
	void UpdateUI();
private:
	TSharedRef<ITableRow> OnGenerateRow(
		TWeakObjectPtr<UNonEngineFile> InItem,
		const TSharedRef<STableViewBase>& OwnerTable
	) const;
	void OnMouseDoubleClick(TWeakObjectPtr<UNonEngineFile> Item) const;

	/** Data **/
	TArray<TWeakObjectPtr<UNonEngineFile>> NonEngineFiles;
	TSharedPtr<SListView<TWeakObjectPtr<UNonEngineFile>>> ListView;
	FProjectCleanerManager* CleanerManager = nullptr;
};