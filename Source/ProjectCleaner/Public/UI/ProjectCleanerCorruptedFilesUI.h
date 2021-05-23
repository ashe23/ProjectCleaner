// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#pragma once

// Engine Headers
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "ProjectCleanerCorruptedFilesUI.generated.h"

UCLASS(Transient)
class UCorruptedFile : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY(DisplayName = "Name", VisibleAnywhere, Category = "CorruptedFile")
	FString Name;
	UPROPERTY(DisplayName = "AbsolutePath", VisibleAnywhere, Category = "CorruptedFile")
	FString AbsolutePath;
};

class SCorruptedFileUISelectionRow : public SMultiColumnTableRow<TWeakObjectPtr<UCorruptedFile>>
{
public:

	SLATE_BEGIN_ARGS(SCorruptedFileUISelectionRow) {}
		SLATE_ARGUMENT(TWeakObjectPtr<UCorruptedFile>, SelectedRowItem)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView)
	{
		SelectedRowItem = InArgs._SelectedRowItem;
		
		SMultiColumnTableRow<TWeakObjectPtr<UCorruptedFile>>::Construct(
			SMultiColumnTableRow<TWeakObjectPtr<UCorruptedFile>>::FArguments()
			.Padding(
				FMargin(0.f, 2.f, 0.f, 0.f)),
			InOwnerTableView
		);
	}

	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& InColumnName) override
	{
		TSharedPtr<SWidget> ColumnWidget;

		if (InColumnName == TEXT("Name"))
		{
			ColumnWidget = SNew(STextBlock).Text(FText::FromString(SelectedRowItem->Name));
		}		
		else if (InColumnName == TEXT("AbsolutePath"))
		{
			ColumnWidget = SNew(STextBlock).Text(FText::FromString(SelectedRowItem->AbsolutePath));
		}
		else 
		{
			ColumnWidget = SNew(STextBlock).Text(FText::FromString("No Data"));
		}

		return ColumnWidget.ToSharedRef();
	}
private:
	TWeakObjectPtr<UCorruptedFile> SelectedRowItem;
};

class SProjectCleanerCorruptedFilesUI : public SCompoundWidget
{
public:
	
	SLATE_BEGIN_ARGS(SProjectCleanerCorruptedFilesUI) {}
		SLATE_ARGUMENT(TSet<FName>, CorruptedFiles);
	SLATE_END_ARGS()
	
	void Construct(const FArguments& InArgs);
	void SetCorruptedFiles(const TSet<FName>& NewCorruptedFiles);
private:
	void RefreshUIContent();
	TSharedRef<ITableRow> OnGenerateRow(
		TWeakObjectPtr<UCorruptedFile> InItem,
		const TSharedRef<STableViewBase>& OwnerTable
	) const;
	void OnMouseDoubleClick(TWeakObjectPtr<UCorruptedFile> Item) const;

	/** Data **/
	TArray<TWeakObjectPtr<UCorruptedFile>> CorruptedFiles;
	TSharedRef<SWidget> WidgetRef = SNullWidget::NullWidget;
};