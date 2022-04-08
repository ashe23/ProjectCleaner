// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#include "UI/ProjectCleanerUnusedAssetsBrowserUI.h"
#include "UI/ProjectCleanerCommands.h"
#include "Core/ProjectCleanerManager.h"
#include "StructsContainer.h"
// Engine Headers
#include "IContentBrowserDataModule.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Editor/ContentBrowser/Public/ContentBrowserModule.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SScrollBorder.h"
#include "Toolkits/GlobalEditorCommonCommands.h"

#define LOCTEXT_NAMESPACE "FProjectCleanerModule"

void SProjectCleanerUnusedAssetsBrowserUI::Construct(const FArguments& InArgs)
{
	if (InArgs._CleanerManager)
	{
		SetCleanerManager(InArgs._CleanerManager);
	}

	ensure(CleanerManager);
	
	ContentBrowserModule = &FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	
	FName CurrentlySelectedVirtualPath;
	IContentBrowserDataModule::Get().GetSubsystem()->ConvertInternalPathToVirtual(FStringView(SelectedPath), CurrentlySelectedVirtualPath);
	
	RegisterCommands();

	AssetPickerConfig.InitialAssetViewType = EAssetViewType::Tile;
	AssetPickerConfig.bCanShowFolders = true;
	AssetPickerConfig.bAddFilterUI = true;
	AssetPickerConfig.bPreloadAssetsForContextMenu = false;
	AssetPickerConfig.bSortByPathInColumnView = true;
	AssetPickerConfig.bShowPathInColumnView = true;
	AssetPickerConfig.bShowBottomToolbar = true;
	AssetPickerConfig.bCanShowDevelopersFolder = CleanerManager->GetCleanerConfigs()->bScanDeveloperContents;
	AssetPickerConfig.bCanShowClasses = false;
	AssetPickerConfig.bAllowDragging = false;
	AssetPickerConfig.bForceShowEngineContent = false;
	AssetPickerConfig.bCanShowRealTimeThumbnails = false;
	AssetPickerConfig.AssetShowWarningText = FText::FromName("No assets");
	AssetPickerConfig.GetCurrentSelectionDelegates.Add(&CurrentSelectionDelegate);
	AssetPickerConfig.RefreshAssetViewDelegates.Add(&RefreshAssetViewDelegate);
	AssetPickerConfig.OnAssetDoubleClicked = FOnAssetDoubleClicked::CreateStatic(
		&SProjectCleanerUnusedAssetsBrowserUI::OnAssetDblClicked
	);
	AssetPickerConfig.OnGetAssetContextMenu = FOnGetAssetContextMenu::CreateRaw(
		this,
		&SProjectCleanerUnusedAssetsBrowserUI::OnGetAssetContextMenu
	);
	AssetPickerConfig.OnFolderEntered = FOnPathSelected::CreateSP(this, &SProjectCleanerUnusedAssetsBrowserUI::OnFolderEntered);
	AssetPickerConfig.SetFilterDelegates.Add(&SetFilterDelegate);

	PathPickerConfig.bAllowContextMenu = true;
	PathPickerConfig.bAllowClassesFolder = false;
	PathPickerConfig.bFocusSearchBoxWhenOpened = false;
	PathPickerConfig.OnPathSelected = FOnPathSelected::CreateSP(this, &SProjectCleanerUnusedAssetsBrowserUI::OnPathSelected);
	PathPickerConfig.bAddDefaultPath = true;
	PathPickerConfig.DefaultPath = SelectedPath;
	PathPickerConfig.OnGetFolderContextMenu = FOnGetFolderContextMenu::CreateSP(
		this, &SProjectCleanerUnusedAssetsBrowserUI::OnGetFolderContextMenu
	);
	PathPickerConfig.SetPathsDelegates.Add(&SetPathsDelegate);
	
	UpdateUI();
	
	ChildSlot
	[
		SNew(SSplitter)
		.PhysicalSplitterHandleSize(2.0f)
		+ SSplitter::Slot()
		.Value(0.3f)
		[
			ContentBrowserModule->Get().CreatePathPicker(PathPickerConfig)
		]
		+ SSplitter::Slot()
		[
			ContentBrowserModule->Get().CreateAssetPicker(AssetPickerConfig)
		]
	];
}

