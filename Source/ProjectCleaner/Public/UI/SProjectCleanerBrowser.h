#pragma once

#include "CoreMinimal.h"
#include "IDetailsView.h"
#include "Widgets/SCompoundWidget.h"
#include "Engine/EngineTypes.h"
#include "SProjectCleanerBrowser.generated.h"


UCLASS()
class UDirectoryFilterSettings : public UObject
{

	GENERATED_BODY()
public:
	UPROPERTY(DisplayName = "Directory", EditAnywhere, Category = "ExcludeThisDirectories", meta = (ContentDir))
	TArray<FDirectoryPath> DirectoryPaths;
};

UCLASS()
class UNonUProjectFiles : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(DisplayName = "Files", VisibleAnywhere, Category = "NonProjectFiles")
	TArray<FString> Files;

	UPROPERTY(DisplayName = "EmptyFolders", VisibleAnywhere, Category = "NonProjectFiles")
	TArray<FString> EmptyFolders;

	UPROPERTY(DisplayName = "UsedSourceFiles", VisibleAnywhere, Category = "NonProjectFiles")
	TMap<FString, FString> UsedSourceFiles;
};

UCLASS()
class UUnusedAssetsUIContainer : public UObject
{
	GENERATED_BODY()

public:
	TArray<FAssetData> UnusedAssets;
};

/**
 * 
 */
class SProjectCleanerBrowser : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SProjectCleanerBrowser) {}
		SLATE_ARGUMENT(UDirectoryFilterSettings*, DirectoryFilterSettings)
		SLATE_ARGUMENT(UNonUProjectFiles*, NonProjectFiles)
		SLATE_ARGUMENT(UUnusedAssetsUIContainer*, UnusedAssets)
	SLATE_END_ARGS()

	typedef TSharedPtr<FString> FComboItemType;

	void Construct(const FArguments& InArgs);

	TSharedPtr<IDetailsView> DirectoryFilterProperty;
	UDirectoryFilterSettings* DirectoryFilterSettings;

	TSharedPtr<IDetailsView> NonProjectFilesProperty;
	UNonUProjectFiles* NonUProjectFiles;

	TSharedPtr<IDetailsView> UnusedAssetsUIContainerProperty;
	UUnusedAssetsUIContainer* UnusedAssetsUIContainer;
private:
	TSharedPtr<SWidget> OnGetAssetContextMenu(const TArray<FAssetData>& SelectedAssets);
	void FindInContentBrowser() const;
	FString GetStringValueForCustomColumn(FAssetData& AssetData, FName ColumnName) const;
	FText GetDisplayTextForCustomColumn(FAssetData& AssetData, FName ColumnName) const;
	

};
