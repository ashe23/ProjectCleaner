// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/Tabs/SProjectCleanerTabIndirectAssets.h"
#include "ProjectCleanerCmds.h"
#include "ProjectCleanerStyles.h"
#include "ProjectCleanerConstants.h"
#include "ProjectCleanerSubsystem.h"
// Engine Headers
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "Widgets/Layout/SScrollBox.h"

void SProjectCleanerTabIndirect::Construct(const FArguments& InArgs)
{
	if (!GEditor) return;
	SubsystemPtr = GEditor->GetEditorSubsystem<UProjectCleanerSubsystem>();
	if (!SubsystemPtr) return;

	Cmds = MakeShareable(new FUICommandList);
	Cmds->MapAction(
		FProjectCleanerCmds::Get().TabIndirectFileOpen,
		FUIAction(
			FExecuteAction::CreateLambda([&]()
			{
				if (!GEditor) return;
				if (!ListView.IsValid()) return;

				const auto& SelectedItems = ListView->GetSelectedItems();
				if (SelectedItems.Num() == 0) return;

				const FString FilePath = SelectedItems[0]->FilePath;
				if (FilePath.IsEmpty() || !FPaths::FileExists(FilePath)) return;

				FPlatformProcess::LaunchFileInDefaultExternalApplication(*FilePath);
			})
		)
	);

	Cmds->MapAction(
		FProjectCleanerCmds::Get().TabIndirectAssetOpen,
		FUIAction(
			FExecuteAction::CreateLambda([&]()
			{
				if (!GEditor) return;
				if (!ListView.IsValid()) return;

				const auto& SelectedItems = ListView->GetSelectedItems();
				if (SelectedItems.Num() == 0) return;

				TArray<UObject*> Assets;
				Assets.Reserve(SelectedItems.Num());

				for (const auto& Item : SelectedItems)
				{
					if (!Item.IsValid()) continue;
					if (!Item->AssetData.IsValid()) continue;

					Assets.Add(Item->AssetData.GetAsset());
				}

				GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAssets(Assets);
			})
		)
	);

	Cmds->MapAction(
		FProjectCleanerCmds::Get().TabIndirectLocateInBrowser,
		FUIAction(
			FExecuteAction::CreateLambda([&]()
			{
				if (!ListView.IsValid()) return;

				const auto& SelectedItems = ListView->GetSelectedItems();
				if (SelectedItems.Num() == 0) return;

				TArray<FAssetData> Assets;
				Assets.Reserve(SelectedItems.Num());

				for (const auto& SelectedItem : SelectedItems)
				{
					Assets.Add(SelectedItem->AssetData);
				}

				const FContentBrowserModule& ModuleContentBrowser = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
				ModuleContentBrowser.Get().SyncBrowserToAssets(Assets);
			})
		)
	);
	
	ListUpdate();

	ChildSlot
	[
		SNew(SOverlay)
		+ SOverlay::Slot()
		.Padding(5.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			  .Padding(FMargin{0.0f, 0.0f, 0.0f, 10.0f})
			  .AutoHeight()
			[
				SNew(STextBlock)
				.AutoWrapText(true)
				.Font(FProjectCleanerStyles::GetFont("Light", 15))
				.Text(FText::FromString(TEXT("List of assets used in source code, config or other files indirectly.")))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.AutoWrapText(true)
				.Font(FProjectCleanerStyles::GetFont("Light", 10))
				.Text(FText::FromString(TEXT("The following assets are considered used and will remain untouched.")))
			]
			+ SVerticalBox::Slot()
			  .Padding(FMargin{0.0f, 10.0f, 0.0f, 0.0f})
			  .AutoHeight()
			[
				SNew(STextBlock)
				.AutoWrapText(true)
				.Font(FProjectCleanerStyles::GetFont("Light", 8))
				.Text(FText::FromString(TEXT("Double click on row to navigate file in FileExplorer")))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.AutoWrapText(true)
				.Font(FProjectCleanerStyles::GetFont("Light", 8))
				.Text(FText::FromString(TEXT("Right click on row to open context menu")))
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
					ListView.ToSharedRef()
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.AutoWrapText(true)
				.Font(FProjectCleanerStyles::GetFont("Light", 8))
				.Text_Raw(this, &SProjectCleanerTabIndirect::GetListTextSummary)
			]
		]
	];

	SubsystemPtr->OnProjectScanned().AddRaw(this, &SProjectCleanerTabIndirect::OnProjectScanned);
}

