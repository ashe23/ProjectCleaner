// // Copyright Ashot Barkhudaryan. All Rights Reserved.
//
// #include "Slate/SPjcTabAssetsBrowser.h"
// #include "PjcStyles.h"
// #include "PjcCmds.h"
// #include "PjcSettings.h"
// #include "PjcConstants.h"
// #include "PjcSubsystem.h"
// #include "PjcFrontendFilters.h"
// // Engine Headers
// #include "AssetManagerEditorModule.h"
// #include "ContentBrowserModule.h"
// #include "IContentBrowserSingleton.h"
// #include "FrontendFilterBase.h"
// #include "ObjectTools.h"
// #include "Settings/ContentBrowserSettings.h"
// #include "Widgets/Input/SSearchBox.h"
// #include "Widgets/Layout/SScrollBox.h"
// #include "Widgets/Layout/SSeparator.h"
// #include "Widgets/Notifications/SProgressBar.h"
//
// void SPjcTreeViewItem::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& OwnerTable)
// {
// 	TreeItem = InArgs._TreeItem;
// 	SearchText = InArgs._SearchText;
//
// 	SMultiColumnTableRow::Construct(SMultiColumnTableRow::FArguments().ToolTipText(FText::FromString(TreeItem->FolderPathRel)), OwnerTable);
// }
//
// TSharedRef<SWidget> SPjcTreeViewItem::GenerateWidgetForColumn(const FName& InColumnName)
// {
// 	if (InColumnName.IsEqual(TEXT("FolderName")))
// 	{
// 		return
// 			SNew(SHorizontalBox)
// 			.ToolTipText(FText::FromString(TreeItem->FolderPathRel))
// 			+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin{2.0f})
// 			[
// 				SNew(SExpanderArrow, SharedThis(this))
// 				.IndentAmount(10)
// 				.ShouldDrawWires(false)
// 			]
// 			+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 2, 0).VAlign(VAlign_Center)
// 			[
// 				SNew(SImage)
// 				.Image(GetFolderIcon())
// 				.ColorAndOpacity(GetFolderColor())
// 			]
// 			+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin{2.0f})
// 			[
// 				SNew(STextBlock).Text(FText::FromString(TreeItem->FolderName)).HighlightText(FText::FromString(SearchText))
// 			];
// 	}
//
// 	if (InColumnName.IsEqual(TEXT("Percent")))
// 	{
// 		return
// 			SNew(SHorizontalBox)
// 			+ SHorizontalBox::Slot().Padding(FMargin{20.0f, 1.0f}).FillWidth(1.0f)
// 			[
// 				SNew(SOverlay)
// 				+ SOverlay::Slot().HAlign(HAlign_Fill).VAlign(VAlign_Fill)
// 				[
// 					SNew(SProgressBar)
// 					.BorderPadding(FVector2D{0.0f, 0.0f})
// 					.Percent(TreeItem->PercentageUnusedNormalized)
// 					.BackgroundImage(FPjcStyles::Get().GetBrush("ProjectCleaner.Progressbar"))
// 					.FillColorAndOpacity(FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Red"))
// 				]
// 				+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
// 				[
// 					SNew(STextBlock)
// 					.AutoWrapText(false)
// 					.ColorAndOpacity(FLinearColor::White)
// 					.Text(FText::AsMemory(TreeItem->UnusedSize, SI))
// 				]
// 			];
// 	}
//
// 	if (InColumnName.IsEqual(TEXT("AssetsTotal")))
// 	{
// 		return SNew(STextBlock).Text(FText::FromString(FString::Printf(TEXT("%d"), TreeItem->AssetsTotal)));
// 	}
//
// 	if (InColumnName.IsEqual(TEXT("AssetsUsed")))
// 	{
// 		return SNew(STextBlock).Text(FText::FromString(FString::Printf(TEXT("%d"), TreeItem->AssetsUsed)));
// 	}
//
// 	if (InColumnName.IsEqual(TEXT("AssetsUnused")))
// 	{
// 		return SNew(STextBlock).Text(FText::FromString(FString::Printf(TEXT("%d"), TreeItem->AssetsUnused)));
// 	}
//
// 	return SNew(STextBlock).Text(FText::FromString(TEXT("")));
// }
//
// const FSlateBrush* SPjcTreeViewItem::GetFolderIcon() const
// {
// 	if (TreeItem->bIsDevFolder)
// 	{
// 		return FEditorStyle::GetBrush(TEXT("ContentBrowser.AssetTreeFolderDeveloper"));
// 	}
//
// 	return FEditorStyle::GetBrush(TreeItem->bIsExpanded ? TEXT("ContentBrowser.AssetTreeFolderOpen") : TEXT("ContentBrowser.AssetTreeFolderClosed"));
// }
//
// FSlateColor SPjcTreeViewItem::GetFolderColor() const
// {
// 	if (TreeItem->bIsExcluded)
// 	{
// 		return FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Yellow");
// 	}
//
// 	if (TreeItem->bIsEngineGenerated)
// 	{
// 		return FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.DarkGray");
// 	}
//
// 	if (TreeItem->bIsEmpty && !TreeItem->bIsEngineGenerated)
// 	{
// 		return FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Red");
// 	}
//
// 	return FLinearColor::Gray;
// }
//
// void SPjcTabAssetsBrowser::Construct(const FArguments& InArgs)
// {
// 	SubsystemPtr = GEditor->GetEditorSubsystem<UPjcSubsystem>();
// 	SettingsPtr = GetMutableDefault<UPjcSettings>();
//
// 	GetMutableDefault<UContentBrowserSettings>()->SetDisplayDevelopersFolder(true);
// 	GetMutableDefault<UContentBrowserSettings>()->PostEditChange();
//
// 	CmdsRegister();
//
// 	const FContentBrowserModule& ModuleContentBrowser = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
//
// 	SAssignNew(TreeView, STreeView<TSharedPtr<FPjcTreeViewItem>>)
// 	.TreeItemsSource(&TreeViewItems)
// 	.SelectionMode(ESelectionMode::Multi)
// 	.OnGenerateRow(this, &SPjcTabAssetsBrowser::OnTreeViewGenerateRow)
// 	.OnGetChildren(this, &SPjcTabAssetsBrowser::OnTreeViewGetChildren)
// 	.OnContextMenuOpening_Raw(this, &SPjcTabAssetsBrowser::OnTreeViewContextMenu)
// 	.OnSelectionChanged(this, &SPjcTabAssetsBrowser::OnTreeViewSelectionChange)
// 	.OnExpansionChanged_Raw(this, &SPjcTabAssetsBrowser::OnTreeViewExpansionChange)
// 	.HeaderRow(GetTreeViewHeaderRow());
//
// 	FAssetPickerConfig AssetPickerConfig;
// 	AssetPickerConfig.bAllowNullSelection = false;
// 	AssetPickerConfig.InitialAssetViewType = EAssetViewType::Tile;
// 	AssetPickerConfig.bCanShowFolders = false;
// 	AssetPickerConfig.bAddFilterUI = true;
// 	AssetPickerConfig.bSortByPathInColumnView = true;
// 	AssetPickerConfig.bShowPathInColumnView = true;
// 	AssetPickerConfig.bShowBottomToolbar = true;
// 	AssetPickerConfig.bCanShowDevelopersFolder = true;
// 	AssetPickerConfig.bCanShowClasses = false;
// 	AssetPickerConfig.bAllowDragging = false;
// 	AssetPickerConfig.bForceShowEngineContent = false;
// 	AssetPickerConfig.bCanShowRealTimeThumbnails = false;
// 	AssetPickerConfig.AssetShowWarningText = FText::FromName("No assets");
// 	AssetPickerConfig.GetCurrentSelectionDelegates.Add(&AssetBrowserDelegateSelection);
// 	AssetPickerConfig.RefreshAssetViewDelegates.Add(&AssetBrowserDelegateRefreshView);
// 	AssetPickerConfig.SetFilterDelegates.Add(&AssetBrowserDelegateFilter);
// 	AssetPickerConfig.Filter = AssetBrowserCreateFilter();
// 	AssetPickerConfig.OnAssetDoubleClicked = FOnAssetDoubleClicked::CreateLambda([](const FAssetData& AssetData)
// 	{
// 		if (!AssetData.IsValid()) return;
// 		if (!GEditor) return;
//
// 		TArray<FName> AssetNames;
// 		AssetNames.Add(AssetData.ToSoftObjectPath().GetAssetPathName());
//
// 		GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorsForAssets(AssetNames);
// 	});
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
//
// 	ChildSlot
// 	[
// 		SNew(SVerticalBox)
// 		+ SVerticalBox::Slot().Padding(FMargin{5.0f})
// 		[
// 			SNew(SSplitter)
// 			.Orientation(Orient_Horizontal)
// 			.PhysicalSplitterHandleSize(3.0f)
// 			.Style(FEditorStyle::Get(), "ContentBrowser.Splitter")
// 			+ SSplitter::Slot().Value(0.5f)
// 			[
// 				SNew(SVerticalBox)
// 				+ SVerticalBox::Slot().Padding(5.0f).FillHeight(1.0f)
// 				[
// 					SNew(SVerticalBox)
// 					+ SVerticalBox::Slot().AutoHeight().Padding(FMargin{0.0f, 0.0f, 0.0f, 5.0f})
// 					[
// 						SNew(SSearchBox)
// 						.HintText(FText::FromString(TEXT("Search Folders...")))
// 						.OnTextChanged(this, &SPjcTabAssetsBrowser::OnTreeViewSearchTextChanged)
// 						.OnTextCommitted(this, &SPjcTabAssetsBrowser::OnTreeViewSearchTextCommitted)
// 					]
// 					+ SVerticalBox::Slot().AutoHeight().Padding(FMargin{0.0f, 0.0f, 0.0f, 2.0f})
// 					[
// 						SNew(SHorizontalBox)
// 						+ SHorizontalBox::Slot().AutoWidth()
// 						[
// 							SNew(SImage)
// 							.Image(FEditorStyle::GetBrush("ContentBrowser.AssetTreeFolderOpen"))
// 							.ColorAndOpacity(FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Red"))
// 						]
// 						+ SHorizontalBox::Slot().Padding(FMargin{0.0f, 2.0f, 0.0f, 0.0f}).AutoWidth()
// 						[
// 							SNew(STextBlock).Text(FText::FromString(TEXT(" - Empty Paths")))
// 						]
// 						+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin{5.0f, 0.0f, 0.0f, 0.0f})
// 						[
// 							SNew(SImage)
// 							.Image(FEditorStyle::GetBrush("ContentBrowser.AssetTreeFolderOpen"))
// 							.ColorAndOpacity(FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Yellow"))
// 						]
// 						+ SHorizontalBox::Slot().Padding(FMargin{0.0f, 2.0f, 0.0f, 0.0f}).AutoWidth()
// 						[
// 							SNew(STextBlock).Text(FText::FromString(TEXT(" - Excluded Paths")))
// 						]
// 						+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin{5.0f, 0.0f, 0.0f, 0.0f})
// 						[
// 							SNew(SImage)
// 							.Image(FEditorStyle::GetBrush("ContentBrowser.AssetTreeFolderOpen"))
// 							.ColorAndOpacity(FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.DarkGray"))
// 						]
// 						+ SHorizontalBox::Slot().Padding(FMargin{0.0f, 2.0f, 0.0f, 0.0f}).AutoWidth()
// 						[
// 							SNew(STextBlock).Text(FText::FromString(TEXT(" - Engine Generated Paths"))).ToolTipText(FText::FromString(TEXT("Folders that generated by engine")))
// 						]
// 					]
// 					+ SVerticalBox::Slot().AutoHeight().Padding(FMargin{0.0f, 0.0f, 0.0f, 5.0f})
// 					[
// 						SNew(SSeparator).Thickness(5.0f)
// 					]
// 					+ SVerticalBox::Slot().FillHeight(1.0f).Padding(FMargin{0.0f, 5.0f, 0.0f, 0.0f})
// 					[
// 						SNew(SScrollBox)
// 						.ScrollWhenFocusChanges(EScrollWhenFocusChanges::NoScroll)
// 						.AnimateWheelScrolling(true)
// 						.AllowOverscroll(EAllowOverscroll::No)
// 						+ SScrollBox::Slot()
// 						[
// 							TreeView.ToSharedRef()
// 						]
// 					]
// 					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right).VAlign(VAlign_Center)
// 					[
// 						SNew(SComboButton)
// 						.ContentPadding(0)
// 						.ForegroundColor_Raw(this, &SPjcTabAssetsBrowser::GetTreeViewOptionsBtnForegroundColor)
// 						.ButtonStyle(FEditorStyle::Get(), "ToggleButton")
// 						.OnGetMenuContent(this, &SPjcTabAssetsBrowser::GetTreeViewOptionsBtnContent)
// 						.ButtonContent()
// 						[
// 							SNew(SHorizontalBox)
// 							+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
// 							[
// 								SNew(SImage).Image(FEditorStyle::GetBrush("GenericViewButton"))
// 							]
// 							+ SHorizontalBox::Slot().AutoWidth().Padding(2.0f, 0.0f, 0.0f, 0.0f).VAlign(VAlign_Center)
// 							[
// 								SNew(STextBlock).Text(FText::FromString(TEXT("View Options")))
// 							]
// 						]
// 					]
// 				]
// 			]
// 			+ SSplitter::Slot()
// 			[
// 				SNew(SVerticalBox)
// 				+ SVerticalBox::Slot().FillHeight(1.0f).Padding(FMargin{5.0f})
// 				[
// 					ModuleContentBrowser.Get().CreateAssetPicker(AssetPickerConfig)
// 				]
// 			]
// 		]
// 	];
// }
//
// void SPjcTabAssetsBrowser::UpdateData(const FPjcScanData& InScanData)
// {
// 	ScanData = InScanData;
//
// 	TreeViewItemsUpdate();
// }
//
// void SPjcTabAssetsBrowser::CmdsRegister()
// {
// 	Cmds = MakeShareable(new FUICommandList);
//
// 	Cmds->MapAction
// 	(
// 		FPjcCmds::Get().PathsOpenInFileExplorer,
// 		FUIAction
// 		(
// 			FExecuteAction::CreateLambda([&]()
// 			{
// 				const auto SelectedAssets = TreeView->GetSelectedItems();
//
// 				for (const auto& Item : SelectedAssets)
// 				{
// 					if (!FPaths::DirectoryExists(Item->FolderPathAbs)) continue;
//
// 					FPlatformProcess::ExploreFolder(*Item->FolderPathAbs);
// 				}
// 			})
// 		)
// 	);
//
// 	Cmds->MapAction
// 	(
// 		FPjcCmds::Get().PathsExclude,
// 		FUIAction
// 		(
// 			FExecuteAction::CreateLambda([&]()
// 			{
// 				const auto Items = TreeView->GetSelectedItems();
// 				for (const auto& Item : Items)
// 				{
// 					if (!Item.IsValid()) continue;
// 					if (Item->bIsExcluded) continue;
//
// 					SettingsPtr->ExcludedPaths.Add(FDirectoryPath{Item->FolderPathRel});
// 				}
//
// 				SettingsPtr->PostEditChange();
// 				SubsystemPtr->ProjectScan();
// 			}),
// 			FCanExecuteAction::CreateLambda([&]
// 			{
// 				if (!TreeView.IsValid()) return false;
// 				if (TreeView.Get()->GetSelectedItems().Num() == 0) return false;
// 				if (!SettingsPtr) return false;
// 				if (!SubsystemPtr) return false;
//
// 				const auto Items = TreeView->GetSelectedItems();
// 				for (const auto& Item : Items)
// 				{
// 					if (!Item->bIsExcluded)
// 					{
// 						return true;
// 					}
// 				}
//
// 				return false;
// 			})
// 		)
// 	);
//
// 	Cmds->MapAction
// 	(
// 		FPjcCmds::Get().PathsInclude,
// 		FUIAction
// 		(
// 			FExecuteAction::CreateLambda([&]()
// 			{
// 				const auto SelectedItems = TreeView->GetSelectedItems();
//
// 				TSet<FString> SelectedPaths;
// 				for (const auto& SelectedItem : SelectedItems)
// 				{
// 					if (!SelectedItem.IsValid()) continue;
//
// 					SelectedPaths.Add(SelectedItem->FolderPathRel);
// 				}
//
// 				SettingsPtr->ExcludedPaths.RemoveAllSwap([&SelectedPaths](const FDirectoryPath& Dir)
// 				{
// 					return SelectedPaths.Contains(Dir.Path);
// 				});
//
// 				SettingsPtr->PostEditChange();
// 				SubsystemPtr->ProjectScan();
// 			}),
// 			FCanExecuteAction::CreateLambda([&]
// 			{
// 				if (!TreeView.IsValid()) return false;
// 				if (TreeView.Get()->GetSelectedItems().Num() == 0) return false;
// 				if (!SettingsPtr) return false;
// 				if (!SubsystemPtr) return false;
//
// 				const auto Items = TreeView->GetSelectedItems();
// 				for (const auto& Item : Items)
// 				{
// 					if (!Item.IsValid()) continue;
//
// 					if (Item->bIsRoot && Item->bIsExcluded)
// 					{
// 						return true;
// 					}
//
// 					if (Item->bIsExcluded && Item->ParentItem.IsValid() && !Item->ParentItem->bIsExcluded)
// 					{
// 						return true;
// 					}
// 				}
//
// 				return false;
// 			})
// 		)
// 	);
//
// 	Cmds->MapAction
// 	(
// 		FPjcCmds::Get().PathsEmptyDelete,
// 		FUIAction
// 		(
// 			FExecuteAction::CreateLambda([&]()
// 			{
// 				const auto SelectedItems = TreeView->GetSelectedItems();
//
// 				TArray<FString> PathsToDelete;
// 				PathsToDelete.Reserve(SelectedItems.Num());
//
// 				for (const auto& Item : SelectedItems)
// 				{
// 					PathsToDelete.Add(Item->FolderPathAbs);
// 				}
//
// 				const int32 PathsDeleted = UPjcSubsystem::DeleteFolders(PathsToDelete, true);
//
// 				const FString Msg = FString::Printf(TEXT("Deleted %d empty folder%s"), PathsDeleted, PathsDeleted > 1 ? TEXT("s") : TEXT(""));
// 				UPjcSubsystem::ShowModal(Msg, EPjcModalStatus::OK, 5.0f);
//
// 				SubsystemPtr->ProjectScan();
// 			}),
// 			FCanExecuteAction::CreateLambda([&]
// 			{
// 				if (!TreeView.IsValid()) return false;
// 				if (TreeView.Get()->GetSelectedItems().Num() == 0) return false;
// 				if (!SubsystemPtr) return false;
//
// 				const auto Items = TreeView->GetSelectedItems();
// 				for (const auto& Item : Items)
// 				{
// 					if (!Item.IsValid()) continue;
//
// 					if (Item->bIsRoot || Item->bIsEngineGenerated || !Item->bIsEmpty)
// 					{
// 						return false;
// 					}
// 				}
//
// 				return true;
// 			})
// 		)
// 	);
//
// 	Cmds->MapAction
// 	(
// 		FPjcCmds::Get().OpenSizeMap,
// 		FUIAction
// 		(
// 			FExecuteAction::CreateLambda([&]()
// 			{
// 				const auto SelectedAssets = AssetBrowserDelegateSelection.Execute();
//
// 				TArray<FName> PackageNames;
// 				PackageNames.Reserve(SelectedAssets.Num());
//
// 				for (const auto& Asset : SelectedAssets)
// 				{
// 					PackageNames.Add(Asset.PackageName);
// 				}
//
// 				IAssetManagerEditorModule::Get().OpenSizeMapUI(PackageNames);
// 			})
// 		)
// 	);
//
// 	Cmds->MapAction
// 	(
// 		FPjcCmds::Get().OpenReferenceViewer,
// 		FUIAction
// 		(
// 			FExecuteAction::CreateLambda([&]()
// 			{
// 				const auto SelectedAssets = AssetBrowserDelegateSelection.Execute();
//
// 				TArray<FName> PackageNames;
// 				PackageNames.Reserve(SelectedAssets.Num());
//
// 				for (const auto& Asset : SelectedAssets)
// 				{
// 					PackageNames.Add(Asset.PackageName);
// 				}
//
// 				IAssetManagerEditorModule::Get().OpenReferenceViewerUI(PackageNames);
// 			})
// 		)
// 	);
//
// 	Cmds->MapAction
// 	(
// 		FPjcCmds::Get().OpenAssetAudit,
// 		FUIAction
// 		(
// 			FExecuteAction::CreateLambda([&]()
// 			{
// 				const auto SelectedAssets = AssetBrowserDelegateSelection.Execute();
//
// 				TArray<FName> PackageNames;
// 				PackageNames.Reserve(SelectedAssets.Num());
//
// 				for (const auto& Asset : SelectedAssets)
// 				{
// 					PackageNames.Add(Asset.PackageName);
// 				}
//
// 				IAssetManagerEditorModule::Get().OpenAssetAuditUI(PackageNames);
// 			})
// 		)
// 	);
//
// 	Cmds->MapAction
// 	(
// 		FPjcCmds::Get().AssetsDelete,
// 		FUIAction
// 		(
// 			FExecuteAction::CreateLambda([&]()
// 			{
// 				const int32 DeletedAssetsNum = ObjectTools::DeleteAssets(AssetBrowserDelegateSelection.Execute());
//
// 				if (DeletedAssetsNum > 0)
// 				{
// 					SubsystemPtr->ProjectScan();
// 				}
// 			}),
// 			FCanExecuteAction::CreateLambda([&]
// 			{
// 				return SubsystemPtr && FilterAllDisabled();
// 			})
// 		)
// 	);
//
// 	Cmds->MapAction
// 	(
// 		FPjcCmds::Get().AssetsExclude,
// 		FUIAction
// 		(
// 			FExecuteAction::CreateLambda([&]()
// 			{
// 				const auto SelectedAssets = AssetBrowserDelegateSelection.Execute();
// 				if (SelectedAssets.Num() == 0) return;
//
// 				for (const auto& SelectedAsset : SelectedAssets)
// 				{
// 					if (SettingsPtr->ExcludedAssets.Contains(SelectedAsset.GetAsset())) continue;
//
// 					SettingsPtr->ExcludedAssets.Add(SelectedAsset.GetAsset());
// 				}
//
// 				SettingsPtr->PostEditChange();
// 				SubsystemPtr->ProjectScan();
// 			}),
// 			FCanExecuteAction::CreateLambda([&]()
// 			{
// 				return !bFilterExcludeActive && SettingsPtr && SubsystemPtr;
// 			})
// 		)
// 	);
//
// 	Cmds->MapAction
// 	(
// 		FPjcCmds::Get().AssetsExcludeByClass,
// 		FUIAction
// 		(
// 			FExecuteAction::CreateLambda([&]()
// 			{
// 				const auto SelectedAssets = AssetBrowserDelegateSelection.Execute();
// 				if (SelectedAssets.Num() == 0) return;
//
// 				for (const auto& SelectedAsset : SelectedAssets)
// 				{
// 					// const FName AssetExactClassName = UPjcSubsystem::GetAssetExactClassName(SelectedAsset);
// 					
// 					// if (SettingsPtr->ExcludedClasses.Contains(AssetExactClassName)) continue;
// 					
// 					SettingsPtr->ExcludedClasses.Add(SelectedAsset.GetClass());
// 				}
//
// 				SettingsPtr->PostEditChange();
// 				SubsystemPtr->ProjectScan();
// 			}),
// 			FCanExecuteAction::CreateLambda([&]()
// 			{
// 				return !bFilterExcludeActive && SettingsPtr && SubsystemPtr;
// 			})
// 		)
// 	);
//
// 	Cmds->MapAction
// 	(
// 		FPjcCmds::Get().AssetsInclude,
// 		FUIAction
// 		(
// 			FExecuteAction::CreateLambda([&]()
// 			{
// 				// todo:ashe23
// 				const auto SelectedAssets = AssetBrowserDelegateSelection.Execute();
// 				if (SelectedAssets.Num() == 0) return;
//
// 				TSet<FName> SelectedAssetsObjectPaths;
// 				SelectedAssetsObjectPaths.Reserve(SelectedAssets.Num());
//
// 				for (const auto& Asset : SelectedAssets)
// 				{
// 					SelectedAssetsObjectPaths.Add(Asset.ObjectPath);
// 				}
//
// 				// if selected asset is not in excluded list, but excluded by path or class => show modal that asset cant be included
//
// 				SettingsPtr->ExcludedAssets.RemoveAllSwap([&SelectedAssetsObjectPaths](const TSoftObjectPtr<UObject>& ExcludedAsset)
// 				{
// 					if (!ExcludedAsset.LoadSynchronous()) return false;
//
// 					return SelectedAssetsObjectPaths.Contains(ExcludedAsset.ToSoftObjectPath().GetAssetPathName());
// 				});
//
// 				SettingsPtr->PostEditChange();
// 				SubsystemPtr->ProjectScan();
// 			}),
// 			FCanExecuteAction::CreateLambda([&]()
// 			{
// 				return bFilterExcludeActive && SettingsPtr && SubsystemPtr;
// 			})
// 		)
// 	);
// }
//
// TSharedRef<SHeaderRow> SPjcTabAssetsBrowser::GetTreeViewHeaderRow() const
// {
// 	return
// 		SNew(SHeaderRow)
// 		+ SHeaderRow::Column(TEXT("FolderName")).HAlignHeader(HAlign_Center).VAlignHeader(VAlign_Center).HeaderContentPadding(FMargin{5.0f}).FillWidth(0.5f)
// 		[
// 			SNew(STextBlock)
// 			.Text(FText::FromString(TEXT("Path")))
// 			.ColorAndOpacity(FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
// 			.Font(FPjcStyles::GetFont("Light", PjcConstants::HeaderRowFontSize))
// 		]
// 		+ SHeaderRow::Column(TEXT("Percent")).HAlignHeader(HAlign_Center).VAlignHeader(VAlign_Center).HeaderContentPadding(FMargin{5.0f}).FillWidth(0.3f)
// 		[
// 			SNew(STextBlock)
// 			.Text(FText::FromString(TEXT("Unused Size")))
// 			.ToolTipText(FText::FromString(TEXT("Total size of unused assets relative to total assets in current path")))
// 			.ColorAndOpacity(FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
// 			.Font(FPjcStyles::GetFont("Light", PjcConstants::HeaderRowFontSize))
// 		]
// 		+ SHeaderRow::Column(TEXT("AssetsTotal")).HAlignHeader(HAlign_Center).VAlignHeader(VAlign_Center).HeaderContentPadding(FMargin{5.0f}).FillWidth(0.1f).HAlignCell(HAlign_Center).
// 		                                          VAlignCell(VAlign_Center)
// 		[
// 			SNew(STextBlock)
// 			.Text(FText::FromString(TEXT("Total")))
// 			.ToolTipText(FText::FromString(TEXT("Total number of assets in current path")))
// 			.ColorAndOpacity(FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
// 			.Font(FPjcStyles::GetFont("Light", PjcConstants::HeaderRowFontSize))
// 		]
// 		+ SHeaderRow::Column(TEXT("AssetsUsed")).HAlignHeader(HAlign_Center).VAlignHeader(VAlign_Center).HeaderContentPadding(FMargin{5.0f}).FillWidth(0.1f).HAlignCell(HAlign_Center).
// 		                                         VAlignCell(VAlign_Center)
// 		[
// 			SNew(STextBlock)
// 			.Text(FText::FromString(TEXT("Used")))
// 			.ToolTipText(FText::FromString(TEXT("Total number of used assets in current path")))
// 			.ColorAndOpacity(FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
// 			.Font(FPjcStyles::GetFont("Light", PjcConstants::HeaderRowFontSize))
// 		]
// 		+ SHeaderRow::Column(TEXT("AssetsUnused")).HAlignHeader(HAlign_Center).VAlignHeader(VAlign_Center).HeaderContentPadding(FMargin{5.0f}).FillWidth(0.1f).HAlignCell(HAlign_Center).
// 		                                           VAlignCell(VAlign_Center)
// 		[
// 			SNew(STextBlock)
// 			.Text(FText::FromString(TEXT("Unused")))
// 			.ToolTipText(FText::FromString(TEXT("Total number of unused assets in current path")))
// 			.ColorAndOpacity(FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
// 			.Font(FPjcStyles::GetFont("Light", PjcConstants::HeaderRowFontSize))
// 		];
// }
//
// TSharedPtr<SWidget> SPjcTabAssetsBrowser::OnTreeViewContextMenu() const
// {
// 	FMenuBuilder MenuBuilder{true, Cmds};
// 	MenuBuilder.BeginSection(TEXT("PathActions"), FText::FromString(TEXT("Navigation")));
// 	{
// 		MenuBuilder.AddMenuEntry(FPjcCmds::Get().PathsOpenInFileExplorer);
// 	}
// 	MenuBuilder.EndSection();
// 	MenuBuilder.BeginSection(TEXT("PathActions"), FText::FromString(TEXT("Actions")));
// 	{
// 		MenuBuilder.AddMenuEntry(FPjcCmds::Get().PathsExclude);
// 		MenuBuilder.AddMenuEntry(FPjcCmds::Get().PathsInclude);
// 		MenuBuilder.AddMenuEntry(FPjcCmds::Get().PathsEmptyDelete);
// 	}
// 	MenuBuilder.EndSection();
//
// 	return MenuBuilder.MakeWidget();
// }
//
// TSharedRef<ITableRow> SPjcTabAssetsBrowser::OnTreeViewGenerateRow(TSharedPtr<FPjcTreeViewItem> Item, const TSharedRef<STableViewBase>& OwnerTable) const
// {
// 	return SNew(SPjcTreeViewItem, OwnerTable).TreeItem(Item).SearchText(TreeViewSearchText);
// }
//
// FARFilter SPjcTabAssetsBrowser::AssetBrowserCreateFilter() const
// {
// 	FARFilter Filter;
//
// 	if (TreeView->GetSelectedItems().Num() > 0)
// 	{
// 		for (const auto& SelectedItem : TreeView->GetSelectedItems())
// 		{
// 			Filter.PackagePaths.Add(FName{*SelectedItem->FolderPathRel});
// 		}
// 	}
//
// 	if (FilterAnyEnabled())
// 	{
// 		if (bFilterPrimaryActive)
// 		{
// 			TSet<FAssetData> AssetsPrimary;
// 			UPjcSubsystem::GetAssetsPrimary(AssetsPrimary);
//
// 			for (const auto& Asset : AssetsPrimary)
// 			{
// 				Filter.PackageNames.Emplace(Asset.PackageName);
// 			}
// 		}
//
// 		if (bFilterExcludeActive)
// 		{
// 			TSet<FAssetData> AssetsExcluded;
// 			UPjcSubsystem::GetAssetsExcluded(AssetsExcluded);
//
// 			for (const auto& Asset : AssetsExcluded)
// 			{
// 				Filter.PackageNames.Emplace(Asset.PackageName);
// 			}
// 		}
//
// 		if (bFilterIndirectActive)
// 		{
// 			TArray<FPjcAssetIndirectUsageInfo> AssetsIndirectInfo;
// 			UPjcSubsystem::GetAssetsIndirectInfos(AssetsIndirectInfo);
//
// 			for (const auto& Info : AssetsIndirectInfo)
// 			{
// 				Filter.PackageNames.Emplace(Info.AssetData.PackageName);
// 			}
// 		}
//
// 		if (bFilterExtReferencedActive)
// 		{
// 			for (const auto& Asset : ScanData.AssetsAll)
// 			{
// 				if (UPjcSubsystem::AssetIsExtReferenced(Asset))
// 				{
// 					Filter.PackageNames.Emplace(Asset.PackageName);
// 				}
// 			}
// 		}
//
// 		if (bFilterUsedActive)
// 		{
// 			for (const auto& Asset : ScanData.AssetsUsed)
// 			{
// 				Filter.PackageNames.Emplace(Asset.PackageName);
// 			}
// 		}
//
// 		return Filter;
// 	}
//
// 	if (ScanData.AssetsUnused.Num() == 0)
// 	{
// 		Filter.TagsAndValues.Add(FName{TEXT("ProjectCleanerEmptyTag")}, FString{TEXT("ProjectCleanerEmptyTag")});
// 	}
// 	else
// 	{
// 		Filter.ObjectPaths.Reserve(ScanData.AssetsUnused.Num());
//
// 		for (const auto& Asset : ScanData.AssetsUnused)
// 		{
// 			Filter.ObjectPaths.Add(Asset.ObjectPath);
// 		}
// 	}
//
// 	return Filter;
// }
//
// FSlateColor SPjcTabAssetsBrowser::GetTreeViewOptionsBtnForegroundColor() const
// {
// 	static const FName InvertedForegroundName("InvertedForeground");
// 	static const FName DefaultForegroundName("DefaultForeground");
//
// 	if (!TreeViewOptionBtn.IsValid()) return FEditorStyle::GetSlateColor(DefaultForegroundName);
//
// 	return TreeViewOptionBtn->IsHovered() ? FEditorStyle::GetSlateColor(InvertedForegroundName) : FEditorStyle::GetSlateColor(DefaultForegroundName);
// }
//
// TSharedRef<SWidget> SPjcTabAssetsBrowser::GetTreeViewOptionsBtnContent()
// {
// 	const TSharedPtr<FExtender> Extender;
// 	FMenuBuilder MenuBuilder(true, nullptr, Extender, true);
//
// 	MenuBuilder.AddMenuSeparator(TEXT("View"));
//
// 	MenuBuilder.AddMenuEntry(
// 		FText::FromString(TEXT("Show Paths Empty")),
// 		FText::FromString(TEXT("Show empty folders in tree view")),
// 		FSlateIcon(),
// 		FUIAction
// 		(
// 			FExecuteAction::CreateLambda([&]
// 			{
// 				SubsystemPtr->bShowPathsEmpty = !SubsystemPtr->bShowPathsEmpty;
// 				SubsystemPtr->PostEditChange();
//
// 				TreeViewItemsUpdate();
// 			}),
// 			FCanExecuteAction::CreateLambda([&]()
// 			{
// 				return SubsystemPtr && SubsystemPtr->bShowPathsUnusedOnly == false;
// 			}),
// 			FGetActionCheckState::CreateLambda([&]()
// 			{
// 				return SubsystemPtr->bShowPathsEmpty ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
// 			})
// 		),
// 		NAME_None,
// 		EUserInterfaceActionType::ToggleButton
// 	);
//
// 	MenuBuilder.AddMenuEntry(
// 		FText::FromString(TEXT("Show Paths Excluded")),
// 		FText::FromString(TEXT("Show excluded folders in tree view")),
// 		FSlateIcon(),
// 		FUIAction
// 		(
// 			FExecuteAction::CreateLambda([&]
// 			{
// 				SubsystemPtr->bShowPathsExcluded = !SubsystemPtr->bShowPathsExcluded;
// 				SubsystemPtr->PostEditChange();
//
// 				TreeViewItemsUpdate();
// 			}),
// 			FCanExecuteAction::CreateLambda([&]()
// 			{
// 				return SubsystemPtr && SubsystemPtr->bShowPathsUnusedOnly == false;
// 			}),
// 			FGetActionCheckState::CreateLambda([&]()
// 			{
// 				return SubsystemPtr->bShowPathsExcluded ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
// 			})
// 		),
// 		NAME_None,
// 		EUserInterfaceActionType::ToggleButton
// 	);
//
// 	MenuBuilder.AddMenuEntry(
// 		FText::FromString(TEXT("Show Paths Engine Generated")),
// 		FText::FromString(TEXT("Show engine generated folders in tree view")),
// 		FSlateIcon(),
// 		FUIAction
// 		(
// 			FExecuteAction::CreateLambda([&]
// 			{
// 				SubsystemPtr->bShowPathsEngineGenerated = !SubsystemPtr->bShowPathsEngineGenerated;
// 				SubsystemPtr->PostEditChange();
//
// 				TreeViewItemsUpdate();
// 			}),
// 			FCanExecuteAction::CreateLambda([&]()
// 			{
// 				return SubsystemPtr && SubsystemPtr->bShowPathsUnusedOnly == false;
// 			}),
// 			FGetActionCheckState::CreateLambda([&]()
// 			{
// 				return SubsystemPtr->bShowPathsEngineGenerated ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
// 			})
// 		),
// 		NAME_None,
// 		EUserInterfaceActionType::ToggleButton
// 	);
//
// 	MenuBuilder.AddMenuEntry(
// 		FText::FromString(TEXT("Show Paths Unused Only")),
// 		FText::FromString(TEXT("Show folders that contain unused assets only")),
// 		FSlateIcon(),
// 		FUIAction
// 		(
// 			FExecuteAction::CreateLambda([&]
// 			{
// 				SubsystemPtr->bShowPathsUnusedOnly = !SubsystemPtr->bShowPathsUnusedOnly;
// 				SubsystemPtr->PostEditChange();
//
// 				TreeViewItemsUpdate();
// 			}),
// 			FCanExecuteAction::CreateLambda([&]()
// 			{
// 				return SubsystemPtr != nullptr;
// 			}),
// 			FGetActionCheckState::CreateLambda([&]()
// 			{
// 				return SubsystemPtr->bShowPathsUnusedOnly ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
// 			})
// 		),
// 		NAME_None,
// 		EUserInterfaceActionType::ToggleButton
// 	);
//
// 	MenuBuilder.AddSeparator();
//
// 	MenuBuilder.AddMenuEntry(
// 		FText::FromString(TEXT("Clear Selection")),
// 		FText::FromString(TEXT("Clear current tree view selection")),
// 		FSlateIcon(),
// 		FUIAction
// 		(
// 			FExecuteAction::CreateLambda([&]
// 			{
// 				if (!TreeView.IsValid()) return;
//
// 				TreeView->ClearSelection();
// 				TreeView->ClearHighlightedItems();
// 			}),
// 			FCanExecuteAction::CreateLambda([&]()
// 			{
// 				return TreeView.IsValid() && TreeView->GetSelectedItems().Num() > 0;
// 			})
// 		),
// 		NAME_None,
// 		EUserInterfaceActionType::Button
// 	);
//
// 	return MenuBuilder.MakeWidget();
// }
//
// TSharedPtr<FPjcTreeViewItem> SPjcTabAssetsBrowser::CreateItem(const FString& InPath) const
// {
// 	TSharedPtr<FPjcTreeViewItem> Item = MakeShareable(new FPjcTreeViewItem);
// 	if (!FPaths::DirectoryExists(InPath)) return Item;
//
// 	Item->FolderPathAbs = UPjcSubsystem::PathConvertToAbs(InPath);
// 	Item->FolderPathRel = UPjcSubsystem::PathConvertToRel(Item->FolderPathAbs);
// 	Item->FolderName = FPaths::GetPathLeaf(Item->FolderPathAbs);
// 	Item->bIsExcluded = UPjcSubsystem::PathIsExcluded(InPath);
// 	Item->bIsEmpty = UPjcSubsystem::PathIsEmpty(InPath);
// 	Item->bIsRoot = Item->FolderPathRel.Equals(PjcConstants::PathRelRoot.ToString());
// 	Item->bIsExpanded = Item->bIsRoot;
// 	Item->bIsEngineGenerated = UPjcSubsystem::PathIsEngineGenerated(InPath);
// 	Item->bIsDevFolder = Item->FolderPathAbs.Equals(FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() / TEXT("Developers")));
//
// 	TArray<FAssetData> AssetsInPath;
// 	TArray<FAssetData> AssetsUsedInPath;
// 	TArray<FAssetData> AssetsUnusedInPath;
//
// 	UPjcSubsystem::FilterAssetsByPath(Item->FolderPathRel, false, ScanData.AssetsAll, AssetsInPath);
// 	UPjcSubsystem::FilterAssetsByPath(Item->FolderPathRel, false, ScanData.AssetsUsed, AssetsUsedInPath);
// 	UPjcSubsystem::FilterAssetsByPath(Item->FolderPathRel, false, ScanData.AssetsUnused, AssetsUnusedInPath);
//
// 	Item->UnusedSize = UPjcSubsystem::GetAssetsTotalSize(AssetsUnusedInPath);
// 	Item->AssetsTotal = AssetsInPath.Num();
// 	Item->AssetsUsed = AssetsUsedInPath.Num();
// 	Item->AssetsUnused = AssetsUnusedInPath.Num();
// 	Item->PercentageUnused = Item->AssetsTotal == 0 ? 0 : Item->AssetsUnused * 100.0f / Item->AssetsTotal;
// 	Item->PercentageUnusedNormalized = FMath::GetMappedRangeValueClamped(FVector2D{0.0f, 100.0f}, FVector2D{0.0f, 1.0f}, Item->PercentageUnused);
//
// 	SetItemVisibility(Item);
//
// 	if (!Item->bIsVisible) return nullptr;
//
// 	return Item;
// }
//
// void SPjcTabAssetsBrowser::SetItemVisibility(const TSharedPtr<FPjcTreeViewItem>& Item) const
// {
// 	if (!Item.IsValid()) return;
// 	if (!SubsystemPtr) return;
//
// 	if (Item->bIsRoot)
// 	{
// 		Item->bIsVisible = true;
// 		return;
// 	}
//
// 	if (SubsystemPtr->bShowPathsUnusedOnly && Item->AssetsUnused == 0)
// 	{
// 		Item->bIsVisible = false;
// 		return;
// 	}
//
// 	if (!SubsystemPtr->bShowPathsExcluded && Item->bIsExcluded)
// 	{
// 		Item->bIsVisible = false;
// 		return;
// 	}
//
// 	if (!SubsystemPtr->bShowPathsEngineGenerated && Item->bIsEngineGenerated)
// 	{
// 		Item->bIsVisible = false;
// 		return;
// 	}
//
// 	if (!SubsystemPtr->bShowPathsEmpty && Item->bIsEmpty)
// 	{
// 		Item->bIsVisible = false;
// 		return;
// 	}
//
// 	// if (Item->bIsEmpty && (Item->bIsEngineGenerated || !SubsystemPtr->bShowPathsEmpty))
// 	// {
// 	// 	Item->bIsVisible = false;
// 	// 	return;
// 	// }
//
// 	Item->bIsVisible = true;
// }
//
// void SPjcTabAssetsBrowser::TreeViewItemsUpdate()
// {
// 	if (!TreeView.IsValid()) return;
//
// 	TSet<TSharedPtr<FPjcTreeViewItem>> ItemsExpanded;
// 	TreeView->GetExpandedItems(ItemsExpanded);
//
// 	TSet<FString> CachedExpandedPaths;
// 	for (const auto& Item : ItemsExpanded)
// 	{
// 		if (!Item.IsValid()) continue;
// 		CachedExpandedPaths.Add(Item->FolderPathRel);
// 	}
//
// 	TreeViewItems.Empty();
//
// 	const TSharedPtr<FPjcTreeViewItem> RootItem = CreateItem(FPaths::ProjectContentDir());
// 	if (!RootItem.IsValid()) return;
//
// 	TreeView->SetItemExpansion(RootItem, true);
//
// 	TArray<TSharedPtr<FPjcTreeViewItem>> Stack;
// 	Stack.Add(RootItem);
//
// 	while (Stack.Num() > 0)
// 	{
// 		const TSharedPtr<FPjcTreeViewItem> CurrentItem = Stack.Pop();
//
// 		if (CachedExpandedPaths.Contains(CurrentItem->FolderPathRel))
// 		{
// 			CurrentItem->bIsExpanded = true;
// 			TreeView->SetItemExpansion(CurrentItem, true);
// 		}
//
// 		TSet<FString> SubFolders;
// 		UPjcSubsystem::GetFoldersInPath(CurrentItem->FolderPathAbs, false, SubFolders);
//
// 		for (const auto& SubFolder : SubFolders)
// 		{
// 			const TSharedPtr<FPjcTreeViewItem> SubItem = CreateItem(SubFolder);
// 			if (!SubItem.IsValid()) continue;
//
// 			SubItem->ParentItem = CurrentItem;
// 			CurrentItem->SubItems.Add(SubItem);
// 			Stack.Add(SubItem);
// 		}
// 	}
//
// 	TreeViewItems.Add(RootItem);
// 	TreeView->RequestTreeRefresh();
// }
//
// void SPjcTabAssetsBrowser::OnTreeViewGetChildren(TSharedPtr<FPjcTreeViewItem> Item, TArray<TSharedPtr<FPjcTreeViewItem>>& OutChildren)
// {
// 	if (!Item.IsValid()) return;
// 	if (Item->SubItems.Num() == 0) return;
//
// 	OutChildren.Append(Item->SubItems);
// }
//
// void SPjcTabAssetsBrowser::OnTreeViewSelectionChange(TSharedPtr<FPjcTreeViewItem> Item, ESelectInfo::Type SelectInfo) const
// {
// 	if (AssetBrowserDelegateFilter.IsBound())
// 	{
// 		AssetBrowserDelegateFilter.Execute(AssetBrowserCreateFilter());
// 	}
// }
//
// void SPjcTabAssetsBrowser::OnTreeViewExpansionChange(TSharedPtr<FPjcTreeViewItem> Item, bool bExpansion) const
// {
// 	// todo:ashe23 
// 	// if (!Item.IsValid()) return;
// 	// if (!TreeView.IsValid()) return;
// 	//
// 	// Item->bIsExpanded = bExpansion;
// 	//
// 	// TreeView->SetItemExpansion(Item, bExpansion);
// 	// TreeView->RequestTreeRefresh();
// }
//
// void SPjcTabAssetsBrowser::OnTreeViewSearchTextChanged(const FText& InText)
// {
// 	TreeViewSearchText = InText.ToString();
//
// 	TreeViewItemsUpdate();
// }
//
// void SPjcTabAssetsBrowser::OnTreeViewSearchTextCommitted(const FText& InText, ETextCommit::Type CommitType)
// {
// 	TreeViewSearchText = InText.ToString();
//
// 	TreeViewItemsUpdate();
// }
//
// bool SPjcTabAssetsBrowser::FilterAnyEnabled() const
// {
// 	return bFilterPrimaryActive || bFilterExcludeActive || bFilterIndirectActive || bFilterExtReferencedActive || bFilterUsedActive;
// }
//
// bool SPjcTabAssetsBrowser::FilterAllDisabled() const
// {
// 	return !bFilterPrimaryActive && !bFilterExcludeActive && !bFilterIndirectActive && !bFilterExtReferencedActive && !bFilterUsedActive;
// }
//
// bool SPjcTabAssetsBrowser::FilterAllEnabled() const
// {
// 	return bFilterPrimaryActive && bFilterExcludeActive && bFilterIndirectActive && bFilterExtReferencedActive && bFilterUsedActive;
// }
