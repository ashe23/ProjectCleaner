// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/SPjcTreeView.h"
#include "Slate/SPjcItemTree.h"
#include "PjcSubsystem.h"
#include "PjcConstants.h"
#include "PjcStyles.h"
#include "PjcCmds.h"
// Engine Headers
// #include "ObjectTools.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"

void SPjcTreeView::Construct(const FArguments& InArgs)
{
	TreeItemsInit();
	
	Cmds = MakeShareable(new FUICommandList);

	Cmds->MapAction(
		FPjcCmds::Get().PathsDelete,
		FExecuteAction::CreateLambda([&]() { })
	);

	SAssignNew(TreeView, STreeView<TSharedPtr<FPjcTreeItem>>)
	.TreeItemsSource(&TreeItems)
	.SelectionMode(ESelectionMode::Multi)
	.OnGenerateRow(this, &SPjcTreeView::OnTreeGenerateRow)
	.OnGetChildren(this, &SPjcTreeView::OnTreeGetChildren)
	.OnContextMenuOpening_Raw(this, &SPjcTreeView::GetTreeContextMenu)
	.OnSelectionChanged_Raw(this, &SPjcTreeView::OnTreeSelectionChanged)
	// .OnExpansionChanged_Raw(this, &SPjcTreeView::OnTreeExpansionChanged)
	.HeaderRow(GetTreeHeaderRow());

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin{5.0f})
		[
			SNew(SSearchBox)
			.HintText(FText::FromString(TEXT("Search Folders...")))
			.OnTextChanged_Raw(this, &SPjcTreeView::OnTreeSearchTextChanged)
			.OnTextCommitted_Raw(this, &SPjcTreeView::OnTreeSearchTextCommitted)
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
			+ SHorizontalBox::Slot().FillWidth(1.0f).HAlign(HAlign_Left).VAlign(VAlign_Center)
			[
				SNew(SComboButton)
				.ContentPadding(0)
				.ForegroundColor_Raw(this, &SPjcTreeView::GetTreeOptionsBtnForegroundColor)
				.ButtonStyle(FEditorStyle::Get(), "ToggleButton")
				.OnGetMenuContent(this, &SPjcTreeView::GetTreeBtnActionsContent)
				.ButtonContent()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(SImage).Image(FEditorStyle::GetBrush("GenericViewButton"))
					]
					+ SHorizontalBox::Slot().AutoWidth().Padding(2.0f, 0.0f, 0.0f, 0.0f).VAlign(VAlign_Center)
					[
						SNew(STextBlock).Text(FText::FromString(TEXT("Actions")))
					]
				]
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f).HAlign(HAlign_Center).VAlign(VAlign_Center)
			[
				SNew(STextBlock).Text_Raw(this, &SPjcTreeView::GetTreeSummaryText)
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f).HAlign(HAlign_Right).VAlign(VAlign_Center)
			[
				SNew(SComboButton)
				.ContentPadding(0)
				.ForegroundColor_Raw(this, &SPjcTreeView::GetTreeOptionsBtnForegroundColor)
				.ButtonStyle(FEditorStyle::Get(), "ToggleButton")
				.OnGetMenuContent(this, &SPjcTreeView::GetTreeBtnOptionsContent)
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
	];
}

TSharedRef<SWidget> SPjcTreeView::GetTreeBtnActionsContent()
{
	const TSharedPtr<FExtender> Extender;
	FMenuBuilder MenuBuilder(true, nullptr, Extender, true);

	MenuBuilder.AddMenuSeparator(TEXT("Actions"));

	MenuBuilder.AddMenuEntry(
		FText::FromString(TEXT("Delete Empty Folders")),
		FText::FromString(TEXT("Delete all empty folders in project")),
		FSlateIcon(),
		FUIAction
		(
			FExecuteAction::CreateLambda([&] { })
		),
		NAME_None,
		EUserInterfaceActionType::Button
	);


	MenuBuilder.AddSeparator();

	MenuBuilder.AddMenuEntry(
		FText::FromString(TEXT("Expand All")),
		FText::FromString(TEXT("Expand all folders recursive")),
		FSlateIcon(),
		FUIAction
		(
			FExecuteAction::CreateLambda([&] { })
		),
		NAME_None,
		EUserInterfaceActionType::Button
	);

	MenuBuilder.AddMenuEntry(
		FText::FromString(TEXT("Collapse All")),
		FText::FromString(TEXT("Collapse all folders recursive")),
		FSlateIcon(),
		FUIAction
		(
			FExecuteAction::CreateLambda([&] { })
		),
		NAME_None,
		EUserInterfaceActionType::Button
	);

	MenuBuilder.AddSeparator();
	MenuBuilder.AddMenuEntry(
		FText::FromString(TEXT("Clear Selection")),
		FText::FromString(TEXT("Clear any selection in tree view")),
		FSlateIcon(),
		FUIAction
		(
			FExecuteAction::CreateLambda([&]
			{
				TreeView->ClearSelection();
				TreeView->ClearHighlightedItems();
			}),
			FCanExecuteAction::CreateLambda([&]()
			{
				return TreeView.IsValid();
			})
		),
		NAME_None,
		EUserInterfaceActionType::Button
	);

	return MenuBuilder.MakeWidget();
}

