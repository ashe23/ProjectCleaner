// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/AssetBrowser/SPjcTreeView.h"
#include "Slate/AssetBrowser/SPjcTreeViewItem.h"
#include "Libs/PjcLibPath.h"
#include "PjcStyles.h"
// Engine Headers
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"

void SPjcTreeView::Construct(const FArguments& InArgs)
{
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
				SAssignNew(TreeView, STreeView<TSharedPtr<FPjcTreeViewItem>>)
				.TreeItemsSource(&TreeViewItems)
				.SelectionMode(ESelectionMode::Multi)
				.OnGenerateRow(this, &SPjcTreeView::OnTreeViewGenerateRow)
				.OnGetChildren(this, &SPjcTreeView::OnTreeViewGetChildren)
				// .OnContextMenuOpening_Raw(this, &SPjcTabAssetsBrowser::OnTreeViewContextMenu)
				// .OnSelectionChanged(this, &SPjcTabAssetsBrowser::OnTreeViewSelectionChange)
				// .OnExpansionChanged_Raw(this, &SPjcTabAssetsBrowser::OnTreeViewExpansionChange)
				.HeaderRow(GetTreeViewHeaderRow())
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
		+ SHeaderRow::Column(TEXT("UnusedSize")).HAlignHeader(HAlign_Center).VAlignHeader(VAlign_Center).HeaderContentPadding(FMargin{5.0f}).FillWidth(0.3f)
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

	TreeViewItems.Empty();

	const TSharedPtr<FPjcTreeViewItem> RootItem = CreateTreeItem(FPjcLibPath::ContentDir());
	if (!RootItem.IsValid()) return;

	TreeView->SetItemExpansion(RootItem, true);

	TArray<TSharedPtr<FPjcTreeViewItem>> Stack;
	Stack.Emplace(RootItem);

	while (Stack.Num() > 0)
	{
		const TSharedPtr<FPjcTreeViewItem> CurrentItem = Stack.Pop();

		TSet<FString> SubPaths;
		FPjcLibPath::GetFoldersInPath(CurrentItem->PathAbs, false, SubPaths);

		for (const auto& SubPath : SubPaths)
		{
			const TSharedPtr<FPjcTreeViewItem> SubItem = CreateTreeItem(SubPath);
			if (!SubItem.IsValid()) continue;

			SubItem->ParentItem = CurrentItem;
			CurrentItem->SubItems.Emplace(SubItem);
			Stack.Emplace(SubItem);
		}
	}

	TreeViewItems.Emplace(RootItem);
	TreeView->RequestTreeRefresh();
}

TSharedPtr<FPjcTreeViewItem> SPjcTreeView::CreateTreeItem(const FString& InPath) const
{
	const FString PathAbs = FPjcLibPath::ToAbsolute(InPath);
	const FString PathName = FPjcLibPath::GetPathName(InPath);
	const FString PathContent = FPjcLibPath::ToAssetPath(InPath);
	const bool bIsDev = FPjcLibPath::IsUnderPath(InPath, FPjcLibPath::DevelopersDir());
	const bool bIsRoot = FPjcLibPath::ContentDir().Equals(InPath);
	const bool bIsEmpty = FPjcLibPath::IsPathEmpty(InPath);

	return MakeShareable(
		new FPjcTreeViewItem{
			FMath::RandRange(0.0f, 10000.0f),
			FMath::RandRange(0.0f, 1.0f),
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
			FExecuteAction::CreateLambda([&]
			{
				// SubsystemPtr->bShowPathsEmpty = !SubsystemPtr->bShowPathsEmpty;
				// SubsystemPtr->PostEditChange();
				//
				// TreeViewItemsUpdate();
			})
			// FCanExecuteAction::CreateLambda([&]()
			// {
			// 	return SubsystemPtr && SubsystemPtr->bShowPathsUnusedOnly == false;
			// }),
			// FGetActionCheckState::CreateLambda([&]()
			// {
			// 	return SubsystemPtr->bShowPathsEmpty ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
			// })
		),
		NAME_None,
		EUserInterfaceActionType::ToggleButton
	);

	return MenuBuilder.MakeWidget();
}
