// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/Tabs/SProjectCleanerTabUnusedAssets.h"
#include "Settings/ProjectCleanerExcludeSettings.h"
#include "ProjectCleanerStyles.h"
#include "ProjectCleanerCmds.h"
#include "ProjectCleanerConstants.h"
#include "ProjectCleanerSubsystem.h"
// Engine Headers
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "Internationalization/BreakIterator.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Kismet/KismetMathLibrary.h"

void SProjectCleanerTreeViewItem::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& OwnerTable)
{
	TreeItem = InArgs._TreeItem;

	SMultiColumnTableRow::Construct(SMultiColumnTableRow::FArguments().ToolTipText(FText::FromString(TreeItem->FolderPathRel)), OwnerTable);
}

TSharedRef<SWidget> SProjectCleanerTreeViewItem::GenerateWidgetForColumn(const FName& InColumnName)
{
	if (InColumnName.IsEqual(TEXT("Name")))
	{
		return
			SNew(SHorizontalBox)
			.ToolTipText(FText::FromString(TreeItem->FolderPathRel))
			+ SHorizontalBox::Slot()
			  .AutoWidth()
			  .Padding(FMargin{2.0f})
			[
				SNew(SExpanderArrow, SharedThis(this))
				.IndentAmount(20)
				.ShouldDrawWires(GetDefault<UProjectCleanerSubsystem>()->bShowTreeViewLines)
			]
			+ SHorizontalBox::Slot()
			  .AutoWidth()
			  .Padding(0, 0, 2, 0)
			  .VAlign(VAlign_Center)
			[
				// Folder Icon
				SNew(SImage)
				.Image(this, &SProjectCleanerTreeViewItem::GetFolderIcon)
				.ColorAndOpacity(this, &SProjectCleanerTreeViewItem::GetFolderColor)
			]
			+ SHorizontalBox::Slot()
			  .AutoWidth()
			  .Padding(FMargin{5.0f})
			[
				SNew(STextBlock).Text(FText::FromString(TreeItem->FolderName))
			];
	}

	if (InColumnName.IsEqual(TEXT("FoldersTotal")))
	{
		return
			SNew(SHorizontalBox)
			.ToolTipText(FText::FromString(TEXT("Number of folders in this path")))
			+ SHorizontalBox::Slot()
			  .FillWidth(1.0f)
			  .HAlign(HAlign_Center)
			  .VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Justification(ETextJustify::Center)
				.Text(FText::FromString(FString::Printf(TEXT("%d"), TreeItem->FoldersTotal)))
			];
	}

	if (InColumnName.IsEqual(TEXT("FoldersEmpty")))
	{
		return
			SNew(SHorizontalBox)
			.ToolTipText(FText::FromString(TEXT("Number of empty folders in this path")))
			+ SHorizontalBox::Slot()
			  .FillWidth(1.0f)
			  .HAlign(HAlign_Center)
			  .VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Justification(ETextJustify::Center)
				.Text(FText::FromString(FString::Printf(TEXT("%d"), TreeItem->FoldersEmpty)))
			];
	}

	if (InColumnName.IsEqual(TEXT("AssetsTotal")))
	{
		return
			SNew(SHorizontalBox)
			.ToolTipText(FText::FromString(TEXT("Number of assets in this path")))
			+ SHorizontalBox::Slot()
			  .FillWidth(1.0f)
			  .HAlign(HAlign_Center)
			  .VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Justification(ETextJustify::Center)
				.Text(FText::FromString(FString::Printf(TEXT("%d"), TreeItem->AssetsTotal)))
			];
	}

	if (InColumnName.IsEqual(TEXT("AssetsUnused")))
	{
		return
			SNew(SHorizontalBox)
			.ToolTipText(FText::FromString(TEXT("Number of unused assets in this path")))
			+ SHorizontalBox::Slot()
			  .FillWidth(1.0f)
			  .HAlign(HAlign_Center)
			  .VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Justification(ETextJustify::Center)
				.Text(FText::FromString(FString::Printf(TEXT("%d"), TreeItem->AssetsUnused)))
			];
	}

	if (InColumnName.IsEqual(TEXT("SizeTotal")))
	{
		return
			SNew(SHorizontalBox)
			.ToolTipText(FText::FromString(TEXT("Total size of all assets in this path")))
			+ SHorizontalBox::Slot()
			  .FillWidth(1.0f)
			  .HAlign(HAlign_Center)
			  .VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Justification(ETextJustify::Center)
				.Text(FText::AsMemory(TreeItem->SizeTotal))
			];
	}

	if (InColumnName.IsEqual(TEXT("SizeUnused")))
	{
		return
			SNew(SHorizontalBox)
			.ToolTipText(FText::FromString(TEXT("Total size of unused assets in this path")))
			+ SHorizontalBox::Slot()
			  .FillWidth(1.0f)
			  .HAlign(HAlign_Center)
			  .VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Justification(ETextJustify::Center)
				.Text(FText::AsMemory(TreeItem->SizeUnused))
			];
	}

	if (InColumnName.IsEqual(TEXT("Percent")))
	{
		return
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			  .Padding(FMargin{20.0f, 5.0f})
			  .FillWidth(1.0f)
			  .HAlign(HAlign_Fill)
			  .VAlign(VAlign_Fill)
			[
				SNew(SOverlay)
				+ SOverlay::Slot()
				  .HAlign(HAlign_Fill)
				  .VAlign(VAlign_Fill)
				[
					SNew(SProgressBar)
					.Percent(TreeItem->PercentUnusedNormalized)
					.FillColorAndOpacity_Raw(this, &SProjectCleanerTreeViewItem::GetProgressBarColor)
				]
				+ SOverlay::Slot()
				  .HAlign(HAlign_Center)
				  .VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.AutoWrapText(false)
					.ColorAndOpacity(FLinearColor{0.0f, 0.0f, 0.0f, 1.0f})
					.Text(FText::FromString(FString::Printf(TEXT("%.2f %%"), TreeItem->PercentUnused)))
				]
			];
	}

	return SNew(STextBlock).Text(FText::FromString(TEXT("")));
}