void SProjectCleanerUnusedAssetsBrowserUI::SetCleanerManager(FProjectCleanerManager* CleanerManagerPtr)
{
	if (!CleanerManagerPtr) return;
	CleanerManager = CleanerManagerPtr;
}

void SProjectCleanerUnusedAssetsBrowserUI::RegisterCommands()
{
	FProjectCleanerCommands::Register();

	Commands = MakeShareable(new FUICommandList);
	Commands->MapAction(
		FGlobalEditorCommonCommands::Get().FindInContentBrowser,
		FUIAction(
			FExecuteAction::CreateRaw(
				this,
				&SProjectCleanerUnusedAssetsBrowserUI::FindInContentBrowser
			),
			FCanExecuteAction::CreateRaw(
				this,
				&SProjectCleanerUnusedAssetsBrowserUI::IsAnythingSelected
			)
		)
	);
	
	Commands->MapAction(FProjectCleanerCommands::Get().DeleteAsset,
		FUIAction
		(
	FExecuteAction::CreateRaw(
				this,
				&SProjectCleanerUnusedAssetsBrowserUI::DeleteAsset
			),
			FCanExecuteAction::CreateRaw(
				this,
				&SProjectCleanerUnusedAssetsBrowserUI::IsAnythingSelected
			)
		)
	);

	Commands->MapAction(
		FProjectCleanerCommands::Get().ExcludeAsset,
		FUIAction
		(
			FExecuteAction::CreateRaw(
				this,
				&SProjectCleanerUnusedAssetsBrowserUI::ExcludeAsset
			),
			FCanExecuteAction::CreateRaw(
				this,
				&SProjectCleanerUnusedAssetsBrowserUI::IsAnythingSelected
			)
		)
	);

	Commands->MapAction(
		FProjectCleanerCommands::Get().ExcludeByType,
		FUIAction
		(
			FExecuteAction::CreateRaw(
				this,
				&SProjectCleanerUnusedAssetsBrowserUI::ExcludeAssetsOfType
			),
			FCanExecuteAction::CreateRaw(
				this,
				&SProjectCleanerUnusedAssetsBrowserUI::IsAnythingSelected
			)
		)
	);

	Commands->MapAction(
		FProjectCleanerCommands::Get().ExcludePath,
		FUIAction
		(
			FExecuteAction::CreateRaw(
				this,
				&SProjectCleanerUnusedAssetsBrowserUI::ExcludePath
			)
		)
	);
}

void SProjectCleanerUnusedAssetsBrowserUI::UpdateUI()
{
	if (!ContentBrowserModule) return;
	if (!CleanerManager->GetCleanerConfigs()) return;

	GenerateFilter();
}

void SProjectCleanerUnusedAssetsBrowserUI::OnFolderEntered(const FString& NewPath)
{
	SelectedPath = NewPath;

	TArray<FString> NewPaths;
	NewPaths.Add(NewPath);
	SetPathsDelegate.Execute(NewPaths);
}

void SProjectCleanerUnusedAssetsBrowserUI::GenerateFilter()
{
	FName CurrentlySelectedVirtualPath;
	IContentBrowserDataModule::Get().GetSubsystem()->ConvertInternalPathToVirtual(FStringView(SelectedPath), CurrentlySelectedVirtualPath);
	
	Filter.Clear();
	Filter.PackagePaths.Add(CurrentlySelectedVirtualPath);
	
	if (CleanerManager->GetUnusedAssets().Num() == 0)
	{
		// this is needed for disabling showing primary assets in browser, when there is no unused assets
		Filter.TagsAndValues.Add(FName{ "ProjectCleanerEmptyTag" }, FString{ "ProjectCleanerEmptyTag" });
	}
	else
	{
		Filter.PackageNames.Reserve(CleanerManager->GetUnusedAssets().Num());
		for (const auto& Asset : CleanerManager->GetUnusedAssets())
		{
			Filter.PackageNames.Add(Asset.PackageName);
		}
	}
	
	AssetPickerConfig.bCanShowDevelopersFolder = CleanerManager->GetCleanerConfigs()->bScanDeveloperContents;
	AssetPickerConfig.Filter = Filter;
	
	if (SetFilterDelegate.IsBound())
	{
		SetFilterDelegate.Execute(Filter);
	}
}

