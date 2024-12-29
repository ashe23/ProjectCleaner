// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/SPjcTabAssetsIndirect.h"
#include "Slate/SPjcItemAssetIndirect.h"
#include "PjcCmds.h"
#include "PjcStyles.h"
#include "PjcConstants.h"
#include "PjcSubsystem.h"
// Engine Headers
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SWidgetSwitcher.h"

void SPjcTabAssetsIndirect::Construct(const FArguments& InArgs)
{
	Cmds = MakeShareable(new FUICommandList);

	Cmds->MapAction(
		FPjcCmds::Get().Refresh,
		FExecuteAction::CreateRaw(this, &SPjcTabAssetsIndirect::OnRefresh)
	);

	Cmds->MapAction(
		FPjcCmds::Get().OpenViewerSizeMap,
		FExecuteAction::CreateRaw(this, &SPjcTabAssetsIndirect::OnOpenSizeMap),
		FCanExecuteAction::CreateRaw(this, &SPjcTabAssetsIndirect::AnyAssetSelected)
	);

	Cmds->MapAction(
		FPjcCmds::Get().OpenViewerReference,
		FExecuteAction::CreateRaw(this, &SPjcTabAssetsIndirect::OnOpenReferenceViewer),
		FCanExecuteAction::CreateRaw(this, &SPjcTabAssetsIndirect::AnyAssetSelected)
	);

	Cmds->MapAction(
		FPjcCmds::Get().OpenViewerAssetsAudit,
		FExecuteAction::CreateRaw(this, &SPjcTabAssetsIndirect::OnOpenAssetAudit),
		FCanExecuteAction::CreateRaw(this, &SPjcTabAssetsIndirect::AnyAssetSelected)
	);

	Cmds->MapAction(
		FPjcCmds::Get().ClearSelection,
		FExecuteAction::CreateRaw(this, &SPjcTabAssetsIndirect::OnClearSelection),
		FCanExecuteAction::CreateRaw(this, &SPjcTabAssetsIndirect::AnyAssetSelected)
	);

	SAssignNew(ListView, SListView<TSharedPtr<FPjcAssetIndirectInfo>>)
	.ListItemsSource(&ItemsFiltered)
	.SelectionMode(ESelectionMode::Multi)
	.OnGenerateRow(this, &SPjcTabAssetsIndirect::OnGenerateRow)
	.OnMouseButtonDoubleClick_Raw(this, &SPjcTabAssetsIndirect::OnMouseDoubleClicked)
	.HeaderRow(GetHeaderRow());

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(5.0f)
		[
			CreateToolbar()
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(5.0f)
		[
			SNew(SSeparator).Thickness(5.0f)
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(5.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(FMargin{10.0f, 0.0f, 5.0f, 5.0f})
			[
				SNew(STextBlock)
				.Justification(ETextJustify::Center)
				.ColorAndOpacity(FPjcStyles::Get().GetColor("ProjectCleaner.Color.Gray"))
				.ShadowOffset(FVector2D{0.5f, 0.5f})
				.ShadowColorAndOpacity(FLinearColor::Black)
				.Font(FPjcStyles::GetFont("Bold", 15))
				.Text(FText::FromString(TEXT("List of assets used in source code or config files.")))
			]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(5.0f)
		[
			SNew(SSeparator).Thickness(5.0f)
		]
		+ SVerticalBox::Slot().FillHeight(1.0f).Padding(5.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(5.0f)
			[
				SNew(SSearchBox)
				.HintText(FText::FromString(TEXT("Search ...")))
				.OnTextChanged_Raw(this, &SPjcTabAssetsIndirect::OnSearchTextChanged)
				.OnTextCommitted_Raw(this, &SPjcTabAssetsIndirect::OnSearchTextCommitted)
			]
			+ SVerticalBox::Slot().FillHeight(1.0f).Padding(5.0f)
			[
				SNew(SWidgetSwitcher)
				.WidgetIndex_Raw(this, &SPjcTabAssetsIndirect::GetWidgetIndex)
				+ SWidgetSwitcher::Slot()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.0f).HAlign(HAlign_Center).VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Justification(ETextJustify::Center)
						.ColorAndOpacity(FPjcStyles::Get().GetColor("ProjectCleaner.Color.Gray"))
						.ShadowOffset(FVector2D{0.5f, 0.5f})
						.ShadowColorAndOpacity(FLinearColor::Black)
						.Font(FPjcStyles::GetFont("Bold", 15))
						.Text(FText::FromString(TEXT("No indirect assets were found.")))
					]
				]
				+ SWidgetSwitcher::Slot()
				[
					SNew(SScrollBox)
					.ScrollWhenFocusChanges(EScrollWhenFocusChanges::NoScroll)
					.AnimateWheelScrolling(true)
					.AllowOverscroll(EAllowOverscroll::No)
					+ SScrollBox::Slot().Padding(5.0f)
					[
						ListView.ToSharedRef()
					]
				]
			]
		]
	];
}

