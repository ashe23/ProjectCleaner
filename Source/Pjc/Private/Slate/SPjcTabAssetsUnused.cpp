// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/SPjcTabAssetsUnused.h"
#include "Slate/SPjcItemStat.h"
#include "Slate/SPjcItemTree.h"
#include "Pjc.h"
#include "PjcCmds.h"
#include "PjcTypes.h"
#include "PjcStyles.h"
#include "PjcSubsystem.h"
#include "PjcConstants.h"
#include "PjcFrontendFilters.h"
// Engine Headers
#include "FileHelpers.h"
#include "Misc/ScopedSlowTask.h"
#include "Settings/ContentBrowserSettings.h"
#include "FrontendFilterBase.h"
#include "ObjectTools.h"
#include "IContentBrowserSingleton.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"

void SPjcTabAssetsUnused::Construct(const FArguments& InArgs)
{
	SubsystemPtr = GEditor->GetEditorSubsystem<UPjcSubsystem>();
	if (!SubsystemPtr) return;

	UContentBrowserSettings* ContentBrowserSettings = GetMutableDefault<UContentBrowserSettings>();
	if (!ContentBrowserSettings) return;

	ContentBrowserSettings->SetDisplayDevelopersFolder(true);
	ContentBrowserSettings->SetDisplayEngineFolder(false);
	ContentBrowserSettings->SetDisplayCppFolders(false);
	ContentBrowserSettings->SetDisplayPluginFolders(false);
	ContentBrowserSettings->PostEditChange();

	Cmds = MakeShareable(new FUICommandList);
	Cmds->MapAction(FPjcCmds::Get().ScanProject, FExecuteAction::CreateRaw(this, &SPjcTabAssetsUnused::OnProjectScan));
	Cmds->MapAction(
		FPjcCmds::Get().CleanProject,
		FExecuteAction::CreateRaw(this, &SPjcTabAssetsUnused::OnProjectClean),
		FCanExecuteAction::CreateRaw(this, &SPjcTabAssetsUnused::CanCleanProject)

	);
	Cmds->MapAction(FPjcCmds::Get().ClearExcludeSettings, FExecuteAction::CreateRaw(this, &SPjcTabAssetsUnused::OnResetExcludeSettings));
	Cmds->MapAction(
		FPjcCmds::Get().DeleteEmptyFolders,
		FExecuteAction::CreateRaw(this, &SPjcTabAssetsUnused::OnDeleteEmptyFolders),
		FCanExecuteAction::CreateRaw(this, &SPjcTabAssetsUnused::CanDeleteEmptyFolders)
	);
	Cmds->MapAction(
		FPjcCmds::Get().PathsExclude,
		FExecuteAction::CreateRaw(this, &SPjcTabAssetsUnused::OnPathExclude),
		FCanExecuteAction::CreateRaw(this, &SPjcTabAssetsUnused::TreeHasSelection)
	);
	Cmds->MapAction(
		FPjcCmds::Get().PathsInclude,
		FExecuteAction::CreateRaw(this, &SPjcTabAssetsUnused::OnPathInclude),
		FCanExecuteAction::CreateRaw(this, &SPjcTabAssetsUnused::TreeHasSelection)
	);
	Cmds->MapAction(
		FPjcCmds::Get().ClearSelection,
		FExecuteAction::CreateRaw(this, &SPjcTabAssetsUnused::OnClearSelection),
		FCanExecuteAction::CreateRaw(this, &SPjcTabAssetsUnused::TreeHasSelection)
	);

	Cmds->MapAction(
		FPjcCmds::Get().OpenViewerSizeMap,
		FExecuteAction::CreateLambda([&]()
		{
			UPjcSubsystem::OpenSizeMapViewer(DelegateSelection.Execute());
		}),
		FCanExecuteAction::CreateLambda([&]()
		{
			return DelegateSelection.Execute().Num() > 0;
		})
	);

	Cmds->MapAction(
		FPjcCmds::Get().OpenViewerReference,
		FExecuteAction::CreateLambda([&]()
		{
			UPjcSubsystem::OpenReferenceViewer(DelegateSelection.Execute());
		}),
		FCanExecuteAction::CreateLambda([&]()
		{
			return DelegateSelection.Execute().Num() > 0;
		})
	);

	Cmds->MapAction(
		FPjcCmds::Get().OpenViewerAssetsAudit,
		FExecuteAction::CreateLambda([&]()
		{
			UPjcSubsystem::OpenAssetAuditViewer(DelegateSelection.Execute());
		}),
		FCanExecuteAction::CreateLambda([&]()
		{
			return DelegateSelection.Execute().Num() > 0;
		})
	);

	Cmds->MapAction(
		FPjcCmds::Get().AssetsExclude,
		FExecuteAction::CreateLambda([&]()
		{
			UPjcAssetExcludeSettings* ExcludeSettings = GetMutableDefault<UPjcAssetExcludeSettings>();
			if (!ExcludeSettings) return;

			for (const auto& Asset : DelegateSelection.Execute())
			{
				ExcludeSettings->ExcludedAssets.Emplace(Asset.GetAsset());
			}
			ExcludeSettings->PostEditChange();

			ScanProject();
		}),
		FCanExecuteAction::CreateLambda([&]()
		{
			return !bFilterAssetsExcludedActive && DelegateSelection.Execute().Num() > 0;
		})
	);

	Cmds->MapAction(
		FPjcCmds::Get().AssetsExcludeByClass,
		FExecuteAction::CreateLambda([&]()
		{
			UPjcAssetExcludeSettings* ExcludeSettings = GetMutableDefault<UPjcAssetExcludeSettings>();
			if (!ExcludeSettings) return;

			for (const auto& Asset : DelegateSelection.Execute())
			{
				const UClass* AssetClass = UPjcSubsystem::GetAssetClassByName(UPjcSubsystem::GetAssetExactClassName(Asset));
				if (!AssetClass) continue;

				ExcludeSettings->ExcludedClasses.Emplace(AssetClass);
			}

			ExcludeSettings->PostEditChange();

			ScanProject();
		}),
		FCanExecuteAction::CreateLambda([&]()
		{
			return !bFilterAssetsExcludedActive && DelegateSelection.Execute().Num() > 0;
		})
	);

	Cmds->MapAction(
		FPjcCmds::Get().AssetsInclude,
		FExecuteAction::CreateLambda([&]()
		{
			// todo:ashe23 check if selected assets are not exclude by paths

			UPjcAssetExcludeSettings* ExcludeSettings = GetMutableDefault<UPjcAssetExcludeSettings>();
			if (!ExcludeSettings) return;

			TSet<FName> ObjectPaths;

			for (const auto& Asset : DelegateSelection.Execute())
			{
				ObjectPaths.Emplace(Asset.ToSoftObjectPath().GetAssetPathName());
			}

			ExcludeSettings->ExcludedAssets.RemoveAllSwap([&](const TSoftObjectPtr<UObject>& ExcludedAsset)
			{
				return ObjectPaths.Contains(ExcludedAsset.ToSoftObjectPath().GetAssetPathName());
			}, false);
			ExcludeSettings->PostEditChange();

			ScanProject();
		}),
		FCanExecuteAction::CreateLambda([&]()
		{
			return bFilterAssetsExcludedActive && DelegateSelection.Execute().Num() > 0;
		})
	);

	Cmds->MapAction(
		FPjcCmds::Get().AssetsIncludeByClass,
		FExecuteAction::CreateLambda([&]()
		{
			UPjcAssetExcludeSettings* ExcludeSettings = GetMutableDefault<UPjcAssetExcludeSettings>();
			if (!ExcludeSettings) return;

			TSet<FName> ClassNames;

			for (const auto& Asset : DelegateSelection.Execute())
			{
				ClassNames.Emplace(UPjcSubsystem::GetAssetExactClassName(Asset));
			}

			ExcludeSettings->ExcludedClasses.RemoveAllSwap([&](const TSoftClassPtr<UObject>& ExcludedClass)
			{
				return ClassNames.Contains(ExcludedClass.Get()->GetFName());
			}, false);

			ExcludeSettings->PostEditChange();

			ScanProject();
		}),
		FCanExecuteAction::CreateLambda([&]()
		{
			return bFilterAssetsExcludedActive && DelegateSelection.Execute().Num() > 0;
		})
	);

	Cmds->MapAction(
		FPjcCmds::Get().AssetsDelete,
		FExecuteAction::CreateLambda([&]()
		{
			ObjectTools::DeleteAssets(DelegateSelection.Execute(), true);

			ScanProject();
		}),
		FCanExecuteAction::CreateLambda([&]()
		{
			return DelegateSelection.Execute().Num() > 0;
		})
	);

	FPropertyEditorModule& PropertyEditor = UPjcSubsystem::GetModulePropertyEditor();
	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.bUpdatesFromSelection = false;
	DetailsViewArgs.bLockable = false;
	DetailsViewArgs.bAllowSearch = false;
	DetailsViewArgs.bShowOptions = true;
	DetailsViewArgs.bAllowFavoriteSystem = false;
	DetailsViewArgs.bShowPropertyMatrixButton = false;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	DetailsViewArgs.ViewIdentifier = "PjcEditorAssetExcludeSettings";

	const auto SettingsProperty = PropertyEditor.CreateDetailView(DetailsViewArgs);
	SettingsProperty->SetObject(GetMutableDefault<UPjcAssetExcludeSettings>());

	SAssignNew(StatsListView, SListView<TSharedPtr<FPjcStatItem>>)
	.ListItemsSource(&StatsListItems)
	.OnGenerateRow(this, &SPjcTabAssetsUnused::OnStatsGenerateRow)
	.SelectionMode(ESelectionMode::None)
	.IsFocusable(false)
	.HeaderRow(GetStatsHeaderRow());

	SAssignNew(TreeListView, STreeView<TSharedPtr<FPjcTreeItem>>)
	.TreeItemsSource(&TreeListItems)
	.SelectionMode(ESelectionMode::Multi)
	.OnGenerateRow(this, &SPjcTabAssetsUnused::OnTreeGenerateRow)
	.OnGetChildren(this, &SPjcTabAssetsUnused::OnTreeGetChildren)
	.OnContextMenuOpening_Raw(this, &SPjcTabAssetsUnused::GetTreeContextMenu)
	.OnSelectionChanged_Raw(this, &SPjcTabAssetsUnused::OnTreeSelectionChanged)
	.OnExpansionChanged_Raw(this, &SPjcTabAssetsUnused::OnTreeExpansionChanged)
	.HeaderRow(GetTreeHeaderRow());

	FAssetPickerConfig AssetPickerConfig;
	AssetPickerConfig.bAddFilterUI = true;
	AssetPickerConfig.bCanShowFolders = false;
	AssetPickerConfig.bAllowDragging = false;
	AssetPickerConfig.SelectionMode = ESelectionMode::Multi;
	AssetPickerConfig.bShowBottomToolbar = true;
	AssetPickerConfig.bCanShowDevelopersFolder = true;
	AssetPickerConfig.bForceShowEngineContent = false;
	AssetPickerConfig.bForceShowPluginContent = false;
	AssetPickerConfig.bAllowNullSelection = false;
	AssetPickerConfig.bCanShowClasses = false;
	AssetPickerConfig.bCanShowRealTimeThumbnails = false;
	AssetPickerConfig.AssetShowWarningText = FText::FromString(TEXT("No assets"));
	AssetPickerConfig.Filter = Filter;
	AssetPickerConfig.bAllowNullSelection = true;
	AssetPickerConfig.GetCurrentSelectionDelegates.Add(&DelegateSelection);
	AssetPickerConfig.RefreshAssetViewDelegates.Add(&DelegateRefreshView);
	AssetPickerConfig.SetFilterDelegates.Add(&DelegateFilter);
	AssetPickerConfig.OnAssetDoubleClicked.BindLambda([](const FAssetData& InAsset)
	{
		UPjcSubsystem::OpenAssetEditor(InAsset);
	});
	AssetPickerConfig.OnGetAssetContextMenu.BindLambda([&](const TArray<FAssetData>&)
	{
		const TSharedPtr<FExtender> Extender;
		FMenuBuilder MenuBuilder(true, Cmds, Extender, true);

		MenuBuilder.BeginSection(TEXT("Info"), FText::FromString(TEXT("Info")));
		{
			MenuBuilder.AddMenuEntry(FPjcCmds::Get().OpenViewerSizeMap);
			MenuBuilder.AddMenuEntry(FPjcCmds::Get().OpenViewerReference);
			MenuBuilder.AddMenuEntry(FPjcCmds::Get().OpenViewerAssetsAudit);
		}
		MenuBuilder.EndSection();

		MenuBuilder.BeginSection(TEXT("Exclusion"), FText::FromString(TEXT("Exclusion")));
		{
			MenuBuilder.AddMenuEntry(FPjcCmds::Get().AssetsExclude);
			MenuBuilder.AddMenuEntry(FPjcCmds::Get().AssetsExcludeByClass);
		}
		MenuBuilder.EndSection();

		MenuBuilder.BeginSection(TEXT("Inclusion"), FText::FromString(TEXT("Inclusion")));
		{
			MenuBuilder.AddMenuEntry(FPjcCmds::Get().AssetsInclude);
			MenuBuilder.AddMenuEntry(FPjcCmds::Get().AssetsIncludeByClass);
		}
		MenuBuilder.EndSection();

		MenuBuilder.BeginSection(TEXT("Deletion"), FText::FromString(TEXT("Deletion")));
		{
			MenuBuilder.AddMenuEntry(FPjcCmds::Get().AssetsDelete);
		}
		MenuBuilder.EndSection();

		return MenuBuilder.MakeWidget();
	});

	const TSharedPtr<FFrontendFilterCategory> DefaultCategory = MakeShareable(new FFrontendFilterCategory(FText::FromString(TEXT("ProjectCleaner Filters")), FText::FromString(TEXT(""))));
	const TSharedPtr<FPjcFilterAssetsUsed> FilterUsed = MakeShareable(new FPjcFilterAssetsUsed(DefaultCategory));
	const TSharedPtr<FPjcFilterAssetsPrimary> FilterPrimary = MakeShareable(new FPjcFilterAssetsPrimary(DefaultCategory));
	const TSharedPtr<FPjcFilterAssetsIndirect> FilterIndirect = MakeShareable(new FPjcFilterAssetsIndirect(DefaultCategory));
	const TSharedPtr<FPjcFilterAssetsCircular> FilterCircular = MakeShareable(new FPjcFilterAssetsCircular(DefaultCategory));
	const TSharedPtr<FPjcFilterAssetsEditor> FilterEditor = MakeShareable(new FPjcFilterAssetsEditor(DefaultCategory));
	FilterExcluded = MakeShareable(new FPjcFilterAssetsExcluded(DefaultCategory));
	const TSharedPtr<FPjcFilterAssetsExtReferenced> FilterExtReferenced = MakeShareable(new FPjcFilterAssetsExtReferenced(DefaultCategory));
	
	FilterUsed->OnFilterChanged().AddLambda([&](const bool bActive)
	{
		bFilterAssetsUsedActive = bActive;
		bFilterAssetsUnusedActive = !AnyFilterActive();

		UpdateContentBrowser();
	});

	FilterPrimary->OnFilterChanged().AddLambda([&](const bool bActive)
	{
		bFilterAssetsPrimaryActive = bActive;
		bFilterAssetsUnusedActive = !AnyFilterActive();

		UpdateContentBrowser();
	});

	FilterIndirect->OnFilterChanged().AddLambda([&](const bool bActive)
	{
		bFilterAssetsIndirectActive = bActive;
		bFilterAssetsUnusedActive = !AnyFilterActive();

		UpdateContentBrowser();
	});

	FilterCircular->OnFilterChanged().AddLambda([&](const bool bActive)
	{
		bFilterAssetsCircularActive = bActive;
		bFilterAssetsUnusedActive = !AnyFilterActive();

		UpdateContentBrowser();
	});

	FilterEditor->OnFilterChanged().AddLambda([&](const bool bActive)
	{
		bFilterAssetsEditorActive = bActive;
		bFilterAssetsUnusedActive = !AnyFilterActive();

		UpdateContentBrowser();
	});

	FilterExcluded->OnFilterChanged().AddLambda([&](const bool bActive)
	{
		bFilterAssetsExcludedActive = bActive;
		bFilterAssetsUnusedActive = !AnyFilterActive();

		UpdateContentBrowser();
	});

	FilterExtReferenced->OnFilterChanged().AddLambda([&](const bool bActive)
	{
		bFilterAssetsExtReferencedActive = bActive;
		bFilterAssetsUnusedActive = !AnyFilterActive();

		UpdateContentBrowser();
	});


	AssetPickerConfig.ExtraFrontendFilters.Emplace(FilterUsed.ToSharedRef());
	AssetPickerConfig.ExtraFrontendFilters.Emplace(FilterPrimary.ToSharedRef());
	AssetPickerConfig.ExtraFrontendFilters.Emplace(FilterIndirect.ToSharedRef());
	AssetPickerConfig.ExtraFrontendFilters.Emplace(FilterCircular.ToSharedRef());
	AssetPickerConfig.ExtraFrontendFilters.Emplace(FilterEditor.ToSharedRef());
	AssetPickerConfig.ExtraFrontendFilters.Emplace(FilterExcluded.ToSharedRef());
	AssetPickerConfig.ExtraFrontendFilters.Emplace(FilterExtReferenced.ToSharedRef());

	UpdateStats();

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().FillHeight(1.0f).Padding(5.0f)
		[
			SNew(SSplitter)
			.PhysicalSplitterHandleSize(3.0f)
			.Style(FEditorStyle::Get(), "DetailsView.Splitter")
			+ SSplitter::Slot().Value(0.2f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(3.0f)
				[
					CreateToolbarMain()
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(3.0f)
				[
					SNew(SSeparator).Thickness(5.0f)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(3.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(1.0f).HAlign(HAlign_Center).VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Justification(ETextJustify::Center)
							.ColorAndOpacity(FPjcStyles::Get().GetColor("ProjectCleaner.Color.Gray"))
							.ShadowOffset(FVector2D{1.5f, 1.5f})
							.ShadowColorAndOpacity(FLinearColor::Black)
							.Font(FPjcStyles::GetFont("Bold", 15))
							.Text(FText::FromString(TEXT("Asset Statistics Summary")))
						]
					]
					+ SVerticalBox::Slot().AutoHeight()
					[
						StatsListView.ToSharedRef()
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(3.0f)
				[
					SNew(SSeparator).Thickness(5.0f)
				]
				+ SVerticalBox::Slot().FillHeight(1.0f).Padding(3.0f)
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
			+ SSplitter::Slot().Value(0.35f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(3.0f)
				[
					CreateToolbarTreeView()
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(3.0f)
				[
					SNew(SSeparator).Thickness(5.0f)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(5.0f)
				[
					SNew(SSearchBox)
					.HintText(FText::FromString(TEXT("Search Folders...")))
					.OnTextChanged_Raw(this, &SPjcTabAssetsUnused::OnTreeSearchTextChanged)
					.OnTextCommitted_Raw(this, &SPjcTabAssetsUnused::OnTreeSearchTextCommitted)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(5.0f, 0.0f, 0.0f, 2.1f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth()
					[
						SNew(SImage)
						.Image(FEditorStyle::GetBrush("ContentBrowser.AssetTreeFolderOpen"))
						.ColorAndOpacity(FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Red"))
					]
					+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin{0.0f, 2.0f, 5.0f, 0.0f})
					[
						SNew(STextBlock).Text(FText::FromString(TEXT(" - Empty Folders")))
					]
					+ SHorizontalBox::Slot().AutoWidth()
					[
						SNew(SImage)
						.Image(FEditorStyle::GetBrush("ContentBrowser.AssetTreeFolderOpen"))
						.ColorAndOpacity(FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Yellow"))
					]
					+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin{0.0f, 2.0f, 5.0f, 0.0f})
					[
						SNew(STextBlock).Text(FText::FromString(TEXT(" - Excluded Folders")))
					]
					+ SHorizontalBox::Slot().AutoWidth()
					[
						SNew(SImage)
						.Image(FEditorStyle::GetBrush("ContentBrowser.AssetTreeFolderOpen"))
						.ColorAndOpacity(FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Blue"))
					]
					+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin{0.0f, 2.0f, 5.0f, 0.0f})
					[
						SNew(STextBlock).Text(FText::FromString(TEXT(" - Engine Folders")))
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(5.0f)
				[
					SNew(SSeparator).Thickness(3.0f)
				]
				+ SVerticalBox::Slot().FillHeight(1.0f).Padding(5.0f)
				[
					SNew(SScrollBox)
					.ScrollWhenFocusChanges(EScrollWhenFocusChanges::NoScroll)
					.AnimateWheelScrolling(true)
					.AllowOverscroll(EAllowOverscroll::No)
					+ SScrollBox::Slot()
					[
						TreeListView.ToSharedRef()
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(5.0f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.0f).HAlign(HAlign_Left).VAlign(VAlign_Center)
					[
						SNew(STextBlock).Text_Raw(this, &SPjcTabAssetsUnused::GetTreeSummaryText)
					]
					+ SHorizontalBox::Slot().FillWidth(1.0f).HAlign(HAlign_Center).VAlign(VAlign_Center)
					[
						SNew(STextBlock).Text_Raw(this, &SPjcTabAssetsUnused::GetTreeSelectionText)
					]
					+ SHorizontalBox::Slot().FillWidth(1.0f).HAlign(HAlign_Right).VAlign(VAlign_Center)
					[
						SNew(SComboButton)
						.ContentPadding(0)
						.ForegroundColor_Raw(this, &SPjcTabAssetsUnused::GetTreeOptionsBtnForegroundColor)
						.ButtonStyle(FEditorStyle::Get(), "ToggleButton")
						.OnGetMenuContent(this, &SPjcTabAssetsUnused::GetTreeBtnOptionsContent)
						.ButtonContent()
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
							[
								SNew(SImage).Image(FEditorStyle::GetBrush("GenericViewButton"))
							]
							+ SHorizontalBox::Slot().AutoWidth().Padding(2.0f, 0.0f, 0.0f, 0.0f).VAlign(VAlign_Center)
							[
								SNew(STextBlock).Text(FText::FromString(TEXT("View Options")))
							]
						]
					]
				]
			]
			+ SSplitter::Slot().Value(0.45f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(3.0f)
				[
					CreateToolbarContentBrowser()
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(3.0f)
				[
					SNew(SSeparator).Thickness(5.0f)
				]
				+ SVerticalBox::Slot().FillHeight(1.0f).Padding(5.0f)
				[
					UPjcSubsystem::GetModuleContentBrowser().Get().CreateAssetPicker(AssetPickerConfig)
				]
			]
		]
	];
}

TSharedRef<SWidget> SPjcTabAssetsUnused::CreateToolbarMain() const
{
	FToolBarBuilder ToolBarBuilder{Cmds, FMultiBoxCustomization::None};
	ToolBarBuilder.BeginSection("PjcSectionMain");
	{
		ToolBarBuilder.AddToolBarButton(FPjcCmds::Get().ScanProject);
		ToolBarBuilder.AddToolBarButton(FPjcCmds::Get().CleanProject);
		ToolBarBuilder.AddSeparator();
		ToolBarBuilder.AddToolBarButton(FPjcCmds::Get().ClearExcludeSettings);
	}
	ToolBarBuilder.EndSection();

	return ToolBarBuilder.MakeWidget();
}

TSharedRef<SWidget> SPjcTabAssetsUnused::CreateToolbarTreeView() const
{
	FToolBarBuilder ToolBarBuilder{Cmds, FMultiBoxCustomization::None};
	ToolBarBuilder.BeginSection("PjcSectionTreeView");
	{
		ToolBarBuilder.AddToolBarButton(FPjcCmds::Get().DeleteEmptyFolders);
		ToolBarBuilder.AddSeparator();
		ToolBarBuilder.AddToolBarButton(FPjcCmds::Get().PathsExclude);
		ToolBarBuilder.AddToolBarButton(FPjcCmds::Get().PathsInclude);
		ToolBarBuilder.AddSeparator();
		ToolBarBuilder.AddToolBarButton(FPjcCmds::Get().ClearSelection);
	}
	ToolBarBuilder.EndSection();

	return ToolBarBuilder.MakeWidget();
}

TSharedRef<SWidget> SPjcTabAssetsUnused::CreateToolbarContentBrowser() const
{
	FToolBarBuilder ToolBarBuilder{Cmds, FMultiBoxCustomization::None};
	ToolBarBuilder.BeginSection("PjcSectionContentBrowser");
	{
		ToolBarBuilder.AddToolBarButton(FPjcCmds::Get().AssetsDelete);
		ToolBarBuilder.AddSeparator();
		ToolBarBuilder.AddToolBarButton(FPjcCmds::Get().AssetsExclude);
		ToolBarBuilder.AddToolBarButton(FPjcCmds::Get().AssetsExcludeByClass);
		ToolBarBuilder.AddToolBarButton(FPjcCmds::Get().AssetsInclude);
		ToolBarBuilder.AddToolBarButton(FPjcCmds::Get().AssetsIncludeByClass);
		ToolBarBuilder.AddSeparator();
		ToolBarBuilder.AddToolBarButton(FPjcCmds::Get().OpenViewerSizeMap);
		ToolBarBuilder.AddToolBarButton(FPjcCmds::Get().OpenViewerReference);
		ToolBarBuilder.AddToolBarButton(FPjcCmds::Get().OpenViewerAssetsAudit);
	}
	ToolBarBuilder.EndSection();

	return ToolBarBuilder.MakeWidget();
}

void SPjcTabAssetsUnused::OnProjectScan()
{
	ScanProject();
}

void SPjcTabAssetsUnused::OnProjectClean()
{
	const FText Title = FText::FromString(TEXT("Project Cleanup"));
	const FText Context = FText::FromString(TEXT("Are you sure you want to delete all unused assets and empty folders in project?"));

	const EAppReturnType::Type ReturnType = FMessageDialog::Open(EAppMsgType::YesNo, Context, &Title);
	if (ReturnType == EAppReturnType::Cancel || ReturnType == EAppReturnType::No) return;

	UPjcSubsystem::DeleteAssetsUnused(true, true);
	UPjcSubsystem::DeleteFoldersEmpty(true, true);

	ScanProject();
}

void SPjcTabAssetsUnused::OnResetExcludeSettings()
{
	UPjcAssetExcludeSettings* AssetExcludeSettings = GetMutableDefault<UPjcAssetExcludeSettings>();
	if (!AssetExcludeSettings) return;

	AssetExcludeSettings->ExcludedAssets.Empty();
	AssetExcludeSettings->ExcludedClasses.Empty();
	AssetExcludeSettings->ExcludedFolders.Empty();
	AssetExcludeSettings->PostEditChange();

	ScanProject();
}

void SPjcTabAssetsUnused::OnDeleteEmptyFolders()
{
	const FText Title = FText::FromString(TEXT("Delete Empty Folder"));
	const FText Context = FText::FromString(TEXT("Are you sure you want to delete all empty folders?"));

	const EAppReturnType::Type ReturnType = FMessageDialog::Open(EAppMsgType::YesNo, Context, &Title);
	if (ReturnType == EAppReturnType::Cancel || ReturnType == EAppReturnType::No) return;

	UPjcSubsystem::DeleteFoldersEmpty(true, true);

	UpdateTreeView();
}

void SPjcTabAssetsUnused::OnPathExclude()
{
	UPjcAssetExcludeSettings* AssetExcludeSettings = GetMutableDefault<UPjcAssetExcludeSettings>();
	if (!AssetExcludeSettings) return;

	const auto& SelectedItems = TreeListView->GetSelectedItems();

	for (const auto& SelectedItem : SelectedItems)
	{
		const bool bAlreadyInList = AssetExcludeSettings->ExcludedFolders.ContainsByPredicate([&](const FDirectoryPath& InDirPath)
		{
			return InDirPath.Path.Equals(SelectedItem->FolderPath);
		});

		if (!bAlreadyInList)
		{
			AssetExcludeSettings->ExcludedFolders.Emplace(FDirectoryPath{SelectedItem->FolderPath});
		}
	}

	AssetExcludeSettings->PostEditChange();

	ScanProject();
}

void SPjcTabAssetsUnused::OnPathInclude()
{
	UPjcAssetExcludeSettings* AssetExcludeSettings = GetMutableDefault<UPjcAssetExcludeSettings>();
	if (!AssetExcludeSettings) return;

	const auto& SelectedItems = TreeListView->GetSelectedItems();

	for (const auto& SelectedItem : SelectedItems)
	{
		AssetExcludeSettings->ExcludedFolders.RemoveAllSwap([&](const FDirectoryPath& InDirPath)
		{
			return InDirPath.Path.Equals(SelectedItem->FolderPath);
		});
	}

	AssetExcludeSettings->PostEditChange();

	ScanProject();
}

void SPjcTabAssetsUnused::OnClearSelection() const
{
	if (!TreeListView.IsValid()) return;

	TreeListView->ClearSelection();
	TreeListView->ClearHighlightedItems();
}

void SPjcTabAssetsUnused::ScanProject()
{
	if (UPjcSubsystem::GetModuleAssetRegistry().Get().IsLoadingAssets())
	{
		UPjcSubsystem::ShowNotificationWithOutputLog(TEXT("Failed to scan project. AssetRegistry is still discovering assets. Please try again after it has finished."), SNotificationItem::CS_Fail, 5.0f);
		return;
	}

	if (GEditor && !GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->CloseAllAssetEditors())
	{
		UPjcSubsystem::ShowNotificationWithOutputLog(TEXT("Failed to scan project, because not all asset editors are closed."), SNotificationItem::CS_Fail, 5.0f);
		return;
	}

	TArray<FAssetData> Redirectors;
	UPjcSubsystem::GetProjectRedirectors(Redirectors);
	UPjcSubsystem::FixProjectRedirectors(Redirectors, true);

	if (UPjcSubsystem::ProjectHasRedirectors())
	{
		UPjcSubsystem::ShowNotificationWithOutputLog(TEXT("Failed to scan project, because not all redirectors are fixed."), SNotificationItem::CS_Fail, 5.0f);
		return;
	}

	if (!FEditorFileUtils::SaveDirtyPackages(true, true, true, false, false, false))
	{
		UPjcSubsystem::ShowNotificationWithOutputLog(TEXT("Failed to scan project, because not all assets have been saved."), SNotificationItem::CS_Fail, 5.0f);
		return;
	}

	const double ScanStartTime = FPlatformTime::Seconds();

	FScopedSlowTask SlowTaskMain{
		4.0f,
		FText::FromString(TEXT("Scanning project assets...")),
		GIsEditor && !IsRunningCommandlet()
	};
	SlowTaskMain.MakeDialog(false, false);

	SlowTaskMain.EnterProgressFrame(1.0f);

	ResetCachedData();

	SlowTaskMain.EnterProgressFrame(1.0f);

	TArray<FString> FoldersTotal;
	TArray<FString> FoldersEmpty;
	TArray<FPjcAssetIndirectInfo> AssetIndirectInfos;
	UPjcSubsystem::GetAssetsAll(AssetsAll);
	UPjcSubsystem::GetAssetsUsed(AssetsUsed);
	UPjcSubsystem::GetAssetsUnused(AssetsUnused);
	UPjcSubsystem::GetAssetsPrimary(AssetsPrimary);
	UPjcSubsystem::GetAssetsIndirect(AssetsIndirect, AssetIndirectInfos);
	UPjcSubsystem::GetAssetsCircular(AssetsCircular);
	UPjcSubsystem::GetAssetsEditor(AssetsEditor);
	UPjcSubsystem::GetAssetsExcluded(AssetsExcluded);
	UPjcSubsystem::GetAssetsExtReferenced(AssetsExtReferenced);
	UPjcSubsystem::GetFolders(FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir()), true, FoldersTotal);
	UPjcSubsystem::GetFoldersEmpty(FoldersEmpty);

	// todo:ashe23 make sure we update content browser when there is no assets if filter enabled
	FilterExcluded->ActiveStateChanged(true);

	for (const FAssetData& Asset : AssetsAll)
	{
		const FString AssetPath = Asset.PackagePath.ToString();
		const int64 AssetSize = UPjcSubsystem::GetAssetSize(Asset);
		UpdateMapInfo(MapNumAssetsAllByPath, MapSizeAssetsAllByPath, AssetPath, AssetSize);
	}

	for (const FAssetData& Asset : AssetsUsed)
	{
		const FString AssetPath = Asset.PackagePath.ToString();
		const int64 AssetSize = UPjcSubsystem::GetAssetSize(Asset);
		UpdateMapInfo(MapNumAssetsUsedByPath, MapSizeAssetsUsedByPath, AssetPath, AssetSize);
	}

	for (const FAssetData& Asset : AssetsUnused)
	{
		const FString AssetPath = Asset.PackagePath.ToString();
		const int64 AssetSize = UPjcSubsystem::GetAssetSize(Asset);
		UpdateMapInfo(MapNumAssetsUnusedByPath, MapSizeAssetsUnusedByPath, AssetPath, AssetSize);
	}

	SlowTaskMain.EnterProgressFrame(1.0f);

	NumAssetsAll = AssetsAll.Num();
	NumAssetsUsed = AssetsUsed.Num();
	NumAssetsUnused = AssetsUnused.Num();
	NumAssetsPrimary = AssetsPrimary.Num();
	NumAssetsIndirect = AssetsIndirect.Num();
	NumAssetsEditor = AssetsEditor.Num();
	NumAssetsExcluded = AssetsExcluded.Num();
	NumAssetsExtReferenced = AssetsExtReferenced.Num();
	NumFoldersTotal = FoldersTotal.Num();
	NumFoldersEmpty = FoldersEmpty.Num();

	SizeAssetsAll = UPjcSubsystem::GetAssetsTotalSize(AssetsAll);
	SizeAssetsUsed = UPjcSubsystem::GetAssetsTotalSize(AssetsUsed);
	SizeAssetsUnused = UPjcSubsystem::GetAssetsTotalSize(AssetsUnused);
	SizeAssetsPrimary = UPjcSubsystem::GetAssetsTotalSize(AssetsPrimary);
	SizeAssetsIndirect = UPjcSubsystem::GetAssetsTotalSize(AssetsIndirect);
	SizeAssetsEditor = UPjcSubsystem::GetAssetsTotalSize(AssetsEditor);
	SizeAssetsExcluded = UPjcSubsystem::GetAssetsTotalSize(AssetsExcluded);
	SizeAssetsExtReferenced = UPjcSubsystem::GetAssetsTotalSize(AssetsExtReferenced);

	const double ScanTime = FPlatformTime::Seconds() - ScanStartTime;

	UE_LOG(LogProjectCleaner, Display, TEXT("Project assets scanned in %.2f seconds."), ScanTime);

	SlowTaskMain.EnterProgressFrame(1.0f);

	UpdateStats();
	UpdateTreeView();
	UpdateContentBrowser();
}

void SPjcTabAssetsUnused::UpdateStats()
{
	StatsListItems.Reset();

	const FMargin FirstLvl{5.0f, 0.0f, 0.0f, 0.0f};
	const FMargin SecondLvl{20.0f, 0.0f, 0.0f, 0.0f};
	const auto ColorRed = FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Red").GetSpecifiedColor();
	const auto ColorYellow = FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Yellow").GetSpecifiedColor();

	StatsListItems.Emplace(
		MakeShareable(
			new FPjcStatItem{
				FText::FromString(TEXT("Unused")),
				FText::AsNumber(NumAssetsUnused),
				FText::AsMemory(SizeAssetsUnused, IEC),
				FText::FromString(TEXT("Unused Assets")),
				FText::FromString(TEXT("Total number of unused assets")),
				FText::FromString(TEXT("Total size of unused assets")),
				NumAssetsUnused > 0 ? ColorRed : FLinearColor::White,
				FirstLvl
			}
		)
	);

	StatsListItems.Emplace(
		MakeShareable(
			new FPjcStatItem{
				FText::FromString(TEXT("Used")),
				FText::AsNumber(NumAssetsUsed),
				FText::AsMemory(SizeAssetsUsed, IEC),
				FText::FromString(TEXT("Used Assets")),
				FText::FromString(TEXT("Total number of used assets")),
				FText::FromString(TEXT("Total size of used assets")),
				FLinearColor::White,
				FirstLvl
			}
		)
	);

	StatsListItems.Emplace(
		MakeShareable(
			new FPjcStatItem{
				FText::FromString(TEXT("Primary")),
				FText::AsNumber(NumAssetsPrimary),
				FText::AsMemory(SizeAssetsPrimary, IEC),
				FText::FromString(TEXT("Primary Assets that defined in AssetManager. Level assets are primary by default.")),
				FText::FromString(TEXT("Total number of primary assets")),
				FText::FromString(TEXT("Total size of primary assets")),
				FLinearColor::White,
				SecondLvl
			}
		)
	);

	StatsListItems.Emplace(
		MakeShareable(
			new FPjcStatItem{
				FText::FromString(TEXT("Editor")),
				FText::AsNumber(NumAssetsEditor),
				FText::AsMemory(SizeAssetsEditor, IEC),
				FText::FromString(TEXT("Editor specific assets. Like EditorUtilitWidgets or EditorTutorial assets.")),
				FText::FromString(TEXT("Total number of Editor assets")),
				FText::FromString(TEXT("Total size of Editor assets")),
				FLinearColor::White,
				SecondLvl
			}
		)
	);

	StatsListItems.Emplace(
		MakeShareable(
			new FPjcStatItem{
				FText::FromString(TEXT("Indirect")),
				FText::AsNumber(NumAssetsIndirect),
				FText::AsMemory(SizeAssetsIndirect, IEC),
				FText::FromString(TEXT("Assets that used in source code or config files.")),
				FText::FromString(TEXT("Total number of Indirect assets")),
				FText::FromString(TEXT("Total size of Indirect assets")),
				FLinearColor::White,
				SecondLvl
			}
		)
	);

	StatsListItems.Emplace(
		MakeShareable(
			new FPjcStatItem{
				FText::FromString(TEXT("ExtReferenced")),
				FText::AsNumber(NumAssetsExtReferenced),
				FText::AsMemory(SizeAssetsExtReferenced, IEC),
				FText::FromString(TEXT("Assets that have external referencers outside Content folder.")),
				FText::FromString(TEXT("Total number of ExtReferenced assets")),
				FText::FromString(TEXT("Total size of ExtReferenced assets")),
				FLinearColor::White,
				SecondLvl
			}
		)
	);

	StatsListItems.Emplace(
		MakeShareable(
			new FPjcStatItem{
				FText::FromString(TEXT("Excluded")),
				FText::AsNumber(NumAssetsExcluded),
				FText::AsMemory(SizeAssetsExcluded, IEC),
				FText::FromString(TEXT("Excluded Assets")),
				FText::FromString(TEXT("Total number of Excluded assets")),
				FText::FromString(TEXT("Total size of Excluded assets")),
				NumAssetsExcluded > 0 ? ColorYellow : FLinearColor::White,
				SecondLvl
			}
		)
	);

	StatsListItems.Emplace(
		MakeShareable(
			new FPjcStatItem{
				FText::FromString(TEXT("Total")),
				FText::AsNumber(NumAssetsAll),
				FText::AsMemory(SizeAssetsAll, IEC),
				FText::FromString(TEXT("All Assets")),
				FText::FromString(TEXT("Total number of assets")),
				FText::FromString(TEXT("Total size of assets")),
				FLinearColor::White,
				FirstLvl
			}
		)
	);

	if (StatsListView.IsValid())
	{
		StatsListView->RebuildList();
	}
}

void SPjcTabAssetsUnused::UpdateTreeView()
{
	if (!TreeListView.IsValid()) return;

	const FString PathContentDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir());

	RootItem.Reset();
	RootItem = MakeShareable(new FPjcTreeItem);
	if (!RootItem.IsValid()) return;

	TSet<TSharedPtr<FPjcTreeItem>> CachedExpandedItems;
	TreeListView->GetExpandedItems(CachedExpandedItems);

	RootItem->FolderPath = PjcConstants::PathRoot.ToString();
	RootItem->FolderName = TEXT("Content");
	RootItem->bIsDev = false;
	RootItem->bIsRoot = true;
	RootItem->bIsEmpty = UPjcSubsystem::FolderIsEmpty(PathContentDir);
	RootItem->bIsExcluded = UPjcSubsystem::FolderIsExcluded(PathContentDir);
	RootItem->bIsExpanded = true;
	RootItem->bIsVisible = true;
	RootItem->NumAssetsTotal = AssetsAll.Num();
	RootItem->NumAssetsUsed = AssetsUsed.Num();
	RootItem->NumAssetsUnused = AssetsUnused.Num();
	RootItem->SizeAssetsUnused = UPjcSubsystem::GetAssetsTotalSize(AssetsUnused);
	RootItem->PercentageUnused = RootItem->NumAssetsTotal == 0 ? 0 : RootItem->NumAssetsUnused * 100.0f / RootItem->NumAssetsTotal;
	RootItem->PercentageUnusedNormalized = FMath::GetMappedRangeValueClamped(FVector2D{0.0f, 100.0f}, FVector2D{0.0f, 1.0f}, RootItem->PercentageUnused);
	RootItem->Parent = nullptr;

	// filling whole tree
	TArray<TSharedPtr<FPjcTreeItem>> Stack;
	Stack.Push(RootItem);

	while (Stack.Num() > 0)
	{
		const auto CurrentItem = Stack.Pop(false);

		TreeListView->SetItemExpansion(CurrentItem, CurrentItem->bIsExpanded);
		
		TArray<FString> SubPaths;
		UPjcSubsystem::GetModuleAssetRegistry().Get().GetSubPaths(CurrentItem->FolderPath, SubPaths, false);

		for (const auto& SubPath : SubPaths)
		{
			const TSharedPtr<FPjcTreeItem> SubItem = MakeShareable(new FPjcTreeItem);
			if (!SubItem.IsValid()) continue;

			SubItem->FolderPath = SubPath;
			SubItem->FolderName = FPaths::GetPathLeaf(SubItem->FolderPath);
			SubItem->bIsDev = SubItem->FolderPath.StartsWith(PjcConstants::PathDevelopers.ToString());
			SubItem->bIsRoot = false;
			SubItem->bIsEmpty = UPjcSubsystem::FolderIsEmpty(SubItem->FolderPath);
			SubItem->bIsExcluded = UPjcSubsystem::FolderIsExcluded(SubItem->FolderPath);
			SubItem->bIsExpanded = TreeItemIsExpanded(SubItem, CachedExpandedItems);
			SubItem->bIsVisible = TreeItemIsVisible(SubItem);
			SubItem->NumAssetsTotal = MapNumAssetsAllByPath.Contains(SubItem->FolderPath) ? MapNumAssetsAllByPath[SubItem->FolderPath] : 0;
			SubItem->NumAssetsUsed = MapNumAssetsUsedByPath.Contains(SubItem->FolderPath) ? MapNumAssetsUsedByPath[SubItem->FolderPath] : 0;
			SubItem->NumAssetsUnused = MapNumAssetsUnusedByPath.Contains(SubItem->FolderPath) ? MapNumAssetsUnusedByPath[SubItem->FolderPath] : 0;
			SubItem->SizeAssetsUnused = MapSizeAssetsUnusedByPath.Contains(SubItem->FolderPath) ? MapSizeAssetsUnusedByPath[SubItem->FolderPath] : 0.0f;
			SubItem->PercentageUnused = SubItem->NumAssetsTotal == 0 ? 0 : SubItem->NumAssetsUnused * 100.0f / SubItem->NumAssetsTotal;
			SubItem->PercentageUnusedNormalized = FMath::GetMappedRangeValueClamped(FVector2D{0.0f, 100.0f}, FVector2D{0.0f, 1.0f}, SubItem->PercentageUnused);
			SubItem->Parent = CurrentItem;

			CurrentItem->SubItems.Emplace(SubItem);
			Stack.Emplace(SubItem);
		}
	}

	SortTreeItems(false);
	
	TreeListItems.Reset();
	TreeListItems.Emplace(RootItem);
	TreeListView->RebuildList();
}

void SPjcTabAssetsUnused::UpdateContentBrowser()
{
	Filter.Clear();

	if (SelectedPaths.Num() > 0)
	{
		Filter.bRecursivePaths = true;

		for (const auto& SelectedPath : SelectedPaths)
		{
			Filter.PackagePaths.Emplace(SelectedPath);
		}
	}

	if (AnyFilterActive())
	{
		if (bFilterAssetsUsedActive)
		{
			Filter.ObjectPaths.Reserve(Filter.ObjectPaths.Num() + AssetsUsed.Num());

			for (const auto& Asset : AssetsUsed)
			{
				Filter.ObjectPaths.Emplace(Asset.ToSoftObjectPath().GetAssetPathName());
			}
		}

		if (bFilterAssetsPrimaryActive)
		{
			Filter.ObjectPaths.Reserve(Filter.ObjectPaths.Num() + AssetsPrimary.Num());

			for (const auto& Asset : AssetsPrimary)
			{
				Filter.ObjectPaths.Emplace(Asset.ToSoftObjectPath().GetAssetPathName());
			}
		}

		if (bFilterAssetsEditorActive)
		{
			Filter.ObjectPaths.Reserve(Filter.ObjectPaths.Num() + AssetsEditor.Num());

			for (const auto& Asset : AssetsEditor)
			{
				Filter.ObjectPaths.Emplace(Asset.ToSoftObjectPath().GetAssetPathName());
			}
		}

		if (bFilterAssetsIndirectActive)
		{
			Filter.ObjectPaths.Reserve(Filter.ObjectPaths.Num() + AssetsIndirect.Num());

			for (const auto& Asset : AssetsIndirect)
			{
				Filter.ObjectPaths.Emplace(Asset.ToSoftObjectPath().GetAssetPathName());
			}
		}

		if (bFilterAssetsExcludedActive)
		{
			Filter.ObjectPaths.Reserve(Filter.ObjectPaths.Num() + AssetsExcluded.Num());

			for (const auto& Asset : AssetsExcluded)
			{
				Filter.ObjectPaths.Emplace(Asset.ToSoftObjectPath().GetAssetPathName());
			}
		}

		if (bFilterAssetsExtReferencedActive)
		{
			Filter.ObjectPaths.Reserve(Filter.ObjectPaths.Num() + AssetsExtReferenced.Num());

			for (const auto& Asset : AssetsExtReferenced)
			{
				Filter.ObjectPaths.Emplace(Asset.ToSoftObjectPath().GetAssetPathName());
			}
		}

		if (bFilterAssetsCircularActive)
		{
			Filter.ObjectPaths.Reserve(Filter.ObjectPaths.Num() + AssetsCircular.Num());

			for (const auto& Asset : AssetsCircular)
			{
				Filter.ObjectPaths.Emplace(Asset.ToSoftObjectPath().GetAssetPathName());
			}
		}

		DelegateFilter.Execute(Filter);

		return;
	}

	if (AssetsUnused.Num() > 0 && bFilterAssetsUnusedActive)
	{
		Filter.ObjectPaths.Reserve(AssetsUnused.Num());

		for (const auto& Asset : AssetsUnused)
		{
			Filter.ObjectPaths.Emplace(Asset.ToSoftObjectPath().GetAssetPathName());
		}
	}
	else
	{
		Filter.TagsAndValues.Emplace(PjcConstants::EmptyTagName, PjcConstants::EmptyTagName.ToString());
	}

	DelegateFilter.Execute(Filter);
}

void SPjcTabAssetsUnused::OnTreeGetChildren(TSharedPtr<FPjcTreeItem> Item, TArray<TSharedPtr<FPjcTreeItem>>& OutChildren)
{
	if (!Item.IsValid()) return;

	for (const auto& SubItem : Item->SubItems)
	{
		if (SubItem->bIsVisible)
		{
			OutChildren.Add(SubItem);
		}
	}
}

void SPjcTabAssetsUnused::OnTreeSearchTextChanged(const FText& InText)
{
	TreeSearchText = InText;
	UpdateTreeView();
}

void SPjcTabAssetsUnused::OnTreeSearchTextCommitted(const FText& InText, ETextCommit::Type Type)
{
	TreeSearchText = InText;
	UpdateTreeView();
}

void SPjcTabAssetsUnused::OnTreeSort(EColumnSortPriority::Type SortPriority, const FName& ColumnName, EColumnSortMode::Type InSortMode)
{
	if (!RootItem.IsValid() || !TreeListView.IsValid()) return;

	LastSortedColumn = ColumnName;

	SortTreeItems(true);

	TreeListView->RebuildList();
}

void SPjcTabAssetsUnused::OnTreeSelectionChanged(TSharedPtr<FPjcTreeItem> Selection, ESelectInfo::Type SelectInfo)
{
	if (!TreeListView.IsValid()) return;

	const auto& SelectedItems = TreeListView->GetSelectedItems();

	SelectedPaths.Reset();
	SelectedPaths.Reserve(SelectedItems.Num());

	for (const auto& SelectedItem : SelectedItems)
	{
		SelectedPaths.Emplace(FName{*SelectedItem->FolderPath});
	}
}

void SPjcTabAssetsUnused::OnTreeExpansionChanged(TSharedPtr<FPjcTreeItem> Item, bool bIsExpanded)
{
	if (!Item.IsValid() || !TreeListView.IsValid()) return;

	Item->bIsExpanded = bIsExpanded;
	TreeListView->SetItemExpansion(Item, Item->bIsExpanded);
	TreeListView->RebuildList();
}

void SPjcTabAssetsUnused::SortTreeItems(const bool UpdateSortingOrder)
{
	auto SortTreeItems = [&](auto& SortMode, auto SortFunc)
	{
		if (UpdateSortingOrder)
		{
			SortMode = SortMode == EColumnSortMode::Ascending ? EColumnSortMode::Descending : EColumnSortMode::Ascending;
		}

		TArray<TSharedPtr<FPjcTreeItem>> Stack;
		Stack.Push(RootItem);

		while (Stack.Num() > 0)
		{
			const auto& CurrentItem = Stack.Pop(false);
			if (!CurrentItem.IsValid()) continue;

			TArray<TSharedPtr<FPjcTreeItem>>& SubItems = CurrentItem->SubItems;
			SubItems.Sort(SortFunc);

			Stack.Append(CurrentItem->SubItems);
		}
	};

	if (LastSortedColumn.IsEqual(TEXT("Path")))
	{
		SortTreeItems(ColumnPathSortMode, [&](const TSharedPtr<FPjcTreeItem>& Item1, const TSharedPtr<FPjcTreeItem>& Item2)
		{
			return ColumnPathSortMode == EColumnSortMode::Ascending ? Item1->FolderPath < Item2->FolderPath : Item1->FolderPath > Item2->FolderPath;
		});
	}

	if (LastSortedColumn.IsEqual(TEXT("NumAssetsTotal")))
	{
		SortTreeItems(ColumnAssetsTotalSortMode, [&](const TSharedPtr<FPjcTreeItem>& Item1, const TSharedPtr<FPjcTreeItem>& Item2)
		{
			return ColumnAssetsTotalSortMode == EColumnSortMode::Ascending ? Item1->NumAssetsTotal < Item2->NumAssetsTotal : Item1->NumAssetsTotal > Item2->NumAssetsTotal;
		});
	}

	if (LastSortedColumn.IsEqual(TEXT("NumAssetsUsed")))
	{
		SortTreeItems(ColumnAssetsUsedSortMode, [&](const TSharedPtr<FPjcTreeItem>& Item1, const TSharedPtr<FPjcTreeItem>& Item2)
		{
			return ColumnAssetsUsedSortMode == EColumnSortMode::Ascending ? Item1->NumAssetsUsed < Item2->NumAssetsUsed : Item1->NumAssetsUsed > Item2->NumAssetsUsed;
		});
	}

	if (LastSortedColumn.IsEqual(TEXT("NumAssetsUnused")))
	{
		SortTreeItems(ColumnAssetsUnusedSortMode, [&](const TSharedPtr<FPjcTreeItem>& Item1, const TSharedPtr<FPjcTreeItem>& Item2)
		{
			return ColumnAssetsUnusedSortMode == EColumnSortMode::Ascending ? Item1->NumAssetsUnused < Item2->NumAssetsUnused : Item1->NumAssetsUnused > Item2->NumAssetsUnused;
		});
	}

	if (LastSortedColumn.IsEqual(TEXT("UnusedPercent")))
	{
		SortTreeItems(ColumnUnusedPercentSortMode, [&](const TSharedPtr<FPjcTreeItem>& Item1, const TSharedPtr<FPjcTreeItem>& Item2)
		{
			return ColumnUnusedPercentSortMode == EColumnSortMode::Ascending ? Item1->PercentageUnused < Item2->PercentageUnused : Item1->PercentageUnused > Item2->PercentageUnused;
		});
	}

	if (LastSortedColumn.IsEqual(TEXT("UnusedSize")))
	{
		SortTreeItems(ColumnUnusedSizeSortMode, [&](const TSharedPtr<FPjcTreeItem>& Item1, const TSharedPtr<FPjcTreeItem>& Item2)
		{
			return ColumnUnusedSizeSortMode == EColumnSortMode::Ascending ? Item1->SizeAssetsUnused < Item2->SizeAssetsUnused : Item1->SizeAssetsUnused > Item2->SizeAssetsUnused;
		});
	}
}

void SPjcTabAssetsUnused::ChangeItemExpansionRecursive(const TSharedPtr<FPjcTreeItem>& Item, const bool bExpansion) const
{
	if (!Item.IsValid() || !TreeListView.IsValid()) return;

	TArray<TSharedPtr<FPjcTreeItem>> Stack;
	Stack.Push(Item);

	while (Stack.Num() > 0)
	{
		const auto CurrentItem = Stack.Pop(false);

		CurrentItem->bIsExpanded = bExpansion;
		TreeListView->SetItemExpansion(CurrentItem, CurrentItem->bIsExpanded);

		for (const auto& SubItem : CurrentItem->SubItems)
		{
			Stack.Push(SubItem);
		}
	}

	TreeListView->RebuildList();
}

bool SPjcTabAssetsUnused::TreeItemIsVisible(const TSharedPtr<FPjcTreeItem>& Item) const
{
	if (!SubsystemPtr) return false;

	if (Item->bIsDev && !SubsystemPtr->bShowFoldersEngine)
	{
		return false;
	}

	if (Item->bIsExcluded && !SubsystemPtr->bShowFoldersExcluded)
	{
		return false;
	}

	if (Item->bIsEmpty && !Item->bIsDev && !Item->bIsExcluded && !SubsystemPtr->bShowFoldersEmpty)
	{
		return false;
	}

	if (Item->NumAssetsUsed > 0 && Item->NumAssetsUnused == 0 && !Item->bIsExcluded && !SubsystemPtr->bShowFoldersUsed)
	{
		return false;
	}

	// todo:ashe23 fix this search part
	// if (!TreeSearchText.IsEmpty())
	// {
	// 	if (TreeItemContainsSearchText(CurrentItem))
	// 	{
	// 		TSharedPtr<FPjcTreeItem> Item = CurrentItem;
	// 		while (Item.IsValid())
	// 		{
	// 			Item->bIsVisible = true;
	// 			Item = Item->Parent;
	// 		}
	// 	}
	// }

	return true;
}

bool SPjcTabAssetsUnused::TreeItemIsExpanded(const TSharedPtr<FPjcTreeItem>& Item, const TSet<TSharedPtr<FPjcTreeItem>>& CachedItems) const
{
	if (!TreeSearchText.IsEmpty() && TreeItemContainsSearchText(Item))
	{
		auto CurrentItem = Item;
		while (CurrentItem.IsValid())
		{
			CurrentItem->bIsExpanded = true;
			TreeListView->SetItemExpansion(CurrentItem, CurrentItem->bIsExpanded);
			CurrentItem = CurrentItem->Parent;
		}

		return true;
	}

	for (const auto& ExpandedItem : CachedItems)
	{
		if (!ExpandedItem.IsValid()) continue;

		if (ExpandedItem->FolderPath.Equals(Item->FolderPath))
		{
			return true;
		}
	}

	return false;
}

bool SPjcTabAssetsUnused::TreeItemContainsSearchText(const TSharedPtr<FPjcTreeItem>& Item) const
{
	TArray<FString> SubPaths;
	UPjcSubsystem::GetModuleAssetRegistry().Get().GetSubPaths(Item->FolderPath, SubPaths, true);

	for (const auto& Path : SubPaths)
	{
		if (Path.Contains(TreeSearchText.ToString()))
		{
			return true;
		}
	}

	return false;
}

bool SPjcTabAssetsUnused::TreeHasSelection() const
{
	return TreeListView.IsValid() && TreeListView->GetSelectedItems().Num() > 0;
}

bool SPjcTabAssetsUnused::CanCleanProject() const
{
	return NumAssetsUnused > 0 || NumFoldersEmpty > 0;
}

bool SPjcTabAssetsUnused::CanDeleteEmptyFolders() const
{
	return NumFoldersEmpty > 0;
}

bool SPjcTabAssetsUnused::AnyFilterActive() const
{
	return
		bFilterAssetsUsedActive ||
		bFilterAssetsPrimaryActive ||
		bFilterAssetsEditorActive ||
		bFilterAssetsIndirectActive ||
		bFilterAssetsExcludedActive ||
		bFilterAssetsExtReferencedActive;
}

FText SPjcTabAssetsUnused::GetTreeSummaryText() const
{
	return FText::FromString(FString::Printf(TEXT("Folders Total - %d"), NumFoldersTotal));
}

FText SPjcTabAssetsUnused::GetTreeSelectionText() const
{
	const int32 NumItemsSelected = TreeListView.IsValid() ? TreeListView->GetSelectedItems().Num() : 0;

	if (NumItemsSelected > 0)
	{
		return FText::FromString(FString::Printf(TEXT("%d selected"), NumItemsSelected));
	}

	return FText::GetEmpty();
}

FSlateColor SPjcTabAssetsUnused::GetTreeOptionsBtnForegroundColor() const
{
	static const FName InvertedForegroundName("InvertedForeground");
	static const FName DefaultForegroundName("DefaultForeground");

	if (!TreeOptionBtn.IsValid()) return FEditorStyle::GetSlateColor(DefaultForegroundName);

	return TreeOptionBtn->IsHovered() ? FEditorStyle::GetSlateColor(InvertedForegroundName) : FEditorStyle::GetSlateColor(DefaultForegroundName);
}

TSharedRef<SWidget> SPjcTabAssetsUnused::GetTreeBtnOptionsContent()
{
	const TSharedPtr<FExtender> Extender;
	FMenuBuilder MenuBuilder(true, Cmds, Extender, true);

	MenuBuilder.AddMenuSeparator(TEXT("View"));

	MenuBuilder.AddMenuEntry(
		FText::FromString(TEXT("Show Folders Empty")),
		FText::FromString(TEXT("Show empty folders in tree view")),
		FSlateIcon(),
		FUIAction
		(
			FExecuteAction::CreateLambda([&]
			{
				SubsystemPtr->bShowFoldersEmpty = !SubsystemPtr->bShowFoldersEmpty;
				SubsystemPtr->PostEditChange();
				UpdateTreeView();
			}),
			FCanExecuteAction::CreateLambda([&]()
			{
				return SubsystemPtr != nullptr;
			}),
			FGetActionCheckState::CreateLambda([&]()
			{
				return SubsystemPtr->bShowFoldersEmpty ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
			})
		),
		NAME_None,
		EUserInterfaceActionType::ToggleButton
	);

	MenuBuilder.AddMenuEntry(
		FText::FromString(TEXT("Show Folders Excluded")),
		FText::FromString(TEXT("Show excluded folders in tree view")),
		FSlateIcon(),
		FUIAction
		(
			FExecuteAction::CreateLambda([&]
			{
				SubsystemPtr->bShowFoldersExcluded = !SubsystemPtr->bShowFoldersExcluded;
				SubsystemPtr->PostEditChange();
				UpdateTreeView();
			}),
			FCanExecuteAction::CreateLambda([&]()
			{
				return SubsystemPtr != nullptr;
			}),
			FGetActionCheckState::CreateLambda([&]()
			{
				return SubsystemPtr->bShowFoldersExcluded ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
			})
		),
		NAME_None,
		EUserInterfaceActionType::ToggleButton
	);

	MenuBuilder.AddMenuEntry(
		FText::FromString(TEXT("Show Folders Used")),
		FText::FromString(TEXT("Show folders that dont contain unused assets in tree view")),
		FSlateIcon(),
		FUIAction
		(
			FExecuteAction::CreateLambda([&]
			{
				SubsystemPtr->bShowFoldersUsed = !SubsystemPtr->bShowFoldersUsed;
				SubsystemPtr->PostEditChange();
				UpdateTreeView();
			}),
			FCanExecuteAction::CreateLambda([&]()
			{
				return SubsystemPtr != nullptr;
			}),
			FGetActionCheckState::CreateLambda([&]()
			{
				return SubsystemPtr->bShowFoldersUsed ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
			})
		),
		NAME_None,
		EUserInterfaceActionType::ToggleButton
	);

	MenuBuilder.AddMenuEntry(
		FText::FromString(TEXT("Show Folders Engine Generated")),
		FText::FromString(TEXT("Show engine generated folders in tree view")),
		FSlateIcon(),
		FUIAction
		(
			FExecuteAction::CreateLambda([&]
			{
				SubsystemPtr->bShowFoldersEngine = !SubsystemPtr->bShowFoldersEngine;
				SubsystemPtr->PostEditChange();
				UpdateTreeView();
			}),
			FCanExecuteAction::CreateLambda([&]()
			{
				return SubsystemPtr != nullptr;
			}),
			FGetActionCheckState::CreateLambda([&]()
			{
				return SubsystemPtr->bShowFoldersEngine ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
			})
		),
		NAME_None,
		EUserInterfaceActionType::ToggleButton
	);

	MenuBuilder.AddSeparator();

	MenuBuilder.AddMenuEntry(
		FText::FromString(TEXT("Expand All")),
		FText::FromString(TEXT("Expand all folders recursively")),
		FSlateIcon(),
		FUIAction
		(
			FExecuteAction::CreateLambda([&]
			{
				ChangeItemExpansionRecursive(RootItem, true);
			}),
			FCanExecuteAction::CreateLambda([&]()
			{
				return RootItem.IsValid() && TreeListView.IsValid();
			})
		),
		NAME_None,
		EUserInterfaceActionType::Button
	);

	MenuBuilder.AddMenuEntry(
		FText::FromString(TEXT("Collapse All")),
		FText::FromString(TEXT("Collapse all folders recursively")),
		FSlateIcon(),
		FUIAction
		(
			FExecuteAction::CreateLambda([&]
			{
				ChangeItemExpansionRecursive(RootItem, false);
			}),
			FCanExecuteAction::CreateLambda([&]()
			{
				return RootItem.IsValid() && TreeListView.IsValid();
			})
		),
		NAME_None,
		EUserInterfaceActionType::Button
	);

	return MenuBuilder.MakeWidget();
}

TSharedPtr<SWidget> SPjcTabAssetsUnused::GetTreeContextMenu() const
{
	FMenuBuilder MenuBuilder{true, Cmds};

	MenuBuilder.BeginSection(TEXT("PjcSectionPathActions"), FText::FromString(TEXT("Actions")));
	{
		MenuBuilder.AddMenuEntry(FPjcCmds::Get().PathsExclude);
		MenuBuilder.AddMenuEntry(FPjcCmds::Get().PathsInclude);
	}
	MenuBuilder.EndSection();

	return MenuBuilder.MakeWidget();
}

TSharedRef<SHeaderRow> SPjcTabAssetsUnused::GetStatsHeaderRow() const
{
	return
		SNew(SHeaderRow)
		+ SHeaderRow::Column("Name")
		  .FillWidth(0.4f)
		  .HAlignCell(HAlign_Left)
		  .VAlignCell(VAlign_Center)
		  .HAlignHeader(HAlign_Center)
		  .HeaderContentPadding(HeaderMargin)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Category")))
			.Font(FPjcStyles::GetFont("Light", 10.0f))
			.ColorAndOpacity(FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
		]
		+ SHeaderRow::Column("Num")
		  .FillWidth(0.3f)
		  .HAlignCell(HAlign_Center)
		  .VAlignCell(VAlign_Center)
		  .HAlignHeader(HAlign_Center)
		  .HeaderContentPadding(HeaderMargin)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Num")))
			.Font(FPjcStyles::GetFont("Light", 10.0f))
			.ColorAndOpacity(FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
		]
		+ SHeaderRow::Column("Size")
		  .FillWidth(0.3f)
		  .HAlignCell(HAlign_Center)
		  .VAlignCell(VAlign_Center)
		  .HAlignHeader(HAlign_Center)
		  .HeaderContentPadding(HeaderMargin)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Size")))
			.Font(FPjcStyles::GetFont("Light", 10.0f))
			.ColorAndOpacity(FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
		];
}

TSharedRef<SHeaderRow> SPjcTabAssetsUnused::GetTreeHeaderRow()
{
	return
		SNew(SHeaderRow)
		+ SHeaderRow::Column(TEXT("Path"))
		  .HAlignHeader(HAlign_Center)
		  .VAlignHeader(VAlign_Center)
		  .HeaderContentPadding(HeaderMargin)
		  .FillWidth(0.4f)
		  .OnSort_Raw(this, &SPjcTabAssetsUnused::OnTreeSort)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Path")))
			.ColorAndOpacity(FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
			.Font(FPjcStyles::GetFont("Light", 10.0f))
		]
		+ SHeaderRow::Column(TEXT("NumAssetsTotal"))
		  .HAlignHeader(HAlign_Center)
		  .VAlignHeader(VAlign_Center)
		  .HeaderContentPadding(HeaderMargin)
		  .FillWidth(0.1f)
		  .OnSort_Raw(this, &SPjcTabAssetsUnused::OnTreeSort)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Total")))
			.ToolTipText(FText::FromString(TEXT("Total number of assets in current path")))
			.ColorAndOpacity(FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
			.Font(FPjcStyles::GetFont("Light", 10.0f))
		]
		+ SHeaderRow::Column(TEXT("NumAssetsUsed"))
		  .HAlignHeader(HAlign_Center)
		  .VAlignHeader(VAlign_Center)
		  .HeaderContentPadding(HeaderMargin)
		  .FillWidth(0.1f)
		  .OnSort_Raw(this, &SPjcTabAssetsUnused::OnTreeSort)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Used")))
			.ToolTipText(FText::FromString(TEXT("Total number of used assets in current path")))
			.ColorAndOpacity(FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
			.Font(FPjcStyles::GetFont("Light", 10.0f))
		]
		+ SHeaderRow::Column(TEXT("NumAssetsUnused"))
		  .HAlignHeader(HAlign_Center)
		  .VAlignHeader(VAlign_Center)
		  .HeaderContentPadding(HeaderMargin)
		  .FillWidth(0.1f)
		  .OnSort_Raw(this, &SPjcTabAssetsUnused::OnTreeSort)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Unused")))
			.ToolTipText(FText::FromString(TEXT("Total number of unused assets in current path")))
			.ColorAndOpacity(FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
			.Font(FPjcStyles::GetFont("Light", 10.0f))
		]
		+ SHeaderRow::Column(TEXT("UnusedPercent"))
		  .HAlignHeader(HAlign_Center)
		  .VAlignHeader(VAlign_Center)
		  .HeaderContentPadding(HeaderMargin)
		  .FillWidth(0.15f)
		  .OnSort_Raw(this, &SPjcTabAssetsUnused::OnTreeSort)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Unused %")))
			.ToolTipText(FText::FromString(TEXT("Percentage of unused assets number relative to total assets number in current path")))
			.ColorAndOpacity(FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
			.Font(FPjcStyles::GetFont("Light", 10.0f))
		]
		+ SHeaderRow::Column(TEXT("UnusedSize"))
		  .HAlignHeader(HAlign_Center)
		  .VAlignHeader(VAlign_Center)
		  .HeaderContentPadding(HeaderMargin)
		  .FillWidth(0.15f)
		  .OnSort_Raw(this, &SPjcTabAssetsUnused::OnTreeSort)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Unused Size")))
			.ToolTipText(FText::FromString(TEXT("Total size of unused assets in current path")))
			.ColorAndOpacity(FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
			.Font(FPjcStyles::GetFont("Light", 10.0f))
		];
}

TSharedRef<ITableRow> SPjcTabAssetsUnused::OnStatsGenerateRow(TSharedPtr<FPjcStatItem> Item, const TSharedRef<STableViewBase>& OwnerTable) const
{
	return SNew(SPjcItemStat, OwnerTable).Item(Item);
}

TSharedRef<ITableRow> SPjcTabAssetsUnused::OnTreeGenerateRow(TSharedPtr<FPjcTreeItem> Item, const TSharedRef<STableViewBase>& OwnerTable) const
{
	return SNew(SPjcItemTree, OwnerTable).Item(Item).HightlightText(TreeSearchText);
}

void SPjcTabAssetsUnused::ResetCachedData()
{
	AssetsAll.Reset();
	AssetsUsed.Reset();
	AssetsUnused.Reset();
	AssetsPrimary.Reset();
	AssetsIndirect.Reset();
	AssetsCircular.Reset();
	AssetsEditor.Reset();
	AssetsExcluded.Reset();
	AssetsExtReferenced.Reset();
	MapNumAssetsAllByPath.Reset();
	MapNumAssetsUsedByPath.Reset();
	MapNumAssetsUnusedByPath.Reset();
	MapSizeAssetsAllByPath.Reset();
	MapSizeAssetsUsedByPath.Reset();
	MapSizeAssetsUnusedByPath.Reset();

	NumAssetsAll = 0;
	NumAssetsUsed = 0;
	NumAssetsUnused = 0;
	NumAssetsPrimary = 0;
	NumAssetsIndirect = 0;
	NumAssetsEditor = 0;
	NumAssetsExcluded = 0;
	NumAssetsExtReferenced = 0;
	NumFoldersTotal = 0;
	NumFoldersEmpty = 0;

	SizeAssetsAll = 0;
	SizeAssetsUsed = 0;
	SizeAssetsUnused = 0;
	SizeAssetsPrimary = 0;
	SizeAssetsIndirect = 0;
	SizeAssetsEditor = 0;
	SizeAssetsExcluded = 0;
	SizeAssetsExtReferenced = 0;
}

void SPjcTabAssetsUnused::UpdateMapInfo(TMap<FString, int32>& MapNum, TMap<FString, int64>& MapSize, const FString& AssetPath, int64 AssetSize)
{
	FString CurrentPath = AssetPath;

	// Iterate through all parent folders and update the asset count and size
	while (!CurrentPath.IsEmpty())
	{
		if (MapNum.Contains(CurrentPath))
		{
			MapNum[CurrentPath]++;
			MapSize[CurrentPath] += AssetSize;
		}
		else
		{
			MapNum.Add(CurrentPath, 1);
			MapSize.Add(CurrentPath, AssetSize);
		}

		// Remove the last folder in the path
		int32 LastSlashIndex;
		if (CurrentPath.FindLastChar('/', LastSlashIndex))
		{
			CurrentPath.LeftInline(LastSlashIndex, false);
		}
		else
		{
			CurrentPath.Empty();
		}
	}
}