const FSlateBrush* SProjectCleanerTreeViewItem::GetFolderIcon() const
{
	if (TreeItem->bDevFolder)
	{
		return FEditorStyle::GetBrush(TEXT("ContentBrowser.AssetTreeFolderDeveloper"));
	}

	return FEditorStyle::GetBrush(TreeItem->bExpanded ? TEXT("ContentBrowser.AssetTreeFolderOpen") : TEXT("ContentBrowser.AssetTreeFolderClosed"));
}

FSlateColor SProjectCleanerTreeViewItem::GetFolderColor() const
{
	if (TreeItem->bExcluded)
	{
		return FProjectCleanerStyles::Get().GetSlateColor("ProjectCleaner.Color.Yellow");
	}

	if (TreeItem->bEmpty)
	{
		return FProjectCleanerStyles::Get().GetSlateColor("ProjectCleaner.Color.Red");
	}

	return FSlateColor{FLinearColor::Gray};
}

FSlateColor SProjectCleanerTreeViewItem::GetProgressBarColor() const
{
	const FLinearColor Green = FProjectCleanerStyles::Get().GetSlateColor("ProjectCleaner.Color.Green").GetSpecifiedColor();
	// const FLinearColor Yellow = FProjectCleanerStyles::Get().GetSlateColor("ProjectCleaner.Color.Yellow").GetSpecifiedColor();
	// const FLinearColor Red = FProjectCleanerStyles::Get().GetSlateColor("ProjectCleaner.Color.Red").GetSpecifiedColor();

	const FLinearColor Color1 = TreeItem->PercentUnusedNormalized < 0.5f ? FLinearColor::Green : FLinearColor::Yellow;
	const FLinearColor Color2 = TreeItem->PercentUnusedNormalized >= 0.5f ? FLinearColor::Yellow : FLinearColor::Red;
	const FLinearColor CurrentColor = UKismetMathLibrary::LinearColorLerp(FLinearColor::Green, FLinearColor::Red, TreeItem->PercentUnusedNormalized);
	const FLinearColor TargetColor = UKismetMathLibrary::LinearColorLerp(Color1, Color2, TreeItem->PercentUnusedNormalized);

	return FSlateColor{CurrentColor};
}

void SProjectCleanerTabUnusedAssets::Construct(const FArguments& InArgs)
{
	if (!GEditor) return;
	SubsystemPtr = GEditor->GetEditorSubsystem<UProjectCleanerSubsystem>();
	if (!SubsystemPtr) return;

	AssetThumbnailPool = MakeShareable(new FAssetThumbnailPool(1024, false));

	// by default Content (/Game) folder must be selected
	SelectedPaths.Add(ProjectCleanerConstants::PathRelRoot.ToString());

	// if project scanned we should update ui
	SubsystemPtr->OnProjectScanned().AddLambda([&]()
	{
		TreeViewItemsUpdate();
		AssetBrowserItemsUpdate();
	});

	// const FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
	// TArray<FAdvancedAssetCategory> AdvancedAssetCategories;
	// AssetToolsModule.Get().GetAllAdvancedAssetCategories(AdvancedAssetCategories);

	CommandsRegister();
	TreeViewItemsUpdate();
	AssetBrowserItemsUpdate();

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
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				  .AutoHeight()
				  .Padding(FMargin{0.0f, 0.0f, 0.0f, 10.0f})
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SImage)
						.Image(FEditorStyle::GetBrush("ContentBrowser.AssetTreeFolderOpen"))
						.ColorAndOpacity(FProjectCleanerStyles::Get().GetSlateColor("ProjectCleaner.Color.Red"))
					]
					+ SHorizontalBox::Slot()
					  .Padding(FMargin{0.0f, 2.0f, 0.0f, 0.0f})
					  .AutoWidth()
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT(" - Empty Folders")))
					]
					+ SHorizontalBox::Slot()
					  .AutoWidth()
					  .Padding(FMargin{5.0f, 0.0f, 0.0f, 0.0f})
					[
						SNew(SImage)
						.Image(FEditorStyle::GetBrush("ContentBrowser.AssetTreeFolderOpen"))
						.ColorAndOpacity(FProjectCleanerStyles::Get().GetSlateColor("ProjectCleaner.Color.Yellow"))
					]
					+ SHorizontalBox::Slot()
					  .Padding(FMargin{0.0f, 2.0f, 0.0f, 0.0f})
					  .AutoWidth()
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT(" - Excluded Folders")))
					]
				]
				+ SVerticalBox::Slot()
				  .AutoHeight()
				  .Padding(FMargin{0.0f, 0.0f, 0.0f, 5.0f})
				[
					SNew(SSeparator)
					.Thickness(5.0f)
				]
				+ SVerticalBox::Slot()
				  .AutoHeight()
				  .Padding(FMargin{0.0f, 0.0f, 0.0f, 5.0f})
				[
					SNew(SSearchBox)
					.HintText(FText::FromString(TEXT("Search Folders...")))
					.OnTextChanged(this, &SProjectCleanerTabUnusedAssets::OnTreeViewSearchBoxTextChanged)
					.OnTextCommitted(this, &SProjectCleanerTabUnusedAssets::OnTreeViewSearchBoxTextCommitted)
				]
				+ SVerticalBox::Slot()
				  .FillHeight(1.0f)
				  .Padding(FMargin{0.0f, 5.0f})
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
				+ SVerticalBox::Slot()
				  .AutoHeight()
				  .HAlign(HAlign_Right)
				  .VAlign(VAlign_Center)
				  .Padding(FMargin{0.0f, 5.0f})
				[
					SNew(SComboButton)
					.ContentPadding(0)
					.ForegroundColor_Raw(this, &SProjectCleanerTabUnusedAssets::GetTreeViewOptionsBtnForegroundColor)
					.ButtonStyle(FEditorStyle::Get(), "ToggleButton")
					.OnGetMenuContent(this, &SProjectCleanerTabUnusedAssets::GetTreeViewOptionsBtnContent)
					.ButtonContent()
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						  .AutoWidth()
						  .VAlign(VAlign_Center)
						[
							SNew(SImage).Image(FEditorStyle::GetBrush("GenericViewButton"))
						]
						+ SHorizontalBox::Slot()
						  .AutoWidth()
						  .Padding(2, 0, 0, 0)
						  .VAlign(VAlign_Center)
						[
							SNew(STextBlock).Text(FText::FromString(TEXT("View Options")))
						]
					]
				]
			]
			+ SSplitter::Slot()
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				  .AutoHeight()
				  .Padding(FMargin{0.0f, 5.0f, 0.0f, 5.0f})
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SAssignNew(FilterComboButtonPtr, SComboButton)
						.ComboButtonStyle(FEditorStyle::Get(), "GenericFilters.ComboButtonStyle")
						.ForegroundColor(FLinearColor::White)
						.ToolTipText(FText::FromString(TEXT("Add an asset filter.")))
						// .OnGetMenuContent(this, &SProjectCleanerTabUnusedAssets::AssetBrowserMakeFilterMenu)
						.HasDownArrow(true)
						.ContentPadding(FMargin(1, 0))
						.AddMetaData<FTagMetaData>(FTagMetaData(TEXT("ContentBrowserFiltersCombo")))
						.ButtonContent()
						[
							SNew(STextBlock)
							.TextStyle(FEditorStyle::Get(), "GenericFilters.TextStyle")
							.Text(FText::FromString(TEXT("Filters")))
						]
					]
					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					[
						SNew(SSearchBox)
						.HintText(FText::FromString(TEXT("Search Assets...")))
						// .OnTextChanged(this, &SProjectCleanerTreeView::OnTreeViewSearchBoxTextChanged)
						// .OnTextCommitted(this, &SProjectCleanerTreeView::OnTreeViewSearchBoxTextCommitted)
					]
				]
				// + SVerticalBox::Slot()
				// .AutoHeight()
				// [
				// 	SNew(SSeparator)
				// 	.Thickness(5.0f)
				// ]
				+ SVerticalBox::Slot()
				  .FillHeight(1.0f)
				  .Padding(FMargin{0.0f, 5.0f})
				[
					SNew(SScrollBox)
					.ScrollWhenFocusChanges(EScrollWhenFocusChanges::NoScroll)
					.AnimateWheelScrolling(true)
					.AllowOverscroll(EAllowOverscroll::No)
					+ SScrollBox::Slot()
					[
						SAssignNew(AssetBrowserListView, STileView<TSharedPtr<FProjectCleanerAssetBrowserItem>>)
						.ItemWidth(100)
						.ItemHeight(166)
						.ListItemsSource(&AssetBrowserListItems)
						.SelectionMode(ESelectionMode::Multi)
						.OnGenerateTile(this, &SProjectCleanerTabUnusedAssets::OnGenerateWidgetForTileView)
						.OnContextMenuOpening(this, &SProjectCleanerTabUnusedAssets::GetAssetBrowserItemContextMenu)
					]
				]
				+ SVerticalBox::Slot()
				  .AutoHeight()
				  .HAlign(HAlign_Right)
				  .VAlign(VAlign_Center)
				  .Padding(FMargin{0.0f, 5.0f})
				[
					SNew(SComboButton)
					.ContentPadding(0)
					.ForegroundColor_Raw(this, &SProjectCleanerTabUnusedAssets::GetTreeViewOptionsBtnForegroundColor)
					.ButtonStyle(FEditorStyle::Get(), "ToggleButton")
					.OnGetMenuContent(this, &SProjectCleanerTabUnusedAssets::GetTreeViewOptionsBtnContent) // todo:ashe23 change content
					.ButtonContent()
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						  .AutoWidth()
						  .VAlign(VAlign_Center)
						[
							SNew(SImage).Image(FEditorStyle::GetBrush("GenericViewButton"))
						]
						+ SHorizontalBox::Slot()
						  .AutoWidth()
						  .Padding(2, 0, 0, 0)
						  .VAlign(VAlign_Center)
						[
							SNew(STextBlock).Text(FText::FromString(TEXT("View Options")))
						]
					]
				]
			]
		]
	];
}

