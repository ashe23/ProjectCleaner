// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#include "UI/ProjectCleanerExcludedAssetsUI.h"
#include "UI/ProjectCleanerCommands.h"
#include "UI/ProjectCleanerStyle.h"
#include "Core/ProjectCleanerManager.h"
#include "StructsContainer.h"
// Engine Headers
#include "IContentBrowserSingleton.h"
#include "IContentBrowserDataModule.h"
#include "Editor/ContentBrowser/Public/ContentBrowserModule.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Toolkits/GlobalEditorCommonCommands.h"

#define LOCTEXT_NAMESPACE "FProjectCleanerModule"

void SProjectCleanerExcludedAssetsUI::Construct(const FArguments& InArgs)
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
	AssetPickerConfig.SelectionMode = ESelectionMode::SingleToggle;
	AssetPickerConfig.bAddFilterUI = true;
	AssetPickerConfig.bShowPathInColumnView = true;
	AssetPickerConfig.bSortByPathInColumnView = true;
	AssetPickerConfig.bForceShowEngineContent = false;
	AssetPickerConfig.bShowBottomToolbar = true;
	AssetPickerConfig.bCanShowDevelopersFolder = CleanerManager->GetCleanerConfigs()->bScanDeveloperContents;
	AssetPickerConfig.bForceShowEngineContent = false;
	AssetPickerConfig.bCanShowClasses = false;
	AssetPickerConfig.bAllowDragging = false;
	AssetPickerConfig.bCanShowFolders = true;
	AssetPickerConfig.AssetShowWarningText = FText::FromName("No assets");
	AssetPickerConfig.GetCurrentSelectionDelegates.Add(&GetCurrentSelectionDelegate);
	AssetPickerConfig.OnAssetDoubleClicked = FOnAssetDoubleClicked::CreateStatic(
		&SProjectCleanerExcludedAssetsUI::OnAssetDblClicked
	);
	AssetPickerConfig.OnGetAssetContextMenu = FOnGetAssetContextMenu::CreateRaw(
		this,
		&SProjectCleanerExcludedAssetsUI::OnGetAssetContextMenu
	);
	AssetPickerConfig.OnFolderEntered = FOnPathSelected::CreateSP(this, &SProjectCleanerExcludedAssetsUI::OnFolderEntered);
	AssetPickerConfig.SetFilterDelegates.Add(&SetFilterDelegate);
	AssetPickerConfig.Filter.PackagePaths.Add(CurrentlySelectedVirtualPath);

	PathPickerConfig.bAllowContextMenu = true;
	PathPickerConfig.bAllowClassesFolder = false;
	PathPickerConfig.bFocusSearchBoxWhenOpened = false;
	PathPickerConfig.OnPathSelected.BindRaw(this, &SProjectCleanerExcludedAssetsUI::OnPathSelected);
	PathPickerConfig.bAddDefaultPath = true;
	PathPickerConfig.DefaultPath = SelectedPath;
	PathPickerConfig.OnGetFolderContextMenu = FOnGetFolderContextMenu::CreateSP(
		this, &SProjectCleanerExcludedAssetsUI::OnGetFolderContextMenu
	);
	PathPickerConfig.SetPathsDelegates.Add(&SetPathsDelegate);
	
	UpdateUI();
	
	ChildSlot
	[
		SNew(SSplitter)
		.PhysicalSplitterHandleSize(3.0f)
		+ SSplitter::Slot()
		.Value(0.2f)
		[
			ContentBrowserModule->Get().CreatePathPicker(PathPickerConfig)
		]
		+ SSplitter::Slot()
		.Value(0.8f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(10.0f, 10.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Top)
				[
					SNew(STextBlock)
					.AutoWrapText(true)
					.Font(FProjectCleanerStyle::Get().GetFontStyle("ProjectCleaner.Font.Light20"))
					.Text(LOCTEXT("exclude_assets_title_text", "Excluded Assets"))
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(10.0f, 10.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Top)
				[
					SNew(SButton)
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					.ToolTipText(LOCTEXT("exclude_assets_tooltip_text", "Includes all assets"))
					.Text(FText::FromString("Include all assets"))
					.OnClicked_Raw(this, &SProjectCleanerExcludedAssetsUI::IncludeAllAssets)
				]
			]
			+ SVerticalBox::Slot()
			.Padding(10.0f, 10.0f)
			[
				SNew(SBox)
				.HeightOverride(300.0f)
				.WidthOverride(300.0f)
				[
					ContentBrowserModule->Get().CreateAssetPicker(AssetPickerConfig)
				]
			]
		]
	];
}

void SProjectCleanerExcludedAssetsUI::SetCleanerManager(FProjectCleanerManager* CleanerManagerPtr)
{
	if (!CleanerManagerPtr) return;
	CleanerManager = CleanerManagerPtr;
}

void SProjectCleanerExcludedAssetsUI::RegisterCommands()
{
	FProjectCleanerCommands::Register();

	Commands = MakeShareable(new FUICommandList);
	Commands->MapAction(
		FGlobalEditorCommonCommands::Get().FindInContentBrowser,
		FUIAction(
			FExecuteAction::CreateRaw(this, &SProjectCleanerExcludedAssetsUI::FindInContentBrowser),
			FCanExecuteAction::CreateRaw(this, &SProjectCleanerExcludedAssetsUI::IsAnythingSelected)
		)
	);

	Commands->MapAction(
		FProjectCleanerCommands::Get().IncludeAsset,
		FUIAction
		(
			FExecuteAction::CreateRaw(this,&SProjectCleanerExcludedAssetsUI::IncludeAssets),
			FCanExecuteAction::CreateRaw(this, &SProjectCleanerExcludedAssetsUI::IsAnythingSelected)
		)
	);

	Commands->MapAction(
		FProjectCleanerCommands::Get().IncludePath,
		FUIAction
		(
			FExecuteAction::CreateRaw(this,&SProjectCleanerExcludedAssetsUI::IncludePath)
		)
	);
}