TSharedPtr<SWidget> SProjectCleanerUnusedAssetsBrowserUI::OnGetFolderContextMenu(const TArray<FString>& SelectedPaths,
	FContentBrowserMenuExtender_SelectedPaths InMenuExtender, FOnCreateNewFolder InOnCreateNewFolder) const
{
	FMenuBuilder MenuBuilder(true, Commands);
	MenuBuilder.BeginSection(TEXT("Exclude"),LOCTEXT("exclude_by_path", "Path"));
	{
		MenuBuilder.AddMenuEntry(FProjectCleanerCommands::Get().ExcludePath);
	}
	MenuBuilder.EndSection();

	return MenuBuilder.MakeWidget();
}

TSharedPtr<SWidget> SProjectCleanerUnusedAssetsBrowserUI::OnGetAssetContextMenu(const TArray<FAssetData>& SelectedAssets) const
{
	FMenuBuilder MenuBuilder{true, Commands};
	MenuBuilder.BeginSection(TEXT("Asset"), LOCTEXT("AssetSectionLabel", "Asset"));
	{
		MenuBuilder.AddMenuEntry(FGlobalEditorCommonCommands::Get().FindInContentBrowser);
		MenuBuilder.AddMenuEntry(FProjectCleanerCommands::Get().DeleteAsset);
		MenuBuilder.AddMenuEntry(FProjectCleanerCommands::Get().ExcludeAsset);
		MenuBuilder.AddMenuEntry(FProjectCleanerCommands::Get().ExcludeByType);
	}
	MenuBuilder.EndSection();

	return MenuBuilder.MakeWidget();
}

void SProjectCleanerUnusedAssetsBrowserUI::OnAssetDblClicked(const FAssetData& AssetData)
{
	if (!GEditor) return;
	
	TArray<FName> AssetNames;
	AssetNames.Add(AssetData.ObjectPath);

	GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorsForAssets(AssetNames);
}

void SProjectCleanerUnusedAssetsBrowserUI::OnPathSelected(const FString& Path)
{
	SelectedPath = Path;
	
	UpdateUI();
}

void SProjectCleanerUnusedAssetsBrowserUI::FindInContentBrowser() const
{
	if (!CurrentSelectionDelegate.IsBound()) return;

	const TArray<FAssetData> CurrentSelection = CurrentSelectionDelegate.Execute();
	if (CurrentSelection.Num() == 0) return;
	
	const FContentBrowserModule& CBModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	CBModule.Get().SyncBrowserToAssets(CurrentSelection);
}

bool SProjectCleanerUnusedAssetsBrowserUI::IsAnythingSelected() const
{
	if (!CurrentSelectionDelegate.IsBound()) return false;

	const TArray<FAssetData> CurrentSelection = CurrentSelectionDelegate.Execute();
	return CurrentSelection.Num() > 0;
}

void SProjectCleanerUnusedAssetsBrowserUI::DeleteAsset() const
{
	if (!CurrentSelectionDelegate.IsBound()) return;

	const TArray<FAssetData> CurrentSelection = CurrentSelectionDelegate.Execute();
	CleanerManager->DeleteSelectedAssets(CurrentSelection);
}

void SProjectCleanerUnusedAssetsBrowserUI::ExcludeAsset() const
{
	if (!CurrentSelectionDelegate.IsBound()) return;

	const TArray<FAssetData> SelectedAssets = CurrentSelectionDelegate.Execute();
	CleanerManager->ExcludeSelectedAssets(SelectedAssets);
}

void SProjectCleanerUnusedAssetsBrowserUI::ExcludeAssetsOfType() const
{
	if (!CurrentSelectionDelegate.IsBound()) return;
	
	const TArray<FAssetData> SelectedAssets = CurrentSelectionDelegate.Execute();
	CleanerManager->ExcludeSelectedAssetsByType(SelectedAssets);
}

void SProjectCleanerUnusedAssetsBrowserUI::ExcludePath() const
{
	CleanerManager->ExcludePath(SelectedPath);
}

#undef LOCTEXT_NAMESPACE