void SProjectCleanerTabUnusedAssets::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	if (!AssetThumbnailPool.IsValid()) return;

	AssetThumbnailPool->Tick(InDeltaTime);
}

void SProjectCleanerTabUnusedAssets::CommandsRegister()
{
	Cmds = MakeShareable(new FUICommandList);

	FUIAction ActionPathExclude;
	ActionPathExclude.ExecuteAction = FExecuteAction::CreateLambda([&]()
	{
		if (!TreeView.IsValid()) return;
		if (!SubsystemPtr) return;

		UProjectCleanerExcludeSettings* ExcludeSettings = GetMutableDefault<UProjectCleanerExcludeSettings>();
		if (!ExcludeSettings) return;

		const auto Items = TreeView->GetSelectedItems();
		for (const auto& Item : Items)
		{
			if (!FPaths::DirectoryExists(Item->FolderPathAbs)) continue;

			ExcludeSettings->ExcludedFolders.Add(FDirectoryPath{Item->FolderPathRel});
		}

		ExcludeSettings->PostEditChange();

		SubsystemPtr->ProjectScan();
	});
	ActionPathExclude.CanExecuteAction = FCanExecuteAction::CreateLambda([&]()
	{
		return TreeView.IsValid() && TreeView.Get()->GetSelectedItems().Num() > 0;
	});

	Cmds->MapAction(FProjectCleanerCmds::Get().PathExclude, ActionPathExclude);

	FUIAction ActionAssetExclude;
	ActionAssetExclude.ExecuteAction = FExecuteAction::CreateLambda([&]()
	{
		if (!SubsystemPtr) return;
		if (!AssetBrowserListView.IsValid()) return;

		UProjectCleanerExcludeSettings* ExcludeSettings = GetMutableDefault<UProjectCleanerExcludeSettings>();
		if (!ExcludeSettings) return;

		const auto SelectedItems = AssetBrowserListView->GetSelectedItems();
		for (const auto& SelectedItem : SelectedItems)
		{
			if (!SelectedItem.IsValid()) continue;
			if (!SelectedItem->AssetData.GetAsset()) continue;

			ExcludeSettings->ExcludedAssets.AddUnique(SelectedItem->AssetData.GetAsset());
		}

		ExcludeSettings->PostEditChange();

		SubsystemPtr->ProjectScan();
	});
	ActionAssetExclude.CanExecuteAction = FCanExecuteAction::CreateLambda([&]()
	{
		return AssetBrowserListView.IsValid() && AssetBrowserListView.Get()->GetSelectedItems().Num() > 0;
	});

	Cmds->MapAction(FProjectCleanerCmds::Get().AssetExclude, ActionAssetExclude);

	FUIAction ActionAssetExcludeByType;
	ActionAssetExcludeByType.ExecuteAction = FExecuteAction::CreateLambda([&]()
	{
		if (!SubsystemPtr) return;
		if (!AssetBrowserListView.IsValid()) return;

		UProjectCleanerExcludeSettings* ExcludeSettings = GetMutableDefault<UProjectCleanerExcludeSettings>();
		if (!ExcludeSettings) return;

		const auto SelectedItems = AssetBrowserListView->GetSelectedItems();
		for (const auto& SelectedItem : SelectedItems)
		{
			if (!SelectedItem.IsValid()) continue;
			if (!SelectedItem->AssetData.GetAsset()) continue;

			const UClass* AssetClass = SubsystemPtr->GetAssetClass(SelectedItem->AssetData);
			if (!AssetClass) continue;

			ExcludeSettings->ExcludedClasses.AddUnique(AssetClass);
		}

		ExcludeSettings->PostEditChange();

		SubsystemPtr->ProjectScan();
	});
	ActionAssetExcludeByType.CanExecuteAction = FCanExecuteAction::CreateLambda([&]()
	{
		return AssetBrowserListView.IsValid() && AssetBrowserListView.Get()->GetSelectedItems().Num() > 0;
	});

	Cmds->MapAction(FProjectCleanerCmds::Get().AssetExcludeByType, ActionAssetExcludeByType);
}

