// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/SPjcTabAssetsUnused.h"
#include "Slate/Items/SPjcItemTree.h"
#include "Slate/Shared/SPjcStatsBasic.h"
#include "PjcCmds.h"
#include "PjcTypes.h"
#include "PjcStyles.h"
#include "PjcConstants.h"
#include "PjcSubsystem.h"
#include "Libs/PjcLibPath.h"
#include "Libs/PjcLibAsset.h"
// Engine Headers
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "ObjectTools.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Settings/ContentBrowserSettings.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"

void SPjcTabAssetsUnused::Construct(const FArguments& InArgs)
{
	if (GEditor)
	{
		SubsystemPtr = GEditor->GetEditorSubsystem<UPjcSubsystem>();
		SubsystemPtr->OnScanAssetsSuccess().AddLambda([&]()
		{
			TreeItemsUpdate();
			TreeItemsFilter();
		});

		SubsystemPtr->OnScanAssetsFailed().AddLambda([&](const FString& ErrMsg)
		{
			UE_LOG(LogTemp, Warning, TEXT("%s"), *ErrMsg);
		});
	}

	Cmds = MakeShareable(new FUICommandList);

	Cmds->MapAction(
		FPjcCmds::Get().TabAssetsUnusedBtnScan,
		FExecuteAction::CreateLambda([&]()
		{
			SubsystemPtr->ScanProjectAssets();
		})
	);

	Cmds->MapAction(
		FPjcCmds::Get().TabAssetsUnusedBtnClean,
		FExecuteAction::CreateLambda([&]()
		{
			ObjectTools::DeleteAssets(SubsystemPtr->GetAssetsUnused().Array(), true);

			// const TSharedRef<SWindow> Window = SNew(SWindow).Title(FText::FromString(TEXT("Some Title"))).ClientSize(FVector2D{600, 400});
			// const TSharedRef<SWidget> Content =
			// 	SNew(SVerticalBox)
			// 	+ SVerticalBox::Slot()
			// 	[
			// 		SNew(STextBlock).Text(FText::FromString(TEXT("Some Content")))
			// 	];
			//
			// Window->SetContent(Content);
			//
			// if (GEditor)
			// {
			// 	GEditor->EditorAddModalWindow(Window);
			// }
		}),
		FCanExecuteAction::CreateLambda([]()
		{
			return true;
		})
	);

	Cmds->MapAction(
		FPjcCmds::Get().PathsExclude,
		FExecuteAction::CreateLambda([]
		{
			UE_LOG(LogTemp, Warning, TEXT("Excluding paths"));
		})
	);

	Cmds->MapAction(
		FPjcCmds::Get().PathsInclude,
		FExecuteAction::CreateLambda([]
		{
			UE_LOG(LogTemp, Warning, TEXT("Including paths"));
		})
	);

	Cmds->MapAction(
		FPjcCmds::Get().PathsDelete,
		FExecuteAction::CreateLambda([]
		{
			UE_LOG(LogTemp, Warning, TEXT("Deleting paths"));
		})
	);

	Cmds->MapAction(
		FPjcCmds::Get().AssetsExclude,
		FExecuteAction::CreateLambda([]
		{
			UE_LOG(LogTemp, Warning, TEXT("Assets Exclude"));
		})
	);

	Cmds->MapAction(
		FPjcCmds::Get().AssetsExcludeByClass,
		FExecuteAction::CreateLambda([]
		{
			UE_LOG(LogTemp, Warning, TEXT("Assets exclude by class"));
		})
	);

	Cmds->MapAction(
		FPjcCmds::Get().AssetsInclude,
		FExecuteAction::CreateLambda([]
		{
			UE_LOG(LogTemp, Warning, TEXT("Assets include"));
		})
	);

	Cmds->MapAction(
		FPjcCmds::Get().AssetsIncludeByClass,
		FExecuteAction::CreateLambda([]
		{
			UE_LOG(LogTemp, Warning, TEXT("Assets include by class"));
		})
	);

	Cmds->MapAction(
		FPjcCmds::Get().AssetsDelete,
		FExecuteAction::CreateLambda([]
		{
			UE_LOG(LogTemp, Warning, TEXT("Assets delete"));
		})
	);

	Cmds->MapAction(
		FPjcCmds::Get().OpenViewerSizeMap,
		FExecuteAction::CreateLambda([]
		{
			UE_LOG(LogTemp, Warning, TEXT("Open Size Map"));
		})
	);

	Cmds->MapAction(
		FPjcCmds::Get().OpenViewerReference,
		FExecuteAction::CreateLambda([]
		{
			UE_LOG(LogTemp, Warning, TEXT("Open Reference Viewer"));
		})
	);

	Cmds->MapAction(
		FPjcCmds::Get().OpenViewerAudit,
		FExecuteAction::CreateLambda([]
		{
			UE_LOG(LogTemp, Warning, TEXT("Open Asset Audit"));
		})
	);

	Cmds->MapAction(
		FPjcCmds::Get().ItemsCollapseAll,
		FExecuteAction::CreateLambda([&]
		{
			TreeItemsCollapseAll();
		})
	);

	Cmds->MapAction(
		FPjcCmds::Get().ItemsExpandAll,
		FExecuteAction::CreateLambda([&]
		{
			TreeItemsExpandAll();
		})
	);

	FPropertyEditorModule& PropertyEditor = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
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
	SettingsProperty->SetObject(GetMutableDefault<UPjcEditorAssetExcludeSettings>());

	const FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");

	UContentBrowserSettings* ContentBrowserSettings = GetMutableDefault<UContentBrowserSettings>();
	if (ContentBrowserSettings)
	{
		ContentBrowserSettings->SetDisplayDevelopersFolder(true);
		ContentBrowserSettings->SetDisplayEngineFolder(false);
		ContentBrowserSettings->SetDisplayCppFolders(false);
		ContentBrowserSettings->SetDisplayPluginFolders(false);
		ContentBrowserSettings->PostEditChange();
	}

	FPathPickerConfig PathPickerConfig;
	PathPickerConfig.bAllowClassesFolder = false;
	PathPickerConfig.bAllowContextMenu = false;
	PathPickerConfig.bFocusSearchBoxWhenOpened = false;

	FAssetPickerConfig AssetPickerConfig;
	AssetPickerConfig.bAllowDragging = false;
	AssetPickerConfig.bCanShowClasses = false;
	AssetPickerConfig.bCanShowFolders = false;
	AssetPickerConfig.bCanShowDevelopersFolder = true;
	AssetPickerConfig.bForceShowEngineContent = false;
	AssetPickerConfig.bForceShowPluginContent = false;
	AssetPickerConfig.bAddFilterUI = true;
	AssetPickerConfig.OnGetAssetContextMenu.BindRaw(this, &SPjcTabAssetsUnused::GetContentBrowserContextMenu);

	SAssignNew(TreeView, STreeView<TSharedPtr<FPjcTreeItem>>)
	.TreeItemsSource(&TreeItems)
	.SelectionMode(ESelectionMode::Multi)
	.OnGenerateRow(this, &SPjcTabAssetsUnused::OnTreeGenerateRow)
	.OnGetChildren(this, &SPjcTabAssetsUnused::OnTreeGetChildren)
	.OnContextMenuOpening_Raw(this, &SPjcTabAssetsUnused::GetTreeContextMenu)
	.OnExpansionChanged_Raw(this, &SPjcTabAssetsUnused::OnTreeExpansionChanged)
	.HeaderRow(GetTreeHeaderRow());

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
					CreateToolbar()
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(3.0f)
				[
					SNew(SSeparator).Thickness(5.0f)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(3.0f)
				[
					SNew(SPjcStatsBasic)
					.Title(FText::FromString(TEXT("Assets Statistics")))
					.HeaderMargin(FMargin{10.0f})
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
			+ SSplitter::Slot().Value(0.3f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(FMargin{5.0f})
				[
					SNew(SSearchBox)
					.HintText(FText::FromString(TEXT("Search Folders...")))
					.OnTextChanged_Raw(this, &SPjcTabAssetsUnused::OnTreeSearchTextChanged)
					.OnTextCommitted_Raw(this, &SPjcTabAssetsUnused::OnTreeSearchTextCommitted)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(FMargin{5.0f, 0.0f, 5.0f, 2.0f})
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
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(FMargin{5.0f, 0.0f, 5.0f, 5.0f})
				[
					SNew(SSeparator).Thickness(5.0f)
				]
				+ SVerticalBox::Slot().FillHeight(1.0f).Padding(FMargin{5.0f, 5.0f, 5.0f, 0.0f})
				[
					SNew(SScrollBox)
					.ScrollWhenFocusChanges(EScrollWhenFocusChanges::NoScroll)
					.AnimateWheelScrolling(true)
					.AllowOverscroll(EAllowOverscroll::No)
					+ SScrollBox::Slot()
					[
						TreeView.ToSharedRef()
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(5.0f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().HAlign(HAlign_Left).VAlign(VAlign_Center)
					[
						SNew(STextBlock).Text_Raw(this, &SPjcTabAssetsUnused::GetTreeSummaryText)
					]
					+ SHorizontalBox::Slot().FillWidth(1.0f).HAlign(HAlign_Right).VAlign(VAlign_Center)
					[
						SNew(SComboButton)
						.ContentPadding(0)
						.ForegroundColor_Raw(this, &SPjcTabAssetsUnused::GetTreeOptionsBtnForegroundColor)
						.ButtonStyle(FEditorStyle::Get(), "ToggleButton")
						.OnGetMenuContent(this, &SPjcTabAssetsUnused::GetTreeOptionsBtnContent)
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
			+ SSplitter::Slot().Value(0.5f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().FillHeight(1.0f).Padding(5.0f)
				[
					ContentBrowserModule.Get().CreateAssetPicker(AssetPickerConfig)
				]
			]
		]
	];
}

SPjcTabAssetsUnused::~SPjcTabAssetsUnused()
{
	if (SubsystemPtr)
	{
		SubsystemPtr->OnScanAssetsSuccess().RemoveAll(this);
		SubsystemPtr->OnScanAssetsFailed().RemoveAll(this);
	}
}

void SPjcTabAssetsUnused::TreeItemsUpdate()
{
	TreeRootItem.Reset();
	TreeRootItem = CreateTreeItem(PjcConstants::PathRoot.ToString());
	if (!TreeRootItem.IsValid()) return;

	TreeItems.Reset();
	TreeItems.Emplace(TreeRootItem);

	const FAssetRegistryModule& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(PjcConstants::ModuleAssetRegistry);
	TArray<TSharedPtr<FPjcTreeItem>> Stack;
	Stack.Push(TreeRootItem);

	TArray<FString> SubFolders;

	while (Stack.Num() > 0)
	{
		SubFolders.Reset();
		const auto CurrentItem = Stack.Pop(false);

		AssetRegistry.Get().GetSubPaths(CurrentItem->FolderPath, SubFolders, false);

		for (const auto& SubFolder : SubFolders)
		{
			const auto SubItem = CreateTreeItem(SubFolder);
			if (!SubItem.IsValid()) continue;

			SubItem->Parent = CurrentItem;
			CurrentItem->SubItems.Emplace(SubItem);

			Stack.Push(SubItem);
		}
	}
}

void SPjcTabAssetsUnused::TreeItemsFilter()
{
	if (!TreeRootItem.IsValid()) return;
	if (!TreeView.IsValid()) return;

	TreeItemsExpanded.Reset();
	TreeView->GetExpandedItems(TreeItemsExpanded);

	TArray<TSharedPtr<FPjcTreeItem>> Stack;
	Stack.Push(TreeRootItem);

	while (Stack.Num() > 0)
	{
		const auto CurrentItem = Stack.Pop(false);

		SetTreeItemVisibility(CurrentItem);
		SetTreeItemExpansion(CurrentItem);

		for (const auto& SubItem : CurrentItem->SubItems)
		{
			Stack.Push(SubItem);
		}
	}

	TreeView->RebuildList();
}

void SPjcTabAssetsUnused::TreeItemsCollapseAll()
{
	if (!TreeRootItem.IsValid()) return;
	if (!TreeView.IsValid()) return;

	TreeItemsExpanded.Reset();

	TArray<TSharedPtr<FPjcTreeItem>> Stack;

	Stack.Push(TreeRootItem);

	while (Stack.Num() > 0)
	{
		const auto CurrentItem = Stack.Pop(false);

		CurrentItem->bIsExpanded = false;
		TreeView->SetItemExpansion(CurrentItem, CurrentItem->bIsExpanded);
	}

	TreeView->RebuildList();
}

void SPjcTabAssetsUnused::TreeItemsExpandAll()
{
	if (!TreeRootItem.IsValid()) return;
	if (!TreeView.IsValid()) return;

	TreeItemsExpanded.Reset();

	TArray<TSharedPtr<FPjcTreeItem>> Stack;

	Stack.Push(TreeRootItem);

	while (Stack.Num() > 0)
	{
		const auto CurrentItem = Stack.Pop(false);

		CurrentItem->bIsExpanded = true;
		TreeView->SetItemExpansion(CurrentItem, CurrentItem->bIsExpanded);
	}

	TreeView->RebuildList();
}

bool SPjcTabAssetsUnused::TreeItemContainsSearchText(const TSharedPtr<FPjcTreeItem>& Item) const
{
	if (!Item.IsValid()) return false;

	auto CurrentItem = Item;

	while (CurrentItem.IsValid())
	{
		if (CurrentItem->FolderName.Contains(SearchText.ToString()))
		{
			return true;
		}
		CurrentItem = CurrentItem->Parent;
	}

	return false;
}

void SPjcTabAssetsUnused::TreeItemExpandParentsRecursive(const TSharedPtr<FPjcTreeItem>& Item) const
{
	if (!Item.IsValid()) return;
	if (!TreeView.IsValid()) return;

	auto CurrentItem = Item;
	while (CurrentItem.IsValid())
	{
		CurrentItem->bIsExpanded = true;
		TreeView->SetItemExpansion(CurrentItem, true);

		CurrentItem = CurrentItem->Parent;
	}
}

void SPjcTabAssetsUnused::TreeItemMakeVisibleParentsRecursive(const TSharedPtr<FPjcTreeItem>& Item) const
{
	if (!Item.IsValid()) return;

	auto CurrentItem = Item;
	while (CurrentItem.IsValid())
	{
		CurrentItem->bIsVisible = true;
		CurrentItem = CurrentItem->Parent;
	}
}

TSharedRef<SWidget> SPjcTabAssetsUnused::CreateToolbar() const
{
	FToolBarBuilder ToolBarBuilder{Cmds, FMultiBoxCustomization::None};
	ToolBarBuilder.BeginSection("PjcTabAssetUnusedScanActions");
	{
		ToolBarBuilder.AddToolBarButton(FPjcCmds::Get().TabAssetsUnusedBtnScan);
		ToolBarBuilder.AddToolBarButton(FPjcCmds::Get().TabAssetsUnusedBtnClean);
	}
	ToolBarBuilder.EndSection();

	return ToolBarBuilder.MakeWidget();
}

TSharedRef<SHeaderRow> SPjcTabAssetsUnused::GetTreeHeaderRow() const
{
	return
		SNew(SHeaderRow)
		+ SHeaderRow::Column(TEXT("Path")).HAlignHeader(HAlign_Center).VAlignHeader(VAlign_Center).HeaderContentPadding(FMargin{5.0f}).FillWidth(0.5f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Path")))
			.ColorAndOpacity(FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
			.Font(FPjcStyles::GetFont("Light", 10.0f))
		]
		+ SHeaderRow::Column(TEXT("NumAssetsTotal")).HAlignHeader(HAlign_Center).VAlignHeader(VAlign_Center).HeaderContentPadding(FMargin{5.0f}).FillWidth(0.1f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Total")))
			.ToolTipText(FText::FromString(TEXT("Total number of assets in current path")))
			.ColorAndOpacity(FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
			.Font(FPjcStyles::GetFont("Light", 10.0f))
		]
		+ SHeaderRow::Column(TEXT("NumAssetsUsed")).HAlignHeader(HAlign_Center).VAlignHeader(VAlign_Center).HeaderContentPadding(FMargin{5.0f}).FillWidth(0.1f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Used")))
			.ToolTipText(FText::FromString(TEXT("Total number of used assets in current path")))
			.ColorAndOpacity(FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
			.Font(FPjcStyles::GetFont("Light", 10.0f))
		]
		+ SHeaderRow::Column(TEXT("NumAssetsUnused")).HAlignHeader(HAlign_Center).VAlignHeader(VAlign_Center).HeaderContentPadding(FMargin{5.0f}).FillWidth(0.1f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Unused")))
			.ToolTipText(FText::FromString(TEXT("Total number of unused assets in current path")))
			.ColorAndOpacity(FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
			.Font(FPjcStyles::GetFont("Light", 10.0f))
		]
		+ SHeaderRow::Column(TEXT("UnusedSize")).HAlignHeader(HAlign_Center).VAlignHeader(VAlign_Center).HeaderContentPadding(FMargin{5.0f}).FillWidth(0.2f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Unused Size")))
			.ToolTipText(FText::FromString(TEXT("Total size of unused assets relative to total assets in current path")))
			.ColorAndOpacity(FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
			.Font(FPjcStyles::GetFont("Light", 10.0f))
		];
}

TSharedRef<ITableRow> SPjcTabAssetsUnused::OnTreeGenerateRow(TSharedPtr<FPjcTreeItem> Item, const TSharedRef<STableViewBase>& OwnerTable) const
{
	return SNew(SPjcItemTree, OwnerTable).Item(Item).HightlightText(SearchText);
}

TSharedPtr<FPjcTreeItem> SPjcTabAssetsUnused::CreateTreeItem(const FString& InFolderPath) const
{
	const bool bIsDev = InFolderPath.StartsWith(PjcConstants::PathDevelopers.ToString()) || InFolderPath.StartsWith(PjcConstants::PathCollections.ToString());
	const bool bIsRoot = InFolderPath.Equals(PjcConstants::PathRoot.ToString());

	const int32 NumAssetsTotalInPath = SubsystemPtr->GetNumAssetsTotalInPath(InFolderPath);
	const int32 NumAssetsUsedInPath = SubsystemPtr->GetNumAssetsUsedInPath(InFolderPath);
	const int32 NumAssetsUnusedInPath = SubsystemPtr->GetNumAssetsUnusedInPath(InFolderPath);
	const float SizeAssetsUnusedInPath = SubsystemPtr->GetSizeAssetsUnusedInPath(InFolderPath);
	const float PercentageUnused = NumAssetsTotalInPath == 0 ? 0 : NumAssetsUnusedInPath * 100.0f / NumAssetsTotalInPath;
	const float PercentageUnusedNormalized = FMath::GetMappedRangeValueClamped(FVector2D{0.0f, 100.0f}, FVector2D{0.0, 1.0f}, PercentageUnused);

	return MakeShareable(
		new FPjcTreeItem{
			InFolderPath,
			bIsRoot ? TEXT("Content") : FPaths::GetPathLeaf(InFolderPath),
			bIsDev,
			bIsRoot,
			FPjcLibPath::IsPathEmpty(InFolderPath),
			FPjcLibPath::IsPathExcluded(InFolderPath),
			bIsRoot,
			true,
			NumAssetsTotalInPath,
			NumAssetsUsedInPath,
			NumAssetsUnusedInPath,
			SizeAssetsUnusedInPath,
			PercentageUnused,
			PercentageUnusedNormalized
		});
}

void SPjcTabAssetsUnused::SetTreeItemVisibility(const TSharedPtr<FPjcTreeItem>& Item) const
{
	if (!Item.IsValid()) return;
	if (!SubsystemPtr) return;

	if (!SubsystemPtr->CanShowFoldersEmpty() && Item->bIsEmpty && !Item->bIsRoot)
	{
		Item->bIsVisible = false;
		return;
	}

	if (!SubsystemPtr->CanShowFoldersExcluded() && Item->bIsExcluded && !Item->bIsRoot)
	{
		Item->bIsVisible = false;
		return;
	}

	if ((Item->bIsVisible = Item->bIsRoot || SearchText.IsEmpty())) return;

	if (TreeItemContainsSearchText(Item))
	{
		TreeItemMakeVisibleParentsRecursive(Item);
	}
}

void SPjcTabAssetsUnused::SetTreeItemExpansion(const TSharedPtr<FPjcTreeItem>& Item)
{
	if (!Item.IsValid()) return;
	if (!TreeView.IsValid()) return;
	if (!Item->bIsVisible)
	{
		Item->bIsExpanded = false;
		TreeView->SetItemExpansion(Item, Item->bIsExpanded);
		return;
	}

	if (TreeItemContainsSearchText(Item))
	{
		TreeItemExpandParentsRecursive(Item);
		return;
	}

	bool bIsCached = false;

	for (const auto& CachedItem : TreeItemsExpanded)
	{
		if (!CachedItem.IsValid()) continue;

		if (CachedItem->FolderPath.Equals(Item->FolderPath))
		{
			bIsCached = true;
			break;
		}
	}

	Item->bIsExpanded = Item->bIsRoot || bIsCached;
	TreeView->SetItemExpansion(Item, Item->bIsExpanded);
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

void SPjcTabAssetsUnused::OnTreeExpansionChanged(TSharedPtr<FPjcTreeItem> Item, const bool bIsExpanded) const
{
	if (!Item.IsValid()) return;
	if (!TreeView.IsValid()) return;

	Item->bIsExpanded = bIsExpanded;
	TreeView->SetItemExpansion(Item, bIsExpanded);
	TreeView->RebuildList();
}

void SPjcTabAssetsUnused::OnTreeSearchTextChanged(const FText& InText)
{
	SearchText = InText;

	TreeItemsFilter();
}

void SPjcTabAssetsUnused::OnTreeSearchTextCommitted(const FText& InText, ETextCommit::Type Type)
{
	SearchText = InText;

	TreeItemsFilter();
}

FSlateColor SPjcTabAssetsUnused::GetTreeOptionsBtnForegroundColor() const
{
	static const FName InvertedForegroundName("InvertedForeground");
	static const FName DefaultForegroundName("DefaultForeground");

	if (!TreeOptionBtn.IsValid()) return FEditorStyle::GetSlateColor(DefaultForegroundName);

	return TreeOptionBtn->IsHovered() ? FEditorStyle::GetSlateColor(InvertedForegroundName) : FEditorStyle::GetSlateColor(DefaultForegroundName);
}

TSharedRef<SWidget> SPjcTabAssetsUnused::GetTreeOptionsBtnContent()
{
	const TSharedPtr<FExtender> Extender;
	FMenuBuilder MenuBuilder(true, nullptr, Extender, true);

	MenuBuilder.AddMenuSeparator(TEXT("View"));

	MenuBuilder.AddMenuEntry(
		FText::FromString(TEXT("Show Folders Empty")),
		FText::FromString(TEXT("Show empty folders in tree view")),
		FSlateIcon(),
		FUIAction
		(
			FExecuteAction::CreateLambda([&]
			{
				SubsystemPtr->ToggleShowFoldersEmpty();
				TreeItemsFilter();
			}),
			FCanExecuteAction::CreateLambda([&]()
			{
				return SubsystemPtr != nullptr;
			}),
			FGetActionCheckState::CreateLambda([&]()
			{
				return SubsystemPtr->CanShowFoldersEmpty() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
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
				SubsystemPtr->ToggleShowFoldersExcluded();
				TreeItemsFilter();
			}),
			FCanExecuteAction::CreateLambda([&]()
			{
				return SubsystemPtr != nullptr;
			}),
			FGetActionCheckState::CreateLambda([&]()
			{
				return SubsystemPtr->CanShowFoldersExcluded() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
			})
		),
		NAME_None,
		EUserInterfaceActionType::ToggleButton
	);

	MenuBuilder.AddSeparator();
	MenuBuilder.AddMenuEntry(FPjcCmds::Get().ItemsCollapseAll);
	MenuBuilder.AddMenuEntry(FPjcCmds::Get().ItemsExpandAll);

	return MenuBuilder.MakeWidget();
}

TSharedPtr<SWidget> SPjcTabAssetsUnused::GetTreeContextMenu() const
{
	FMenuBuilder MenuBuilder{true, Cmds};

	MenuBuilder.BeginSection(TEXT("PjcTabAssetsUnusedPathActions"), FText::FromString(TEXT("Actions")));
	{
		MenuBuilder.AddMenuEntry(FPjcCmds::Get().PathsExclude);
		MenuBuilder.AddMenuEntry(FPjcCmds::Get().PathsInclude);
		MenuBuilder.AddMenuEntry(FPjcCmds::Get().PathsDelete);
	}
	MenuBuilder.EndSection();

	return MenuBuilder.MakeWidget();
}

FText SPjcTabAssetsUnused::GetTreeSummaryText() const
{
	return FText{};
}

TSharedPtr<SWidget> SPjcTabAssetsUnused::GetContentBrowserContextMenu(const TArray<FAssetData>& Assets) const
{
	FMenuBuilder MenuBuilder{true, Cmds};

	MenuBuilder.BeginSection(TEXT("PjcAssetsInfo"), FText::FromString(TEXT("Info")));
	{
		MenuBuilder.AddMenuEntry(FPjcCmds::Get().OpenViewerSizeMap);
		MenuBuilder.AddMenuEntry(FPjcCmds::Get().OpenViewerReference);
		MenuBuilder.AddMenuEntry(FPjcCmds::Get().OpenViewerAudit);
	}
	MenuBuilder.EndSection();

	MenuBuilder.BeginSection(TEXT("PjcAssetActions"), FText::FromString(TEXT("Actions")));
	{
		MenuBuilder.AddMenuEntry(FPjcCmds::Get().AssetsExclude);
		MenuBuilder.AddMenuEntry(FPjcCmds::Get().AssetsExcludeByClass);
		MenuBuilder.AddMenuEntry(FPjcCmds::Get().AssetsInclude);
		MenuBuilder.AddMenuEntry(FPjcCmds::Get().AssetsIncludeByClass);
		MenuBuilder.AddMenuEntry(FPjcCmds::Get().AssetsDelete);
	}
	MenuBuilder.EndSection();

	return MenuBuilder.MakeWidget();
}
