// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "SLate/AssetBrowser/SPjcAssetBrowser.h"
#include "Slate/AssetStats/SPjcAssetStats.h"
#include "EditorSettings/PjcEditorAssetExcludeSettings.h"
#include "PjcStyles.h"
#include "PjcSubsystem.h"
#include "Libs/PjcLibEditor.h"
// Engine Headers
#include "ContentBrowserModule.h"
#include "EditorWidgetsModule.h"
#include "IContentBrowserSingleton.h"
#include "PjcConstants.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SWidgetSwitcher.h"

void SPjcAssetBrowser::Construct(const FArguments& InArgs)
{
	if (!GEditor) return;

	SubsystemPtr = GEditor->GetEditorSubsystem<UPjcSubsystem>();
	if (!SubsystemPtr) return;

	SubsystemPtr->OnScanAssets().AddRaw(this, &SPjcAssetBrowser::OnScanAssets);

	FEditorWidgetsModule& EditorWidgetsModule = FModuleManager::LoadModuleChecked<FEditorWidgetsModule>("EditorWidgets");
	const TSharedRef<SWidget> AssetDiscoveryIndicator = EditorWidgetsModule.CreateAssetDiscoveryIndicator(EAssetDiscoveryIndicatorScaleMode::Scale_None, FMargin(16, 8), false);

	FPropertyEditorModule& PropertyEditor = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.bUpdatesFromSelection = false;
	DetailsViewArgs.bLockable = false;
	DetailsViewArgs.bAllowSearch = false;
	DetailsViewArgs.bShowOptions = true;
	DetailsViewArgs.bAllowFavoriteSystem = false;
	DetailsViewArgs.bShowPropertyMatrixButton = false;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	DetailsViewArgs.ViewIdentifier = "PjcScaSettingsEditor";

	const auto SettingsProperty = PropertyEditor.CreateDetailView(DetailsViewArgs);
	SettingsProperty->SetObject(GetMutableDefault<UPjcEditorAssetExcludeSettings>());

	const FContentBrowserModule& ModuleContentBrowser = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>(PjcConstants::ModuleContentBrowser);

	Filter.TagsAndValues.Add(PjcConstants::EmptyTagName, PjcConstants::EmptyTagName.ToString());

	FAssetPickerConfig AssetPickerConfig;
	AssetPickerConfig.bAllowNullSelection = false;
	AssetPickerConfig.InitialAssetViewType = EAssetViewType::Tile;
	AssetPickerConfig.bCanShowFolders = false;
	AssetPickerConfig.bAddFilterUI = true;
	AssetPickerConfig.bSortByPathInColumnView = true;
	AssetPickerConfig.bShowPathInColumnView = true;
	AssetPickerConfig.bShowBottomToolbar = true;
	AssetPickerConfig.bCanShowDevelopersFolder = true;
	AssetPickerConfig.bCanShowClasses = false;
	AssetPickerConfig.bAllowDragging = false;
	AssetPickerConfig.bForceShowEngineContent = false;
	AssetPickerConfig.bCanShowRealTimeThumbnails = false;
	AssetPickerConfig.AssetShowWarningText = FText::FromName("No Unused Assets To Display.");
	AssetPickerConfig.OnAssetDoubleClicked.BindRaw(this, &SPjcAssetBrowser::OnAssetDblClick);
	AssetPickerConfig.GetCurrentSelectionDelegates.Add(&DelegateSelection);
	AssetPickerConfig.RefreshAssetViewDelegates.Add(&DelegateRefreshView);
	AssetPickerConfig.SetFilterDelegates.Add(&DelegateFilter);
	AssetPickerConfig.Filter = Filter;

	//
	// 	AssetPickerConfig.OnGetAssetContextMenu = FOnGetAssetContextMenu::CreateLambda([&](const TArray<FAssetData>&)
	// 	{
	// 		FMenuBuilder MenuBuilder{true, Cmds};
	// 		MenuBuilder.BeginSection(TEXT("AssetInfoActions"), FText::FromName(TEXT("Info")));
	// 		{
	// 			MenuBuilder.AddMenuEntry(FPjcCmds::Get().OpenSizeMap);
	// 			MenuBuilder.AddMenuEntry(FPjcCmds::Get().OpenReferenceViewer);
	// 			MenuBuilder.AddMenuEntry(FPjcCmds::Get().OpenAssetAudit);
	// 		}
	// 		MenuBuilder.EndSection();
	// 		MenuBuilder.BeginSection(TEXT("AssetExcludeActions"), FText::FromName(TEXT("Exclusion")));
	// 		{
	// 			MenuBuilder.AddMenuEntry(FPjcCmds::Get().AssetsExclude);
	// 			MenuBuilder.AddMenuEntry(FPjcCmds::Get().AssetsInclude);
	// 			MenuBuilder.AddMenuEntry(FPjcCmds::Get().AssetsExcludeByClass);
	// 			MenuBuilder.AddMenuEntry(FPjcCmds::Get().AssetsIncludeByClass);
	// 		}
	// 		MenuBuilder.EndSection();
	// 		MenuBuilder.BeginSection(TEXT("AssetDeletionActions"), FText::FromName(TEXT("Deletion")));
	// 		{
	// 			MenuBuilder.AddMenuEntry(FPjcCmds::Get().AssetsDelete);
	// 		}
	// 		MenuBuilder.EndSection();
	//
	// 		return MenuBuilder.MakeWidget();
	// 	});
	//
	// 	const TSharedPtr<FFrontendFilterCategory> DefaultCategory = MakeShareable(new FFrontendFilterCategory(FText::FromString(TEXT("ProjectCleaner Filters")), FText::FromString(TEXT(""))));
	// 	const TSharedPtr<FPjcFilterAssetsPrimary> FilterPrimary = MakeShareable(new FPjcFilterAssetsPrimary(DefaultCategory));
	// 	const TSharedPtr<FPjcFilterAssetsExcluded> FilterExcluded = MakeShareable(new FPjcFilterAssetsExcluded(DefaultCategory));
	// 	const TSharedPtr<FPjcFilterAssetsIndirect> FilterIndirect = MakeShareable(new FPjcFilterAssetsIndirect(DefaultCategory));
	// 	const TSharedPtr<FPjcFilterAssetsExtReferenced> FilterExtReferenced = MakeShareable(new FPjcFilterAssetsExtReferenced(DefaultCategory));
	// 	const TSharedPtr<FPjcFilterAssetsUsed> FilterUsed = MakeShareable(new FPjcFilterAssetsUsed(DefaultCategory));
	//
	// 	FilterPrimary->OnFilterChange().AddLambda([&](const bool bActive)
	// 	{
	// 		bFilterPrimaryActive = bActive;
	// 		if (AssetBrowserDelegateFilter.IsBound())
	// 		{
	// 			AssetBrowserDelegateFilter.Execute(AssetBrowserCreateFilter());
	// 		}
	// 	});
	//
	// 	FilterExcluded->OnFilterChange().AddLambda([&](const bool bActive)
	// 	{
	// 		bFilterExcludeActive = bActive;
	// 		if (AssetBrowserDelegateFilter.IsBound())
	// 		{
	// 			AssetBrowserDelegateFilter.Execute(AssetBrowserCreateFilter());
	// 		}
	// 	});
	//
	// 	FilterIndirect->OnFilterChange().AddLambda([&](const bool bActive)
	// 	{
	// 		bFilterIndirectActive = bActive;
	// 		if (AssetBrowserDelegateFilter.IsBound())
	// 		{
	// 			AssetBrowserDelegateFilter.Execute(AssetBrowserCreateFilter());
	// 		}
	// 	});
	//
	// 	FilterExtReferenced->OnFilterChange().AddLambda([&](const bool bActive)
	// 	{
	// 		bFilterExtReferencedActive = bActive;
	// 		if (AssetBrowserDelegateFilter.IsBound())
	// 		{
	// 			AssetBrowserDelegateFilter.Execute(AssetBrowserCreateFilter());
	// 		}
	// 	});
	//
	// 	FilterUsed->OnFilterChange().AddLambda([&](const bool bActive)
	// 	{
	// 		bFilterUsedActive = bActive;
	// 		if (AssetBrowserDelegateFilter.IsBound())
	// 		{
	// 			AssetBrowserDelegateFilter.Execute(AssetBrowserCreateFilter());
	// 		}
	// 	});
	//
	// 	AssetPickerConfig.ExtraFrontendFilters.Add(FilterUsed.ToSharedRef());
	// 	AssetPickerConfig.ExtraFrontendFilters.Add(FilterPrimary.ToSharedRef());
	// 	AssetPickerConfig.ExtraFrontendFilters.Add(FilterExcluded.ToSharedRef());
	// 	AssetPickerConfig.ExtraFrontendFilters.Add(FilterIndirect.ToSharedRef());
	// 	AssetPickerConfig.ExtraFrontendFilters.Add(FilterExtReferenced.ToSharedRef());

	ChildSlot
	[
		SNew(SSplitter)
		.Style(FEditorStyle::Get(), "DetailsView.Splitter")
		.PhysicalSplitterHandleSize(3.0f)
		+ SSplitter::Slot()
		.Value(0.3f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(5.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SButton)
					.ContentPadding(FMargin{3.0f})
					.OnClicked_Raw(this, &SPjcAssetBrowser::OnBtnScanAssetsClick)
					.ButtonColorAndOpacity(FPjcStyles::Get().GetColor("ProjectCleaner.Color.Blue"))
					[
						SNew(STextBlock)
						.Justification(ETextJustify::Center)
						.ToolTipText(FText::FromString(TEXT("Scan project for unused assets and empty folders")))
						.ColorAndOpacity(FPjcStyles::Get().GetColor("ProjectCleaner.Color.White"))
						.ShadowOffset(FVector2D{1.5f, 1.5f})
						.ShadowColorAndOpacity(FLinearColor::Black)
						.Font(FPjcStyles::GetFont("Bold", 10))
						.Text(FText::FromString(TEXT("Scan Project")))
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin{5.0f, 0.0f, 0.0f, 0.0f})
				[
					SNew(SButton)
					.ContentPadding(FMargin{3.0f})
					.ButtonColorAndOpacity(FPjcStyles::Get().GetColor("ProjectCleaner.Color.Red"))
					[
						SNew(STextBlock)
						.Justification(ETextJustify::Center)
						.ToolTipText(FText::FromString(TEXT("Cleanup project based on CleanupMethod setting.")))
						.ColorAndOpacity(FPjcStyles::Get().GetColor("ProjectCleaner.Color.White"))
						.ShadowOffset(FVector2D{1.5f, 1.5f})
						.ShadowColorAndOpacity(FLinearColor::Black)
						.Font(FPjcStyles::GetFont("Bold", 10))
						.Text(FText::FromString(TEXT("Clean Project")))
					]
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(5.0f)
			[
				SNew(SSeparator).Thickness(5.0f)
			]

			+ SVerticalBox::Slot().AutoHeight().Padding(5.0f)
			[
				SNew(SPjcAssetStats)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(5.0f)
			[
				SNew(SSeparator).Thickness(5.0f)
			]
			+ SVerticalBox::Slot().FillHeight(1.0f).Padding(5.0f)
			[
				SNew(SBox)
				[
					SNew(SScrollBox)
					.ScrollWhenFocusChanges(EScrollWhenFocusChanges::NoScroll)
					.AnimateWheelScrolling(true)
					.AllowOverscroll(EAllowOverscroll::No)
					+ SScrollBox::Slot()
					[
						SettingsProperty
					]
				]
			]
		]
		+ SSplitter::Slot()
		.Value(0.7f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().FillHeight(1.0f)
			[
				SNew(SOverlay)
				+ SOverlay::Slot().Padding(5.0f)
				[
					SNew(SSplitter)
					.Style(FEditorStyle::Get(), "DetailsView.Splitter")
					.PhysicalSplitterHandleSize(3.0f)
					+ SSplitter::Slot().Value(0.3f)
					[
						SNew(STextBlock).Text(FText::FromString(TEXT("TreeView")))
					]
					+ SSplitter::Slot().Value(0.7f)
					[
						ModuleContentBrowser.Get().CreateAssetPicker(AssetPickerConfig)
					]
				]
				+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(5.0f)
				[
					AssetDiscoveryIndicator
				]
			]
		]
	];
}

SPjcAssetBrowser::~SPjcAssetBrowser()
{
	if (!SubsystemPtr) return;

	SubsystemPtr->OnScanAssets().RemoveAll(this);
}

FReply SPjcAssetBrowser::OnBtnScanAssetsClick() const
{
	if (SubsystemPtr)
	{
		FPjcScanDataAssets Data;
		SubsystemPtr->ScanAssets(FPjcLibEditor::GetEditorAssetExcludeSettings(), Data);
	}

	return FReply::Handled();
}

void SPjcAssetBrowser::OnScanAssets(const FPjcScanDataAssets& InScanDataAssets)
{
	Filter.Clear();

	if (InScanDataAssets.AssetsUnused.Num() == 0)
	{
		// this is needed for disabling showing primary assets in browser, when there is no unused assets
		Filter.TagsAndValues.Add(PjcConstants::EmptyTagName, PjcConstants::EmptyTagName.ToString());

		DelegateFilter.Execute(Filter);
		return;
	}

	Filter.PackageNames.Reserve(InScanDataAssets.AssetsUnused.Num());

	for (const auto& Asset : InScanDataAssets.AssetsUnused)
	{
		Filter.PackageNames.Emplace(Asset.PackageName);
	}

	DelegateFilter.Execute(Filter);
}

void SPjcAssetBrowser::OnAssetDblClick(const FAssetData& AssetData)
{
	FPjcLibEditor::OpenAssetEditor(AssetData);
}
