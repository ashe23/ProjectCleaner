// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#include "UI/ProjectCleanerExcludedAssetsUI.h"
#include "UI/ProjectCleanerCommands.h"
#include "UI/ProjectCleanerStyle.h"
// Engine Headers
#include "IContentBrowserSingleton.h"
#include "DSP/PassiveFilter.h"
#include "Editor/ContentBrowser/Public/ContentBrowserModule.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Toolkits/GlobalEditorCommonCommands.h"

#define LOCTEXT_NAMESPACE "FProjectCleanerModule"

void SProjectCleanerExcludedAssetsUI::Construct(const FArguments& InArgs)
{
	ContentBrowserModule = &FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	
	if (InArgs._ExcludedAssets)
	{
		SetExcludedAssets(*InArgs._ExcludedAssets);
	}

	if (InArgs._LinkedAssets)
	{
		SetLinkedAssets(*InArgs._LinkedAssets);
	}

	if (InArgs._PrimaryAssetClasses)
	{
		SetPrimaryAssetClasses(*InArgs._PrimaryAssetClasses);
	}

	if (InArgs._CleanerConfigs)
	{
		SetCleanerConfigs(InArgs._CleanerConfigs);
	}
	
	SelectedPath = FName{TEXT("/Game")};
	RegisterCommands();
	UpdateUI();
}

void SProjectCleanerExcludedAssetsUI::SetUIData(const TArray<FAssetData>& NewExcludedAssets, const TArray<FAssetData>& NewLinkedAssets, const TSet<FName>& NewPrimaryAssetClasses, UCleanerConfigs* NewConfigs)
{
	SetExcludedAssets(NewExcludedAssets);
	SetLinkedAssets(NewLinkedAssets);
	SetPrimaryAssetClasses(NewPrimaryAssetClasses);
	SetCleanerConfigs(NewConfigs);

	UpdateUI();
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
}

void SProjectCleanerExcludedAssetsUI::SetExcludedAssets(const TArray<FAssetData>& Assets)
{
	ExcludedAssets.Reset();
	ExcludedAssets.Reserve(Assets.Num());
	ExcludedAssets = MoveTempIfPossible(Assets);
}

void SProjectCleanerExcludedAssetsUI::SetLinkedAssets(const TArray<FAssetData>& Assets)
{
	LinkedAssets.Reset();
	LinkedAssets.Reserve(Assets.Num());
	LinkedAssets = MoveTempIfPossible(Assets);
}

void SProjectCleanerExcludedAssetsUI::SetCleanerConfigs(UCleanerConfigs* Configs)
{
	if (!Configs) return;
	CleanerConfigs = Configs;
}

void SProjectCleanerExcludedAssetsUI::SetPrimaryAssetClasses(const TSet<FName>& NewPrimaryAssetClasses)
{
	PrimaryAssetClasses.Reset();
	PrimaryAssetClasses.Reserve(NewPrimaryAssetClasses.Num());
	PrimaryAssetClasses = MoveTempIfPossible(NewPrimaryAssetClasses);
}

void SProjectCleanerExcludedAssetsUI::UpdateUI()
{
	if (!CleanerConfigs) return;
	
	PathPickerConfig.bAllowContextMenu = false;
	PathPickerConfig.bAllowClassesFolder = false;
	PathPickerConfig.bFocusSearchBoxWhenOpened = false;
	PathPickerConfig.OnPathSelected.BindRaw(this, &SProjectCleanerExcludedAssetsUI::OnPathSelected);
	PathPickerConfig.bAddDefaultPath = true;
	PathPickerConfig.DefaultPath = SelectedPath.ToString();
	
	ChildSlot
	[
		SNew(SSplitter)
		.Style(FEditorStyle::Get(), "ContentBrowser.Splitter")
		.PhysicalSplitterHandleSize(3.0f)
		+ SSplitter::Slot()
		.Value(0.2f)
		[
			ContentBrowserModule->Get().CreatePathPicker(PathPickerConfig)
		]
		+ SSplitter::Slot()
		.Value(0.4f)
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
					.ToolTipText(LOCTEXT("exclude_assets_tooltip_text", "Removes all assets from excluded list"))
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
					GetExcludedAssetsView().ToSharedRef()
				]
			]
		]
		+ SSplitter::Slot()
		.Value(0.4f)
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
					.Text(LOCTEXT("exclude_assets_linked_assets_title_text", "Linked Assets"))
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(10.0f, 14.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Top)
				[
					SNew(STextBlock)
					.AutoWrapText(true)
					.Font(FProjectCleanerStyle::Get().GetFontStyle("ProjectCleaner.Font.Light10"))
					.Text(LOCTEXT("exclude_assets_linked_assets_tip_info_text", "All referenced and dependency assets of excluded assets"))
				]
			]
			+ SVerticalBox::Slot()
			.Padding(10.0, 10.0f)
			[
				SNew(SBox)
				.HeightOverride(300)
				.WidthOverride(300)
				[
					GetLinkedAssetsView().ToSharedRef()
				]			
			]
		]
	];
}

