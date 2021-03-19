#include "UI/SProjectCleanerBrowser.h"
// Engine Headers
#include "PropertyEditorModule.h"
#include "Modules/ModuleManager.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Editor/ContentBrowser/Public/ContentBrowserModule.h"
#include "AssetManagerEditorModule.h"
#include "IContentBrowserSingleton.h"
#include "ContentBrowserDelegates.h"

void SProjectCleanerBrowser::Construct(const FArguments& InArgs)
{

	FPropertyEditorModule& PropertyEditor = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

	FDetailsViewArgs FolderFilterArgs;
	FolderFilterArgs.bUpdatesFromSelection = false;
	FolderFilterArgs.bLockable = false;
	FolderFilterArgs.bAllowSearch = false;
	FolderFilterArgs.bShowOptions = false;
	FolderFilterArgs.bAllowFavoriteSystem = false;
	FolderFilterArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	FolderFilterArgs.ViewIdentifier = "ExcludeDirectoriesFromScan";


	FDetailsViewArgs NonProjectFilesSettings;
	NonProjectFilesSettings.bUpdatesFromSelection = false;
	NonProjectFilesSettings.bLockable = false;
	NonProjectFilesSettings.bShowOptions = false;
	NonProjectFilesSettings.bAllowFavoriteSystem = false;
	NonProjectFilesSettings.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	NonProjectFilesSettings.bShowPropertyMatrixButton = false;
	NonProjectFilesSettings.ViewIdentifier = "NonProjectFileList";

	FDetailsViewArgs UnusedAssetsUIContainerSettings;
	UnusedAssetsUIContainerSettings.ViewIdentifier = "UnusedAssetsUIContainerSettings";
	
	DirectoryFilterProperty = PropertyEditor.CreateDetailView(FolderFilterArgs);
	NonProjectFilesProperty = PropertyEditor.CreateDetailView(NonProjectFilesSettings);
	UnusedAssetsUIContainerProperty = PropertyEditor.CreateDetailView(UnusedAssetsUIContainerSettings);

	if(InArgs._DirectoryFilterSettings)
	{
		DirectoryFilterSettings = InArgs._DirectoryFilterSettings;
		DirectoryFilterProperty->SetObject(DirectoryFilterSettings);
	}

	if(InArgs._NonProjectFiles)
	{
		NonUProjectFiles = InArgs._NonProjectFiles;
		NonProjectFilesProperty->SetObject(NonUProjectFiles);
	}

	if(InArgs._UnusedAssets)
	{
		UnusedAssetsUIContainer = InArgs._UnusedAssets;
		UnusedAssetsUIContainerProperty->SetObject(UnusedAssetsUIContainer);
	}

	FARFilter Filter;
	for (const auto& Asset : UnusedAssetsUIContainer->UnusedAssets)
	{
		Filter.PackageNames.Add(Asset.PackageName);
	}		

	FAssetPickerConfig Config;
	Config.InitialAssetViewType = EAssetViewType::Column;
	Config.bAddFilterUI = true;
	Config.bShowPathInColumnView = true;
	Config.bSortByPathInColumnView = true;
	Config.bForceShowEngineContent = false;
	Config.Filter = Filter;
	// Config.OnGetAssetContextMenu = FOnGetAssetContextMenu::CreateRaw(
	// 	this,
	// 	&SProjectCleanerBrowser::OnGetAssetContextMenu
	// );

	// Config.CustomColumns.Emplace(
 //        IAssetManagerEditorModule::DiskSizeName,
 //        TEXT("Exclusive Disk Size"),
 //        TEXT("Size of saved file on disk for only this asset"),
 //        UObject::FAssetRegistryTag::TT_Numerical,
 //        FOnGetCustomAssetColumnData::CreateSP(this, &SProjectCleanerBrowser::GetStringValueForCustomColumn),
 //        FOnGetCustomAssetColumnDisplayText::CreateSP(this, &SProjectCleanerBrowser::GetDisplayTextForCustomColumn)
 //    );
	Config.HiddenColumnNames.Add(TEXT("Class"));
	Config.HiddenColumnNames.Add(TEXT("Path"));
	Config.HiddenColumnNames.Add(TEXT("BlueprintType"));

	Config.bShowBottomToolbar = false;
	Config.bCanShowDevelopersFolder = false;

	FContentBrowserModule& ContentBrowser = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	

	ChildSlot
	[
		SNew(SScrollBox)
        + SScrollBox::Slot()
        [
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
            .Padding(FMargin{40})
            .AutoHeight()
            [
				ContentBrowser.Get().CreateAssetPicker(Config)
            ]    
        	+ SVerticalBox::Slot()
        	.Padding(FMargin(20))
        	.AutoHeight()
        	[
				SNew(SVerticalBox)
				+SVerticalBox::Slot()
				.Padding(20)
				[
					SNew(SBorder)
					.Padding(FMargin(20))
					[
						DirectoryFilterProperty.ToSharedRef()
					]
				]
			]
			+ SVerticalBox::Slot()
            .Padding(FMargin{40})
            .AutoHeight()
            [
				NonProjectFilesProperty.ToSharedRef()
            ]           
		]
		
	];

}

TSharedPtr<SWidget> SProjectCleanerBrowser::OnGetAssetContextMenu(const TArray<FAssetData>& SelectedAssets)
{
	 return nullptr;
}

void SProjectCleanerBrowser::FindInContentBrowser() const
{
}

FString SProjectCleanerBrowser::GetStringValueForCustomColumn(FAssetData& AssetData, FName ColumnName) const
{
	FString OutValue;
	IAssetManagerEditorModule::Get().GetStringValueForCustomColumn(AssetData, ColumnName, OutValue);
	return OutValue;
}

FText SProjectCleanerBrowser::GetDisplayTextForCustomColumn(FAssetData& AssetData, FName ColumnName) const
{
	FText OutValue;
	IAssetManagerEditorModule::Get().GetDisplayTextForCustomColumn(AssetData, ColumnName, OutValue);
	return OutValue;
}