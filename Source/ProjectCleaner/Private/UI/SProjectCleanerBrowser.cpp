#include "UI/SProjectCleanerBrowser.h"
// Engine Headers
#include "PropertyEditorModule.h"
#include "Modules/ModuleManager.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Editor/ContentBrowser/Public/ContentBrowserModule.h"
// #include "AssetManagerEditorModule.h"
#include "IContentBrowserSingleton.h"
#include "ContentBrowserDelegates.h"
#include "Toolkits/GlobalEditorCommonCommands.h"
#include "UI/ProjectCleanerBrowserCommands.h"

void SProjectCleanerBrowser::Construct(const FArguments& InArgs)
{
	FPropertyEditorModule& PropertyEditor = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

	FProjectCleanerBrowserCommands::Register();

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

	NonProjectFilesProperty = PropertyEditor.CreateDetailView(NonProjectFilesSettings);
	UnusedAssetsUIContainerProperty = PropertyEditor.CreateDetailView(UnusedAssetsUIContainerSettings);

	if (InArgs._NonProjectFiles)
	{
		NonUProjectFiles = InArgs._NonProjectFiles;
		NonProjectFilesProperty->SetObject(NonUProjectFiles);
	}

	if (InArgs._UnusedAssets)
	{
		UnusedAssetsUIContainer = InArgs._UnusedAssets;
		UnusedAssetsUIContainerProperty->SetObject(UnusedAssetsUIContainer);
	}

	FARFilter Filter;
	Filter.bRecursiveClasses = true;
	Filter.RecursiveClassesExclusionSet.Add(UWorld::StaticClass()->GetFName());
	// for (const auto& Asset : *UnusedAssetsUIContainer->UnusedAssets) // todo:ashe23 garbage collection issue
	// {
	// 	Filter.PackageNames.Add(Asset.PackageName);
	// }
	
	Commands = MakeShareable(new FUICommandList);
	Commands->MapAction(
		FGlobalEditorCommonCommands::Get().FindInContentBrowser,
		FUIAction(
			FExecuteAction::CreateRaw(this, &SProjectCleanerBrowser::FindInContentBrowser),
			FCanExecuteAction::CreateRaw(this, &SProjectCleanerBrowser::IsAnythingSelected)
		)
	);
	Commands->MapAction(
		FProjectCleanerBrowserCommands::Get().ViewReferences,
		FUIAction()
	);

	FAssetPickerConfig Config;
	Config.InitialAssetViewType = EAssetViewType::Tile;
	Config.bAddFilterUI = true;
	Config.bShowPathInColumnView = true;
	Config.bSortByPathInColumnView = true;
	Config.bForceShowEngineContent = false;
	Config.Filter = Filter;
	Config.GetCurrentSelectionDelegates.Add(&GetCurrentSelectionDelegate);
	Config.OnGetAssetContextMenu = FOnGetAssetContextMenu::CreateRaw(
		this,
		&SProjectCleanerBrowser::OnGetAssetContextMenu
	);

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

	Config.bShowBottomToolbar = true;
	Config.bCanShowDevelopersFolder = false;

	FContentBrowserModule& ContentBrowser = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>(
		"ContentBrowser");


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
	FMenuBuilder MenuBuilder{true, Commands};
	MenuBuilder.BeginSection(
		TEXT("Asset"),
		NSLOCTEXT("ReferenceViewerSchema", "AssetSectionLabel", "Asset")
	);
	{
		MenuBuilder.AddMenuEntry(FGlobalEditorCommonCommands::Get().FindInContentBrowser);
	}
	MenuBuilder.EndSection();
	MenuBuilder.BeginSection(
		TEXT("References"),
		NSLOCTEXT("ReferenceViewerSchema", "ReferencesSectionLabel", "References")
	);
	{
		MenuBuilder.AddMenuEntry(FProjectCleanerBrowserCommands::Get().ViewReferences);
	}
	MenuBuilder.EndSection();


	return MenuBuilder.MakeWidget();
}

void SProjectCleanerBrowser::FindInContentBrowser() const
{
	if (!GetCurrentSelectionDelegate.IsBound()) return;

	const TArray<FAssetData> CurrentSelection = GetCurrentSelectionDelegate.Execute();
	if (CurrentSelection.Num() > 0)
	{
		FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>(
			"ContentBrowser");
		ContentBrowserModule.Get().SyncBrowserToAssets(CurrentSelection);
	}
}

bool SProjectCleanerBrowser::IsAnythingSelected() const
{
	if (!GetCurrentSelectionDelegate.IsBound()) return false;

	const TArray<FAssetData> CurrentSelection = GetCurrentSelectionDelegate.Execute();
	return CurrentSelection.Num() > 0;
}