SProjectCleanerTabIndirect::~SProjectCleanerTabIndirect()
{
	SubsystemPtr->OnProjectScanned().RemoveAll(this);
	SubsystemPtr = nullptr;
}

void SProjectCleanerTabIndirect::OnProjectScanned()
{
	ListUpdate();
}

void SProjectCleanerTabIndirect::ListUpdate()
{
	if (!SubsystemPtr) return;

	if (!ListView.IsValid())
	{
		SAssignNew(ListView, SListView<TSharedPtr<FProjectCleanerIndirectAssetInfo>>)
			.ListItemsSource(&ListItems)
			.SelectionMode(ESelectionMode::SingleToggle)
			.OnGenerateRow(this, &SProjectCleanerTabIndirect::OnListGenerateRow)
			.OnMouseButtonDoubleClick_Raw(this, &SProjectCleanerTabIndirect::OnListItemDblClick)
			.OnContextMenuOpening_Raw(this, &SProjectCleanerTabIndirect::OnListContextMenu)
			.HeaderRow(GetListHeaderRow());
	}

	ListItems.Reset();

	TotalSize = SubsystemPtr->GetAssetsTotalSize(SubsystemPtr->GetScanData().AssetsIndirect);

	for (const auto& IndirectAsset : SubsystemPtr->GetScanData().AssetsIndirectInfo)
	{
		const TSharedPtr<FProjectCleanerIndirectAssetInfo> NewItem = MakeShareable(new FProjectCleanerIndirectAssetInfo);
		if (!NewItem) continue;

		NewItem->AssetData = IndirectAsset.AssetData;
		NewItem->FilePath = IndirectAsset.FilePath;
		NewItem->LineNum = IndirectAsset.LineNum;
		ListItems.Add(NewItem);
	}

	ListSort();
	ListView->RequestListRefresh();
}

void SProjectCleanerTabIndirect::ListSort()
{
	if (ListSortMode == EColumnSortMode::Ascending)
	{
		ListItems.Sort([&]
		(
			const TSharedPtr<FProjectCleanerIndirectAssetInfo>& Data1,
			const TSharedPtr<FProjectCleanerIndirectAssetInfo>& Data2)
			{
				if (ListSortColumn.IsEqual(TEXT("AssetName")))
				{
					return Data1->AssetData.AssetName.Compare(Data2->AssetData.AssetName) < 0;
				}

				if (ListSortColumn.IsEqual(TEXT("AssetPath")))
				{
					return Data1->AssetData.PackagePath.Compare(Data2->AssetData.PackagePath) < 0;
				}

				if (ListSortColumn.IsEqual(TEXT("FilePath")))
				{
					return Data1->FilePath.Compare(Data2->FilePath) < 0;
				}

				return Data1->AssetData.PackagePath.Compare(Data2->AssetData.PackagePath) < 0;
			}
		);
	}

	if (ListSortMode == EColumnSortMode::Descending)
	{
		ListItems.Sort([&]
		(
			const TSharedPtr<FProjectCleanerIndirectAssetInfo>& Data1,
			const TSharedPtr<FProjectCleanerIndirectAssetInfo>& Data2)
			{
				if (ListSortColumn.IsEqual(TEXT("AssetName")))
				{
					return Data1->AssetData.AssetName.Compare(Data2->AssetData.AssetName) > 0;
				}

				if (ListSortColumn.IsEqual(TEXT("AssetPath")))
				{
					return Data1->AssetData.PackagePath.Compare(Data2->AssetData.PackagePath) > 0;
				}

				if (ListSortColumn.IsEqual(TEXT("FilePath")))
				{
					return Data1->FilePath.Compare(Data2->FilePath) > 0;
				}

				return Data1->AssetData.PackagePath.Compare(Data2->AssetData.PackagePath) > 0;
			}
		);
	}
}

void SProjectCleanerTabIndirect::OnListSort(EColumnSortPriority::Type SortPriority, const FName& Name, EColumnSortMode::Type SortMode)
{
	switch (ListSortMode)
	{
		case EColumnSortMode::Ascending:
			ListSortMode = EColumnSortMode::Descending;
			break;
		case EColumnSortMode::Descending:
			ListSortMode = EColumnSortMode::Ascending;
			break;
		case EColumnSortMode::None:
			ListSortMode = EColumnSortMode::Ascending;
			break;
		default:
			ListSortMode = EColumnSortMode::Descending;
	}

	ListSortColumn = Name;

	ListSort();

	if (ListView.IsValid())
	{
		ListView->RequestListRefresh();
	}
}

