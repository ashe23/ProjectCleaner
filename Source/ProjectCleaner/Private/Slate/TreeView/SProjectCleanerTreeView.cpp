// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/TreeView/SProjectCleanerTreeView.h"
#include "Slate/TreeView/SProjectCleanerTreeViewItem.h"
#include "Settings/ProjectCleanerExcludeSettings.h"
#include "ProjectCleanerCmds.h"
#include "ProjectCleanerStyles.h"
#include "ProjectCleanerSubsystem.h"
#include "ProjectCleanerTypes.h"
// Engine Headers
#include "ProjectCleanerConstants.h"
#include "Libs/ProjectCleanerLibAsset.h"
#include "Libs/ProjectCleanerLibPath.h"
#include "Settings/ProjectCleanerSettings.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Layout/SScrollBox.h"

void SProjectCleanerTreeView::Construct(const FArguments& InArgs)
{
	this->OnPathSelected = InArgs._OnPathSelected;

	if (!GEditor) return;
	SubsystemPtr = GEditor->GetEditorSubsystem<UProjectCleanerSubsystem>();
	if (!SubsystemPtr) return;

	CommandsRegister();
	ItemsUpdate();

	ChildSlot
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
			.OnTextChanged(this, &SProjectCleanerTreeView::OnSearchBoxTextChanged)
			.OnTextCommitted(this, &SProjectCleanerTreeView::OnSearchBoxTextCommitted)
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
			.ForegroundColor_Raw(this, &SProjectCleanerTreeView::GetOptionsBtnForegroundColor)
			.ButtonStyle(FEditorStyle::Get(), "ToggleButton")
			.OnGetMenuContent(this, &SProjectCleanerTreeView::GetOptionsBtnContent)
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
	];

	SubsystemPtr->OnProjectScanned().AddRaw(this, &SProjectCleanerTreeView::OnProjectScanned);
}

SProjectCleanerTreeView::~SProjectCleanerTreeView()
{
	SubsystemPtr->OnProjectScanned().RemoveAll(this);
	SubsystemPtr = nullptr;
}

void SProjectCleanerTreeView::CommandsRegister()
{
	Cmds = MakeShareable(new FUICommandList);
	Cmds->MapAction
	(
		FProjectCleanerCmds::Get().PathExclude,
		FUIAction
		(
			FExecuteAction::CreateLambda([&]()
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
			}),
			FCanExecuteAction::CreateLambda([&]
			{
				return TreeView.IsValid() && TreeView.Get()->GetSelectedItems().Num() > 0;
			}),
			FIsActionChecked::CreateLambda([] { return true; }),
			FIsActionButtonVisible::CreateLambda([&]() { return true; })
		)
	);
	Cmds->MapAction
	(
		FProjectCleanerCmds::Get().PathShowInExplorer,
		FUIAction
		(
			FExecuteAction::CreateLambda([&]()
			{
				if (!TreeView.IsValid()) return;

				const auto Items = TreeView->GetSelectedItems();
				for (const auto& Item : Items)
				{
					if (!FPaths::DirectoryExists(Item->FolderPathAbs)) continue;

					FPlatformProcess::ExploreFolder(*Item->FolderPathAbs);
				}
			}),
			FCanExecuteAction::CreateLambda([&]
			{
				return TreeView.IsValid() && TreeView.Get()->GetSelectedItems().Num() > 0;
			}),
			FIsActionChecked::CreateLambda([] { return true; }),
			FIsActionButtonVisible::CreateLambda([&]() { return true; })
		)
	);
}

void SProjectCleanerTreeView::OnProjectScanned()
{
	ItemsUpdate();
}

