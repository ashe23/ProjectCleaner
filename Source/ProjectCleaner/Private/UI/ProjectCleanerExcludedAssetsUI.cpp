#include "UI/ProjectCleanerExcludedAssetsUI.h"
#include "UI/ProjectCleanerBrowserCommands.h"
// Engine Headers
#include "IContentBrowserSingleton.h"
#include "ObjectTools.h"
#include "Editor/ContentBrowser/Public/ContentBrowserModule.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Toolkits/GlobalEditorCommonCommands.h"

#define LOCTEXT_NAMESPACE "FProjectCleanerModule"

void SProjectCleanerExcludedAssetsUI::Construct(const FArguments& InArgs)
{
	SetExcludedAssets(InArgs._ExcludedAssets);

	FProjectCleanerBrowserCommands::Register();

	Commands = MakeShareable(new FUICommandList);
	Commands->MapAction(
		FGlobalEditorCommonCommands::Get().FindInContentBrowser,
		FUIAction(
			FExecuteAction::CreateRaw(this, &SProjectCleanerExcludedAssetsUI::FindInContentBrowser),
			FCanExecuteAction::CreateRaw(this, &SProjectCleanerExcludedAssetsUI::IsAnythingSelected)
		)
	);

	Commands->MapAction(
		FProjectCleanerBrowserCommands::Get().MarkUnused,
		FUIAction
		(
			FExecuteAction::CreateRaw(this,&SProjectCleanerExcludedAssetsUI::MarkUnused),
			FCanExecuteAction::CreateRaw(this, &SProjectCleanerExcludedAssetsUI::IsAnythingSelected)
		)
	);
	
	RefreshUIContent();
}

void SProjectCleanerExcludedAssetsUI::SetExcludedAssets(const TArray<FAssetData>& Assets)
{
	ExcludedAssets.Reset();
	ExcludedAssets.Reserve(Assets.Num());
	
	for(const auto& Asset : Assets)
	{
		ExcludedAssets.Add(Asset);
	}

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
	Config.bCanShowDevelopersFolder = false;
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

	FContentBrowserModule& ContentBrowser = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	WidgetRef = SNew(SVerticalBox)
	+ SVerticalBox::Slot()
	.AutoHeight()
	.Padding(20.0f)
	[
		SNew(STextBlock)
		.AutoWrapText(true)
		.Font(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Light.ttf"),20))
		.Text(LOCTEXT("excludedassets", "Excluded Assets"))
	]
	+SVerticalBox::Slot()
	.AutoHeight()
	.Padding(20.0f)
	[
		SNew(STextBlock)
		.AutoWrapText(true)
		.Font(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Light.ttf"),10))
		.Text(LOCTEXT("excludedassetsinfo", "When excluding assets, its related assets also excluded"))
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

TSharedPtr<SWidget> SProjectCleanerExcludedAssetsUI::OnGetAssetContextMenu(const TArray<FAssetData>& SelectedAssets)
{
	FMenuBuilder MenuBuilder{true, Commands};
	MenuBuilder.BeginSection(
		TEXT("Asset"),
		NSLOCTEXT("ReferenceViewerSchema", "AssetSectionLabel", "Asset")
	);
	{
		MenuBuilder.AddMenuEntry(FGlobalEditorCommonCommands::Get().FindInContentBrowser);
		MenuBuilder.AddMenuEntry(FProjectCleanerBrowserCommands::Get().MarkUnused);
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

void SProjectCleanerExcludedAssetsUI::MarkUnused() const
{
	if (!GetCurrentSelectionDelegate.IsBound()) return;

	const TArray<FAssetData> CurrentSelection = GetCurrentSelectionDelegate.Execute();
	if(!CurrentSelection.Num()) return;

	OnUserMarkUnused.ExecuteIfBound(CurrentSelection);
}


#undef LOCTEXT_NAMESPACE
