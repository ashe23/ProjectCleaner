#include "ProjectCleanerExcludedAssetsUI.h"
// Engine Headers
#include "IContentBrowserSingleton.h"
#include "ObjectTools.h"
#include "Editor/ContentBrowser/Public/ContentBrowserModule.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Toolkits/GlobalEditorCommonCommands.h"

#define LOCTEXT_NAMESPACE "FProjectCleanerModule"

void SProjectCleanerExcludedAssetsUI::Construct(const FArguments& InArgs)
{
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
	// Config.GetCurrentSelectionDelegates.Add(&GetCurrentSelectionDelegate);
	// Config.OnAssetDoubleClicked = FOnAssetDoubleClicked::CreateRaw(
	// 	this,
	// 	&SProjectCleanerUnusedAssetsBrowserUI::OnAssetDblClicked
	// );
	// Config.OnGetAssetContextMenu = FOnGetAssetContextMenu::CreateRaw(
	// 	this,
	// 	&SProjectCleanerUnusedAssetsBrowserUI::OnGetAssetContextMenu
	// );

	FARFilter Filter;
	Filter.TagsAndValues.Add(FName{"ProjectCleanerEmptyTag"}, FString{"ProjectCleanerEmptyTag"});
	// if (UnusedAssets.Num() == 0)
	// {
	// 	// this is needed when there is no assets to show ,
	// 	// asset picker will show remaining assets in content browser,
	// 	// we must not show them
	// 	Filter.TagsAndValues.Add(FName{"ProjectCleanerEmptyTag"}, FString{"ProjectCleanerEmptyTag"});
	// }
	// else
	// {
	// 	// excluding level assets from showing and filtering
	// 	Filter.bRecursiveClasses = true;
	// 	Filter.RecursiveClassesExclusionSet.Add(UWorld::StaticClass()->GetFName());
	// }
	//
	// for(const auto& Asset : UnusedAssets)
	// {
	// 	Filter.PackageNames.Add(Asset.PackageName);
	// }
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