TSharedRef<SWidget> SPjcTreeView::GetTreeBtnOptionsContent()
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
			FExecuteAction::CreateLambda([&] { })
		),
		NAME_None,
		EUserInterfaceActionType::ToggleButton
	);

	MenuBuilder.AddMenuEntry(
		FText::FromString(TEXT("Show Folders Excluded")),
		FText::FromString(TEXT("Show empty folders in tree view")),
		FSlateIcon(),
		FUIAction
		(
			FExecuteAction::CreateLambda([&] { })
		),
		NAME_None,
		EUserInterfaceActionType::ToggleButton
	);

	MenuBuilder.AddMenuEntry(
		FText::FromString(TEXT("Show Folders Unused Only")),
		FText::FromString(TEXT("Show empty folders in tree view")),
		FSlateIcon(),
		FUIAction
		(
			FExecuteAction::CreateLambda([&] { })
		),
		NAME_None,
		EUserInterfaceActionType::ToggleButton
	);

	MenuBuilder.AddMenuEntry(
		FText::FromString(TEXT("Show Folders Engine Generated")),
		FText::FromString(TEXT("Show empty folders in tree view")),
		FSlateIcon(),
		FUIAction
		(
			FExecuteAction::CreateLambda([&] { })
		),
		NAME_None,
		EUserInterfaceActionType::ToggleButton
	);

	return MenuBuilder.MakeWidget();
}

TSharedPtr<SWidget> SPjcTreeView::GetTreeContextMenu() const
{
	FMenuBuilder MenuBuilder{true, Cmds};

	MenuBuilder.BeginSection(TEXT("PjcSectionPathActions"), FText::FromString(TEXT("Actions")));
	{
		MenuBuilder.AddMenuEntry(FPjcCmds::Get().PathsExclude);
		MenuBuilder.AddMenuEntry(FPjcCmds::Get().PathsInclude);
		MenuBuilder.AddMenuEntry(FPjcCmds::Get().PathsDelete);
	}
	MenuBuilder.EndSection();

	return MenuBuilder.MakeWidget();
}

TSharedRef<SHeaderRow> SPjcTreeView::GetTreeHeaderRow()
{
	const FMargin HeaderPadding{5.0f};

	return
		SNew(SHeaderRow)
		+ SHeaderRow::Column(TEXT("Path"))
		  .HAlignHeader(HAlign_Center)
		  .VAlignHeader(VAlign_Center)
		  .HeaderContentPadding(HeaderPadding)
		  .FillWidth(0.4f)
		  .OnSort_Raw(this, &SPjcTreeView::OnTreeSort)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Path")))
			.ColorAndOpacity(FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
			.Font(FPjcStyles::GetFont("Light", 10.0f))
		]
		+ SHeaderRow::Column(TEXT("NumAssetsTotal"))
		  .HAlignHeader(HAlign_Center)
		  .VAlignHeader(VAlign_Center)
		  .HeaderContentPadding(HeaderPadding)
		  .FillWidth(0.1f)
		  .OnSort_Raw(this, &SPjcTreeView::OnTreeSort)
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
		  .HeaderContentPadding(HeaderPadding)
		  .FillWidth(0.1f)
		  .OnSort_Raw(this, &SPjcTreeView::OnTreeSort)
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
		  .HeaderContentPadding(HeaderPadding)
		  .FillWidth(0.1f)
		  .OnSort_Raw(this, &SPjcTreeView::OnTreeSort)
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
		  .HeaderContentPadding(HeaderPadding)
		  .FillWidth(0.15f)
		  .OnSort_Raw(this, &SPjcTreeView::OnTreeSort)
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
		  .HeaderContentPadding(HeaderPadding)
		  .FillWidth(0.15f)
		  .OnSort_Raw(this, &SPjcTreeView::OnTreeSort)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Unused Size")))
			.ToolTipText(FText::FromString(TEXT("Total size of unused assets in current path")))
			.ColorAndOpacity(FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
			.Font(FPjcStyles::GetFont("Light", 10.0f))
		];
}

