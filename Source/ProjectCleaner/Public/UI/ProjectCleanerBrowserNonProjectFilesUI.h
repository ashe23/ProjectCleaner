#pragma once

// Engine Headers
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "ProjectCleanerBrowserNonProjectFilesUI.generated.h"


UCLASS()
class UNonProjectFilesInfo : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(DisplayName = "Files", VisibleAnywhere, Category = "NonProjectFiles")
	TArray<FString> Files;

	UPROPERTY(DisplayName = "EmptyFolders", VisibleAnywhere, Category = "EmptyFolders")
	TArray<FString> EmptyFolders;
};

UCLASS()
class UAssetsUsedInSourceCode : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY(DisplayName = "AssetName", VisibleAnywhere)
	FName AssetName;
	UPROPERTY(DisplayName = "AssetPath", VisibleAnywhere)
	FName AssetPath;
	UPROPERTY(DisplayName = "SourceCodePath", VisibleAnywhere)
	FName SourceCodePath;
};

// todo:ashe23 TESTING
class SAssetsUsedInSourceCodeSelectionRow
: public SMultiColumnTableRow<TWeakObjectPtr<UAssetsUsedInSourceCode>>
{
public:
	SLATE_BEGIN_ARGS(SAssetsUsedInSourceCodeSelectionRow) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef< STableViewBase >& InOwnerTableView)
	{
		SMultiColumnTableRow< TWeakObjectPtr< UAssetsUsedInSourceCode > >::Construct(SMultiColumnTableRow< TWeakObjectPtr< UAssetsUsedInSourceCode > >::FArguments().Padding(FMargin(0.f,2.f,0.f,0.f)), InOwnerTableView);
	}

	virtual TSharedRef<SWidget> GenerateWidgetForColumn( const FName& ColumnName ) override
	{
		TSharedPtr< SWidget > ColumnWidget;

		ColumnWidget = SNew(STextBlock).Text(TEXT("AAA"));

		return ColumnWidget.ToSharedRef();
	}

private:
	// TWeakObjectPtr<UAssetsUsedInSourceCode> AssetsUsedInSourceCode;
};




//todo:ashe23 TESTING END

class SProjectCleanerBrowserNonProjectFilesUI : public SCompoundWidget
{
public:
SLATE_BEGIN_ARGS(SProjectCleanerBrowserNonProjectFilesUI)
		{
		}

		SLATE_ARGUMENT(UNonProjectFilesInfo*, NonProjectFiles)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	TSharedPtr<IDetailsView> NonProjectFilesProperty;
	UNonProjectFilesInfo* NonUProjectFilesInfo = nullptr;

	TArray<TWeakObjectPtr<UAssetsUsedInSourceCode>> AssetsUsedInSourceCodes;
	TSharedRef< ITableRow > OnGenerateRow( TWeakObjectPtr<UDeviceProfile> InItem, const TSharedRef< STableViewBase >& OwnerTable );
};