TSharedRef<SWidget> SPjcTabAssetsIndirect::CreateToolbar() const
{
	FToolBarBuilder ToolBarBuilder{Cmds, FMultiBoxCustomization::None};
	ToolBarBuilder.BeginSection(NAME_None);
	{
		ToolBarBuilder.AddToolBarButton(
			FPjcCmds::Get().Refresh,
			NAME_None,
			FText::FromString(TEXT("Scan")),
			FText::FromString(TEXT("Scan for indirect assets and their usage info"))
		);
		ToolBarBuilder.AddSeparator();
		ToolBarBuilder.AddToolBarButton(FPjcCmds::Get().OpenViewerSizeMap);
		ToolBarBuilder.AddToolBarButton(FPjcCmds::Get().OpenViewerReference);
		ToolBarBuilder.AddToolBarButton(FPjcCmds::Get().OpenViewerAssetsAudit);
		ToolBarBuilder.AddSeparator();
		ToolBarBuilder.AddToolBarButton(FPjcCmds::Get().ClearSelection);
	}
	ToolBarBuilder.EndSection();

	return ToolBarBuilder.MakeWidget();
}

TSharedRef<SHeaderRow> SPjcTabAssetsIndirect::GetHeaderRow()
{
	return
			SNew(SHeaderRow)
			+ SHeaderRow::Column(TEXT("AssetName"))
			.HAlignHeader(HAlign_Center)
			.VAlignHeader(VAlign_Center)
			.FillWidth(0.1f)
			.HeaderContentPadding(FMargin{5.0f})
			.OnSort_Raw(this, &SPjcTabAssetsIndirect::OnListSort)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("Asset Name")))
				.ColorAndOpacity(FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
				.Font(FPjcStyles::GetFont("Light", 10.0f))
				.ToolTipText(FText::FromString(TEXT("Indirect asset name")))
			]
			+ SHeaderRow::Column(TEXT("AssetPath"))
			.HAlignHeader(HAlign_Center)
			.VAlignHeader(VAlign_Center)
			.FillWidth(0.4f)
			.HeaderContentPadding(FMargin{5.0f})
			.OnSort_Raw(this, &SPjcTabAssetsIndirect::OnListSort)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("Asset Path")))
				.ColorAndOpacity(FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
				.Font(FPjcStyles::GetFont("Light", 10.0f))
				.ToolTipText(FText::FromString(TEXT("Indirect asset path in content browser")))
			]
			+ SHeaderRow::Column(TEXT("FilePath"))
			.HAlignHeader(HAlign_Center)
			.VAlignHeader(VAlign_Center)
			.FillWidth(0.4f)
			.HeaderContentPadding(FMargin{5.0f})
			.OnSort_Raw(this, &SPjcTabAssetsIndirect::OnListSort)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("File Path")))
				.ColorAndOpacity(FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
				.Font(FPjcStyles::GetFont("Light", 10.0f))
				.ToolTipText(FText::FromString(TEXT("Absolute file path where asset is used.")))
			]
			+ SHeaderRow::Column(TEXT("FileLine"))
			.HAlignHeader(HAlign_Center)
			.VAlignHeader(VAlign_Center)
			.FillWidth(0.1f)
			.HeaderContentPadding(FMargin{5.0f})
			.OnSort_Raw(this, &SPjcTabAssetsIndirect::OnListSort)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("File Line")))
				.ColorAndOpacity(FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
				.Font(FPjcStyles::GetFont("Light", 10.0f))
				.ToolTipText(FText::FromString(TEXT("File line number where asset is used")))
			];
}