TSharedRef<ITableRow> SProjectCleanerTabUnusedAssets::OnTreeViewGenerateRow(TSharedPtr<FProjectCleanerTreeViewItem> Item, const TSharedRef<STableViewBase>& OwnerTable) const
{
	return SNew(SProjectCleanerTreeViewItem, OwnerTable).TreeItem(Item);
}

void SProjectCleanerTabUnusedAssets::TreeViewItemsUpdate()
{
	if (!SubsystemPtr) return;

	if (!TreeView.IsValid())
	{
		SAssignNew(TreeView, STreeView<TSharedPtr<FProjectCleanerTreeViewItem>>)
		.TreeItemsSource(&TreeViewItems)
		.SelectionMode(ESelectionMode::Multi)
		.OnGenerateRow(this, &SProjectCleanerTabUnusedAssets::OnTreeViewGenerateRow)
		.OnGetChildren(this, &SProjectCleanerTabUnusedAssets::OnTreeViewGetChildren)
		.OnContextMenuOpening_Raw(this, &SProjectCleanerTabUnusedAssets::GetTreeViewItemContextMenu)
		.HeaderRow(GetTreeViewHeaderRow())
		.OnMouseButtonDoubleClick(this, &SProjectCleanerTabUnusedAssets::OnTreeViewItemMouseDblClick)
		.OnSelectionChanged(this, &SProjectCleanerTabUnusedAssets::OnTreeViewSelectionChange)
		.OnExpansionChanged(this, &SProjectCleanerTabUnusedAssets::OnTreeViewExpansionChange);
	}

	// caching expanded and selected items in order to keep them , when we updating data
	TreeViewItemsExpanded.Reset();
	TreeViewItemsSelected.Reset();
	TreeView->GetExpandedItems(TreeViewItemsExpanded);
	TreeView->GetSelectedItems(TreeViewItemsSelected);
	TreeView->ClearHighlightedItems();

	TreeViewItems.Reset();

	// creating root item
	const TSharedPtr<FProjectCleanerTreeViewItem> RootTreeItem = TreeViewItemCreate(FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("Content")));
	if (!RootTreeItem) return;

	// traversing and filling its child items
	TArray<TSharedPtr<FProjectCleanerTreeViewItem>> Stack;
	Stack.Push(RootTreeItem);
	TreeViewItems.Add(RootTreeItem);

	if (TreeViewItemsExpanded.Num() == 0)
	{
		TreeView->SetItemExpansion(RootTreeItem, true);
	}

	if (TreeViewItemsSelected.Num() == 0)
	{
		TreeView->SetItemSelection(RootTreeItem, true);
	}

	while (Stack.Num() > 0)
	{
		const auto CurrentItem = Stack.Pop();

		TArray<FString> SubFolders;
		IFileManager::Get().FindFiles(SubFolders, *(CurrentItem->FolderPathAbs / TEXT("*")), false, true);

		CurrentItem->SubItems.Reserve(SubFolders.Num());

		for (const auto& SubFolder : SubFolders)
		{
			const TSharedPtr<FProjectCleanerTreeViewItem> SubDirItem = TreeViewItemCreate(CurrentItem->FolderPathAbs / SubFolder);
			if (!SubDirItem.IsValid()) continue;

			CurrentItem->SubItems.Add(SubDirItem);
			Stack.Push(SubDirItem);
		}
	}

	// if (!TreeViewSearchText.IsEmpty())
	// {
	// 	TArray<int32> Indices;
	// 	for (int32 i = 0; i < TreeViewItems.Num(); ++i)
	// 	{
	// 		const int32 SearchIndex = TreeViewItems[i]->FolderName.Find(TreeViewSearchText);
	// 		if (SearchIndex == INDEX_NONE)
	// 		{
	// 			Indices.Add(i);
	// 			// TreeViewItems.RemoveSingleSwap(Item, false);
	// 			// TreeView->SetItem(TreeItem, false);
	// 		}
	// 	}
	//
	// 	for (const auto& Index : Indices)
	// 	{
	// 		if (!TreeViewItems.IsValidIndex(Index)) continue;
	// 		TreeViewItems.RemoveAtSwap(Index, 1, false);
	// 	}
	//
	// 	TreeViewItems.Shrink();
	// }

	TreeView->RequestTreeRefresh();
}

void SProjectCleanerTabUnusedAssets::OnTreeViewSearchBoxTextChanged(const FText& InSearchText)
{
	TreeViewSearchText.Reset();
	TreeViewSearchText = InSearchText.ToString();

	TreeViewItemsUpdate();
}

void SProjectCleanerTabUnusedAssets::OnTreeViewSearchBoxTextCommitted(const FText& InSearchText, ETextCommit::Type InCommitType)
{
	TreeViewSearchText.Reset();
	TreeViewSearchText = InSearchText.ToString();

	TreeViewItemsUpdate();
}

void SProjectCleanerTabUnusedAssets::OnTreeViewItemMouseDblClick(TSharedPtr<FProjectCleanerTreeViewItem> Item)
{
	if (!Item.IsValid()) return;

	TreeViewToggleExpansionRecursive(Item, !Item->bExpanded);
}

