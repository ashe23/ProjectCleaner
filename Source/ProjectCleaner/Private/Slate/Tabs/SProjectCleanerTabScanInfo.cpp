// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/Tabs/SProjectCleanerTabScanInfo.h"
// Engine Headers
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "ProjectCleanerSubsystem.h"

void SProjectCleanerTabScanInfo::Construct(const FArguments& InArgs)
{
	if (!GEditor) return;
	Subsystem = GEditor->GetEditorSubsystem<UProjectCleanerSubsystem>();
	if (!Subsystem) return;

	Subsystem->OnProjectScanned().AddLambda([&]()
	{
		UpdateView();
	});

	UpdateView();
}

void SProjectCleanerTabScanInfo::UpdateView()
{
	const FContentBrowserModule& ModuleContentBrowser = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");

	FAssetPickerConfig AssetPickerConfig;
	AssetPickerConfig.InitialAssetViewType = EAssetViewType::Tile;
	AssetPickerConfig.bCanShowFolders = true;
	AssetPickerConfig.bAddFilterUI = true;
	AssetPickerConfig.bPreloadAssetsForContextMenu = false;
	AssetPickerConfig.bSortByPathInColumnView = true;
	AssetPickerConfig.bShowPathInColumnView = true;
	AssetPickerConfig.bShowBottomToolbar = true;
	AssetPickerConfig.bCanShowDevelopersFolder = true;
	AssetPickerConfig.bCanShowClasses = false;
	AssetPickerConfig.bAllowDragging = false;
	AssetPickerConfig.bForceShowEngineContent = false;
	AssetPickerConfig.bCanShowRealTimeThumbnails = false;
	AssetPickerConfig.AssetShowWarningText = FText::FromName("No assets");
	// AssetPickerConfig.GetCurrentSelectionDelegates.Add(&CurrentSelectionDelegate);
	// AssetPickerConfig.RefreshAssetViewDelegates.Add(&RefreshAssetViewDelegate);
	// AssetPickerConfig.OnAssetDoubleClicked = FOnAssetDoubleClicked::CreateStatic(
	// 	&SProjectCleanerUnusedAssetsBrowserUI::OnAssetDblClicked
	// );
	// AssetPickerConfig.OnGetAssetContextMenu = FOnGetAssetContextMenu::CreateRaw(
	// 	this,
	// 	&SProjectCleanerUnusedAssetsBrowserUI::OnGetAssetContextMenu
	// );

	FARFilter Filter;
	if (Subsystem->GetScanData().AssetsUnused.Num() == 0)
	{
		// this is needed for disabling showing primary assets in browser, when there is no unused assets
		Filter.TagsAndValues.Add(FName{"ProjectCleanerEmptyTag"}, FString{"ProjectCleanerEmptyTag"});
	}
	else
	{
		Filter.PackageNames.Reserve(Subsystem->GetScanData().AssetsUnused.Num());
		for (const auto& Asset : Subsystem->GetScanData().AssetsUnused)
		{
			Filter.PackageNames.Add(Asset.PackageName);
		}
	}

	// if (!SelectedPath.IsNone())
	// {
	// 	Filter.PackagePaths.Add(SelectedPath);
	// }

	// AssetPickerConfig.bCanShowDevelopersFolder = CleanerManager->GetCleanerConfigs()->bScanDeveloperContents;
	AssetPickerConfig.Filter = Filter;

	ChildSlot
	[
		ModuleContentBrowser.Get().CreateAssetPicker(AssetPickerConfig)
	];
}

void SProjectCleanerTabScanInfo::UpdateTreeView()
{
}

void SProjectCleanerTabScanInfo::UpdateAssetBrowser()
{
}