void SProjectCleanerTreeView::ItemsUpdate()
{
	if (!SubsystemPtr) return;

	Items.Reset();

	if (!TreeView.IsValid())
	{
		SAssignNew(TreeView, STreeView<TSharedPtr<FProjectCleanerTreeViewItem>>)
		.TreeItemsSource(&Items)
		.SelectionMode(ESelectionMode::Multi)
		.OnGenerateRow(this, &SProjectCleanerTreeView::OnGenerateRow)
		.OnGetChildren(this, &SProjectCleanerTreeView::OnGetChildren)
		.OnContextMenuOpening_Raw(this, &SProjectCleanerTreeView::GetItemContextMenu)
		.HeaderRow(GetHeaderRow())
		.OnMouseButtonDoubleClick(this, &SProjectCleanerTreeView::OnItemMouseDblClick)
		.OnSelectionChanged(this, &SProjectCleanerTreeView::OnSelectionChange)
		.OnExpansionChanged(this, &SProjectCleanerTreeView::OnExpansionChange);
	}

	// caching expanded and selected items in order to keep them , when we updating data
	ItemsExpanded.Reset();
	ItemsSelected.Reset();
	TreeView->GetExpandedItems(ItemsExpanded);
	TreeView->GetSelectedItems(ItemsSelected);
	TreeView->ClearHighlightedItems();

	const TSharedPtr<FProjectCleanerTreeViewItem> RootItem = ItemCreate(FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("Content")));
	if (!RootItem) return;

	// traversing and filling its child items
	TArray<TSharedPtr<FProjectCleanerTreeViewItem>> Temp;
	TArray<TSharedPtr<FProjectCleanerTreeViewItem>> Stack;
	Stack.Push(RootItem);
	Items.AddUnique(RootItem);

	if (ItemsExpanded.Num() == 0)
	{
		TreeView->SetItemExpansion(RootItem, true);
	}

	if (ItemsSelected.Num() == 0)
	{
		TreeView->SetItemSelection(RootItem, true);
	}

	while (Stack.Num() > 0)
	{
		const auto CurrentItem = Stack.Pop();

		TArray<FString> SubFolders;
		IFileManager::Get().FindFiles(SubFolders, *(CurrentItem->FolderPathAbs / TEXT("*")), false, true);

		// TArray<FString> SubFoldersAll;
		// IFileManager::Get().FindFilesRecursive(SubFoldersAll, *CurrentItem->FolderPathAbs, TEXT("*.*"),false, true);

		// if (!SearchText.IsEmpty() && !SubFoldersAll.Contains(SearchText)) continue;

		CurrentItem->SubItems.Reserve(SubFolders.Num());

		for (const auto& SubFolder : SubFolders)
		{
			const TSharedPtr<FProjectCleanerTreeViewItem> SubDirItem = ItemCreate(CurrentItem->FolderPathAbs / SubFolder);
			if (!SubDirItem.IsValid()) continue;

			CurrentItem->SubItems.Add(SubDirItem);
			SubDirItem->Parent = CurrentItem;
			Temp.Add(SubDirItem);
			Stack.Push(SubDirItem);
		}
	}


	TreeView->RequestTreeRefresh();
}

TSharedPtr<FProjectCleanerTreeViewItem> SProjectCleanerTreeView::ItemCreate(const FString& InFolderPathAbs) const
{
	// if (!SubsystemPtr) return {};
	// if (!SubsystemPtr->CanShowFolder(InFolderPathAbs)) return {};
	//
	TSharedPtr<FProjectCleanerTreeViewItem> TreeItem = MakeShareable(new FProjectCleanerTreeViewItem());
	if (!TreeItem.IsValid()) return {};
	//
	// const bool bIsProjectContentFolder = InFolderPathAbs.Equals(FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("Content")));
	// const bool bIsProjectDeveloperFolder = InFolderPathAbs.Equals(FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() / TEXT("Developers")));
	//
	// TreeItem->FolderPathAbs = SubsystemPtr->PathConvertToAbs(InFolderPathAbs);
	// TreeItem->FolderPathRel = SubsystemPtr->PathConvertToRel(TreeItem->FolderPathAbs);
	// TreeItem->FolderName = bIsProjectContentFolder ? TEXT("Content") : FPaths::GetPathLeaf(InFolderPathAbs);
	// TreeItem->FoldersTotal = GetFoldersTotalNum(*TreeItem.Get());
	// TreeItem->FoldersEmpty = GetFoldersEmptyNum(*TreeItem.Get());
	// TreeItem->AssetsTotal = GetAssetsTotalNum(*TreeItem.Get());
	// TreeItem->AssetsUnused = GetAssetsUnusedNum(*TreeItem.Get());
	// TreeItem->SizeTotal = GetSizeTotal(*TreeItem.Get());
	// TreeItem->SizeUnused = GetSizeUnused(*TreeItem.Get());
	// TreeItem->bDevFolder = bIsProjectDeveloperFolder;
	// TreeItem->bExpanded = bIsProjectContentFolder;
	// TreeItem->bEmpty = SubsystemPtr->FolderIsEmpty(TreeItem->FolderPathAbs);
	// TreeItem->bExcluded = SubsystemPtr->FolderIsExcluded(InFolderPathAbs);
	// TreeItem->PercentUnused = TreeItem->AssetsTotal == 0 ? 0.0f : TreeItem->AssetsUnused * 100.0f / TreeItem->AssetsTotal;
	// TreeItem->PercentUnusedNormalized = FMath::GetMappedRangeValueClamped(FVector2D{0.0f, 100.0f}, FVector2D{0.0f, 1.0f}, TreeItem->PercentUnused);
	//
	// // do not filter root folder
	// if (!SubsystemPtr->bShowFoldersEmpty && !bIsProjectContentFolder && TreeItem->bEmpty) return {};
	// if (!SubsystemPtr->bShowFoldersExcluded && !bIsProjectContentFolder && TreeItem->bExcluded) return {};
	//
	// for (const auto& ExpandedItem : ItemsExpanded)
	// {
	// 	if (ExpandedItem->FolderPathAbs.Equals(TreeItem->FolderPathAbs))
	// 	{
	// 		TreeItem->bExpanded = true;
	// 		TreeView->SetItemExpansion(TreeItem, true);
	// 		break;
	// 	}
	// }
	//
	// for (const auto& SelectedItem : ItemsSelected)
	// {
	// 	if (SelectedItem->FolderPathAbs.Equals(TreeItem->FolderPathAbs))
	// 	{
	// 		TreeView->SetItemSelection(TreeItem, true);
	// 		TreeView->SetItemHighlighted(TreeItem, true);
	// 		break;
	// 	}
	// }

	return TreeItem;
}