TSharedPtr<SWidget> SProjectCleanerExcludedAssetsUI::GetExcludedAssetsView()
{
	FAssetPickerConfig AssetPickerConfig;
	AssetPickerConfig.InitialAssetViewType = EAssetViewType::Tile;
	AssetPickerConfig.SelectionMode = ESelectionMode::SingleToggle;
	AssetPickerConfig.bAddFilterUI = true;
	AssetPickerConfig.bShowPathInColumnView = true;
	AssetPickerConfig.bSortByPathInColumnView = true;
	AssetPickerConfig.bForceShowEngineContent = false;
	AssetPickerConfig.bShowBottomToolbar = true;
	AssetPickerConfig.bCanShowDevelopersFolder = CleanerConfigs->bScanDeveloperContents;
	AssetPickerConfig.bForceShowEngineContent = false;
	AssetPickerConfig.bCanShowClasses = false;
	AssetPickerConfig.bAllowDragging = false;
	AssetPickerConfig.bCanShowFolders = false;
	AssetPickerConfig.AssetShowWarningText = FText::FromName("No assets");
	AssetPickerConfig.GetCurrentSelectionDelegates.Add(&GetCurrentSelectionDelegate);
	AssetPickerConfig.OnAssetSelected = FOnAssetSelected::CreateStatic(
		&SProjectCleanerExcludedAssetsUI::OnAssetSelected
	);
	AssetPickerConfig.OnAssetDoubleClicked = FOnAssetDoubleClicked::CreateStatic(
		&SProjectCleanerExcludedAssetsUI::OnAssetDblClicked
	);
	AssetPickerConfig.OnGetAssetContextMenu = FOnGetAssetContextMenu::CreateRaw(
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
		Filter.RecursiveClassesExclusionSet.Reserve(PrimaryAssetClasses.Num());
		Filter.RecursiveClassesExclusionSet = MoveTempIfPossible(PrimaryAssetClasses);
	}

	if (!SelectedPath.IsNone())
	{
		Filter.PackagePaths.Add(SelectedPath);
	}

	Filter.PackageNames.Reserve(ExcludedAssets.Num());
	for(const auto& Asset : ExcludedAssets)
	{
		Filter.PackageNames.Add(Asset.PackageName);
	}
	AssetPickerConfig.Filter = Filter;

	return ContentBrowserModule->Get().CreateAssetPicker(AssetPickerConfig);
}

TSharedPtr<SWidget> SProjectCleanerExcludedAssetsUI::GetLinkedAssetsView()
{
	FAssetPickerConfig AssetPickerConfig;
	AssetPickerConfig.InitialAssetViewType = EAssetViewType::Tile;
	AssetPickerConfig.SelectionMode = ESelectionMode::None;
	AssetPickerConfig.bAddFilterUI = true;
	AssetPickerConfig.bShowPathInColumnView = true;
	AssetPickerConfig.bSortByPathInColumnView = true;
	AssetPickerConfig.bForceShowEngineContent = false;
	AssetPickerConfig.bShowBottomToolbar = true;
	AssetPickerConfig.bCanShowDevelopersFolder = CleanerConfigs->bScanDeveloperContents;
	AssetPickerConfig.bForceShowEngineContent = false;
	AssetPickerConfig.bCanShowClasses = false;
	AssetPickerConfig.bAllowDragging = false;
	AssetPickerConfig.bCanShowFolders = false;
	AssetPickerConfig.AssetShowWarningText = FText::FromName("No assets");

	// todo:ashe23 on asset selected show only linked assets of current selected asset
	
	FARFilter Filter;
	if (LinkedAssets.Num() == 0)
	{
		Filter.TagsAndValues.Add(FName{ "ProjectCleanerEmptyTag" }, FString{ "ProjectCleanerEmptyTag" });
	}
	else
	{
		Filter.bRecursiveClasses = true;
		Filter.RecursiveClassesExclusionSet.Reserve(PrimaryAssetClasses.Num());
		Filter.RecursiveClassesExclusionSet = MoveTempIfPossible(PrimaryAssetClasses);
	}
	Filter.PackageNames.Reserve(LinkedAssets.Num());
	for (const auto& Asset : LinkedAssets)
	{
		Filter.PackageNames.Add(Asset.PackageName);
	}
	AssetPickerConfig.Filter = Filter;

	return ContentBrowserModule->Get().CreateAssetPicker(AssetPickerConfig);
}

TSharedPtr<SWidget> SProjectCleanerExcludedAssetsUI::OnGetAssetContextMenu(const TArray<FAssetData>& SelectedAssets) const
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

void SProjectCleanerExcludedAssetsUI::OnAssetDblClicked(const FAssetData& AssetData)
{
	TArray<FName> AssetNames;
	AssetNames.Add(AssetData.ObjectPath);

	GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorsForAssets(AssetNames);
}

void SProjectCleanerExcludedAssetsUI::OnAssetSelected(const FAssetData& AssetData)
{
	AssetData.PrintAssetData();
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

	OnUserIncludedAssets.Execute(SelectedAssets, false);
}

FReply SProjectCleanerExcludedAssetsUI::IncludeAllAssets() const
{
	if (!GetCurrentSelectionDelegate.IsBound()) return FReply::Handled();
	if (!OnUserIncludedAssets.IsBound()) return FReply::Handled();

	OnUserIncludedAssets.Execute(ExcludedAssets, true);
	return FReply::Handled();
}

void SProjectCleanerExcludedAssetsUI::OnPathSelected(const FString& Path)
{
	SelectedPath = FName{Path};
	PathPickerConfig.DefaultPath = Path;
	UpdateUI();
}

#undef LOCTEXT_NAMESPACE
