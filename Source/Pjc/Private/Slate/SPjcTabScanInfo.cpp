// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/SPjcTabScanInfo.h"
#include "PjcStyles.h"
#include "PjcConstants.h"
// Engine Headers
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "PjcSubsystem.h"
#include "PjcTypes.h"
#include "Libs/PjcLibAsset.h"
#include "Libs/PjcLibPath.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Notifications/SProgressBar.h"

void SPjcTreeViewItem::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& OwnerTable)
{
	TreeItem = InArgs._TreeItem;
	SearchText = InArgs._SearchText;

	SMultiColumnTableRow::Construct(SMultiColumnTableRow::FArguments().ToolTipText(FText::FromString(TreeItem->PathRel)), OwnerTable);
}

TSharedRef<SWidget> SPjcTreeViewItem::GenerateWidgetForColumn(const FName& InColumnName)
{
	FNumberFormattingOptions NumberFormattingOptions;
	NumberFormattingOptions.SetUseGrouping(true);
	NumberFormattingOptions.SetMinimumFractionalDigits(0);
	
	if (InColumnName.IsEqual(TEXT("FolderName")))
	{
		return
			SNew(SHorizontalBox)
			.ToolTipText(FText::FromString(TreeItem->PathRel))
			+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin{2.0f})
			[
				SNew(SExpanderArrow, SharedThis(this))
				.IndentAmount(10)
				.ShouldDrawWires(false)
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 2, 0).VAlign(VAlign_Center)
			[
				SNew(SImage)
				.Image(GetFolderIcon())
				.ColorAndOpacity(GetFolderColor())
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin{2.0f})
			[
				SNew(STextBlock).Text(FText::FromString(TreeItem->FolderName)).HighlightText(FText::FromString(SearchText))
			];
	}

	if (InColumnName.IsEqual(TEXT("Percent")))
	{
		return
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().Padding(FMargin{20.0f, 1.0f}).FillWidth(1.0f)
			[
				SNew(SOverlay)
				+ SOverlay::Slot().HAlign(HAlign_Fill).VAlign(VAlign_Fill)
				[
					SNew(SProgressBar)
					.BorderPadding(FVector2D{0.0f, 0.0f})
					.Percent(TreeItem->PercentUnusedNormalized)
					.BackgroundImage(FPjcStyles::Get().GetBrush("ProjectCleaner.Progressbar"))
					.FillColorAndOpacity(FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Red"))
				]
				+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.AutoWrapText(false)
					.ColorAndOpacity(FLinearColor::White)
					.Text(FText::AsMemory(TreeItem->SizeUnused, IEC))
				]
			];
	}

	if (InColumnName.IsEqual(TEXT("AssetsTotal")))
	{
		const FString StrNum = FText::AsNumber(TreeItem->NumTotal, &NumberFormattingOptions).ToString();
		
		return SNew(STextBlock).Text(FText::FromString(FString::Printf(TEXT("%s"), *StrNum)));
	}

	if (InColumnName.IsEqual(TEXT("AssetsUsed")))
	{
		const FString StrNum = FText::AsNumber(TreeItem->NumUsed, &NumberFormattingOptions).ToString();
		
		return SNew(STextBlock).Text(FText::FromString(FString::Printf(TEXT("%s"), *StrNum)));
	}

	if (InColumnName.IsEqual(TEXT("AssetsUnused")))
	{
		const FString StrNum = FText::AsNumber(TreeItem->NumUnused, &NumberFormattingOptions).ToString();
		
		return SNew(STextBlock).Text(FText::FromString(FString::Printf(TEXT("%s"), *StrNum)));
	}

	return SNew(STextBlock).Text(FText::FromString(TEXT("")));
}

const FSlateBrush* SPjcTreeViewItem::GetFolderIcon() const
{
	if (TreeItem->bIsDevFolder)
	{
		return FEditorStyle::GetBrush(TEXT("ContentBrowser.AssetTreeFolderDeveloper"));
	}

	return FEditorStyle::GetBrush(TreeItem->bIsExpanded ? TEXT("ContentBrowser.AssetTreeFolderOpen") : TEXT("ContentBrowser.AssetTreeFolderClosed"));
}

