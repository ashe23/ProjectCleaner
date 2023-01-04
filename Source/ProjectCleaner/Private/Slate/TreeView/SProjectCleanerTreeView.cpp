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
#include "Misc/ScopedSlowTask.h"
#include "Settings/ProjectCleanerSettings.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/Notifications/SProgressBar.h"

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
		SNew(SWidgetSwitcher)
		.IsEnabled_Raw(this, &SProjectCleanerTreeView::WidgetEnabled)
		.WidgetIndex_Raw(this, &SProjectCleanerTreeView::WidgetGetIndex)
		+ SWidgetSwitcher::Slot()
		  .HAlign(HAlign_Fill)
		  .VAlign(VAlign_Fill)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SProgressBar)
				.Percent(0.5f)
			]
		]
		+ SWidgetSwitcher::Slot()
		  .HAlign(HAlign_Fill)
		  .VAlign(VAlign_Fill)
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
	bUpdatingView = true;
	
	if (!SubsystemPtr) return;

	TreeViewItems.Reset();
	Items.Reset();

	if (!TreeView.IsValid())
	{
		SAssignNew(TreeView, STreeView<TSharedPtr<FProjectCleanerTreeViewItem>>)
		.TreeItemsSource(&TreeViewItems)
		.SelectionMode(ESelectionMode::Multi)
		.OnGenerateRow(this, &SProjectCleanerTreeView::OnGenerateRow)
		.OnGetChildren(this, &SProjectCleanerTreeView::OnGetChildren)
		.OnContextMenuOpening_Raw(this, &SProjectCleanerTreeView::GetItemContextMenu)
		.HeaderRow(GetHeaderRow())
		.OnMouseButtonDoubleClick(this, &SProjectCleanerTreeView::OnItemMouseDblClick)
		.OnSelectionChanged(this, &SProjectCleanerTreeView::OnSelectionChange)
		.OnExpansionChanged(this, &SProjectCleanerTreeView::OnExpansionChange);
	}

	if (SubsystemPtr->GetScanData().ScanResult != EProjectCleanerScanResult::Success) return;

	const TSharedPtr<FProjectCleanerTreeViewItem> RootItem = ItemCreate(UProjectCleanerLibPath::GetFolderContent());
	if (!RootItem.IsValid()) return;


	// caching expanded and selected items in order to keep them , when we updating data
	TreeViewItemsExpanded.Reset();
	TreeViewItemsSelected.Reset();
	TreeView->GetExpandedItems(TreeViewItemsExpanded);
	TreeView->GetSelectedItems(TreeViewItemsSelected);
	TreeView->ClearHighlightedItems();
	TreeView->ClearSelection();

	TreeViewItems.AddUnique(RootItem);

	Items.Reserve(SubsystemPtr->GetScanData().FoldersAll.Num() + 1);
	Items.Add(RootItem);

	// todo:ashe23 optimize 
	FScopedSlowTask SlowTask{
		static_cast<float>(SubsystemPtr->GetScanData().FoldersAll.Num()),
		FText::FromString(TEXT("Updating TreeView ...")),
		GIsEditor && !IsRunningCommandlet()
	};
	SlowTask.MakeDialog();

	for (const auto& Folder : SubsystemPtr->GetScanData().FoldersAll)
	{
		SlowTask.EnterProgressFrame(1.0f, FText::FromString(Folder));

		const auto Item = ItemCreate(Folder);
		if (!Item.IsValid()) continue;

		Items.Add(Item);
	}

	for (const auto& Item : Items)
	{
		TreeView->SetItemExpansion(Item, Item->bExpanded);
	}

	TreeView->RequestTreeRefresh();

	bUpdatingView = false;
}

