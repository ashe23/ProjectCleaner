#include "UI/ProjectCleanerUnusedAssetsBrowserUI.h"
#include "UI/ProjectCleanerBrowserCommands.h"
// Engine Headers
#include "IContentBrowserSingleton.h"
#include "ObjectTools.h"
#include "Editor/ContentBrowser/Public/ContentBrowserModule.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Toolkits/GlobalEditorCommonCommands.h"

#define LOCTEXT_NAMESPACE "FProjectCleanerModule"

void SProjectCleanerUnusedAssetsBrowserUI::Construct(const FArguments& InArgs)
{
	SetUnusedAssets(InArgs._UnusedAssets);

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
	Commands->MapAction(FProjectCleanerBrowserCommands::Get().DeleteAsset,
		FUIAction
		(
	FExecuteAction::CreateRaw(this, &SProjectCleanerUnusedAssetsBrowserUI::DeleteAsset),
			FCanExecuteAction::CreateRaw(this, &SProjectCleanerUnusedAssetsBrowserUI::IsAnythingSelected)
		)
	);

	Commands->MapAction(
		FProjectCleanerBrowserCommands::Get().ExcludeFromDeletion,
		FUIAction
		(
			FExecuteAction::CreateRaw(this,&SProjectCleanerUnusedAssetsBrowserUI::ExcludeAsset),
			FCanExecuteAction::CreateRaw(this, &SProjectCleanerUnusedAssetsBrowserUI::IsAnythingSelected)
		)
	);

	RefreshUIContent();
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
		MenuBuilder.AddMenuEntry(FProjectCleanerBrowserCommands::Get().DeleteAsset);
		MenuBuilder.AddMenuEntry(FProjectCleanerBrowserCommands::Get().ExcludeFromDeletion);
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
	if (CurrentSelection.Num() > 0)
	{
		int32 DeletedAssetsNum = ObjectTools::DeleteAssets(CurrentSelection);
		if (DeletedAssetsNum == 0) return;
		OnUserDeletedAssets.ExecuteIfBound();
	}

}

void SProjectCleanerUnusedAssetsBrowserUI::ExcludeAsset() const
{
	if(!GetCurrentSelectionDelegate.IsBound()) return;

	const TArray<FAssetData> CurrentSelection = GetCurrentSelectionDelegate.Execute();
	if(!CurrentSelection.Num()) return;

	OnUserExcludedAssets.ExecuteIfBound(CurrentSelection);
}

void SProjectCleanerUnusedAssetsBrowserUI::RefreshUIContent()
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
	.Padding(FMargin{0.0f, 0.0f, 0.0f, 20.0f})
	[
		SNew(SBorder)
		.Padding(30.0f)
		.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
		[
			SNew(STextBlock)
			.AutoWrapText(true)
			.Font(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Light.ttf"),20))
			.Text(LOCTEXT("cleanernotetext", "Unused assets are those, that are not used in any level.\nSo before starting make sure you delete all levels that never used in project."))
		]
	]
	+ SVerticalBox::Slot()
	.AutoHeight()
	.Padding(20.0f)
	[
		SNew(STextBlock)
		.AutoWrapText(true)
		.Font(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Light.ttf"),20))
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