void SProjectCleanerTabUnusedAssets::OnTreeViewGetChildren(TSharedPtr<FProjectCleanerTreeViewItem> Item, TArray<TSharedPtr<FProjectCleanerTreeViewItem>>& OutChildren) const
{
	if (!Item.IsValid()) return;

	OutChildren.Append(Item->SubItems);
}

void SProjectCleanerTabUnusedAssets::OnTreeViewSelectionChange(TSharedPtr<FProjectCleanerTreeViewItem> Item, ESelectInfo::Type SelectType)
{
	if (!Item.IsValid()) return;
	if (!TreeView.IsValid()) return;

	const auto SelectedItems = TreeView->GetSelectedItems();
	SelectedPaths.Reset();
	SelectedPaths.Reserve(SelectedItems.Num());

	for (const auto& SelectedItem : SelectedItems)
	{
		if (!SelectedItem.IsValid()) continue;

		SelectedPaths.Add(SelectedItem->FolderPathRel);
	}

	AssetBrowserItemsUpdate();
}

void SProjectCleanerTabUnusedAssets::OnTreeViewExpansionChange(TSharedPtr<FProjectCleanerTreeViewItem> Item, bool bExpanded) const
{
	if (!Item.IsValid()) return;
	if (!TreeView.IsValid()) return;

	Item->bExpanded = bExpanded;

	TreeView->SetItemExpansion(Item, bExpanded);
}

void SProjectCleanerTabUnusedAssets::TreeViewToggleExpansionRecursive(TSharedPtr<FProjectCleanerTreeViewItem> Item, const bool bExpanded)
{
	if (!Item.IsValid()) return;
	if (!TreeView.IsValid()) return;

	TreeView->SetItemExpansion(Item, bExpanded);

	for (const auto& SubDir : Item->SubItems)
	{
		TreeViewToggleExpansionRecursive(SubDir, bExpanded);
	}
}

TSharedPtr<SWidget> SProjectCleanerTabUnusedAssets::GetTreeViewItemContextMenu() const
{
	FMenuBuilder MenuBuilder{true, Cmds};
	MenuBuilder.BeginSection(TEXT("Actions"), FText::FromString(TEXT("Path Actions")));
	{
		MenuBuilder.AddMenuEntry(FProjectCleanerCmds::Get().PathExclude);
	}
	MenuBuilder.EndSection();

	return MenuBuilder.MakeWidget();
}

TSharedPtr<FProjectCleanerTreeViewItem> SProjectCleanerTabUnusedAssets::TreeViewItemCreate(const FString& InFolderPathAbs) const
{
	if (!SubsystemPtr) return {};

	const TSharedPtr<FProjectCleanerTreeViewItem> TreeItem = MakeShareable(new FProjectCleanerTreeViewItem());
	if (!TreeItem.IsValid()) return {};

	const bool bIsProjectContentFolder = InFolderPathAbs.Equals(FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("Content")));
	const bool bIsProjectDeveloperFolder = InFolderPathAbs.Equals(FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("Developers")));


	TreeItem->FolderPathAbs = FPaths::ConvertRelativePathToFull(InFolderPathAbs);
	TreeItem->FolderPathRel = SubsystemPtr->PathConvertToRel(InFolderPathAbs);
	TreeItem->FolderName = bIsProjectContentFolder ? TEXT("Content") : FPaths::GetPathLeaf(InFolderPathAbs);
	TreeItem->FoldersTotal = GetFoldersTotalNum(InFolderPathAbs);
	TreeItem->FoldersEmpty = GetFoldersEmptyNum(InFolderPathAbs);
	TreeItem->AssetsTotal = GetAssetsTotalNum(InFolderPathAbs);
	TreeItem->AssetsUnused = GetAssetsUnusedNum(InFolderPathAbs);
	TreeItem->SizeTotal = GetSizeTotal(InFolderPathAbs);
	TreeItem->SizeUnused = GetSizeUnused(InFolderPathAbs);
	TreeItem->bDevFolder = bIsProjectDeveloperFolder;
	TreeItem->bEmpty = IsFolderEmpty(InFolderPathAbs);
	TreeItem->bExcluded = IsFolderExcluded(InFolderPathAbs);
	TreeItem->PercentUnused = TreeItem->AssetsTotal == 0 ? 0.0f : TreeItem->AssetsUnused * 100.0f / TreeItem->AssetsTotal;
	TreeItem->PercentUnusedNormalized = FMath::GetMappedRangeValueClamped(FVector2D{0.0f, 100.0f}, FVector2D{0.0f, 1.0f}, TreeItem->PercentUnused);
	TreeItem->bExpanded = bIsProjectContentFolder;

	// do not filter root folder
	if (!SubsystemPtr->bShowFoldersEmpty && !bIsProjectContentFolder && TreeItem->bEmpty) return {};
	if (!SubsystemPtr->bShowFoldersExcluded && !bIsProjectContentFolder && TreeItem->bExcluded) return {};

	for (const auto& ExpandedItem : TreeViewItemsExpanded)
	{
		if (ExpandedItem->FolderPathAbs.Equals(TreeItem->FolderPathAbs))
		{
			TreeItem->bExpanded = true;
			TreeView->SetItemExpansion(TreeItem, true);
			break;
		}
	}

	// todo:ashe23 add to selected paths?
	for (const auto& SelectedItem : TreeViewItemsSelected)
	{
		if (SelectedItem->FolderPathAbs.Equals(TreeItem->FolderPathAbs))
		{
			TreeView->SetItemSelection(TreeItem, true);
			TreeView->SetItemHighlighted(TreeItem, true);
			break;
		}
	}


	return TreeItem;
}