TSharedRef<ITableRow> SProjectCleanerTreeView::OnGenerateRow(TSharedPtr<FProjectCleanerTreeViewItem> Item, const TSharedRef<STableViewBase>& OwnerTable) const
{
	return SNew(SProjectCleanerTreeViewItem, OwnerTable).TreeItem(Item).SearchText(SearchText);
}

TSharedRef<SHeaderRow> SProjectCleanerTreeView::GetHeaderRow() const
{
	return
		SNew(SHeaderRow)
		+ SHeaderRow::Column(TEXT("Name"))
		  .HAlignHeader(HAlign_Center)
		  .VAlignHeader(VAlign_Center)
		  .HeaderContentPadding(FMargin{5.0f})
		  .ManualWidth(300.0f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Name")))
			.ToolTipText(FText::FromString(TEXT("Folder Name")))
			.ColorAndOpacity(FProjectCleanerStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
			.Font(FProjectCleanerStyles::GetFont("Light", ProjectCleanerConstants::HeaderRowFontSize))
		]
		+ SHeaderRow::Column(TEXT("Percent"))
		  .HAlignHeader(HAlign_Center)
		  .VAlignHeader(VAlign_Center)
		  .HeaderContentPadding(FMargin{5.0f})
		  .ManualWidth(200.0f)
		[
			SNew(STextBlock)
			.ToolTipText(FText::FromString(TEXT("Percent of unused number relative to total number of assets in folder")))
			.Text(FText::FromString(TEXT("% of Unused")))
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
		];
}

TSharedPtr<SWidget> SProjectCleanerTreeView::GetItemContextMenu() const
{
	FMenuBuilder MenuBuilder{true, Cmds};
	MenuBuilder.BeginSection(TEXT("Actions"), FText::FromString(TEXT("Path Actions")));
	{
		MenuBuilder.AddMenuEntry(FProjectCleanerCmds::Get().PathExclude);
		MenuBuilder.AddMenuEntry(FProjectCleanerCmds::Get().PathShowInExplorer);
	}
	MenuBuilder.EndSection();

	return MenuBuilder.MakeWidget();
}

FSlateColor SProjectCleanerTreeView::GetOptionsBtnForegroundColor() const
{
	static const FName InvertedForegroundName("InvertedForeground");
	static const FName DefaultForegroundName("DefaultForeground");

	if (!OptionsComboButton.IsValid()) return FEditorStyle::GetSlateColor(DefaultForegroundName);

	return OptionsComboButton->IsHovered() ? FEditorStyle::GetSlateColor(InvertedForegroundName) : FEditorStyle::GetSlateColor(DefaultForegroundName);
}

TSharedRef<SWidget> SProjectCleanerTreeView::GetOptionsBtnContent()
{
	const TSharedPtr<FExtender> Extender;
	FMenuBuilder MenuBuilder(true, nullptr, Extender, true);

	MenuBuilder.AddMenuSeparator(TEXT("View"));
	MenuBuilder.AddMenuEntry(
		FText::FromString(TEXT("Show Lines")),
		FText::FromString(TEXT("Show tree view organizer lines")),
		FSlateIcon(),
		FUIAction
		(
			FExecuteAction::CreateLambda([&]
			{
				GetMutableDefault<UProjectCleanerSettings>()->ToggleShowTreeViewLines();
				GetMutableDefault<UProjectCleanerSettings>()->PostEditChange();

				ItemsUpdate();
			}),
			FCanExecuteAction::CreateLambda([&]()
			{
				return GetDefault<UProjectCleanerSettings>() != nullptr;
			}),
			FGetActionCheckState::CreateLambda([&]()
			{
				return GetDefault<UProjectCleanerSettings>()->bShowTreeViewLines ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
			})
		),
		NAME_None,
		EUserInterfaceActionType::ToggleButton
	);

	MenuBuilder.AddSeparator();

	MenuBuilder.AddMenuEntry(
		FText::FromString(TEXT("Show Folders Empty")),
		FText::FromString(TEXT("Show empty folders in tree view")),
		FSlateIcon(),
		FUIAction
		(
			FExecuteAction::CreateLambda([&]
			{
				GetMutableDefault<UProjectCleanerSettings>()->ToggleShowTreeViewFoldersEmpty();
				GetMutableDefault<UProjectCleanerSettings>()->PostEditChange();

				ItemsUpdate();
			}),
			FCanExecuteAction::CreateLambda([&]()
			{
				return GetDefault<UProjectCleanerSettings>() != nullptr;
			}),
			FGetActionCheckState::CreateLambda([&]()
			{
				return GetDefault<UProjectCleanerSettings>()->bShowTreeViewFoldersEmpty ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
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
				GetMutableDefault<UProjectCleanerSettings>()->ToggleShowTreeViewFoldersExcluded();
				GetMutableDefault<UProjectCleanerSettings>()->PostEditChange();

				ItemsUpdate();
			}),
			FCanExecuteAction::CreateLambda([&]()
			{
				return GetDefault<UProjectCleanerSettings>() != nullptr;
			}),
			FGetActionCheckState::CreateLambda([&]()
			{
				return GetDefault<UProjectCleanerSettings>()->bShowTreeViewFoldersExcluded ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
			})
		),
		NAME_None,
		EUserInterfaceActionType::ToggleButton
	);

	MenuBuilder.AddSeparator();

	MenuBuilder.AddMenuEntry(
		FText::FromString(TEXT("Clear Selection")),
		FText::FromString(TEXT("Clear current tree view selection")),
		FSlateIcon(),
		FUIAction
		(
			FExecuteAction::CreateLambda([&]
			{
				if (!TreeView.IsValid()) return;

				TreeView->ClearSelection();
				TreeView->ClearHighlightedItems();
			}),
			FCanExecuteAction::CreateLambda([&]()
			{
				return TreeView.IsValid() && TreeView->GetSelectedItems().Num() > 0;
			})
		),
		NAME_None,
		EUserInterfaceActionType::Button
	);

	return MenuBuilder.MakeWidget();
}

void SProjectCleanerTreeView::OnSearchBoxTextChanged(const FText& InSearchText)
{
	SearchText = InSearchText.ToString();

	ItemsUpdate();
}

void SProjectCleanerTreeView::OnSearchBoxTextCommitted(const FText& InSearchText, ETextCommit::Type InCommitType)
{
	SearchText = InSearchText.ToString();

	ItemsUpdate();
}

void SProjectCleanerTreeView::OnItemMouseDblClick(TSharedPtr<FProjectCleanerTreeViewItem> Item)
{
	if (!Item.IsValid()) return;
	if (Item->SubItems.Num() == 0) return;

	ToggleExpansionRecursive(Item, !Item->bExpanded);
}

void SProjectCleanerTreeView::OnGetChildren(TSharedPtr<FProjectCleanerTreeViewItem> Item, TArray<TSharedPtr<FProjectCleanerTreeViewItem>>& OutChildren) const
{
	if (!Item.IsValid()) return;

	OutChildren.Append(Item->SubItems);
}

void SProjectCleanerTreeView::OnSelectionChange(TSharedPtr<FProjectCleanerTreeViewItem> Item, ESelectInfo::Type SelectType)
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

	if (OnPathSelected.IsBound())
	{
		OnPathSelected.Execute(SelectedPaths);
	}
}