TSharedRef<ITableRow> SPjcTreeView::OnTreeGenerateRow(TSharedPtr<FPjcTreeItem> Item, const TSharedRef<STableViewBase>& OwnerTable) const
{
	return SNew(SPjcItemTree, OwnerTable).Item(Item).HightlightText(SearchText);
}

void SPjcTreeView::OnTreeGetChildren(TSharedPtr<FPjcTreeItem> Item, TArray<TSharedPtr<FPjcTreeItem>>& OutChildren)
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

void SPjcTreeView::OnTreeSearchTextChanged(const FText& InText)
{
	SearchText = InText;
}

void SPjcTreeView::OnTreeSearchTextCommitted(const FText& InText, ETextCommit::Type Type)
{
	SearchText = InText;
}

void SPjcTreeView::OnTreeSort(EColumnSortPriority::Type SortPriority, const FName& ColumnName, EColumnSortMode::Type InSortMode)
{
	if (!RootItem.IsValid() || !TreeView.IsValid()) return;

	auto SortTreeItems = [&](auto& SortMode, auto SortFunc)
	{
		SortMode = SortMode == EColumnSortMode::Ascending ? EColumnSortMode::Descending : EColumnSortMode::Ascending;

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

	if (ColumnName.IsEqual(TEXT("Path")))
	{
		SortTreeItems(ColumnPathSortMode, [&](const TSharedPtr<FPjcTreeItem>& Item1, const TSharedPtr<FPjcTreeItem>& Item2)
		{
			return ColumnPathSortMode == EColumnSortMode::Ascending ? Item1->FolderPath < Item2->FolderPath : Item1->FolderPath > Item2->FolderPath;
		});
	}

	if (ColumnName.IsEqual(TEXT("NumAssetsTotal")))
	{
		SortTreeItems(ColumnAssetsTotalSortMode, [&](const TSharedPtr<FPjcTreeItem>& Item1, const TSharedPtr<FPjcTreeItem>& Item2)
		{
			return ColumnAssetsTotalSortMode == EColumnSortMode::Ascending ? Item1->NumAssetsTotal < Item2->NumAssetsTotal : Item1->NumAssetsTotal > Item2->NumAssetsTotal;
		});
	}

	if (ColumnName.IsEqual(TEXT("NumAssetsUsed")))
	{
		SortTreeItems(ColumnAssetsUsedSortMode, [&](const TSharedPtr<FPjcTreeItem>& Item1, const TSharedPtr<FPjcTreeItem>& Item2)
		{
			return ColumnAssetsUsedSortMode == EColumnSortMode::Ascending ? Item1->NumAssetsUsed < Item2->NumAssetsUsed : Item1->NumAssetsUsed > Item2->NumAssetsUsed;
		});
	}

	if (ColumnName.IsEqual(TEXT("NumAssetsUnused")))
	{
		SortTreeItems(ColumnAssetsUnusedSortMode, [&](const TSharedPtr<FPjcTreeItem>& Item1, const TSharedPtr<FPjcTreeItem>& Item2)
		{
			return ColumnAssetsUnusedSortMode == EColumnSortMode::Ascending ? Item1->NumAssetsUnused < Item2->NumAssetsUnused : Item1->NumAssetsUnused > Item2->NumAssetsUnused;
		});
	}

	if (ColumnName.IsEqual(TEXT("UnusedPercent")))
	{
		SortTreeItems(ColumnUnusedPercentSortMode, [&](const TSharedPtr<FPjcTreeItem>& Item1, const TSharedPtr<FPjcTreeItem>& Item2)
		{
			return ColumnUnusedPercentSortMode == EColumnSortMode::Ascending ? Item1->PercentageUnused < Item2->PercentageUnused : Item1->PercentageUnused > Item2->PercentageUnused;
		});
	}

	if (ColumnName.IsEqual(TEXT("UnusedSize")))
	{
		SortTreeItems(ColumnUnusedSizeSortMode, [&](const TSharedPtr<FPjcTreeItem>& Item1, const TSharedPtr<FPjcTreeItem>& Item2)
		{
			return ColumnUnusedSizeSortMode == EColumnSortMode::Ascending ? Item1->SizeAssetsUnused < Item2->SizeAssetsUnused : Item1->SizeAssetsUnused > Item2->SizeAssetsUnused;
		});
	}

	TreeView->RebuildList();
}

void SPjcTreeView::OnTreeSelectionChanged(TSharedPtr<FPjcTreeItem> Selection, ESelectInfo::Type SelectInfo)
{
	if (!TreeView.IsValid()) return;

	const auto ItemsSelected = TreeView->GetSelectedItems();

	TSet<FString> SelectedPaths;
	SelectedPaths.Reserve(ItemsSelected.Num());

	for (const auto& Item : ItemsSelected)
	{
		if (!Item.IsValid()) continue;

		SelectedPaths.Emplace(Item->FolderPath);
	}
}

void SPjcTreeView::TreeItemsInit()
{
	TMap<EPjcAssetCategory, TSet<FAssetData>> AssetsCategoryMapping;
	UPjcSubsystem::AssetCategoryMappingInit(AssetsCategoryMapping);

	TreeItemsUpdateData(AssetsCategoryMapping);
}

void SPjcTreeView::TreeItemsUpdateData(TMap<EPjcAssetCategory, TSet<FAssetData>>& AssetsCategoryMapping)
{
	if (!TreeView.IsValid()) return;
	
	MapNumAssetsAllByPath.Reset();
	MapNumAssetsUsedByPath.Reset();
	MapNumAssetsUnusedByPath.Reset();
	MapSizeAssetsAllByPath.Reset();
	MapSizeAssetsUsedByPath.Reset();
	MapSizeAssetsUnusedByPath.Reset();

	for (const FAssetData& Asset : AssetsCategoryMapping[EPjcAssetCategory::Any])
	{
		const FString AssetPath = Asset.PackagePath.ToString();
		const int64 AssetSize = UPjcSubsystem::GetAssetSize(Asset);
		UpdateMapInfo(MapNumAssetsAllByPath, MapSizeAssetsAllByPath, AssetPath, AssetSize);
	}

	for (const FAssetData& Asset : AssetsCategoryMapping[EPjcAssetCategory::Used])
	{
		const FString AssetPath = Asset.PackagePath.ToString();
		const int64 AssetSize = UPjcSubsystem::GetAssetSize(Asset);
		UpdateMapInfo(MapNumAssetsUsedByPath, MapSizeAssetsUsedByPath, AssetPath, AssetSize);
	}

	for (const FAssetData& Asset : AssetsCategoryMapping[EPjcAssetCategory::Unused])
	{
		const FString AssetPath = Asset.PackagePath.ToString();
		const int64 AssetSize = UPjcSubsystem::GetAssetSize(Asset);
		UpdateMapInfo(MapNumAssetsUnusedByPath, MapSizeAssetsUnusedByPath, AssetPath, AssetSize);
	}

	TSet<TSharedPtr<FPjcTreeItem>> CachedExpandedItems;
	TreeView->GetExpandedItems(CachedExpandedItems);

	RootItem.Reset();
	TreeItems.Reset();

	// creating root item
	const FString PathContentDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir());

	RootItem = MakeShareable(new FPjcTreeItem);
	if (!RootItem.IsValid()) return;

	RootItem->FolderPath = PjcConstants::PathRoot.ToString();
	RootItem->FolderName = TEXT("Content");
	RootItem->bIsDev = false;
	RootItem->bIsRoot = true;
	RootItem->bIsEmpty = UPjcSubsystem::PathIsEmpty(PathContentDir);
	RootItem->bIsExcluded = UPjcSubsystem::PathIsExcluded(PathContentDir);
	RootItem->bIsExpanded = ItemIsExpanded(RootItem, CachedExpandedItems);
	RootItem->bIsVisible = true;
	RootItem->NumAssetsTotal = AssetsCategoryMapping[EPjcAssetCategory::Any].Num();
	RootItem->NumAssetsUsed = AssetsCategoryMapping[EPjcAssetCategory::Used].Num();
	RootItem->NumAssetsUnused = AssetsCategoryMapping[EPjcAssetCategory::Unused].Num();
	RootItem->SizeAssetsUnused = UPjcSubsystem::GetAssetsTotalSize(AssetsCategoryMapping[EPjcAssetCategory::Unused]);
	RootItem->PercentageUnused = RootItem->NumAssetsTotal == 0 ? 0 : RootItem->NumAssetsUnused * 100.0f / RootItem->NumAssetsTotal;
	RootItem->PercentageUnusedNormalized = FMath::GetMappedRangeValueClamped(FVector2D{0.0f, 100.0f}, FVector2D{0.0f, 1.0f}, RootItem->PercentageUnused);
	RootItem->Parent = nullptr;

	TreeItems.Emplace(RootItem);
	TreeView->SetItemExpansion(RootItem, RootItem->bIsExpanded);

	// filling whole tree
	TArray<TSharedPtr<FPjcTreeItem>> Stack;
	Stack.Push(RootItem);

	while (Stack.Num() > 0)
	{
		const auto CurrentItem = Stack.Pop(false);

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
			SubItem->bIsEmpty = UPjcSubsystem::PathIsEmpty(SubItem->FolderPath);
			SubItem->bIsExcluded = UPjcSubsystem::PathIsExcluded(SubItem->FolderPath);
			SubItem->bIsExpanded = ItemIsExpanded(SubItem, CachedExpandedItems);
			SubItem->bIsVisible = true;
			SubItem->NumAssetsTotal = MapNumAssetsAllByPath.Contains(SubItem->FolderPath) ? MapNumAssetsAllByPath[SubItem->FolderPath] : 0;
			SubItem->NumAssetsUsed = MapNumAssetsUsedByPath.Contains(SubItem->FolderPath) ? MapNumAssetsUsedByPath[SubItem->FolderPath] : 0;
			SubItem->NumAssetsUnused = MapNumAssetsUnusedByPath.Contains(SubItem->FolderPath) ? MapNumAssetsUnusedByPath[SubItem->FolderPath] : 0;
			SubItem->SizeAssetsUnused = MapSizeAssetsUnusedByPath.Contains(SubItem->FolderPath) ? MapSizeAssetsUnusedByPath[SubItem->FolderPath] : 0.0f;
			SubItem->PercentageUnused = SubItem->NumAssetsTotal == 0 ? 0 : SubItem->NumAssetsUnused * 100.0f / SubItem->NumAssetsTotal;
			SubItem->PercentageUnusedNormalized = FMath::GetMappedRangeValueClamped(FVector2D{0.0f, 100.0f}, FVector2D{0.0f, 1.0f}, SubItem->PercentageUnused);
			SubItem->Parent = CurrentItem;

			TreeView->SetItemExpansion(SubItem, SubItem->bIsExpanded);
			CurrentItem->SubItems.Emplace(SubItem);
			Stack.Emplace(SubItem);
		}
	}

	TreeView->RebuildList();
}

