// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/SPjcTabAssetsUnused.h"
#include "Slate/Items/SPjcItemStat.h"
#include "Slate/Items/SPjcItemTree.h"
#include "PjcCmds.h"
#include "PjcTypes.h"
#include "PjcStyles.h"
#include "PjcConstants.h"
// Engine Headers
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Settings/ContentBrowserSettings.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"

void SPjcTabAssetsUnused::Construct(const FArguments& InArgs)
{
	Cmds = MakeShareable(new FUICommandList);

	Cmds->MapAction(
		FPjcCmds::Get().TabAssetsUnusedBtnScan,
		FExecuteAction::CreateLambda([&]()
		{
			TreeItemsFilter();
		})
	);

	Cmds->MapAction(
		FPjcCmds::Get().TabAssetsUnusedBtnClean,
		FExecuteAction::CreateLambda([]()
		{
			UE_LOG(LogTemp, Warning, TEXT("Cleaning project"));

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
	AssetPickerConfig.bForceShowEngineContent = false;
	AssetPickerConfig.bForceShowPluginContent = false;
	AssetPickerConfig.bAddFilterUI = true;

	SAssignNew(StatView, SListView<TSharedPtr<FPjcStatItem>>)
	.ListItemsSource(&StatItems)
	.OnGenerateRow(this, &SPjcTabAssetsUnused::OnStatGenerateRow)
	.SelectionMode(ESelectionMode::None)
	.IsFocusable(false)
	.HeaderRow(GetStatHeaderRow());

	SAssignNew(TreeView, STreeView<TSharedPtr<FPjcTreeItem>>)
	.TreeItemsSource(&TreeItems)
	.SelectionMode(ESelectionMode::Multi)
	.OnGenerateRow(this, &SPjcTabAssetsUnused::OnTreeGenerateRow)
	.OnGetChildren(this, &SPjcTabAssetsUnused::OnTreeGetChildren)
	.OnExpansionChanged_Raw(this, &SPjcTabAssetsUnused::OnTreeExpansionChanged)
	.HeaderRow(GetTreeHeaderRow());

	StatItemsInit();
	TreeItemsInit();

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
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.0f).HAlign(HAlign_Center).VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Justification(ETextJustify::Center)
						.ColorAndOpacity(FPjcStyles::Get().GetColor("ProjectCleaner.Color.Gray"))
						.ShadowOffset(FVector2D{1.5f, 1.5f})
						.ShadowColorAndOpacity(FLinearColor::Black)
						.Font(FPjcStyles::GetFont("Bold", 15))
						.Text(FText::FromString(TEXT("Assets Statistics")))
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(3.0f)
				[
					StatView.ToSharedRef()
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

void SPjcTabAssetsUnused::StatItemsInit()
{
	StatItems.Reset();

	const FMargin FirstLvl{5.0f, 0.0f, 0.0f, 0.0f};
	const FMargin SecondLvl{20.0f, 0.0f, 0.0f, 0.0f};

	StatItems.Emplace(
		MakeShareable(
			new FPjcStatItem{
				FText::FromString(TEXT("Unused")),
				FText::FromString(TEXT("0")),
				FText::FromString(TEXT("0")),
				FText::FromString(TEXT("Unused Assets")),
				FText::FromString(TEXT("Total number of unused assets")),
				FText::FromString(TEXT("Total size of unused assets")),
				FLinearColor::Red,
				FirstLvl
			}
		)
	);

	StatItems.Emplace(
		MakeShareable(
			new FPjcStatItem{
				FText::FromString(TEXT("Used")),
				FText::FromString(TEXT("0")),
				FText::FromString(TEXT("0")),
				FText::FromString(TEXT("Used Assets")),
				FText::FromString(TEXT("Total number of used assets")),
				FText::FromString(TEXT("Total size of used assets")),
				FLinearColor::White,
				FirstLvl
			}
		)
	);

	StatItems.Emplace(
		MakeShareable(
			new FPjcStatItem{
				FText::FromString(TEXT("Primary")),
				FText::FromString(TEXT("0")),
				FText::FromString(TEXT("0")),
				FText::FromString(TEXT("Primary Assets")),
				FText::FromString(TEXT("Total number of primary assets")),
				FText::FromString(TEXT("Total size of primary assets")),
				FLinearColor::White,
				SecondLvl
			}
		)
	);

	StatItems.Emplace(
		MakeShareable(
			new FPjcStatItem{
				FText::FromString(TEXT("Editor")),
				FText::FromString(TEXT("0")),
				FText::FromString(TEXT("0")),
				FText::FromString(TEXT("Editor Assets")),
				FText::FromString(TEXT("Total number of Editor assets")),
				FText::FromString(TEXT("Total size of Editor assets")),
				FLinearColor::White,
				SecondLvl
			}
		)
	);

	StatItems.Emplace(
		MakeShareable(
			new FPjcStatItem{
				FText::FromString(TEXT("Indirect")),
				FText::FromString(TEXT("0")),
				FText::FromString(TEXT("0")),
				FText::FromString(TEXT("Indirect Assets")),
				FText::FromString(TEXT("Total number of Indirect assets")),
				FText::FromString(TEXT("Total size of Indirect assets")),
				FLinearColor::White,
				SecondLvl
			}
		)
	);

	StatItems.Emplace(
		MakeShareable(
			new FPjcStatItem{
				FText::FromString(TEXT("ExtReferenced")),
				FText::FromString(TEXT("0")),
				FText::FromString(TEXT("0")),
				FText::FromString(TEXT("ExtReferenced Assets")),
				FText::FromString(TEXT("Total number of ExtReferenced assets")),
				FText::FromString(TEXT("Total size of ExtReferenced assets")),
				FLinearColor::White,
				SecondLvl
			}
		)
	);

	StatItems.Emplace(
		MakeShareable(
			new FPjcStatItem{
				FText::FromString(TEXT("Excluded")),
				FText::FromString(TEXT("0")),
				FText::FromString(TEXT("0")),
				FText::FromString(TEXT("Excluded Assets")),
				FText::FromString(TEXT("Total number of Excluded assets")),
				FText::FromString(TEXT("Total size of Excluded assets")),
				FLinearColor::White,
				SecondLvl
			}
		)
	);

	StatItems.Emplace(
		MakeShareable(
			new FPjcStatItem{
				FText::FromString(TEXT("Total")),
				FText::FromString(TEXT("0")),
				FText::FromString(TEXT("0")),
				FText::FromString(TEXT("All Assets")),
				FText::FromString(TEXT("Total number of assets")),
				FText::FromString(TEXT("Total size of assets")),
				FLinearColor::White,
				FirstLvl
			}
		)
	);

	if (StatView.IsValid())
	{
		StatView->RebuildList();
	}
}

void SPjcTabAssetsUnused::TreeItemsInit()
{
	if (!TreeView.IsValid()) return;

	TreeRootItem.Reset();

	TreeRootItem = CreateTreeItem(PjcConstants::PathRoot.ToString());
	if (!TreeRootItem.IsValid()) return;

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

	TreeItems.Reset();
	TreeItems.Emplace(TreeRootItem);
	TreeView->RebuildList();
}

void SPjcTabAssetsUnused::TreeItemsFilter()
{
	if (!TreeRootItem.IsValid()) return;
	if (!TreeView.IsValid()) return;

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

TSharedRef<SHeaderRow> SPjcTabAssetsUnused::GetStatHeaderRow() const
{
	const FMargin HeaderContentPadding{5.0f};

	return
		SNew(SHeaderRow)
		+ SHeaderRow::Column("Name").FillWidth(0.4f).HAlignCell(HAlign_Left).VAlignCell(VAlign_Center).HAlignHeader(HAlign_Center).HeaderContentPadding(HeaderContentPadding)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Category")))
			.ColorAndOpacity(FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
			.Font(FPjcStyles::GetFont("Light", 10.0f))
		]
		+ SHeaderRow::Column("Num").FillWidth(0.3f).HAlignCell(HAlign_Center).VAlignCell(VAlign_Center).HAlignHeader(HAlign_Center).HeaderContentPadding(HeaderContentPadding)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Num")))
			.ColorAndOpacity(FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
			.Font(FPjcStyles::GetFont("Light", 10.0f))
		]
		+ SHeaderRow::Column("Size").FillWidth(0.3f).HAlignCell(HAlign_Center).VAlignCell(VAlign_Center).HAlignHeader(HAlign_Center).HeaderContentPadding(HeaderContentPadding)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Size")))
			.ColorAndOpacity(FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
			.Font(FPjcStyles::GetFont("Light", 10.0f))
		];
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

TSharedRef<ITableRow> SPjcTabAssetsUnused::OnStatGenerateRow(TSharedPtr<FPjcStatItem> Item, const TSharedRef<STableViewBase>& OwnerTable) const
{
	return SNew(SPjcItemStat, OwnerTable).Item(Item);
}

TSharedRef<ITableRow> SPjcTabAssetsUnused::OnTreeGenerateRow(TSharedPtr<FPjcTreeItem> Item, const TSharedRef<STableViewBase>& OwnerTable) const
{
	return SNew(SPjcItemTree, OwnerTable).Item(Item).HightlightText(SearchText);
}

TSharedPtr<FPjcTreeItem> SPjcTabAssetsUnused::CreateTreeItem(const FString& InFolderPath) const
{
	const bool bIsDev = InFolderPath.StartsWith(PjcConstants::PathDevelopers.ToString());
	const bool bIsRoot = InFolderPath.Equals(PjcConstants::PathRoot.ToString());

	return MakeShareable(new FPjcTreeItem{
		InFolderPath,
		bIsRoot ? TEXT("Content") : FPaths::GetPathLeaf(InFolderPath),
		bIsDev,
		bIsRoot,
		false,
		false,
		bIsRoot ? true : false,
		true,
		0,
		0,
		0,
		0,
		0.0f,
		0.0f
	});
}

void SPjcTabAssetsUnused::SetTreeItemVisibility(const TSharedPtr<FPjcTreeItem>& Item) const
{
	if (!Item.IsValid()) return;

	Item->bIsVisible = Item->bIsRoot || SearchText.IsEmpty();

	if (Item->bIsVisible) return;

	if (TreeItemContainsSearchText(Item))
	{
		TreeItemMakeVisibleParentsRecursive(Item);
	}
}

void SPjcTabAssetsUnused::SetTreeItemExpansion(const TSharedPtr<FPjcTreeItem>& Item)
{
	if (!Item.IsValid()) return;
	if (!TreeView.IsValid()) return;

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

TSharedRef<SWidget> SPjcTabAssetsUnused::GetTreeOptionsBtnContent() const
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
			FExecuteAction::CreateLambda([&] { }),
			FCanExecuteAction::CreateLambda([&]()
			{
				return true;
			}),
			FGetActionCheckState::CreateLambda([&]()
			{
				return ECheckBoxState::Checked;
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
			FExecuteAction::CreateLambda([&] { }),
			FCanExecuteAction::CreateLambda([&]()
			{
				return true;
			}),
			FGetActionCheckState::CreateLambda([&]()
			{
				return ECheckBoxState::Checked;
			})
		),
		NAME_None,
		EUserInterfaceActionType::ToggleButton
	);

	return MenuBuilder.MakeWidget();
}

FText SPjcTabAssetsUnused::GetTreeSummaryText() const
{
	return FText{};
}
