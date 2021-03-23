#include "UI/ProjectCleanerUnusedAssetsBrowserUI.h"
// Engine Headers
#include "IContentBrowserSingleton.h"
#include "Editor/ContentBrowser/Public/ContentBrowserModule.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Toolkits/GlobalEditorCommonCommands.h"
#include "UI/ProjectCleanerBrowserCommands.h"

#define LOCTEXT_NAMESPACE "FProjectCleanerModule"

void SProjectCleanerUnusedAssetsBrowserUI::Construct(const FArguments& InArgs)
{
	UnusedAssets = InArgs._UnusedAssets;

	FProjectCleanerBrowserCommands::Register();
	
	FDetailsViewArgs UnusedAssetsUISettingsDetailsViewArgs;
	UnusedAssetsUISettingsDetailsViewArgs.ViewIdentifier = "UnusedAssetsUISettings";

	Commands = MakeShareable(new FUICommandList);
	Commands->MapAction(
        FGlobalEditorCommonCommands::Get().FindInContentBrowser,
        FUIAction(
            FExecuteAction::CreateRaw(this, &SProjectCleanerUnusedAssetsBrowserUI::FindInContentBrowser),
            FCanExecuteAction::CreateRaw(this, &SProjectCleanerUnusedAssetsBrowserUI::IsAnythingSelected)
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
	Config.bShowBottomToolbar = true;
	Config.bCanShowDevelopersFolder = false;
	Config.GetCurrentSelectionDelegates.Add(&GetCurrentSelectionDelegate);
	Config.OnGetAssetContextMenu = FOnGetAssetContextMenu::CreateRaw(
		this,
		&SProjectCleanerUnusedAssetsBrowserUI::OnGetAssetContextMenu
	);

	FARFilter Filter;
	Filter.bRecursiveClasses = true;
	Filter.RecursiveClassesExclusionSet.Add(UWorld::StaticClass()->GetFName());

	// todo:ashe23 fix if unused assets is empty content browser shows all assets
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

TSharedPtr<SWidget> SProjectCleanerUnusedAssetsBrowserUI::OnGetAssetContextMenu(
	const TArray<FAssetData>& SelectedAssets)
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

void SProjectCleanerUnusedAssetsBrowserUI::FindInContentBrowser() const
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

bool SProjectCleanerUnusedAssetsBrowserUI::IsAnythingSelected() const
{
	if (!GetCurrentSelectionDelegate.IsBound()) return false;

	const TArray<FAssetData> CurrentSelection = GetCurrentSelectionDelegate.Execute();
	return CurrentSelection.Num() > 0;
}

#undef LOCTEXT_NAMESPACE