void SProjectCleanerTreeView::OnExpansionChange(TSharedPtr<FProjectCleanerTreeViewItem> Item, bool bExpanded) const
{
	if (!Item.IsValid()) return;
	if (!TreeView.IsValid()) return;

	Item->bExpanded = bExpanded;

	TreeView->SetItemExpansion(Item, bExpanded);
}

void SProjectCleanerTreeView::ToggleExpansionRecursive(TSharedPtr<FProjectCleanerTreeViewItem> Item, const bool bExpanded)
{
	if (!Item.IsValid()) return;
	if (!TreeView.IsValid()) return;

	TreeView->SetItemExpansion(Item, bExpanded);

	for (const auto& SubDir : Item->SubItems)
	{
		ToggleExpansionRecursive(SubDir, bExpanded);
	}
}

int32 SProjectCleanerTreeView::GetFoldersTotalNum(const FProjectCleanerTreeViewItem& Item) const
{
	if (!SubsystemPtr) return 0;

	int32 Num = 0;

	// for (const auto& Folder : SubsystemPtr->GetScanData().FoldersAll)
	// {
	// 	if (!SubsystemPtr->bShowFoldersEmpty && SubsystemPtr->FolderIsEmpty(Folder)) continue;
	// 	if (!SubsystemPtr->bShowFoldersExcluded && SubsystemPtr->FolderIsExcluded(Folder)) continue;
	// 	if (Folder.Equals(Item.FolderPathAbs)) continue;
	// 	if (FPaths::IsUnderDirectory(Folder, Item.FolderPathAbs))
	// 	{
	// 		++Num;
	// 	}
	// }

	return Num;
}