void SProjectCleanerTabIndirect::OnListItemDblClick(TSharedPtr<FProjectCleanerIndirectAssetInfo> Item) const
{
	if (!Item.IsValid()) return;

	const FString DirPath = FPaths::GetPath(Item->FilePath);
	if (!FPaths::DirectoryExists(DirPath)) return;

	FPlatformProcess::ExploreFolder(*DirPath);
}

TSharedPtr<SHeaderRow> SProjectCleanerTabIndirect::GetListHeaderRow()
{
	return
		SNew(SHeaderRow)
		+ SHeaderRow::Column(FName("AssetName"))
		  .HAlignCell(HAlign_Center)
		  .VAlignCell(VAlign_Center)
		  .HAlignHeader(HAlign_Center)
		  .HeaderContentPadding(FMargin(10.0f))
		  .FillWidth(0.2f)
		  .OnSort_Raw(this, &SProjectCleanerTabIndirect::OnListSort)
		[
			SNew(STextBlock)
			.ColorAndOpacity(FProjectCleanerStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
			.Font(FProjectCleanerStyles::GetFont("Light", ProjectCleanerConstants::HeaderRowFontSize))
			.Text(FText::FromString(TEXT("Asset Name")))
		]
		+ SHeaderRow::Column(FName("AssetPath"))
		  .HAlignCell(HAlign_Left)
		  .VAlignCell(VAlign_Center)
		  .HAlignHeader(HAlign_Center)
		  .HeaderContentPadding(FMargin(10.0f))
		  .FillWidth(0.2f)
		  .OnSort_Raw(this, &SProjectCleanerTabIndirect::OnListSort)
		[
			SNew(STextBlock)
			.ColorAndOpacity(FProjectCleanerStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
			.Font(FProjectCleanerStyles::GetFont("Light", ProjectCleanerConstants::HeaderRowFontSize))
			.Text(FText::FromString(TEXT("Asset Path")))
		]
		+ SHeaderRow::Column(FName("FilePath"))
		  .HAlignCell(HAlign_Left)
		  .VAlignCell(VAlign_Center)
		  .HAlignHeader(HAlign_Center)
		  .HeaderContentPadding(FMargin(10.0f))
		  .FillWidth(0.5f)
		  .OnSort_Raw(this, &SProjectCleanerTabIndirect::OnListSort)
		[
			SNew(STextBlock)
			.ColorAndOpacity(FProjectCleanerStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
			.Font(FProjectCleanerStyles::GetFont("Light", ProjectCleanerConstants::HeaderRowFontSize))
			.Text(FText::FromString(TEXT("File Path")))
		]
		+ SHeaderRow::Column(FName("Line"))
		  .HAlignCell(HAlign_Center)
		  .VAlignCell(VAlign_Center)
		  .HAlignHeader(HAlign_Center)
		  .HeaderContentPadding(FMargin(10.0f))
		  .FillWidth(0.1f)
		[
			SNew(STextBlock)
			.ColorAndOpacity(FProjectCleanerStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
			.Font(FProjectCleanerStyles::GetFont("Light", ProjectCleanerConstants::HeaderRowFontSize))
			.Text(FText::FromString(TEXT("Line Number")))
		];
}

TSharedPtr<SWidget> SProjectCleanerTabIndirect::OnListContextMenu() const
{
	FMenuBuilder MenuBuilder{true, Cmds};
	MenuBuilder.BeginSection(TEXT("Actions"), FText::FromString(TEXT("Actions")));
	{
		MenuBuilder.AddMenuEntry(FProjectCleanerCmds::Get().TabIndirectFileOpen);
		MenuBuilder.AddMenuEntry(FProjectCleanerCmds::Get().TabIndirectAssetOpen);
		MenuBuilder.AddMenuEntry(FProjectCleanerCmds::Get().TabIndirectLocateInBrowser);
	}
	MenuBuilder.EndSection();

	return MenuBuilder.MakeWidget();
}

TSharedRef<ITableRow> SProjectCleanerTabIndirect::OnListGenerateRow(TSharedPtr<FProjectCleanerIndirectAssetInfo> InItem, const TSharedRef<STableViewBase>& OwnerTable) const
{
	return SNew(SProjectCleanerTabIndirectItem, OwnerTable).ListItem(InItem);
}

FText SProjectCleanerTabIndirect::GetListTextSummary() const
{
	return FText::FromString(
		FString::Printf(
			TEXT("%d item%s. Total Size: %s"),
			ListItems.Num(),
			ListItems.Num() > 1 ? TEXT("s") : TEXT(""),
			*FText::AsMemory(TotalSize).ToString()
		)
	);
}
