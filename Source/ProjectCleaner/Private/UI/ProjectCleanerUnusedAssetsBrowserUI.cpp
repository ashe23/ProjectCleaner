// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#include "UI/ProjectCleanerUnusedAssetsBrowserUI.h"
#include "UI/ProjectCleanerCommands.h"
// Engine Headers
#include "ObjectTools.h"
#include "Editor/ContentBrowser/Public/ContentBrowserModule.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SScrollBorder.h"
#include "Toolkits/GlobalEditorCommonCommands.h"

#define LOCTEXT_NAMESPACE "FProjectCleanerModule"

void SProjectCleanerUnusedAssetsBrowserUI::Construct(const FArguments& InArgs)
{
	ContentBrowserModule = &FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");

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
	
	if (InArgs._CleanerConfigs)
	{
		SetCleanerConfigs(InArgs._CleanerConfigs);
	}
	if (InArgs._UnusedAssets)
	{
		SetUnusedAssets(*InArgs._UnusedAssets);
	}
	if (InArgs._PrimaryAssetClasses)
	{
		SetPrimaryAssetClasses(*InArgs._PrimaryAssetClasses);
	}

	AssetPickerConfig.InitialAssetViewType = EAssetViewType::Tile;
	AssetPickerConfig.bCanShowFolders = true;
	AssetPickerConfig.bAddFilterUI = true;
	AssetPickerConfig.bPreloadAssetsForContextMenu = false;
	AssetPickerConfig.bSortByPathInColumnView = true;
	AssetPickerConfig.bShowPathInColumnView = true;
	AssetPickerConfig.bShowBottomToolbar = true;
	AssetPickerConfig.bCanShowDevelopersFolder = CleanerConfigs->bScanDeveloperContents;
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

	SelectedPath = FName{TEXT("/Game")};
	PathPickerConfig.bAllowContextMenu = false;
	PathPickerConfig.bAllowClassesFolder = false;
	PathPickerConfig.bFocusSearchBoxWhenOpened = false;
	PathPickerConfig.OnPathSelected.BindRaw(this, &SProjectCleanerUnusedAssetsBrowserUI::OnPathSelected);
	PathPickerConfig.bAddDefaultPath = true;
	PathPickerConfig.DefaultPath = SelectedPath.ToString();

	UpdateUI();
}

void SProjectCleanerUnusedAssetsBrowserUI::SetUnusedAssets(const TArray<FAssetData>& NewUnusedAssets)
{
	UnusedAssets.Reset();
	UnusedAssets.Reserve(NewUnusedAssets.Num());
	UnusedAssets = MoveTempIfPossible(NewUnusedAssets);
}

void SProjectCleanerUnusedAssetsBrowserUI::SetCleanerConfigs(UCleanerConfigs* Configs)
{
	if (!Configs) return;

	CleanerConfigs = Configs;
}

void SProjectCleanerUnusedAssetsBrowserUI::SetPrimaryAssetClasses(const TSet<FName>& NewPrimaryAssetClasses)
{
	PrimaryAssetClasses.Reset();
	PrimaryAssetClasses.Reserve(NewPrimaryAssetClasses.Num());
	PrimaryAssetClasses = MoveTempIfPossible(NewPrimaryAssetClasses);
}

void SProjectCleanerUnusedAssetsBrowserUI::SetUIData(const TArray<FAssetData>& NewUnusedAssets, UCleanerConfigs* NewConfigs, const TSet<FName>& NewPrimaryAssetClasses)
{
	SetUnusedAssets(NewUnusedAssets);
	SetCleanerConfigs(NewConfigs);
	SetPrimaryAssetClasses(NewPrimaryAssetClasses);

	UpdateUI();
}

void SProjectCleanerUnusedAssetsBrowserUI::UpdateUI()
{
	if (!CleanerConfigs) return;
	if (!ContentBrowserModule) return;

	GenerateFilter();
	
	ChildSlot
	[
		SNew(SSplitter)
		.Style(FEditorStyle::Get(), "ContentBrowser.Splitter")
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

void SProjectCleanerUnusedAssetsBrowserUI::GenerateFilter()
{
	Filter.Clear();
	
	if (UnusedAssets.Num() == 0)
	{
		// this is needed for disabling showing primary assets in browser, when there is no unused assets
		Filter.TagsAndValues.Add(FName{ "ProjectCleanerEmptyTag" }, FString{ "ProjectCleanerEmptyTag" });
	}
	else
	{
		// excluding primary assets from showing and filtering
		Filter.bRecursiveClasses = true;
		Filter.RecursiveClassesExclusionSet.Reserve(PrimaryAssetClasses.Num());
		Filter.PackageNames.Reserve(UnusedAssets.Num());
		Filter.RecursiveClassesExclusionSet = MoveTempIfPossible(PrimaryAssetClasses);

		for (const auto& Asset : UnusedAssets)
		{
			Filter.PackageNames.Add(Asset.PackageName);
		}
	}

	if (!SelectedPath.IsNone())
	{
		Filter.PackagePaths.Add(SelectedPath);
	}

	AssetPickerConfig.Filter = Filter;
}

TSharedPtr<SWidget> SProjectCleanerUnusedAssetsBrowserUI::OnGetAssetContextMenu(const TArray<FAssetData>& SelectedAssets) const
{
	FMenuBuilder MenuBuilder{true, Commands};
	MenuBuilder.BeginSection(
		TEXT("Asset"),
		NSLOCTEXT("ReferenceViewerSchema", "AssetSectionLabel", "Asset")
	);
	{
		MenuBuilder.AddMenuEntry(FGlobalEditorCommonCommands::Get().FindInContentBrowser);
		MenuBuilder.AddMenuEntry(FProjectCleanerCommands::Get().DeleteAsset);
		MenuBuilder.AddMenuEntry(FProjectCleanerCommands::Get().ExcludeAsset);
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
	SelectedPath = FName{Path};
	PathPickerConfig.DefaultPath = Path;
	UpdateUI();
}

void SProjectCleanerUnusedAssetsBrowserUI::FindInContentBrowser() const
{
	if (!CurrentSelectionDelegate.IsBound()) return;

	const TArray<FAssetData> CurrentSelection = CurrentSelectionDelegate.Execute();
	if (CurrentSelection.Num() > 0)
	{
		FContentBrowserModule& CBModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
		CBModule.Get().SyncBrowserToAssets(CurrentSelection);
	}
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
	if (!CurrentSelection.Num()) return;
	
	const int32 DeletedAssetsNum = ObjectTools::DeleteAssets(CurrentSelection);
	if (DeletedAssetsNum == 0) return;
	if (!OnUserDeletedAssets.IsBound()) return;
	OnUserDeletedAssets.Execute();
}

void SProjectCleanerUnusedAssetsBrowserUI::ExcludeAsset() const
{
	if(!CurrentSelectionDelegate.IsBound()) return;

	const TArray<FAssetData> SelectedAssets = CurrentSelectionDelegate.Execute();
	if(!SelectedAssets.Num()) return;
	
	if(!OnUserExcludedAssets.IsBound()) return;
	OnUserExcludedAssets.Execute(SelectedAssets);
}

#undef LOCTEXT_NAMESPACE