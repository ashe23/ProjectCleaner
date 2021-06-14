// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#include "UI/ProjectCleanerExcludedAssetsUI.h"
#include "ProjectCleanerCommands.h"
#include "ProjectCleanerStyle.h"
// Engine Headers
#include "IContentBrowserSingleton.h"
#include "Editor/ContentBrowser/Public/ContentBrowserModule.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Toolkits/GlobalEditorCommonCommands.h"
//#include "Layout/Visibility.h"

#define LOCTEXT_NAMESPACE "FProjectCleanerModule"

void SProjectCleanerExcludedAssetsUI::Construct(const FArguments& InArgs)
{
	SetExcludedAssets(InArgs._ExcludedAssets);
	SetLinkedAssets(InArgs._LinkedAssets);

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
	
	RefreshUIContent();
}

void SProjectCleanerExcludedAssetsUI::SetExcludedAssets(const TSet<FAssetData>& Assets)
{
	ExcludedAssets.Reset();
	ExcludedAssets.Reserve(Assets.Num());
	
	for(const auto& Asset : Assets)
	{
		ExcludedAssets.Add(Asset);
	}

	RefreshUIContent();
}

void SProjectCleanerExcludedAssetsUI::SetLinkedAssets(const TArray<FAssetData>& Assets)
{
	LinkedAssets.Reset();
	LinkedAssets.Reserve(Assets.Num());

	for (const auto& Asset : Assets)
	{
		LinkedAssets.Add(Asset);
	}

	RefreshUIContent();
}

void SProjectCleanerExcludedAssetsUI::SetCleanerConfigs(const FCleanerConfigs& Configs)
{
	CleanerConfigs = Configs;

	RefreshUIContent();
}

