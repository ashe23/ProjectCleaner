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

UCLASS(Transient)
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

class SProjectCleanerBrowserNonProjectFilesUI : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SProjectCleanerBrowserNonProjectFilesUI) {}
		SLATE_ARGUMENT(UNonProjectFilesInfo*, NonProjectFiles)
	SLATE_END_ARGS()
	void Construct(const FArguments& InArgs);

private:
	TSharedPtr<IDetailsView> NonProjectFilesProperty;
	UNonProjectFilesInfo* NonUProjectFilesInfo = nullptr;
	TArray< TWeakObjectPtr<UAssetsUsedInSourceCode> > AssetsUsedInSourceCode;
	TSharedRef< SWidget > OnGenerateWidgetForUsedAssets( TSharedPtr<FString> InItem, const TSharedRef< STableViewBase >& OwnerTable );
};