int32 SProjectCleanerTreeView::GetFoldersEmptyNum(const FProjectCleanerTreeViewItem& Item) const
{
	if (!GetDefault<UProjectCleanerSettings>()->bShowTreeViewFoldersEmpty) return 0;

	int32 Num = 0;

	for (const auto& Folder : SubsystemPtr->GetScanData().FoldersEmpty)
	{
		if (FPaths::IsUnderDirectory(Folder, Item.FolderPathAbs) && !Folder.Equals(Item.FolderPathAbs))
		{
			++Num;
		}
	}

	return Num;
}

int32 SProjectCleanerTreeView::GetAssetsTotalNum(const FProjectCleanerTreeViewItem& Item) const
{
	int32 Num = 0;

	for (const auto& Asset : SubsystemPtr->GetScanData().AssetsAll)
	{
		const FString AssetPathAbs = UProjectCleanerLibPath::ConvertToAbs(Asset.PackagePath.ToString());
		if (!AssetPathAbs.IsEmpty() && FPaths::IsUnderDirectory(AssetPathAbs, Item.FolderPathAbs))
		{
			++Num;
		}
	}

	return Num;
}

int32 SProjectCleanerTreeView::GetAssetsUnusedNum(const FProjectCleanerTreeViewItem& Item) const
{
	int32 Num = 0;

	for (const auto& Asset : SubsystemPtr->GetScanData().AssetsUnused)
	{
		const FString AssetPathAbs = UProjectCleanerLibPath::ConvertToAbs(Asset.PackagePath.ToString());
		if (!AssetPathAbs.IsEmpty() && FPaths::IsUnderDirectory(AssetPathAbs, Item.FolderPathAbs))
		{
			++Num;
		}
	}

	return Num;
}

int64 SProjectCleanerTreeView::GetSizeTotal(const FProjectCleanerTreeViewItem& Item) const
{
	TArray<FAssetData> FilteredAssets;
	FilteredAssets.Reserve(SubsystemPtr->GetScanData().AssetsAll.Num());

	for (const auto& Asset : SubsystemPtr->GetScanData().AssetsAll)
	{
		const FString AssetPathAbs = UProjectCleanerLibPath::ConvertToAbs(Asset.PackagePath.ToString());
		if (!AssetPathAbs.IsEmpty() && FPaths::IsUnderDirectory(AssetPathAbs, Item.FolderPathAbs))
		{
			FilteredAssets.Add(Asset);
		}
	}

	FilteredAssets.Shrink();

	return UProjectCleanerLibAsset::GetAssetsTotalSize(FilteredAssets);
}

int64 SProjectCleanerTreeView::GetSizeUnused(const FProjectCleanerTreeViewItem& Item) const
{
	TArray<FAssetData> FilteredAssets;
	FilteredAssets.Reserve(SubsystemPtr->GetScanData().AssetsUnused.Num());

	for (const auto& Asset : SubsystemPtr->GetScanData().AssetsUnused)
	{
		const FString AssetPathAbs = UProjectCleanerLibPath::ConvertToAbs(Asset.PackagePath.ToString());
		if (!AssetPathAbs.IsEmpty() && FPaths::IsUnderDirectory(AssetPathAbs, Item.FolderPathAbs))
		{
			FilteredAssets.Add(Asset);
		}
	}

	FilteredAssets.Shrink();

	return UProjectCleanerLibAsset::GetAssetsTotalSize(FilteredAssets);
}
