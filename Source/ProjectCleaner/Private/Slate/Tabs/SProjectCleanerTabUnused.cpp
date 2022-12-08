// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/Tabs/SProjectCleanerTabUnused.h"
#include "Slate/TreeView/SProjectCleanerTreeView.h"
#include "ProjectCleaner.h"
#include "ProjectCleanerScanner.h"
#include "ProjectCleanerConstants.h"
#include "ProjectCleanerScanSettings.h"
// Engine Headers
// #include "ContentBrowserModule.h"
// #include "ContentBrowserItem.h"
// #include "FrontendFilterBase.h"
// #include "IContentBrowserSingleton.h"
#include "ProjectCleanerStyles.h"
#include "Slate/ProjectCleanerAssetBrowser.h"
#include "Widgets/Layout/SSeparator.h"

void SProjectCleanerTabUnused::Construct(const FArguments& InArgs)
{
	Scanner = InArgs._Scanner;
	if (!Scanner.IsValid()) return;

	ScanSettings = GetMutableDefault<UProjectCleanerScanSettings>();
	if (!ScanSettings.IsValid()) return;

	Scanner->OnScanFinished().AddLambda([&]()
	{
		UpdateView();
	});

	SelectedPaths.Add(ProjectCleanerConstants::PathRelRoot.ToString());

	UpdateView();
}