TSharedRef<ITableRow> SPjcTabAssetsIndirect::OnGenerateRow(TSharedPtr<FPjcAssetIndirectInfo> Item, const TSharedRef<STableViewBase>& OwnerTable) const
{
	return SNew(SPjcItemAssetIndirect, OwnerTable).Item(Item).HighlightText(SearchText);
}

void SPjcTabAssetsIndirect::OnMouseDoubleClicked(TSharedPtr<FPjcAssetIndirectInfo> Item)
{
	if (!Item.IsValid()) return;

	UPjcSubsystem::OpenAssetEditor(Item->Asset);
}

void SPjcTabAssetsIndirect::ListUpdateData()
{
	TArray<FAssetData> AssetsIndirect;
	TArray<FPjcAssetIndirectInfo> AssetIndirectInfos;

	UPjcSubsystem::GetAssetsIndirect(AssetsIndirect, AssetIndirectInfos, true);

	ItemsAll.Reset(AssetIndirectInfos.Num());

	for (const auto& AssetIndirectInfo : AssetIndirectInfos)
	{
		const TSharedPtr<FPjcAssetIndirectInfo> Item = MakeShareable(new FPjcAssetIndirectInfo{AssetIndirectInfo.Asset, AssetIndirectInfo.FilePath, AssetIndirectInfo.FileNum});
		if (Item.IsValid())
		{
			ItemsAll.Emplace(Item);
		}
	}
}

void SPjcTabAssetsIndirect::ListUpdateView()
{
	if (!ListView.IsValid()) return;
	if (ItemsAll.Num() == 0) return;

	ItemsFiltered.Reset(ItemsAll.Num());

	const FString SearchStr = SearchText.ToString();

	for (const auto& Item : ItemsAll)
	{
		if (
			!Item.IsValid() ||
			(
				!SearchText.IsEmpty() &&
				!Item->FilePath.Contains(SearchStr) &&
				!Item->Asset.AssetName.ToString().Contains(SearchStr) &&
				!Item->Asset.PackagePath.ToString().Contains(SearchStr)
			)
		)
		{
			continue;
		}

		ItemsFiltered.Emplace(MakeShareable(new FPjcAssetIndirectInfo{Item->Asset, Item->FilePath, Item->FileNum}));
	}

	ListView->ClearSelection();
	ListView->ClearHighlightedItems();
	ListView->RebuildList();
}