FSlateColor SPjcTreeViewItem::GetFolderColor() const
{
	if (TreeItem->bIsExcluded)
	{
		return FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Yellow");
	}

	if (TreeItem->bIsEmpty)
	{
		return FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Red");
	}

	return FLinearColor::Gray;
}

void SPjcTabScanInfo::Construct(const FArguments& InArgs)
{
	GEditor->GetEditorSubsystem<UPjcSubsystem>()->OnProjectScan().AddRaw(this, &SPjcTabScanInfo::UpdateData);
	
	const FContentBrowserModule& ModuleContentBrowser = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");

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
	AssetPickerConfig.AssetShowWarningText = FText::FromName("No assets");
	// AssetPickerConfig.GetCurrentSelectionDelegates.Add(&AssetBrowserDelegateSelection);
	// AssetPickerConfig.RefreshAssetViewDelegates.Add(&AssetBrowserDelegateRefreshView);
	// AssetPickerConfig.SetFilterDelegates.Add(&AssetBrowserDelegateFilter);
	// AssetPickerConfig.Filter = AssetBrowserCreateFilter();
	AssetPickerConfig.OnAssetDoubleClicked = FOnAssetDoubleClicked::CreateLambda([](const FAssetData& AssetData)
	{
		if (!AssetData.IsValid()) return;
		if (!GEditor) return;

		TArray<FName> AssetNames;
		AssetNames.Add(AssetData.ToSoftObjectPath().GetAssetPathName());

		GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorsForAssets(AssetNames);
	});

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

	SAssignNew(TreeView, STreeView<TSharedPtr<FPjcTreeViewItem>>)
	.TreeItemsSource(&TreeViewItems)
	.SelectionMode(ESelectionMode::Multi)
	.OnGenerateRow(this, &SPjcTabScanInfo::OnTreeViewGenerateRow)
	.OnGetChildren(this, &SPjcTabScanInfo::OnTreeViewGetChildren)
	// .OnContextMenuOpening_Raw(this, &SPjcTabAssetsBrowser::OnTreeViewContextMenu)
	// .OnSelectionChanged(this, &SPjcTabAssetsBrowser::OnTreeViewSelectionChange)
	// .OnExpansionChanged_Raw(this, &SPjcTabAssetsBrowser::OnTreeViewExpansionChange)
	.HeaderRow(GetTreeViewHeaderRow());

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().Padding(FMargin{5.0f})
		[
			SNew(SSplitter)
			.Orientation(Orient_Horizontal)
			.PhysicalSplitterHandleSize(3.0f)
			.Style(FEditorStyle::Get(), "ContentBrowser.Splitter")
			+ SSplitter::Slot().Value(0.5f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().Padding(5.0f).FillHeight(1.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().Padding(FMargin{0.0f, 0.0f, 0.0f, 5.0f})
					[
						SNew(SSearchBox)
						.HintText(FText::FromString(TEXT("Search Folders...")))
						// .OnTextChanged(this, &SPjcTabAssetsBrowser::OnTreeViewSearchTextChanged)
						// .OnTextCommitted(this, &SPjcTabAssetsBrowser::OnTreeViewSearchTextCommitted)
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(FMargin{0.0f, 0.0f, 0.0f, 2.0f})
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth()
						[
							SNew(SImage)
							.Image(FEditorStyle::GetBrush("ContentBrowser.AssetTreeFolderOpen"))
							.ColorAndOpacity(FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Red"))
						]
						+ SHorizontalBox::Slot().Padding(FMargin{0.0f, 2.0f, 0.0f, 0.0f}).AutoWidth()
						[
							SNew(STextBlock).Text(FText::FromString(TEXT(" - Empty Paths")))
						]
						+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin{5.0f, 0.0f, 0.0f, 0.0f})
						[
							SNew(SImage)
							.Image(FEditorStyle::GetBrush("ContentBrowser.AssetTreeFolderOpen"))
							.ColorAndOpacity(FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Yellow"))
						]
						+ SHorizontalBox::Slot().Padding(FMargin{0.0f, 2.0f, 0.0f, 0.0f}).AutoWidth()
						[
							SNew(STextBlock).Text(FText::FromString(TEXT(" - Excluded Paths")))
						]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(FMargin{0.0f, 0.0f, 0.0f, 5.0f})
					[
						SNew(SSeparator).Thickness(5.0f)
					]
					+ SVerticalBox::Slot().FillHeight(1.0f).Padding(FMargin{0.0f, 5.0f, 0.0f, 0.0f})
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
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right).VAlign(VAlign_Center)
					[
						SNew(SComboButton)
						.ContentPadding(0)
						.ForegroundColor_Raw(this, &SPjcTabScanInfo::GetTreeViewOptionsBtnForegroundColor)
						.ButtonStyle(FEditorStyle::Get(), "ToggleButton")
						.OnGetMenuContent(this, &SPjcTabScanInfo::GetTreeViewOptionsBtnContent)
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
			+ SSplitter::Slot()
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().FillHeight(1.0f).Padding(FMargin{5.0f})
				[
					ModuleContentBrowser.Get().CreateAssetPicker(AssetPickerConfig)
				]
			]
		]
	];
}

SPjcTabScanInfo::~SPjcTabScanInfo()
{
	GEditor->GetEditorSubsystem<UPjcSubsystem>()->OnProjectScan().RemoveAll(this);
}

void SPjcTabScanInfo::UpdateData(const FPjcScanResult& InScanResult)
{
	RootItem.Reset();
	
	RootItem = MakeShareable(new FPjcTreeViewItem);
	RootItem->SizeUnused = FPjcLibAsset::GetAssetsSize(InScanResult.AssetsUnused.Array());
	RootItem->NumTotal = InScanResult.AssetsAll.Num();
	RootItem->NumUsed = InScanResult.AssetsUsed.Num();
	RootItem->NumUnused = InScanResult.AssetsUnused.Num();
	RootItem->PercentUnused = RootItem->NumTotal == 0 ? 0 : RootItem->NumUnused * 100.0f / RootItem->NumTotal;
	RootItem->PercentUnusedNormalized = FMath::GetMappedRangeValueClamped(FVector2D{0.0f, 100.0f}, FVector2D{0.0f, 1.0f}, RootItem->PercentUnused);
	RootItem->bIsRoot = true;
	RootItem->bIsEmpty = FPjcLibPath::IsPathEmpty(FPaths::ProjectContentDir());
	RootItem->bIsExcluded = FPjcLibPath::IsPathExcluded(FPaths::ProjectContentDir());
	RootItem->bIsDevFolder = false;
	RootItem->bIsExpanded = true;
	RootItem->ParentItem = nullptr;
	RootItem->PathAbs = FPjcLibPath::ContentDir();
	RootItem->PathRel = FPjcLibPath::ToAssetPath(RootItem->PathAbs);
	RootItem->FolderName = FPjcLibPath::GetPathName(RootItem->PathAbs);

	TSet<FString> SubFolders;
	FPjcLibPath::GetFoldersInPath(FPjcLibPath::ContentDir(), false, SubFolders);
	
	for (const FString& SubFolder : SubFolders)
	{
		const TSharedPtr<FPjcTreeViewItem> SubItem = MakeShareable(new FPjcTreeViewItem);
		if (!SubItem) continue;

		SubItem->SizeUnused = 0; // todo:ashe23
		SubItem->NumTotal = 0;
		SubItem->NumUsed = 0;
		SubItem->NumUnused = 0;
		SubItem->PercentUnused = SubItem->NumTotal == 0 ? 0 : SubItem->NumUnused * 100.0f / SubItem->NumTotal;
		SubItem->PercentUnusedNormalized = FMath::GetMappedRangeValueClamped(FVector2D{0.0f, 100.0f}, FVector2D{0.0f, 1.0f}, SubItem->PercentUnused);
		SubItem->bIsRoot = false;
		SubItem->bIsEmpty = FPjcLibPath::IsPathEmpty(SubFolder);
		SubItem->bIsExcluded = FPjcLibPath::IsPathExcluded(SubFolder);
		SubItem->bIsDevFolder = false;
		SubItem->bIsExpanded = true;
		SubItem->ParentItem = RootItem;
		SubItem->PathAbs = SubFolder;
		SubItem->PathRel = FPjcLibPath::ToAssetPath(SubFolder);
		SubItem->FolderName = FPjcLibPath::GetPathName(SubFolder);

		RootItem->SubItems.Emplace(SubItem);
	}

	TreeViewItems.Empty();
	TreeViewItems.Emplace(RootItem);

	TreeView->RequestTreeRefresh();
}

TSharedRef<SHeaderRow> SPjcTabScanInfo::GetTreeViewHeaderRow() const
{
	return
		SNew(SHeaderRow)
		+ SHeaderRow::Column(TEXT("FolderName")).HAlignHeader(HAlign_Center).VAlignHeader(VAlign_Center).HeaderContentPadding(FMargin{5.0f}).FillWidth(0.5f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Path")))
			.ColorAndOpacity(FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
			.Font(FPjcStyles::GetFont("Light", PjcConstants::HeaderRowFontSize))
		]
		+ SHeaderRow::Column(TEXT("Percent")).HAlignHeader(HAlign_Center).VAlignHeader(VAlign_Center).HeaderContentPadding(FMargin{5.0f}).FillWidth(0.3f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Unused Size")))
			.ToolTipText(FText::FromString(TEXT("Total size of unused assets relative to total assets in current path")))
			.ColorAndOpacity(FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
			.Font(FPjcStyles::GetFont("Light", PjcConstants::HeaderRowFontSize))
		]
		+ SHeaderRow::Column(TEXT("AssetsTotal")).HAlignHeader(HAlign_Center).VAlignHeader(VAlign_Center).HeaderContentPadding(FMargin{5.0f}).FillWidth(0.1f).HAlignCell(HAlign_Center).
		                                          VAlignCell(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Total")))
			.ToolTipText(FText::FromString(TEXT("Total number of assets in current path")))
			.ColorAndOpacity(FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
			.Font(FPjcStyles::GetFont("Light", PjcConstants::HeaderRowFontSize))
		]
		+ SHeaderRow::Column(TEXT("AssetsUsed")).HAlignHeader(HAlign_Center).VAlignHeader(VAlign_Center).HeaderContentPadding(FMargin{5.0f}).FillWidth(0.1f).HAlignCell(HAlign_Center).
		                                         VAlignCell(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Used")))
			.ToolTipText(FText::FromString(TEXT("Total number of used assets in current path")))
			.ColorAndOpacity(FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
			.Font(FPjcStyles::GetFont("Light", PjcConstants::HeaderRowFontSize))
		]
		+ SHeaderRow::Column(TEXT("AssetsUnused")).HAlignHeader(HAlign_Center).VAlignHeader(VAlign_Center).HeaderContentPadding(FMargin{5.0f}).FillWidth(0.1f).HAlignCell(HAlign_Center).
		                                           VAlignCell(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Unused")))
			.ToolTipText(FText::FromString(TEXT("Total number of unused assets in current path")))
			.ColorAndOpacity(FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
			.Font(FPjcStyles::GetFont("Light", PjcConstants::HeaderRowFontSize))
		];
}

TSharedRef<ITableRow> SPjcTabScanInfo::OnTreeViewGenerateRow(TSharedPtr<FPjcTreeViewItem> Item, const TSharedRef<STableViewBase>& OwnerTable) const
{
	return SNew(SPjcTreeViewItem, OwnerTable).TreeItem(Item).SearchText(TreeViewSearchText);
}

void SPjcTabScanInfo::OnTreeViewGetChildren(TSharedPtr<FPjcTreeViewItem> Item, TArray<TSharedPtr<FPjcTreeViewItem>>& OutChildren)
{
	if (!Item.IsValid()) return;
	if (Item->SubItems.Num() == 0) return;

	OutChildren.Append(Item->SubItems);
}

TSharedRef<SWidget> SPjcTabScanInfo::GetTreeViewOptionsBtnContent()
{
	const TSharedPtr<FExtender> Extender;
	FMenuBuilder MenuBuilder(true, nullptr, Extender, true);
	return MenuBuilder.MakeWidget();
}

FSlateColor SPjcTabScanInfo::GetTreeViewOptionsBtnForegroundColor() const
{
	static const FName InvertedForegroundName("InvertedForeground");
	static const FName DefaultForegroundName("DefaultForeground");

	if (!TreeViewOptionBtn.IsValid()) return FEditorStyle::GetSlateColor(DefaultForegroundName);

	return TreeViewOptionBtn->IsHovered() ? FEditorStyle::GetSlateColor(InvertedForegroundName) : FEditorStyle::GetSlateColor(DefaultForegroundName);
}
