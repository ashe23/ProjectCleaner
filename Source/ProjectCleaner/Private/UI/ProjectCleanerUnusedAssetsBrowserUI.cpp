#include "UI/ProjectCleanerUnusedAssetsBrowserUI.h"
// Engine Headers
#include "IContentBrowserSingleton.h"
#include "Editor/ContentBrowser/Public/ContentBrowserModule.h"
#include "Widgets/Layout/SScrollBox.h"

#define LOCTEXT_NAMESPACE "FProjectCleanerModule"

void SProjectCleanerUnusedAssetsBrowserUI::Construct(const FArguments& InArgs)
{
	UnusedAssets = InArgs._UnusedAssets;	
	
	FDetailsViewArgs UnusedAssetsUISettingsDetailsViewArgs;
	UnusedAssetsUISettingsDetailsViewArgs.ViewIdentifier = "UnusedAssetsUISettings";

	FAssetPickerConfig Config;
	Config.InitialAssetViewType = EAssetViewType::Tile;
	Config.bAddFilterUI = true;
	Config.bShowPathInColumnView = true;
	Config.bSortByPathInColumnView = true;
	Config.bForceShowEngineContent = false;
	Config.bShowBottomToolbar = true;
	Config.bCanShowDevelopersFolder = false;

	FARFilter Filter;
	Filter.bRecursiveClasses = true;
	Filter.RecursiveClassesExclusionSet.Add(UWorld::StaticClass()->GetFName());
	for(const auto& Asset : UnusedAssets)
	{
		Filter.PackageNames.Add(Asset->PackageName);
	}
	Config.Filter = Filter;

	FContentBrowserModule& ContentBrowser = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>(
		"ContentBrowser");

	const FSlateFontInfo FontInfo = FSlateFontInfo(
            FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Light.ttf"),
            20
        );
	
	ChildSlot
	[
		SNew(SVerticalBox)
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(20.0f)
        [
            SNew(STextBlock)
            .AutoWrapText(true)
            .Font(FontInfo)
            .Text(LOCTEXT("UnusedAssets", "All Unused Assets"))
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(20.0f)
        [
			ContentBrowser.Get().CreateAssetPicker(Config)
        ]
	];
}

#undef LOCTEXT_NAMESPACE