void SProjectCleanerExcludedAssetsUI::UpdateUI()
{
	if (!ContentBrowserModule) return;
	if (!CleanerManager->GetCleanerConfigs()) return;
	
	GenerateFilter();
}

TSharedPtr<SWidget> SProjectCleanerExcludedAssetsUI::OnGetAssetContextMenu(const TArray<FAssetData>& SelectedAssets) const
{
	FMenuBuilder MenuBuilder{true, Commands};
	MenuBuilder.BeginSection(TEXT("Asset"),LOCTEXT("AssetSectionLabel", "Asset"));
	{
		MenuBuilder.AddMenuEntry(FGlobalEditorCommonCommands::Get().FindInContentBrowser);
		MenuBuilder.AddMenuEntry(FProjectCleanerCommands::Get().IncludeAsset);
	}
	MenuBuilder.EndSection();

	return MenuBuilder.MakeWidget();
}

TSharedPtr<SWidget> SProjectCleanerExcludedAssetsUI::OnGetFolderContextMenu(const TArray<FString>& SelectedPaths,
	FContentBrowserMenuExtender_SelectedPaths InMenuExtender, FOnCreateNewFolder InOnCreateNewFolder) const
{
	FMenuBuilder MenuBuilder(true, Commands);
	MenuBuilder.BeginSection(TEXT("Include"), LOCTEXT("include_by_path", "Path"));
	{
		MenuBuilder.AddMenuEntry(FProjectCleanerCommands::Get().IncludePath);
	}
	MenuBuilder.EndSection();

	return MenuBuilder.MakeWidget();
}

void SProjectCleanerExcludedAssetsUI::OnAssetDblClicked(const FAssetData& AssetData)
{
	TArray<FName> AssetNames;
	AssetNames.Add(AssetData.ObjectPath);

	GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorsForAssets(AssetNames);
}

void SProjectCleanerExcludedAssetsUI::FindInContentBrowser() const
{
	if (!GetCurrentSelectionDelegate.IsBound()) return;

	const TArray<FAssetData> CurrentSelection = GetCurrentSelectionDelegate.Execute();
	if (CurrentSelection.Num() > 0)
	{
		const FContentBrowserModule& CBModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
		CBModule.Get().SyncBrowserToAssets(CurrentSelection);
	}
}

bool SProjectCleanerExcludedAssetsUI::IsAnythingSelected() const
{
	if (!GetCurrentSelectionDelegate.IsBound()) return false;

	const TArray<FAssetData> CurrentSelection = GetCurrentSelectionDelegate.Execute();
	return CurrentSelection.Num() > 0;
}

void SProjectCleanerExcludedAssetsUI::IncludeAssets() const
{
	if (!GetCurrentSelectionDelegate.IsBound()) return;

	const TArray<FAssetData> SelectedAssets = GetCurrentSelectionDelegate.Execute();
	CleanerManager->IncludeSelectedAssets(SelectedAssets);
}

void SProjectCleanerExcludedAssetsUI::IncludePath() const
{
	CleanerManager->IncludePath(SelectedPath);
}

FReply SProjectCleanerExcludedAssetsUI::IncludeAllAssets() const
{
	CleanerManager->IncludeAllAssets();

	return FReply::Handled();
}

void SProjectCleanerExcludedAssetsUI::GenerateFilter()
{
	FName CurrentlySelectedVirtualPath;
	IContentBrowserDataModule::Get().GetSubsystem()->ConvertInternalPathToVirtual(FStringView(SelectedPath), CurrentlySelectedVirtualPath);
	
	Filter.Clear();
	Filter.PackagePaths.Add(CurrentlySelectedVirtualPath);
	
	if (CleanerManager->GetExcludedAssets().Num() == 0)
	{
		// this is needed for disabling showing primary assets in browser, when there is no unused assets
		Filter.TagsAndValues.Add(FName{ "ProjectCleanerEmptyTag" }, FString{ "ProjectCleanerEmptyTag" });
	}
	else
	{
		Filter.PackageNames.Reserve(CleanerManager->GetExcludedAssets().Num());
		for (const auto& Asset : CleanerManager->GetExcludedAssets())
		{
			Filter.PackageNames.Add(Asset);
		}
	}
	
	AssetPickerConfig.bCanShowDevelopersFolder = CleanerManager->GetCleanerConfigs()->bScanDeveloperContents;

	if (SetFilterDelegate.IsBound())
	{
		SetFilterDelegate.Execute(Filter);
	}
}

void SProjectCleanerExcludedAssetsUI::OnPathSelected(const FString& Path)
{
	SelectedPath = Path;
	
	UpdateUI();
}

void SProjectCleanerExcludedAssetsUI::OnFolderEntered(const FString& NewPath)
{
	SelectedPath = NewPath;

	TArray<FString> NewPaths;
	NewPaths.Add(NewPath);
	SetPathsDelegate.Execute(NewPaths);
}

#undef LOCTEXT_NAMESPACE
