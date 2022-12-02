// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/Tabs/SProjectCleanerTabUnused.h"
#include "Slate/TreeView/SProjectCleanerTreeView.h"
#include "ProjectCleaner.h"
#include "ProjectCleanerScanner.h"
#include "ProjectCleanerConstants.h"
#include "ProjectCleanerScanSettings.h"
// Engine Headers
#include "ContentBrowserModule.h"
#include "ContentBrowserItem.h"
#include "FrontendFilterBase.h"
#include "IContentBrowserSingleton.h"

void SProjectCleanerTabUnused::Construct(const FArguments& InArgs)
{
	Scanner = InArgs._Scanner;
	if (!Scanner.IsValid()) return;
	
	ScanSettings = GetMutableDefault<UProjectCleanerScanSettings>();
	if (!ScanSettings.IsValid()) return;

	UpdateView();
}

void SProjectCleanerTabUnused::UpdateView()
{
	const FContentBrowserModule& ModuleContentBrowser = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");

	class FFrontendFilter_ProjectCleaner : public FFrontendFilter
	{
	public:
		explicit FFrontendFilter_ProjectCleaner(TSharedPtr<FFrontendFilterCategory> InCategory) : FFrontendFilter(InCategory)
		{
		}

		virtual FString GetName() const override { return TEXT("ProjectCleanerAssets"); }
		virtual FText GetDisplayName() const override { return FText::FromString(TEXT("ProjectCleanerAssets")); }
		virtual FText GetToolTipText() const override { return FText::FromString(TEXT("ProjectCleanerAssets ToolTip")); }
		virtual FLinearColor GetColor() const override { return FLinearColor{FColor::FromHex(TEXT("#06d6a0"))}; }

		virtual bool PassesFilter(const FContentBrowserItem& InItem) const override
		{
			FAssetData AssetData;
			if (InItem.Legacy_TryGetAssetData(AssetData))
			{
				return AssetData.AssetClass.IsEqual(UStaticMesh::StaticClass()->GetFName());
			}

			return false;
		}
	};

	FGetCurrentSelectionDelegate CurrentSelectionDelegate;

	FARFilter Filter;
	if (Scanner.Get()->GetAssetsUnused().Num() == 0)
	{
		// this is needed for disabling showing primary assets in browser, when there is no unused assets
		Filter.TagsAndValues.Add(FName{ "ProjectCleanerEmptyTag" }, FString{ "ProjectCleanerEmptyTag" });
	}
	else
	{
		Filter.PackageNames.Reserve(Scanner.Get()->GetAssetsUnused().Num());
		for (const auto& Asset : Scanner.Get()->GetAssetsUnused())
		{
			Filter.PackageNames.Add(Asset.PackageName);
		}
	}

	// todo:ashe23 change later
	Filter.PackagePaths.Add(ProjectCleanerConstants::PathRelRoot);
	
	FAssetPickerConfig AssetPickerConfig;
	AssetPickerConfig.Filter = Filter;
	AssetPickerConfig.SelectionMode = ESelectionMode::Multi;
	AssetPickerConfig.InitialAssetViewType = EAssetViewType::Tile;
	AssetPickerConfig.bCanShowFolders = false;
	AssetPickerConfig.bAddFilterUI = true;
	AssetPickerConfig.bPreloadAssetsForContextMenu = false;
	AssetPickerConfig.bSortByPathInColumnView = false;
	AssetPickerConfig.bShowPathInColumnView = false;
	AssetPickerConfig.bShowBottomToolbar = true;
	// AssetPickerConfig.bCanShowDevelopersFolder = ScanSettings->bScanDeveloperContents;
	AssetPickerConfig.bCanShowClasses = false;
	AssetPickerConfig.bAllowDragging = false;
	AssetPickerConfig.bForceShowEngineContent = false;
	AssetPickerConfig.bCanShowRealTimeThumbnails = false;
	AssetPickerConfig.AssetShowWarningText = FText::FromName("No assets");

	const TSharedPtr<FFrontendFilterCategory> ProjectCleanerCategory = MakeShareable(
		new FFrontendFilterCategory(
			FText::FromString(TEXT("ProjectCleaner Filters")),
			FText::FromString(TEXT("ProjectCleaner Filter Tooltip"))
		)
	);
	AssetPickerConfig.ExtraFrontendFilters.Add(MakeShareable(new FFrontendFilter_ProjectCleaner(ProjectCleanerCategory)));

	AssetPickerConfig.GetCurrentSelectionDelegates.Add(&CurrentSelectionDelegate);
	// AssetPickerConfig.RefreshAssetViewDelegates.Add(&RefreshAssetViewDelegate);

	FProjectCleanerTreeViewSelectionChangeDelegate SelectionChangeDelegate;
	SelectionChangeDelegate.BindRaw(this, &SProjectCleanerTabUnused::OnTreeViewSelectionChange);
	
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
				SAssignNew(ProjectCleanerTreeView, SProjectCleanerTreeView)
				.OnSelectionChange(SelectionChangeDelegate)
				.Scanner(Scanner)
			]
			+ SSplitter::Slot()
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.Padding(FMargin{0.0f, 5.0f})
				[
					ModuleContentBrowser.Get().CreateAssetPicker(AssetPickerConfig)
				]
			]
		]
	];
}

void SProjectCleanerTabUnused::OnTreeViewSelectionChange(const TSharedPtr<FProjectCleanerTreeViewItem>& Item)
{
	UE_LOG(LogProjectCleaner, Warning, TEXT("%s"), *Item->FolderPathRel);
}