void SProjectCleanerTabUnused::UpdateView()
{
	if (!Scanner.IsValid()) return;
	if (!ScanSettings.IsValid()) return;
	// if (Scanner->GetScannerDataState() != EProjectCleanerScannerDataState::Actual) return;

	// making sure tree view is valid
	if (!ProjectCleanerTreeView.IsValid())
	{
		SAssignNew(ProjectCleanerTreeView, SProjectCleanerTreeView).Scanner(Scanner);

		ProjectCleanerTreeView->OnPathSelected().AddLambda([&](const TSet<FString>& InSelectedPaths)
		{
			SelectedPaths.Reset();
			SelectedPaths.Append(InSelectedPaths);

			if (ProjectCleanerAssetBrowser.IsValid())
			{
				ProjectCleanerAssetBrowser.Get()->UpdateView();
			}
			// UpdateView();
			UE_LOG(LogProjectCleaner, Warning, TEXT("Selected Paths Num: %d"), SelectedPaths.Num());
		});

		ProjectCleanerTreeView->OnPathExcluded().AddLambda([&](const TSet<FString>& InExcludedPaths)
		{
			UE_LOG(LogProjectCleaner, Warning, TEXT("Excluded Paths Num: %d"), InExcludedPaths.Num());
		});

		ProjectCleanerTreeView->OnPathIncluded().AddLambda([&](const TSet<FString>& InIncludedPaths)
		{
			UE_LOG(LogProjectCleaner, Warning, TEXT("Included Paths Num: %d"), InIncludedPaths.Num());
		});

		ProjectCleanerTreeView->OnPathCleaned().AddLambda([&](const TSet<FString>& InCleanedPaths)
		{
			UE_LOG(LogProjectCleaner, Warning, TEXT("Cleaned Paths Num: %d"), InCleanedPaths.Num());
		});
	}

	// creating asset view
	// UpdateFilter();

	// ProjectCleanerTreeView->TreeItemsUpdate();

	// const FContentBrowserModule& ModuleContentBrowser = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	//
	// class FFrontendFilter_ProjectCleaner : public FFrontendFilter
	// {
	// public:
	// 	explicit FFrontendFilter_ProjectCleaner(TSharedPtr<FFrontendFilterCategory> InCategory, const FString& InCurrentSelectedPath)
	// 		: FFrontendFilter(InCategory),
	// 		  SelectedPath(InCurrentSelectedPath)
	// 	{
	// 	}
	//
	// 	virtual FString GetName() const override { return TEXT("ExcludedAssets"); }
	// 	virtual FText GetDisplayName() const override { return FText::FromString(TEXT("Excluded Assets")); }
	// 	virtual FText GetToolTipText() const override { return FText::FromString(TEXT("Show all excluded assets in selected path")); }
	// 	virtual FLinearColor GetColor() const override { return FLinearColor{FColor::FromHex(TEXT("#ffcb77"))}; }
	//
	// 	virtual bool PassesFilter(const FContentBrowserItem& InItem) const override
	// 	{
	// 		const UProjectCleanerScanSettings* ScanSettings = GetDefault<UProjectCleanerScanSettings>();
	// 		if (!ScanSettings) return false;
	//
	// 		FAssetData AssetData;
	// 		if (!InItem.Legacy_TryGetAssetData(AssetData)) return false;
	//
	// 		for (const auto& ExcludedFolder : ScanSettings->ExcludedFolders)
	// 		{
	// 			if (AssetData.PackagePath.ToString().Equals(ExcludedFolder.Path) && AssetData.PackagePath.ToString().Equals(SelectedPath))
	// 			{
	// 				return true;
	// 			}
	// 		}
	//
	// 		return false;
	// 	}
	//
	// 	FString SelectedPath;
	// };
	//
	//
	// // todo:ashe23 we must show excluded assets also
	// FARFilter Filter;
	// if (Scanner.Get()->GetAssetsUnused().Num() == 0)
	// {
	// 	// this is needed for disabling showing primary assets in browser, when there is no unused assets
	// 	Filter.TagsAndValues.Add(FName{"ProjectCleanerEmptyTag"}, FString{"ProjectCleanerEmptyTag"});
	// }
	// else
	// {
	// 	Filter.PackageNames.Reserve(Scanner.Get()->GetAssetsUnused().Num());
	// 	for (const auto& Asset : Scanner.Get()->GetAssetsUnused())
	// 	{
	// 		Filter.PackageNames.Add(Asset.PackageName);
	// 	}
	// }
	//
	// Filter.PackagePaths.Add(FName{*PathSelected});
	//
	// const TSharedPtr<FFrontendFilterCategory> ProjectCleanerCategory = MakeShareable(
	// 	new FFrontendFilterCategory(
	// 		FText::FromString(TEXT("ProjectCleaner Filters")),
	// 		FText::FromString(TEXT(""))
	// 	)
	// );
	// const auto FrontendFilter = MakeShareable(new FFrontendFilter_ProjectCleaner(ProjectCleanerCategory, PathSelected));
	//
	// FAssetPickerConfig AssetPickerConfig;
	// AssetPickerConfig.Filter = Filter;
	// AssetPickerConfig.SelectionMode = ESelectionMode::Multi;
	// AssetPickerConfig.InitialAssetViewType = EAssetViewType::Tile;
	// AssetPickerConfig.bCanShowFolders = false;
	// AssetPickerConfig.bAddFilterUI = true;
	// AssetPickerConfig.bPreloadAssetsForContextMenu = false;
	// AssetPickerConfig.bSortByPathInColumnView = false;
	// AssetPickerConfig.bShowPathInColumnView = false;
	// AssetPickerConfig.bShowBottomToolbar = true;
	// AssetPickerConfig.bCanShowDevelopersFolder = ScanSettings->bScanDeveloperContents;
	// AssetPickerConfig.bCanShowClasses = false;
	// AssetPickerConfig.bAllowDragging = false;
	// AssetPickerConfig.bForceShowEngineContent = false;
	// AssetPickerConfig.bCanShowRealTimeThumbnails = false;
	// AssetPickerConfig.AssetShowWarningText = FText::FromName("No assets");
	// AssetPickerConfig.ExtraFrontendFilters.Add(FrontendFilter);

	// FGetCurrentSelectionDelegate CurrentSelectionDelegate;
	// AssetPickerConfig.GetCurrentSelectionDelegates.Add(&CurrentSelectionDelegate);
	// AssetPickerConfig.RefreshAssetViewDelegates.Add(&RefreshAssetViewDelegate);

	// AssetPickerConfig.OnAssetDoubleClicked = FOnAssetDoubleClicked::CreateStatic(
	// 	&SProjectCleanerUnusedAssetsBrowserUI::OnAssetDblClicked
	// );
	// AssetPickerConfig.OnGetAssetContextMenu = FOnGetAssetContextMenu::CreateRaw(
	// 	this,
	// 	&SProjectCleanerUnusedAssetsBrowserUI::OnGetAssetContextMenu
	// );

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		  .Padding(FMargin{5.0f})
		  .FillHeight(1.0f)
		[
			SNew(SSplitter)
			.Style(FEditorStyle::Get(), "ContentBrowser.Splitter")
			.PhysicalSplitterHandleSize(5.0f)
			.Orientation(Orient_Vertical)
			+ SSplitter::Slot()
			[
				ProjectCleanerTreeView.ToSharedRef()
			]
			+ SSplitter::Slot()
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				  .Padding(FMargin{0.0f, 5.0f})
				  .AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SButton)
						.ContentPadding(FMargin{5.0f})
						// .OnClicked_Raw(this, &SProjectCleaner::OnBtnCleanProjectClick)
						.ButtonColorAndOpacity(FProjectCleanerStyles::Get().GetColor("ProjectCleaner.Color.Blue"))
						[
							SNew(STextBlock)
							.Justification(ETextJustify::Center)
							.ToolTipText(FText::FromString(TEXT("Include all excluded assets and mark them as unused")))
							.ColorAndOpacity(FProjectCleanerStyles::Get().GetColor("ProjectCleaner.Color.White"))
							.ShadowOffset(FVector2D{1.5f, 1.5f})
							.ShadowColorAndOpacity(FLinearColor::Black)
							.Font(FProjectCleanerStyles::GetFont("Bold", 10))
							.Text(FText::FromString(TEXT("Include all excluded assets")))
						]
					]
				]
				// + SVerticalBox::Slot()
				//   .Padding(FMargin{0.0f, 5.0f})
				//   .AutoHeight()
				// [
				// 	SNew(SSeparator)
				// 	.Thickness(5.0f)
				// ]
				// + SVerticalBox::Slot()
				//   .Padding(FMargin{0.0f, 5.0f})
				//   .FillHeight(1.0f)
				// [
				// 	SAssignNew(ProjectCleanerAssetBrowser, SProjectCleanerAssetBrowser)
				// 	.Scanner(Scanner)
				// 	// ModuleContentBrowser.Get().CreateAssetPicker(AssetPickerConfig)
				// ]
			]
		]
	];
}

void SProjectCleanerTabUnused::UpdateFilter()
{
	// Filter.Clear();
	//
	// if (Scanner->GetAssetsUnused().Num() == 0)
	// {
	// 	// this is needed for disabling showing primary assets in browser, when there is no unused assets
	// 	Filter.TagsAndValues.Add(FName{"ProjectCleanerEmptyTag"}, FString{"ProjectCleanerEmptyTag"});
	//
	// 	return;
	// }
	//
	// for (const auto& UnusedAsset : Scanner->GetAssetsUnused())
	// {
	// 	Filter.ObjectPaths.Add(UnusedAsset.ObjectPath);
	// }
	//
	// if (SelectedPaths.Num() == 0)
	// {
	// 	Filter.PackagePaths.Add(ProjectCleanerConstants::PathRelRoot);
	// }
	// else
	// {
	// 	for (const auto& SelectedPath : SelectedPaths)
	// 	{
	// 		Filter.PackagePaths.Add(FName{*SelectedPath});
	// 	}
	// }
}
