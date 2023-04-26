// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/TreeView/SPjcTreeView.h"
#include "Slate/TreeView/SPjcTreeViewItem.h"
#include "Libs/PjcLibPath.h"
#include "PjcStyles.h"
// Engine Headers
#include "PjcSubsystem.h"
#include "PjcTypes.h"
#include "EditorSettings/PjcEditorAssetExcludeSettings.h"
#include "Libs/PjcLibAsset.h"
#include "Misc/ScopedSlowTask.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"

void SPjcTreeView::Construct(const FArguments& InArgs)
{
	if (!GEditor) return;

	SubsystemPtr = GEditor->GetEditorSubsystem<UPjcSubsystem>();
	if (!SubsystemPtr) return;

	SubsystemPtr->OnScanAssets().AddRaw(this, &SPjcTreeView::OnScanAssets);

	SAssignNew(TreeView, STreeView<TSharedPtr<FPjcTreeViewItem>>)
	.TreeItemsSource(&TreeViewItems)
	.SelectionMode(ESelectionMode::Multi)
	.OnGenerateRow(this, &SPjcTreeView::OnTreeViewGenerateRow)
	.OnGetChildren(this, &SPjcTreeView::OnTreeViewGetChildren)
	// .OnContextMenuOpening_Raw(this, &SPjcTabAssetsBrowser::OnTreeViewContextMenu)
	.OnSelectionChanged(this, &SPjcTreeView::OnTreeViewSelectionChange)
	// .OnExpansionChanged_Raw(this, &SPjcTabAssetsBrowser::OnTreeViewExpansionChange)
	.HeaderRow(GetTreeViewHeaderRow());

	TreeViewListUpdate();

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin{0.0f, 0.0f, 5.0f, 5.0f})
		[
			SNew(SSearchBox)
			.HintText(FText::FromString(TEXT("Search...")))
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin{0.0f, 0.0f, 5.0f, 2.0f})
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
				SNew(STextBlock).Text(FText::FromString(TEXT(" - Empty Paths")))
			]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SImage)
				.Image(FEditorStyle::GetBrush("ContentBrowser.AssetTreeFolderOpen"))
				.ColorAndOpacity(FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Yellow"))
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin{0.0f, 2.0f, 5.0f, 0.0f})
			[
				SNew(STextBlock).Text(FText::FromString(TEXT(" - Excluded Paths")))
			]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin{0.0f, 0.0f, 5.0f, 5.0f})
		[
			SNew(SSeparator).Thickness(5.0f)
		]
		+ SVerticalBox::Slot().FillHeight(1.0f).Padding(FMargin{0.0f, 5.0f, 5.0f, 0.0f})
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
				SNew(STextBlock).Text_Raw(this, &SPjcTreeView::GetSummaryText)
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f).HAlign(HAlign_Right).VAlign(VAlign_Center)
			[
				SNew(SComboButton)
				.ContentPadding(0)
				.ForegroundColor_Raw(this, &SPjcTreeView::GetTreeViewOptionsBtnForegroundColor)
				.ButtonStyle(FEditorStyle::Get(), "ToggleButton")
				.OnGetMenuContent(this, &SPjcTreeView::GetTreeViewOptionsBtnContent)
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

SPjcTreeView::~SPjcTreeView()
{
	if (!SubsystemPtr) return;

	SubsystemPtr->OnScanAssets().RemoveAll(this);
}

FPjcDelegatePathSelectionChanged& SPjcTreeView::OnPathSelectionChanged()
{
	return DelegatePathSelectionChanged;
}

TSharedRef<SHeaderRow> SPjcTreeView::GetTreeViewHeaderRow() const
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

TSharedRef<ITableRow> SPjcTreeView::OnTreeViewGenerateRow(TSharedPtr<FPjcTreeViewItem> Item, const TSharedRef<STableViewBase>& OwnerTable) const
{
	return SNew(SPjcTreeViewItem, OwnerTable).Item(Item).HighlightText(SearchText);
}

void SPjcTreeView::OnTreeViewGetChildren(TSharedPtr<FPjcTreeViewItem> Item, TArray<TSharedPtr<FPjcTreeViewItem>>& OutChildren)
{
	if (!Item.IsValid()) return;
	if (Item->SubItems.Num() == 0) return;

	OutChildren.Append(Item->SubItems);
}

void SPjcTreeView::OnTreeViewSelectionChange(TSharedPtr<FPjcTreeViewItem> Item, ESelectInfo::Type SelectInfo) const
{
	if (!TreeView.IsValid()) return;

	const TArray<TSharedPtr<FPjcTreeViewItem>> SelectedItems = TreeView->GetSelectedItems();
	if (SelectedItems.Num() == 0) return;

	TArray<FName> SelectedPaths;

	SelectedPaths.Reserve(SelectedItems.Num());

	for (const auto& SelectedItem : SelectedItems)
	{
		if (!SelectedItem.IsValid()) continue;

		SelectedPaths.Emplace(SelectedItem->PathContent);
	}

	if (DelegatePathSelectionChanged.IsBound())
	{
		DelegatePathSelectionChanged.Execute(SelectedPaths);
	}
}

FSlateColor SPjcTreeView::GetTreeViewOptionsBtnForegroundColor() const
{
	static const FName InvertedForegroundName("InvertedForeground");
	static const FName DefaultForegroundName("DefaultForeground");

	if (!TreeViewOptionBtn.IsValid()) return FEditorStyle::GetSlateColor(DefaultForegroundName);

	return TreeViewOptionBtn->IsHovered() ? FEditorStyle::GetSlateColor(InvertedForegroundName) : FEditorStyle::GetSlateColor(DefaultForegroundName);
}

FText SPjcTreeView::GetSummaryText() const
{
	if (TreeView.IsValid() && TreeView->GetSelectedItems().Num() > 0)
	{
		const FString NumStr = FText::AsNumber(TreeView->GetSelectedItems().Num()).ToString();

		return FText::FromString(FString::Printf(TEXT("%s selected"), *NumStr));
	}

	return FText{};
}

void SPjcTreeView::TreeViewListUpdate()
{
	if (!TreeView.IsValid()) return;

	TArray<FString> Paths;
	FPjcLibAsset::GetCachedPaths(Paths);

	const FPjcScanDataAssets& ScanDataAssets = SubsystemPtr->GetLastScanDataAssets();

	SizesByPaths.Reset();
	PercentageByPaths.Reset();
	NumAssetsTotalByPaths.Reset();
	NumAssetsUsedByPaths.Reset();
	NumAssetsUnusedByPaths.Reset();

	SizesByPaths.Reserve(Paths.Num());
	PercentageByPaths.Reserve(Paths.Num());
	NumAssetsTotalByPaths.Reserve(Paths.Num());
	NumAssetsUsedByPaths.Reserve(Paths.Num());
	NumAssetsUnusedByPaths.Reserve(Paths.Num());

	TArray<FAssetData> AssetsTotalInPath;
	TSet<FAssetData> AssetsTotalInPathSet;
	const TSet<FAssetData> AssetsUsedSet{ScanDataAssets.AssetsUsed};
	const TSet<FAssetData> AssetsUnusedSet{ScanDataAssets.AssetsUnused};

	for (const auto& Path : Paths)
	{
		FPjcLibAsset::GetAssetsByPath(Path, true, AssetsTotalInPath);

		AssetsTotalInPathSet.Reset();
		AssetsTotalInPathSet.Append(AssetsTotalInPath);

		const TSet<FAssetData> AssetsUnusedInPath = AssetsTotalInPathSet.Difference(AssetsUsedSet);
		const TSet<FAssetData> AssetsUsedInPath = AssetsTotalInPathSet.Difference(AssetsUnusedSet);

		const int64 AssetsUnusedTotalSize = FPjcLibAsset::GetAssetsSize(AssetsUnusedInPath.Array());
		const float Percentage = AssetsTotalInPath.Num() == 0 ? 0 : AssetsUnusedInPath.Num() * 100.0f / AssetsTotalInPath.Num();
		const float PercentageNormalized = FMath::GetMappedRangeValueClamped(FVector2D{0.0f, 100.0f}, FVector2D{0.0f, 1.0f}, Percentage);

		SizesByPaths.FindOrAdd(Path, AssetsUnusedTotalSize);
		PercentageByPaths.FindOrAdd(Path, PercentageNormalized);

		NumAssetsTotalByPaths.FindOrAdd(Path, AssetsTotalInPath.Num());
		NumAssetsUsedByPaths.FindOrAdd(Path, AssetsUsedInPath.Num());
		NumAssetsUnusedByPaths.FindOrAdd(Path, AssetsUnusedInPath.Num());
	}

	TreeViewItems.Empty();

	const TSharedPtr<FPjcTreeViewItem> RootItem = CreateTreeItem(FPjcLibPath::ContentDir());
	if (!RootItem.IsValid()) return;

	TreeView->SetItemExpansion(RootItem, true);

	TArray<TSharedPtr<FPjcTreeViewItem>> Stack;
	Stack.Emplace(RootItem);

	while (Stack.Num() > 0)
	{
		const TSharedPtr<FPjcTreeViewItem> CurrentItem = Stack.Pop();

		TArray<FString> SubPaths;
		FPjcLibAsset::GetSubPaths(CurrentItem->PathContent, false, SubPaths);

		for (const auto& SubPath : SubPaths)
		{
			const TSharedPtr<FPjcTreeViewItem> SubItem = CreateTreeItem(FPjcLibPath::ToAbsolute(SubPath));
			if (!SubItem.IsValid()) continue;

			SubItem->ParentItem = CurrentItem;
			CurrentItem->SubItems.Emplace(SubItem);
			Stack.Emplace(SubItem);
		}
	}

	TreeViewItems.Emplace(RootItem);
	TreeView->RequestTreeRefresh();
}

void SPjcTreeView::OnScanAssets()
{
	TreeViewListUpdate();
}

TSharedPtr<FPjcTreeViewItem> SPjcTreeView::CreateTreeItem(const FString& InPath) const
{
	const FString PathAbs = FPjcLibPath::ToAbsolute(InPath);
	const FString PathName = FPjcLibPath::GetPathName(InPath);
	const FString PathContent = FPjcLibPath::ToAssetPath(InPath);
	const bool bIsDev = FPjcLibPath::IsUnderPath(InPath, FPjcLibPath::DevelopersDir());
	const bool bIsRoot = FPjcLibPath::ContentDir().Equals(InPath);
	const bool bIsEmpty = FPjcLibPath::IsPathEmpty(InPath);

	const int64 Size = SizesByPaths.Contains(PathContent) ? *SizesByPaths.Find(PathContent) : 0;
	const float Percent = PercentageByPaths.Contains(PathContent) ? *PercentageByPaths.Find(PathContent) : 0.0f;
	const int32 NumAssetsTotal = NumAssetsTotalByPaths.Contains(PathContent) ? *NumAssetsTotalByPaths.Find(PathContent) : 0;
	const int32 NumAssetsUsed = NumAssetsUsedByPaths.Contains(PathContent) ? *NumAssetsUsedByPaths.Find(PathContent) : 0;
	const int32 NumAssetsUnused = NumAssetsUnusedByPaths.Contains(PathContent) ? *NumAssetsUnusedByPaths.Find(PathContent) : 0;

	return MakeShareable(
		new FPjcTreeViewItem{
			Size,
			NumAssetsTotal,
			NumAssetsUsed,
			NumAssetsUnused,
			Percent,
			bIsDev,
			bIsRoot,
			bIsEmpty,
			false,
			false,
			PathAbs,
			PathName,
			PathContent
		}
	);
}

TSharedRef<SWidget> SPjcTreeView::GetTreeViewOptionsBtnContent() const
{
	const TSharedPtr<FExtender> Extender;
	FMenuBuilder MenuBuilder(true, nullptr, Extender, true);

	MenuBuilder.AddMenuSeparator(TEXT("View"));

	MenuBuilder.AddMenuEntry(
		FText::FromString(TEXT("Show Paths Empty")),
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
		FText::FromString(TEXT("Show Paths Excluded")),
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