void SPjcTabAssetsIndirect::OnListSort(EColumnSortPriority::Type SortPriority, const FName& ColumnName, EColumnSortMode::Type InSortMode)
{
	if (!ListView.IsValid()) return;

	auto SortListItems = [&](auto& SortMode, auto SortFunc)
	{
		SortMode = SortMode == EColumnSortMode::Ascending ? EColumnSortMode::Descending : EColumnSortMode::Ascending;

		ItemsFiltered.Sort(SortFunc);
	};

	if (ColumnName.IsEqual(TEXT("AssetName")))
	{
		SortListItems(ColumnSortModeAssetName, [&](const TSharedPtr<FPjcAssetIndirectInfo>& Item1, const TSharedPtr<FPjcAssetIndirectInfo>& Item2)
		{
			return ColumnSortModeAssetName == EColumnSortMode::Ascending
				       ? Item1->Asset.AssetName.ToString() < Item2->Asset.AssetName.ToString()
				       : Item1->Asset.AssetName.ToString() > Item2->Asset.AssetName.ToString();
		});
	}

	if (ColumnName.IsEqual(TEXT("AssetPath")))
	{
		SortListItems(ColumnSortModeAssetPath, [&](const TSharedPtr<FPjcAssetIndirectInfo>& Item1, const TSharedPtr<FPjcAssetIndirectInfo>& Item2)
		{
			return ColumnSortModeAssetPath == EColumnSortMode::Ascending
				       ? Item1->Asset.PackagePath.ToString() < Item2->Asset.PackagePath.ToString()
				       : Item1->Asset.PackagePath.ToString() > Item2->Asset.PackagePath.ToString();
		});
	}

	if (ColumnName.IsEqual(TEXT("FilePath")))
	{
		SortListItems(ColumnSortModeFilePath, [&](const TSharedPtr<FPjcAssetIndirectInfo>& Item1, const TSharedPtr<FPjcAssetIndirectInfo>& Item2)
		{
			return ColumnSortModeFilePath == EColumnSortMode::Ascending ? Item1->FilePath < Item2->FilePath : Item1->FilePath > Item2->FilePath;
		});
	}

	if (ColumnName.IsEqual(TEXT("FileLine")))
	{
		SortListItems(ColumnSortModeFileLine, [&](const TSharedPtr<FPjcAssetIndirectInfo>& Item1, const TSharedPtr<FPjcAssetIndirectInfo>& Item2)
		{
			return ColumnSortModeFileLine == EColumnSortMode::Ascending ? Item1->FileNum < Item2->FileNum : Item1->FileNum > Item2->FileNum;
		});
	}

	ListView->RebuildList();
}

void SPjcTabAssetsIndirect::OnSearchTextChanged(const FText& InText)
{
	SearchText = InText;
	ListUpdateView();
}

void SPjcTabAssetsIndirect::OnSearchTextCommitted(const FText& InText, ETextCommit::Type)
{
	SearchText = InText;
	ListUpdateView();
}

int32 SPjcTabAssetsIndirect::GetWidgetIndex() const
{
	return ItemsAll.Num() == 0 ? PjcConstants::WidgetIndexIdle : PjcConstants::WidgetIndexWorking;
}

void SPjcTabAssetsIndirect::OnRefresh()
{
	ListUpdateData();
	ListUpdateView();
}

void SPjcTabAssetsIndirect::OnOpenSizeMap() const
{
	const auto SelectedItems = ListView->GetSelectedItems();

	TArray<FAssetData> Assets;
	Assets.Reserve(SelectedItems.Num());

	for (const auto& SelectedItem : SelectedItems)
	{
		if (!SelectedItem.IsValid()) continue;

		Assets.Emplace(SelectedItem->Asset);
	}

	UPjcSubsystem::OpenSizeMapViewer(Assets);
}

void SPjcTabAssetsIndirect::OnOpenReferenceViewer() const
{
	const auto SelectedItems = ListView->GetSelectedItems();

	TArray<FAssetData> Assets;
	Assets.Reserve(SelectedItems.Num());

	for (const auto& SelectedItem : SelectedItems)
	{
		if (!SelectedItem.IsValid()) continue;

		Assets.Emplace(SelectedItem->Asset);
	}

	UPjcSubsystem::OpenReferenceViewer(Assets);
}

void SPjcTabAssetsIndirect::OnOpenAssetAudit() const
{
	const auto SelectedItems = ListView->GetSelectedItems();

	TArray<FAssetData> Assets;
	Assets.Reserve(SelectedItems.Num());

	for (const auto& SelectedItem : SelectedItems)
	{
		if (!SelectedItem.IsValid()) continue;

		Assets.Emplace(SelectedItem->Asset);
	}

	UPjcSubsystem::OpenAssetAuditViewer(Assets);
}

void SPjcTabAssetsIndirect::OnClearSelection() const
{
	ListView->ClearSelection();
	ListView->ClearHighlightedItems();
}

bool SPjcTabAssetsIndirect::AnyAssetSelected() const
{
	return ListView.IsValid() && ListView->GetSelectedItems().Num() > 0;
}
