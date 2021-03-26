#pragma once

// Engine Headers
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "ProjectCleanerNonProjectFilesUI.generated.h"


UCLASS(Transient)
class UNonProjectFilesUIStruct : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY(DisplayName = "FileName", VisibleAnywhere, Category="NonProjectFile")
	FString FileName;

	UPROPERTY(DisplayName = "FilePath", VisibleAnywhere, Category="NonProjectFile")
	FString FilePath;
};

class SNonProjectFileUISelectionRow : public SMultiColumnTableRow<TWeakObjectPtr<UNonProjectFilesUIStruct>>
{
public:
	SLATE_BEGIN_ARGS(SNonProjectFileUISelectionRow){}
		SLATE_ARGUMENT(TWeakObjectPtr<UNonProjectFilesUIStruct>, SelectedRowItem)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView)
	{
		SelectedRowItem = InArgs._SelectedRowItem;

		SMultiColumnTableRow<TWeakObjectPtr<UNonProjectFilesUIStruct>>::Construct(
		SMultiColumnTableRow<TWeakObjectPtr<UNonProjectFilesUIStruct>>::FArguments()
		.Padding(
            FMargin(0.f, 2.f, 0.f, 0.f)),
            InOwnerTableView
		);
	}

	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& InColumnName) override
	{
		TSharedPtr<SWidget> ColumnWidget;

		if(InColumnName == TEXT("FileName"))
		{
			ColumnWidget = SNew(STextBlock).Text(FText::FromString(SelectedRowItem->FileName));			
		}
		else if(InColumnName == TEXT("FilePath"))
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
	TWeakObjectPtr<UNonProjectFilesUIStruct> SelectedRowItem;
};


class SProjectCleanerNonProjectFilesUI : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SProjectCleanerNonProjectFilesUI) {}
		SLATE_ARGUMENT(TArray<struct FNonProjectFile>, NonProjectFiles);
	SLATE_END_ARGS()
	
	void Construct(const FArguments& InArgs);
	void SetNonProjectFiles(const TArray<FNonProjectFile> NewNonProjectFiles);	
private:
	TSharedRef<ITableRow> OnGenerateRow(TWeakObjectPtr<UNonProjectFilesUIStruct> InItem, const TSharedRef<STableViewBase>& OwnerTable);
	void OnMouseDoubleClick(TWeakObjectPtr<UNonProjectFilesUIStruct> Item);

	TArray<TWeakObjectPtr<UNonProjectFilesUIStruct>> NonProjectFilesUIStructs;
	TArray<struct FNonProjectFile> NonProjectFiles;

};