void SProjectCleanerExcludedAssetsUI::RefreshUIContent()
{
	FAssetPickerConfig Config;
	Config.InitialAssetViewType = EAssetViewType::Tile;
	Config.bAddFilterUI = true;
	Config.bShowPathInColumnView = true;
	Config.bSortByPathInColumnView = true;
	Config.bForceShowEngineContent = false;
	Config.bShowBottomToolbar = true;
	Config.bCanShowDevelopersFolder = CleanerConfigs.bScanDeveloperContentsFolder;
	Config.bForceShowEngineContent = false;
	Config.bCanShowClasses = false;
	Config.bAllowDragging = false;	
	Config.AssetShowWarningText = FText::FromName("No assets");
	Config.GetCurrentSelectionDelegates.Add(&GetCurrentSelectionDelegate);
	Config.OnAssetDoubleClicked = FOnAssetDoubleClicked::CreateRaw(
		this,
		&SProjectCleanerExcludedAssetsUI::OnAssetDblClicked
	);
	Config.OnGetAssetContextMenu = FOnGetAssetContextMenu::CreateRaw(
		this,
		&SProjectCleanerExcludedAssetsUI::OnGetAssetContextMenu
	);

	FARFilter Filter;
	if (ExcludedAssets.Num() == 0)
	{
		// this is needed when there is no assets to show ,
		// asset picker will show remaining assets in content browser,
		// we must not show them
		Filter.TagsAndValues.Add(FName{"ProjectCleanerEmptyTag"}, FString{"ProjectCleanerEmptyTag"});
	}
	else
	{
		// excluding level assets from showing and filtering
		Filter.bRecursiveClasses = true;
		Filter.RecursiveClassesExclusionSet.Add(UWorld::StaticClass()->GetFName());
	}

	Filter.PackageNames.Reserve(ExcludedAssets.Num());
	for(const auto& Asset : ExcludedAssets)
	{
		Filter.PackageNames.Add(Asset.PackageName);
	}
	Config.Filter = Filter;

	// linked assets asset picker
	FAssetPickerConfig LinkedAssetsConfig;
	LinkedAssetsConfig.InitialAssetViewType = EAssetViewType::Tile;
	LinkedAssetsConfig.bAddFilterUI = false;
	LinkedAssetsConfig.bShowPathInColumnView = true;
	LinkedAssetsConfig.bSortByPathInColumnView = true;
	LinkedAssetsConfig.bForceShowEngineContent = false;
	LinkedAssetsConfig.bShowBottomToolbar = false;
	LinkedAssetsConfig.bCanShowDevelopersFolder = CleanerConfigs.bScanDeveloperContentsFolder;
	LinkedAssetsConfig.bForceShowEngineContent = false;
	LinkedAssetsConfig.bCanShowClasses = false;
	LinkedAssetsConfig.bAllowDragging = false;
	LinkedAssetsConfig.AssetShowWarningText = FText::FromName("No assets");

	FARFilter LinkedAssetsFilter;
	if (LinkedAssets.Num() == 0)
	{
		LinkedAssetsFilter.TagsAndValues.Add(FName{ "ProjectCleanerEmptyTag" }, FString{ "ProjectCleanerEmptyTag" });
	}
	else
	{
		LinkedAssetsFilter.bRecursiveClasses = true;
		LinkedAssetsFilter.RecursiveClassesExclusionSet.Add(UWorld::StaticClass()->GetFName());
	}
	LinkedAssetsFilter.PackageNames.Reserve(LinkedAssets.Num());
	for (const auto& Asset : LinkedAssets)
	{
		LinkedAssetsFilter.PackageNames.Add(Asset.PackageName);
	}
	LinkedAssetsConfig.Filter = LinkedAssetsFilter;

	FContentBrowserModule& ContentBrowser = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	WidgetRef = SNew(SVerticalBox)
	+ SVerticalBox::Slot()
	.AutoHeight()
	.Padding(FMargin(0.0f, 5.0f))
	[
		SNew(STextBlock)
		.AutoWrapText(true)
		.Font(FProjectCleanerStyle::Get().GetFontStyle("ProjectCleaner.Font.Light20"))
		.Text(LOCTEXT("excludedassets", "Excluded Assets"))
	]
	+SVerticalBox::Slot()
	.AutoHeight()
	.Padding(FMargin(0.0f, 10.0f))
	[
		SNew(STextBlock)
		.AutoWrapText(true)
		.Font(FProjectCleanerStyle::Get().GetFontStyle("ProjectCleaner.Font.Light10"))
		.Text(LOCTEXT("excludedassetsinfo", "When excluding assets, its related assets also excluded"))
	]
	+ SVerticalBox::Slot()
	.AutoHeight()
	//.Padding(20.0f)
	[
		ContentBrowser.Get().CreateAssetPicker(Config)
	]
	+ SVerticalBox::Slot()
	.AutoHeight()
	.Padding(FMargin(0.0f, 5.0f))
	[
		SNew(STextBlock)
		.AutoWrapText(true)
		.Font(FProjectCleanerStyle::Get().GetFontStyle("ProjectCleaner.Font.Light20"))
		.Text(LOCTEXT("linked_assets", "Linked Assets"))
	]
	+ SVerticalBox::Slot()
	.AutoHeight()
	//.Padding(20.0f)
	[
		ContentBrowser.Get().CreateAssetPicker(LinkedAssetsConfig)
	];

	// todo:ashe23 maybe add later 
	//WidgetRef->SetVisibility((ExcludedAssets.Num() > 0 ? EVisibility::Visible : EVisibility::Hidden));
	
	ChildSlot
	[
		WidgetRef
	];
}

TSharedPtr<SWidget> SProjectCleanerExcludedAssetsUI::OnGetAssetContextMenu(const TArray<FAssetData>& SelectedAssets)
{
	FMenuBuilder MenuBuilder{true, Commands};
	MenuBuilder.BeginSection(
		TEXT("Asset"),
		NSLOCTEXT("ReferenceViewerSchema", "AssetSectionLabel", "Asset")
	);
	{
		MenuBuilder.AddMenuEntry(FGlobalEditorCommonCommands::Get().FindInContentBrowser);
		MenuBuilder.AddMenuEntry(FProjectCleanerCommands::Get().IncludeAsset);
	}
	MenuBuilder.EndSection();

	return MenuBuilder.MakeWidget();
}

void SProjectCleanerExcludedAssetsUI::OnAssetDblClicked(const FAssetData& AssetData) const
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
		FContentBrowserModule& CBModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
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
	if(!SelectedAssets.Num()) return;

	if(!OnUserIncludedAssets.IsBound()) return;

	OnUserIncludedAssets.Execute(SelectedAssets);
}

#undef LOCTEXT_NAMESPACE