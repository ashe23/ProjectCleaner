// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#include "UI/ProjectCleanerUnusedAssetsBrowserUI.h"
#include "ProjectCleanerCommands.h"
#include "ProjectCleanerStyle.h"
// Engine Headers
#include "IContentBrowserSingleton.h"
#include "ObjectTools.h"
#include "Editor/ContentBrowser/Public/ContentBrowserModule.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Toolkits/GlobalEditorCommonCommands.h"

#define LOCTEXT_NAMESPACE "FProjectCleanerModule"

void SProjectCleanerUnusedAssetsBrowserUI::Construct(const FArguments& InArgs)
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
	
	SetUnusedAssets(InArgs._UnusedAssets);
}

void SProjectCleanerUnusedAssetsBrowserUI::SetUnusedAssets(const TArray<FAssetData>& NewUnusedAssets)
{
	UnusedAssets.Reset();
	UnusedAssets.Reserve(NewUnusedAssets.Num());
	for(const auto& Asset : NewUnusedAssets)
	{
		UnusedAssets.Add(Asset);
	}
	
	RefreshUIContent();
}

TSharedPtr<SWidget> SProjectCleanerUnusedAssetsBrowserUI::OnGetAssetContextMenu(const TArray<FAssetData>& SelectedAssets)
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

void SProjectCleanerUnusedAssetsBrowserUI::OnAssetDblClicked(const FAssetData& AssetData) const
{
	TArray<FName> AssetNames;
	AssetNames.Add(AssetData.ObjectPath);

	GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorsForAssets(AssetNames);
}

void SProjectCleanerUnusedAssetsBrowserUI::FindInContentBrowser() const
{
	if (!GetCurrentSelectionDelegate.IsBound()) return;

	const TArray<FAssetData> CurrentSelection = GetCurrentSelectionDelegate.Execute();
	if (CurrentSelection.Num() > 0)
	{
		FContentBrowserModule& CBModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
		CBModule.Get().SyncBrowserToAssets(CurrentSelection);
	}
}

bool SProjectCleanerUnusedAssetsBrowserUI::IsAnythingSelected() const
{
	if (!GetCurrentSelectionDelegate.IsBound()) return false;

	const TArray<FAssetData> CurrentSelection = GetCurrentSelectionDelegate.Execute();
	return CurrentSelection.Num() > 0;
}

void SProjectCleanerUnusedAssetsBrowserUI::DeleteAsset() const
{
	if (!GetCurrentSelectionDelegate.IsBound()) return;

	const TArray<FAssetData> CurrentSelection = GetCurrentSelectionDelegate.Execute();
	if (!CurrentSelection.Num()) return;
	
	const int32 DeletedAssetsNum = ObjectTools::DeleteAssets(CurrentSelection);
	if (DeletedAssetsNum == 0) return;
	if (!OnUserDeletedAssets.IsBound()) return;
	OnUserDeletedAssets.Execute();
}

void SProjectCleanerUnusedAssetsBrowserUI::ExcludeAsset() const
{
	if(!GetCurrentSelectionDelegate.IsBound()) return;

	const TArray<FAssetData> SelectedAssets = GetCurrentSelectionDelegate.Execute();
	if(!SelectedAssets.Num()) return;
	
	if(!OnUserExcludedAssets.IsBound()) return;
	OnUserExcludedAssets.Execute(SelectedAssets);
}

void SProjectCleanerUnusedAssetsBrowserUI::RefreshUIContent()
{
	FAssetPickerConfig Config;
	Config.InitialAssetViewType = EAssetViewType::Tile;
	Config.bAddFilterUI = true;
	Config.bPreloadAssetsForContextMenu = false;
	Config.bShowPathInColumnView = true;
	Config.bSortByPathInColumnView = true;
	Config.bForceShowEngineContent = false;
	Config.bShowBottomToolbar = true;
	Config.bCanShowDevelopersFolder = true;
	Config.bForceShowEngineContent = false;
	Config.bCanShowClasses = false;
	Config.bAllowDragging = false;	
	Config.AssetShowWarningText = FText::FromName("No assets");
	Config.GetCurrentSelectionDelegates.Add(&GetCurrentSelectionDelegate);
	Config.OnAssetDoubleClicked = FOnAssetDoubleClicked::CreateRaw(
		this,
		&SProjectCleanerUnusedAssetsBrowserUI::OnAssetDblClicked
	);
	Config.OnGetAssetContextMenu = FOnGetAssetContextMenu::CreateRaw(
		this,
		&SProjectCleanerUnusedAssetsBrowserUI::OnGetAssetContextMenu
	);

	FARFilter Filter;
	if (UnusedAssets.Num() == 0)
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
	
	for(const auto& Asset : UnusedAssets)
	{
		Filter.PackageNames.Add(Asset.PackageName);
	}
	Config.Filter = Filter;

	FContentBrowserModule& ContentBrowser = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	WidgetRef = SNew(SVerticalBox)
	+ SVerticalBox::Slot()
	.AutoHeight()
	.Padding(20.0f)
	[
		SNew(STextBlock)
		.AutoWrapText(true)
		.Font(FProjectCleanerStyle::Get().GetFontStyle("ProjectCleaner.Font.Light20"))
		.Text(LOCTEXT("UnusedAssets", "All Unused Assets"))
	]
	+ SVerticalBox::Slot()
	.AutoHeight()
	.Padding(20.0f)
	[
		ContentBrowser.Get().CreateAssetPicker(Config)
	];
	
	ChildSlot
	[
		WidgetRef
	];
}

#undef LOCTEXT_NAMESPACE