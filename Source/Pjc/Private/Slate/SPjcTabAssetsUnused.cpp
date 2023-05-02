// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/SPjcTabAssetsUnused.h"
#include "Slate/Items/SPjcItemStat.h"
#include "Slate/Items/SPjcItemTree.h"
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
	}

	Cmds = MakeShareable(new FUICommandList);

	Cmds->MapAction(
		FPjcCmds::Get().TabAssetsUnusedBtnScan,
		FExecuteAction::CreateLambda([&]()
		{
			TreeItemsUpdate();
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
	.OnContextMenuOpening_Raw(this, &SPjcTabAssetsUnused::GetTreeContextMenu)
	.OnExpansionChanged_Raw(this, &SPjcTabAssetsUnused::OnTreeExpansionChanged)
	.HeaderRow(GetTreeHeaderRow());
	
	StatItemsInit();
	TreeItemsUpdate();
	TreeItemsFilter();

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
				FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Red").GetSpecifiedColor(),
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
				FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Yellow").GetSpecifiedColor(),
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

TSharedRef<SWidget> SPjcTabAssetsUnused::CreateToolbar()
{
	FToolBarBuilder ToolBarBuilder{Cmds, FMultiBoxCustomization::None};
	ToolBarBuilder.BeginSection("PjcTabAssetUnusedScanActions");
	{
		ToolBarBuilder.AddToolBarButton(FPjcCmds::Get().TabAssetsUnusedBtnScan);
		ToolBarBuilder.AddToolBarButton(FPjcCmds::Get().TabAssetsUnusedBtnClean);
	}
	ToolBarBuilder.EndSection();

	ToolBarBuilder.BeginSection("PjcTabAssetsUnusedSettingsActions");
	{
		ToolBarBuilder.AddToolBarWidget(GetSettingsWidget());
	}
	ToolBarBuilder.EndSection();

	return ToolBarBuilder.MakeWidget();
}

TSharedRef<SWidget> SPjcTabAssetsUnused::GetSettingsWidget()
{
	return
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 15.0f, 0.0f, 0.0f)
		[
			SNew(SCheckBox)
			.IsChecked_Raw(this, &SPjcTabAssetsUnused::GetCheckboxStateScanFoldersDev)
			.OnCheckStateChanged_Raw(this, &SPjcTabAssetsUnused::OnScanFoldersDevStateChanged)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("Scan Developers Content")))
				.ToolTipText(FText::FromString("If enabled, Developers folders will be scanned."))
			]
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SCheckBox)
			.IsChecked_Raw(this, &SPjcTabAssetsUnused::GetCheckboxStateCleanAssetsUnused)
			.OnCheckStateChanged_Raw(this, &SPjcTabAssetsUnused::OnCleanAssetsUnusedStateChanged)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("Clean Unused Assets")))
				.ToolTipText(FText::FromString(TEXT("If enabled, will delete all unused assets when CleanProject btn clicked.")))
			]
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SCheckBox)
			.IsChecked_Raw(this, &SPjcTabAssetsUnused::GetCheckboxStateCleanFoldersEmpty)
			.OnCheckStateChanged_Raw(this, &SPjcTabAssetsUnused::OnCleanFoldersEmptyStateChanged)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("Clean Empty Folders")))
				.ToolTipText(FText::FromString(TEXT("If enabled, will delete all empty folders in project")))
			]
		];
}

ECheckBoxState SPjcTabAssetsUnused::GetCheckboxStateScanFoldersDev() const
{
	return SubsystemPtr && SubsystemPtr->CanScanFoldersDev() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

ECheckBoxState SPjcTabAssetsUnused::GetCheckboxStateCleanAssetsUnused() const
{
	return SubsystemPtr && SubsystemPtr->CanCleanAssetsUnused() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

ECheckBoxState SPjcTabAssetsUnused::GetCheckboxStateCleanFoldersEmpty() const
{
	return SubsystemPtr && SubsystemPtr->CanCleanFoldersEmpty() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void SPjcTabAssetsUnused::OnScanFoldersDevStateChanged(ECheckBoxState BoxState)
{
	if (!SubsystemPtr) return;

	SubsystemPtr->ToggleScanFoldersDev();

	UContentBrowserSettings* Settings = GetMutableDefault<UContentBrowserSettings>();
	if (Settings)
	{
		Settings->SetDisplayDevelopersFolder(SubsystemPtr->CanScanFoldersDev());
		Settings->PostEditChange();
	}

	TreeItemsFilter();
}

void SPjcTabAssetsUnused::OnCleanAssetsUnusedStateChanged(ECheckBoxState BoxState)
{
	if (!SubsystemPtr) return;

	SubsystemPtr->ToggleCleanAssetsUnused();
}

void SPjcTabAssetsUnused::OnCleanFoldersEmptyStateChanged(ECheckBoxState BoxState)
{
	if (!SubsystemPtr) return;

	SubsystemPtr->ToggleCleanFoldersEmpty();
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
	const bool bIsDev = InFolderPath.StartsWith(PjcConstants::PathDevelopers.ToString()) || InFolderPath.StartsWith(PjcConstants::PathCollections.ToString());
	const bool bIsRoot = InFolderPath.Equals(PjcConstants::PathRoot.ToString());

	// const FAssetRegistryModule& AssetRegistry = FPjcLibAsset::GetAssetRegistry();
	//
	// TArray<FAssetData> AssetsInPath;
	// AssetRegistry.Get().GetAssetsByPath(FName{*InFolderPath}, AssetsInPath, true);

	return MakeShareable(
		new FPjcTreeItem{
			InFolderPath,
			bIsRoot ? TEXT("Content") : FPaths::GetPathLeaf(InFolderPath),
			bIsDev,
			bIsRoot,
			FPjcLibPath::IsPathEmpty(InFolderPath),
			FPjcLibPath::IsPathExcluded(InFolderPath),
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

	if (!SubsystemPtr->CanScanFoldersDev() && Item->bIsDev)
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

TSharedPtr<SWidget> SPjcTabAssetsUnused::GetTreeContextMenu()
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

TSharedPtr<SWidget> SPjcTabAssetsUnused::GetContentBrowserContextMenu(const TArray<FAssetData>& Assets)
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
