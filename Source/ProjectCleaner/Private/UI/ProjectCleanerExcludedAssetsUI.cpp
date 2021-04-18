#include "UI/ProjectCleanerExcludedAssetsUI.h"
#include "UI/ProjectCleanerBrowserCommands.h"
// Engine Headers
#include "IContentBrowserSingleton.h"
#include "Editor/ContentBrowser/Public/ContentBrowserModule.h"

#define LOCTEXT_NAMESPACE "FProjectCleanerModule"

void SProjectCleanerExcludedAssetsUI::Construct(const FArguments& InArgs)
{
	SetExcludedAssets(InArgs._ExcludedAssets);
	RefreshUIContent();

	FProjectCleanerBrowserCommands::Register();
	
	Commands = MakeShareable(new FUICommandList);
	Commands->MapAction(FProjectCleanerBrowserCommands::Get().AddForDeletion, FUIAction(
		FExecuteAction::CreateRaw(this, &SProjectCleanerExcludedAssetsUI::AddForDeletion),
        FCanExecuteAction::CreateRaw(this, &SProjectCleanerExcludedAssetsUI::IsAnythingSelected)
    ));
}

void SProjectCleanerExcludedAssetsUI::RefreshUIContent()
{
	FAssetPickerConfig Config;
	Config.InitialAssetViewType = EAssetViewType::List;
	Config.bAddFilterUI = true;
	Config.bShowPathInColumnView = true;
	Config.bSortByPathInColumnView = true;
	Config.bForceShowEngineContent = false;
	Config.bShowBottomToolbar = true;
	Config.bCanShowDevelopersFolder = false;
	Config.bAllowDragging = false;
	Config.AssetShowWarningText = FText::FromName("No assets");
	Config.GetCurrentSelectionDelegates.Add(&GetCurrentSelectionDelegate);
	Config.OnGetAssetContextMenu = FOnGetAssetContextMenu::CreateRaw(
        this,
        &SProjectCleanerExcludedAssetsUI::OnGetAssetContextMenu
    );

	FARFilter Filter;
	if(ExcludedAssets.Num() == 0)
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
	for(const auto& Asset : ExcludedAssets)
	{
		Filter.PackageNames.Add(Asset.PackageName);
	}
	Config.Filter = Filter;
	FContentBrowserModule& ContentBrowser = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");

	
	WidgetRef = SNew(SVerticalBox)
	+ SVerticalBox::Slot()
	.Padding(FMargin{40.0f})
	.AutoHeight()
	[
		SNew(SVerticalBox)
		+SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(STextBlock)
			.AutoWrapText(true)
			.Font(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Light.ttf"),20))
			.Text(LOCTEXT("corruptedfiles", "Excluded Assets"))
		]
	]
	+ SVerticalBox::Slot()
	.Padding(FMargin{40.0f})
	.AutoHeight()
	[
		ContentBrowser.Get().CreateAssetPicker(Config)
	];
	
	ChildSlot
	[
		WidgetRef
	];
}

void SProjectCleanerExcludedAssetsUI::SetExcludedAssets(const TSet<FAssetData>& NewExcludedAssets)
{
	if (NewExcludedAssets.Num() == 0) return;

	for(const auto& File : NewExcludedAssets)
	{
		ExcludedAssets.Add(File);
	}
	
	RefreshUIContent();
}

TSharedPtr<SWidget> SProjectCleanerExcludedAssetsUI::OnGetAssetContextMenu(
    const TArray<FAssetData>& SelectedAssets)
{
	FMenuBuilder MenuBuilder{true, Commands};
	MenuBuilder.BeginSection(
        TEXT("Asset"),
        NSLOCTEXT("ReferenceViewerSchema", "AssetSectionLabel", "Asset")
    );
	{
		MenuBuilder.AddMenuEntry(FProjectCleanerBrowserCommands::Get().AddForDeletion);
	}
	MenuBuilder.EndSection();


	return MenuBuilder.MakeWidget();
}

bool SProjectCleanerExcludedAssetsUI::IsAnythingSelected() const
{
	if (!GetCurrentSelectionDelegate.IsBound()) return false;

	const TArray<FAssetData> CurrentSelection = GetCurrentSelectionDelegate.Execute();
	return CurrentSelection.Num() > 0;
}

void SProjectCleanerExcludedAssetsUI::AddForDeletion()
{
	if (!GetCurrentSelectionDelegate.IsBound()) return;

	const TArray<FAssetData> CurrentSelection = GetCurrentSelectionDelegate.Execute();

	// todo:ashe23 return selected assets to deletion list
}