TSharedPtr<FProjectCleanerTreeViewItem> SProjectCleanerTreeView::ItemCreate(const FString& InFolderPathAbs) const
{
	if (!SubsystemPtr) return {};

	TSharedPtr<FProjectCleanerTreeViewItem> TreeItem = MakeShareable(new FProjectCleanerTreeViewItem());
	if (!TreeItem.IsValid()) return {};

	TreeItem->FolderPathAbs = UProjectCleanerLibPath::ConvertToAbs(InFolderPathAbs);
	if (TreeItem->FolderPathAbs.IsEmpty()) return {};

	TreeItem->FolderPathRel = UProjectCleanerLibPath::ConvertToRel(TreeItem->FolderPathAbs);
	TreeItem->FolderName = FPaths::GetPathLeaf(TreeItem->FolderPathAbs);
	TreeItem->FoldersTotal = GetFoldersTotalNum(*TreeItem);
	TreeItem->FoldersEmpty = GetFoldersEmptyNum(*TreeItem);
	TreeItem->AssetsTotal = GetAssetsTotalNum(*TreeItem);
	TreeItem->AssetsUnused = GetAssetsUnusedNum(*TreeItem);
	TreeItem->SizeTotal = GetSizeTotal(*TreeItem);
	TreeItem->SizeUnused = GetSizeUnused(*TreeItem);
	TreeItem->bRoot = TreeItem->FolderPathAbs.Equals(UProjectCleanerLibPath::GetFolderContent());
	TreeItem->bDevFolder = TreeItem->FolderPathAbs.Equals(UProjectCleanerLibPath::GetFolderDevelopers());
	TreeItem->bExpanded = ItemIsExpanded(*TreeItem);
	TreeItem->bEngineGenerated = UProjectCleanerLibPath::FolderIsEngineGenerated(TreeItem->FolderPathAbs);
	TreeItem->bEmpty = SubsystemPtr->GetScanData().FoldersEmpty.Contains(TreeItem->FolderPathAbs);
	TreeItem->bExcluded = UProjectCleanerLibPath::FolderIsExcluded(InFolderPathAbs);
	TreeItem->bVisible = ItemIsVisible(*TreeItem);
	TreeItem->PercentUnused = TreeItem->AssetsTotal == 0 ? 0.0f : TreeItem->AssetsUnused * 100.0f / TreeItem->AssetsTotal;
	TreeItem->PercentUnusedNormalized = FMath::GetMappedRangeValueClamped(FVector2D{0.0f, 100.0f}, FVector2D{0.0f, 1.0f}, TreeItem->PercentUnused);

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

	MenuBuilder.AddMenuEntry(
		FText::FromString(TEXT("Show Folders Engine")),
		FText::FromString(TEXT("Show engine generated folders in tree view")),
		FSlateIcon(),
		FUIAction
		(
			FExecuteAction::CreateLambda([&]
			{
				GetMutableDefault<UProjectCleanerSettings>()->ToggleShowTreeViewFoldersEngineGenerated();
				GetMutableDefault<UProjectCleanerSettings>()->PostEditChange();

				ItemsUpdate();
			}),
			FCanExecuteAction::CreateLambda([&]()
			{
				return GetDefault<UProjectCleanerSettings>() != nullptr;
			}),
			FGetActionCheckState::CreateLambda([&]()
			{
				return GetDefault<UProjectCleanerSettings>()->bShowTreeViewFoldersEngineGenerated ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
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

				ItemsUpdate();
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

	TArray<TSharedPtr<FProjectCleanerTreeViewItem>> SubItems;
	GetSubItems(Item, SubItems);

	if (SubItems.Num() == 0) return;

	ToggleExpansionRecursive(Item, !Item->bExpanded);
}

void SProjectCleanerTreeView::OnGetChildren(TSharedPtr<FProjectCleanerTreeViewItem> Item, TArray<TSharedPtr<FProjectCleanerTreeViewItem>>& OutChildren)
{
	if (!Item.IsValid()) return;

	TArray<TSharedPtr<FProjectCleanerTreeViewItem>> SubItems;
	GetSubItems(Item, SubItems);

	for (const auto& SubItem : SubItems)
	{
		if (SubItem->bVisible)
		{
			OutChildren.Add(SubItem);
		}
	}
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

	TArray<TSharedPtr<FProjectCleanerTreeViewItem>> SubItems;
	GetSubItems(Item, SubItems);

	for (const auto& SubItem : SubItems)
	{
		ToggleExpansionRecursive(SubItem, bExpanded);
	}
}

void SProjectCleanerTreeView::GetSubItems(const TSharedPtr<FProjectCleanerTreeViewItem>& Item, TArray<TSharedPtr<FProjectCleanerTreeViewItem>>& SubItems)
{
	if (!Item.IsValid()) return;

	SubItems.Reset();

	TArray<FString> SubFolders;
	IFileManager::Get().FindFiles(SubFolders, *(Item->FolderPathAbs / TEXT("*")), false, true);

	for (const auto& SubFolder : SubFolders)
	{
		const FString SubFolderPathAbs = Item->FolderPathAbs / SubFolder;

		for (const auto& It : Items)
		{
			if (!It.IsValid()) continue;

			if (It->FolderPathAbs.Equals(SubFolderPathAbs))
			{
				SubItems.AddUnique(It);
			}
		}
	}
}

int32 SProjectCleanerTreeView::GetFoldersTotalNum(const FProjectCleanerTreeViewItem& Item) const
{
	if (!SubsystemPtr) return 0;

	int32 Num = 0;

	for (const auto& Folder : SubsystemPtr->GetScanData().FoldersAll)
	{
		if (!GetDefault<UProjectCleanerSettings>()->bShowTreeViewFoldersEmpty && UProjectCleanerLibPath::FolderIsEmpty(Folder)) continue;
		if (!GetDefault<UProjectCleanerSettings>()->bShowTreeViewFoldersExcluded && UProjectCleanerLibPath::FolderIsExcluded(Folder)) continue;
		if (Folder.Equals(Item.FolderPathAbs)) continue;
		if (FPaths::IsUnderDirectory(Folder, Item.FolderPathAbs))
		{
			++Num;
		}
	}

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

bool SProjectCleanerTreeView::WidgetEnabled() const
{
	return bUpdatingView == false;
}

int32 SProjectCleanerTreeView::WidgetGetIndex() const
{
	return bUpdatingView ? 0 : 1;
}

bool SProjectCleanerTreeView::ItemIsVisible(const FProjectCleanerTreeViewItem& Item) const
{
	if (Item.bRoot) return true;
	if (Item.bEngineGenerated && !GetDefault<UProjectCleanerSettings>()->bShowTreeViewFoldersEngineGenerated) return false;
	if (Item.bEmpty && !GetDefault<UProjectCleanerSettings>()->bShowTreeViewFoldersEmpty) return false;
	if (Item.bExcluded && !GetDefault<UProjectCleanerSettings>()->bShowTreeViewFoldersExcluded) return false;
	if (SearchText.IsEmpty()) return true;
	if (Item.FolderName.Find(SearchText) != INDEX_NONE) return true;

	TArray<FString> SubFolders;
	IFileManager::Get().FindFilesRecursive(SubFolders, *Item.FolderPathAbs, TEXT("*.*"), false, true);

	for (const auto& SubFolder : SubFolders)
	{
		if (FPaths::GetPathLeaf(SubFolder).Find(SearchText) != INDEX_NONE)
		{
			return true;
		}
	}

	return false;
}

bool SProjectCleanerTreeView::ItemIsExpanded(const FProjectCleanerTreeViewItem& Item) const
{
	if (Item.bRoot) return true;

	if (SearchText.IsEmpty())
	{
		// restoring old selection and expansion from cache
		for (const auto& ExpandedItem : TreeViewItemsExpanded)
		{
			if (ExpandedItem->FolderPathAbs.Equals(Item.FolderPathAbs))
			{
				return true;
			}
		}
	}

	if (Item.FolderName.Find(SearchText) != INDEX_NONE) return true;

	TArray<FString> SubFolders;
	IFileManager::Get().FindFilesRecursive(SubFolders, *Item.FolderPathAbs, TEXT("*.*"), false, true);

	for (const auto& SubFolder : SubFolders)
	{
		if (FPaths::GetPathLeaf(SubFolder).Find(SearchText) != INDEX_NONE)
		{
			return true;
		}
	}

	return false;
}