TSharedRef<SHeaderRow> SProjectCleanerTabUnusedAssets::GetTreeViewHeaderRow() const
{
	return
		SNew(SHeaderRow)
		+ SHeaderRow::Column(TEXT("Name"))
		  .HAlignHeader(HAlign_Center)
		  .VAlignHeader(VAlign_Center)
		  .HeaderContentPadding(FMargin{5.0f})
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Name")))
			.ToolTipText(FText::FromString(TEXT("Folder Name")))
			.ColorAndOpacity(FProjectCleanerStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
			.Font(FProjectCleanerStyles::GetFont("Light", ProjectCleanerConstants::HeaderRowFontSize))
		]
		+ SHeaderRow::Column(TEXT("FoldersTotal"))
		  .HAlignHeader(HAlign_Center)
		  .VAlignHeader(VAlign_Center)
		  .HeaderContentPadding(FMargin{5.0f})
		[
			SNew(STextBlock)
			.ToolTipText(FText::FromString(TEXT("Total number of folders")))
			.Text(FText::FromString(TEXT("Folders (Total)")))
			.ColorAndOpacity(FProjectCleanerStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
			.Font(FProjectCleanerStyles::GetFont("Light", ProjectCleanerConstants::HeaderRowFontSize))
		]
		+ SHeaderRow::Column(TEXT("FoldersEmpty"))
		  .HAlignHeader(HAlign_Center)
		  .VAlignHeader(VAlign_Center)
		  .HeaderContentPadding(FMargin{5.0f})
		[
			SNew(STextBlock)
			.ToolTipText(FText::FromString(TEXT("Total number of empty folders")))
			.Text(FText::FromString(TEXT("Folders (Empty)")))
			.ColorAndOpacity(FProjectCleanerStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
			.Font(FProjectCleanerStyles::GetFont("Light", ProjectCleanerConstants::HeaderRowFontSize))
		]
		+ SHeaderRow::Column(TEXT("AssetsTotal"))
		  .HAlignHeader(HAlign_Center)
		  .VAlignHeader(VAlign_Center)
		  .HeaderContentPadding(FMargin{5.0f})
		[
			SNew(STextBlock)
			.ToolTipText(FText::FromString(TEXT("Total number of assets")))
			.Text(FText::FromString(TEXT("Assets (Total)")))
			.ColorAndOpacity(FProjectCleanerStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
			.Font(FProjectCleanerStyles::GetFont("Light", ProjectCleanerConstants::HeaderRowFontSize))
		]
		+ SHeaderRow::Column(TEXT("AssetsUnused"))
		  .HAlignHeader(HAlign_Center)
		  .VAlignHeader(VAlign_Center)
		  .HeaderContentPadding(FMargin{5.0f})
		[
			SNew(STextBlock)
			.ToolTipText(FText::FromString(TEXT("Total number of unused assets")))
			.Text(FText::FromString(TEXT("Assets (Unused)")))
			.ColorAndOpacity(FProjectCleanerStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
			.Font(FProjectCleanerStyles::GetFont("Light", ProjectCleanerConstants::HeaderRowFontSize))
		]
		+ SHeaderRow::Column(TEXT("SizeTotal"))
		  .HAlignHeader(HAlign_Center)
		  .VAlignHeader(VAlign_Center)
		  .HeaderContentPadding(FMargin{5.0f})
		[
			SNew(STextBlock)
			.ToolTipText(FText::FromString(TEXT("Total size of all assets")))
			.Text(FText::FromString(TEXT("Size (Total)")))
			.ColorAndOpacity(FProjectCleanerStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
			.Font(FProjectCleanerStyles::GetFont("Light", ProjectCleanerConstants::HeaderRowFontSize))
		]
		+ SHeaderRow::Column(TEXT("SizeUnused"))
		  .HAlignHeader(HAlign_Center)
		  .VAlignHeader(VAlign_Center)
		  .HeaderContentPadding(FMargin{5.0f})
		[
			SNew(STextBlock)
			.ToolTipText(FText::FromString(TEXT("Total size of unused assets")))
			.Text(FText::FromString(TEXT("Size (Unused)")))
			.ColorAndOpacity(FProjectCleanerStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
			.Font(FProjectCleanerStyles::GetFont("Light", ProjectCleanerConstants::HeaderRowFontSize))
		]
		+ SHeaderRow::Column(TEXT("Percent"))
		  .HAlignHeader(HAlign_Center)
		  .VAlignHeader(VAlign_Center)
		  .HeaderContentPadding(FMargin{5.0f})
		[
			SNew(STextBlock)
			.ToolTipText(FText::FromString(TEXT("Percent of unused number relative to total number of assets in folder")))
			.Text(FText::FromString(TEXT("% of Unused")))
			.ColorAndOpacity(FProjectCleanerStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
			.Font(FProjectCleanerStyles::GetFont("Light", ProjectCleanerConstants::HeaderRowFontSize))
		];
}

TSharedRef<ITableRow> SProjectCleanerTabUnusedAssets::OnGenerateWidgetForTileView(TSharedPtr<FProjectCleanerAssetBrowserItem> InItem, const TSharedRef<STableViewBase>& OwnerTable) const
{
	const TSharedPtr<FAssetThumbnail> AssetThumbnail = MakeShareable(new FAssetThumbnail(InItem->AssetData, 90, 90, AssetThumbnailPool));
	FAssetThumbnailConfig ThumbnailConfig;
	ThumbnailConfig.bAllowFadeIn = true;

	return
		SNew(STableRow<TSharedPtr<FProjectCleanerAssetBrowserItem>>, OwnerTable)
		.Style(FEditorStyle::Get(), "ContentBrowser.AssetListView.TableRow")
		.Padding(FMargin{5.0f})
		[
			SNew(SBorder)
			// .Padding(4.0f)
			.BorderImage(FEditorStyle::GetBrush("ContentBrowser.ThumbnailShadow"))
			// .BorderBackgroundColor(FLinearColor::Gray)
			.ColorAndOpacity(FLinearColor{1.0f, 1.0f, 1.0f, 1.0f})
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				  .AutoHeight()
				  .HAlign(HAlign_Center)
				[
					SNew(SBox)
					.Padding(0)
					.WidthOverride(90)
					.HeightOverride(90)
					[
						AssetThumbnail->MakeThumbnailWidget(ThumbnailConfig)
					]
				]
				+ SVerticalBox::Slot()
				  .FillHeight(1.0f)
				  .HAlign(HAlign_Fill)
				  .VAlign(VAlign_Fill)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					  .FillWidth(1.0f)
					  .HAlign(HAlign_Fill)
					  .VAlign(VAlign_Fill)
					  .Padding(FMargin{3.0f, 2.0f})
					[
						SNew(STextBlock)
						.WrapTextAt(100.0f)
						.LineBreakPolicy(FBreakIterator::CreateCamelCaseBreakIterator())
						.Font(FProjectCleanerStyles::GetFont("Light", 10))
						.Text(FText::FromString(InItem->AssetData.AssetName.ToString()))
					]
				]
				+ SVerticalBox::Slot()
				  .FillHeight(1.0f)
				  .HAlign(HAlign_Fill)
				  .VAlign(VAlign_Bottom)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					  .FillWidth(1.0f)
					  .HAlign(HAlign_Left)
					  .VAlign(VAlign_Center)
					  .Padding(FMargin{3.0f, 2.0f})
					[
						SNew(STextBlock)
						.Font(FProjectCleanerStyles::GetFont("Light", 8))
						.Text(FText::FromString(InItem->AssetData.AssetClass.ToString()))
					]
					// + SHorizontalBox::Slot()
					//   .AutoWidth()
					//   .HAlign(HAlign_Center)
					//   .VAlign(VAlign_Center)
					//   .Padding(FMargin{3.0f, 2.0f})
					// [
					// 	SNew(SBox)
					// 	.WidthOverride(16.0f)
					// 	.HeightOverride(16.0f)
					// 	[
					// 		SNew(SImage)
					// 		.Image(FProjectCleanerStyles::Get().GetBrush("ProjectCleaner.IconCircle16"))
					// 		.ColorAndOpacity(FProjectCleanerStyles::Get().GetColor("ProjectCleaner.Color.Yellow"))
					// 	]
					// ]
				]
			]
		];
}

TSharedPtr<SWidget> SProjectCleanerTabUnusedAssets::GetAssetBrowserItemContextMenu() const
{
	FMenuBuilder MenuBuilder{true, Cmds};
	MenuBuilder.BeginSection(TEXT("Actions"), FText::FromString(TEXT("Asset Actions")));
	{
		MenuBuilder.AddMenuEntry(FProjectCleanerCmds::Get().AssetExclude);
		MenuBuilder.AddMenuEntry(FProjectCleanerCmds::Get().AssetExcludeByType);
	}
	MenuBuilder.EndSection();

	return MenuBuilder.MakeWidget();
}

void SProjectCleanerTabUnusedAssets::AssetBrowserItemsUpdate()
{
	if (!SubsystemPtr) return;

	AssetBrowserListItems.Reset();
	AssetBrowserListItems.Reserve(SubsystemPtr->GetAssetsUnused().Num());

	for (const auto& Asset : SubsystemPtr->GetAssetsUnused())
	{
		// if (!IsUnderSelectedPaths(Asset.PackagePath.ToString())) continue;

		TSharedPtr<FProjectCleanerAssetBrowserItem> NewItem = MakeShareable(new FProjectCleanerAssetBrowserItem);
		if (!NewItem.IsValid()) continue;

		NewItem->AssetData = Asset;

		AssetBrowserListItems.Add(NewItem);
	}

	if (AssetBrowserListView.IsValid())
	{
		AssetBrowserListView->RequestListRefresh();
	}
}

// TSharedRef<SWidget> SProjectCleanerTabUnusedAssets::AssetBrowserMakeFilterMenu()
// {
// }

bool SProjectCleanerTabUnusedAssets::IsUnderSelectedPaths(const FString& InFolderRel) const
{
	for (const auto& SelectedPath : SelectedPaths)
	{
		if (InFolderRel.StartsWith(SelectedPath))
		{
			return true;
		}
	}

	return false;
}

bool SProjectCleanerTabUnusedAssets::IsFolderEmpty(const FString& InFolderPath) const
{
	return SubsystemPtr->GetFoldersEmpty().Contains(InFolderPath);
}

bool SProjectCleanerTabUnusedAssets::IsFolderExcluded(const FString& InFolderPath) const
{
	for (const auto& ExcludedFolder : GetDefault<UProjectCleanerExcludeSettings>()->ExcludedFolders)
	{
		const FString ExcludedFolderPathAbs = SubsystemPtr->PathConvertToAbs(ExcludedFolder.Path);

		if (FPaths::IsUnderDirectory(InFolderPath, ExcludedFolderPathAbs))
		{
			return true;
		}
	}

	return false;
}

int32 SProjectCleanerTabUnusedAssets::GetFoldersTotalNum(const FString& InFolderPath) const
{
	int32 Num = 0;

	for (const auto& Folder : SubsystemPtr->GetFoldersTotal())
	{
		if (!SubsystemPtr->bShowFoldersEmpty && IsFolderEmpty(Folder)) continue;
		if (!SubsystemPtr->bShowFoldersExcluded && IsFolderExcluded(Folder)) continue;
		if (Folder.Equals(InFolderPath)) continue;
		if (FPaths::IsUnderDirectory(Folder, InFolderPath))
		{
			++Num;
		}
	}

	return Num;
}

int32 SProjectCleanerTabUnusedAssets::GetFoldersEmptyNum(const FString& InFolderPath) const
{
	if (!SubsystemPtr->bShowFoldersEmpty) return 0;

	int32 Num = 0;

	for (const auto& Folder : SubsystemPtr->GetFoldersEmpty())
	{
		if (FPaths::IsUnderDirectory(Folder, InFolderPath) && !Folder.Equals(InFolderPath))
		{
			++Num;
		}
	}

	return Num;
}

int32 SProjectCleanerTabUnusedAssets::GetAssetsTotalNum(const FString& InFolderPath) const
{
	int32 Num = 0;

	for (const auto& Asset : SubsystemPtr->GetAssetsAll())
	{
		const FString AssetPathAbs = SubsystemPtr->PathConvertToAbs(Asset.PackagePath.ToString());
		if (FPaths::IsUnderDirectory(AssetPathAbs, InFolderPath))
		{
			++Num;
		}
	}

	return Num;
}

int32 SProjectCleanerTabUnusedAssets::GetAssetsUnusedNum(const FString& InFolderPath) const
{
	int32 Num = 0;

	for (const auto& Asset : SubsystemPtr->GetAssetsUnused())
	{
		const FString AssetPathAbs = SubsystemPtr->PathConvertToAbs(Asset.PackagePath.ToString());
		if (FPaths::IsUnderDirectory(AssetPathAbs, InFolderPath))
		{
			++Num;
		}
	}

	return Num;
}

int64 SProjectCleanerTabUnusedAssets::GetSizeTotal(const FString& InFolderPath) const
{
	TArray<FAssetData> FilteredAssets;
	FilteredAssets.Reserve(SubsystemPtr->GetAssetsAll().Num());

	for (const auto& Asset : SubsystemPtr->GetAssetsAll())
	{
		const FString AssetPathAbs = SubsystemPtr->PathConvertToAbs(Asset.PackagePath.ToString());
		if (FPaths::IsUnderDirectory(AssetPathAbs, InFolderPath))
		{
			FilteredAssets.Add(Asset);
		}
	}

	FilteredAssets.Shrink();

	return SubsystemPtr->GetAssetsTotalSize(FilteredAssets);
}

int64 SProjectCleanerTabUnusedAssets::GetSizeUnused(const FString& InFolderPath) const
{
	TArray<FAssetData> FilteredAssets;
	FilteredAssets.Reserve(SubsystemPtr->GetAssetsUnused().Num());

	for (const auto& Asset : SubsystemPtr->GetAssetsUnused())
	{
		const FString AssetPathAbs = SubsystemPtr->PathConvertToAbs(Asset.PackagePath.ToString());
		if (FPaths::IsUnderDirectory(AssetPathAbs, InFolderPath))
		{
			FilteredAssets.Add(Asset);
		}
	}

	FilteredAssets.Shrink();

	return SubsystemPtr->GetAssetsTotalSize(FilteredAssets);
}

FSlateColor SProjectCleanerTabUnusedAssets::GetTreeViewOptionsBtnForegroundColor() const
{
	static const FName InvertedForegroundName("InvertedForeground");
	static const FName DefaultForegroundName("DefaultForeground");

	if (!TreeViewOptionsComboButton.IsValid()) return FEditorStyle::GetSlateColor(DefaultForegroundName);

	return TreeViewOptionsComboButton->IsHovered() ? FEditorStyle::GetSlateColor(InvertedForegroundName) : FEditorStyle::GetSlateColor(DefaultForegroundName);
}

TSharedRef<SWidget> SProjectCleanerTabUnusedAssets::GetTreeViewOptionsBtnContent()
{
	const TSharedPtr<FExtender> Extender;
	FMenuBuilder MenuBuilder(true, nullptr, Extender, true);

	FUIAction ActionShowLines;
	ActionShowLines.ExecuteAction = FExecuteAction::CreateLambda([&]()
	{
		if (!SubsystemPtr) return;

		SubsystemPtr->bShowTreeViewLines = !SubsystemPtr->bShowTreeViewLines;
		SubsystemPtr->PostEditChange();

		TreeViewItemsUpdate();
	});
	ActionShowLines.CanExecuteAction = FCanExecuteAction::CreateLambda([&]()
	{
		return SubsystemPtr != nullptr;
	});
	ActionShowLines.GetActionCheckState = FGetActionCheckState::CreateLambda([&]()
	{
		return SubsystemPtr && SubsystemPtr->bShowTreeViewLines ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
	});
	MenuBuilder.AddMenuSeparator(TEXT("View"));
	MenuBuilder.AddMenuEntry(
		FText::FromString(TEXT("Show Lines")),
		FText::FromString(TEXT("Show tree view organizer lines")),
		FSlateIcon(),
		ActionShowLines,
		NAME_None,
		EUserInterfaceActionType::ToggleButton
	);

	FUIAction ActionShowEmptyFolders;
	ActionShowEmptyFolders.ExecuteAction = FExecuteAction::CreateLambda([&]()
	{
		if (!SubsystemPtr) return;

		SubsystemPtr->bShowFoldersEmpty = !SubsystemPtr->bShowFoldersEmpty;
		SubsystemPtr->PostEditChange();

		TreeViewItemsUpdate();
	});
	ActionShowEmptyFolders.CanExecuteAction = FCanExecuteAction::CreateLambda([&]()
	{
		return SubsystemPtr != nullptr;
	});
	ActionShowEmptyFolders.GetActionCheckState = FGetActionCheckState::CreateLambda([&]()
	{
		return SubsystemPtr && SubsystemPtr->bShowFoldersEmpty ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
	});
	MenuBuilder.AddSeparator();
	MenuBuilder.AddMenuEntry(
		FText::FromString(TEXT("Show Empty Folders")),
		FText::FromString(TEXT("Show Empty Folders")),
		FSlateIcon(),
		ActionShowEmptyFolders,
		NAME_None,
		EUserInterfaceActionType::ToggleButton
	);

	FUIAction ActionShowExcludedFolders;
	ActionShowExcludedFolders.ExecuteAction = FExecuteAction::CreateLambda([&]()
	{
		SubsystemPtr->bShowFoldersExcluded = !SubsystemPtr->bShowFoldersExcluded;
		SubsystemPtr->PostEditChange();

		TreeViewItemsUpdate();
	});
	ActionShowExcludedFolders.CanExecuteAction = FCanExecuteAction::CreateLambda([&]()
	{
		return SubsystemPtr != nullptr;
	});
	ActionShowExcludedFolders.GetActionCheckState = FGetActionCheckState::CreateLambda([&]()
	{
		return SubsystemPtr && SubsystemPtr->bShowFoldersExcluded ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
	});
	MenuBuilder.AddMenuEntry(
		FText::FromString(TEXT("Show Excluded Folders")),
		FText::FromString(TEXT("Show Excluded Folders")),
		FSlateIcon(),
		ActionShowExcludedFolders,
		NAME_None,
		EUserInterfaceActionType::ToggleButton
	);

	FUIAction ActionClearSelection;
	ActionClearSelection.ExecuteAction = FExecuteAction::CreateLambda([&]()
	{
		if (!TreeView.IsValid()) return;

		TreeView->ClearSelection();
		TreeView->ClearHighlightedItems();
	});
	ActionClearSelection.CanExecuteAction = FCanExecuteAction::CreateLambda([&]()
	{
		return TreeView.IsValid() && TreeView->GetSelectedItems().Num() > 0;
	});
	MenuBuilder.AddSeparator();
	MenuBuilder.AddMenuEntry(
		FText::FromString(TEXT("Clear Selection")),
		FText::FromString(TEXT("Clear current tree view selection")),
		FSlateIcon(),
		ActionClearSelection,
		NAME_None,
		EUserInterfaceActionType::Button
	);

	return MenuBuilder.MakeWidget();
}