void SPjcTreeView::TreeItemsUpdateView() {}

bool SPjcTreeView::ItemIsExpanded(const TSharedPtr<FPjcTreeItem>& Item, const TSet<TSharedPtr<FPjcTreeItem>>& ExpandedItems)
{
	if (!Item.IsValid()) return false;
	if (ExpandedItems.Num() == 0) return false;

	for (const auto& ExpandedItem : ExpandedItems)
	{
		if (!ExpandedItem.IsValid()) continue;

		if (ExpandedItem->FolderPath.Equals(Item->FolderPath))
		{
			return true;
		}
	}

	return false;
}

FText SPjcTreeView::GetTreeSummaryText() const
{
	const int32 NumItemsSelected = TreeView.IsValid() ? TreeView->GetSelectedItems().Num() : 0;

	if (NumItemsSelected > 0)
	{
		return FText::FromString(FString::Printf(TEXT("%d selected"), NumItemsSelected));
	}

	return FText::GetEmpty();
}

FSlateColor SPjcTreeView::GetTreeOptionsBtnForegroundColor() const
{
	static const FName InvertedForegroundName("InvertedForeground");
	static const FName DefaultForegroundName("DefaultForeground");

	if (!TreeOptionBtn.IsValid()) return FEditorStyle::GetSlateColor(DefaultForegroundName);

	return TreeOptionBtn->IsHovered() ? FEditorStyle::GetSlateColor(InvertedForegroundName) : FEditorStyle::GetSlateColor(DefaultForegroundName);
}

void SPjcTreeView::UpdateMapInfo(TMap<FString, int32>& MapNum, TMap<FString, int64>& MapSize, const FString& AssetPath, int64 AssetSize)
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
