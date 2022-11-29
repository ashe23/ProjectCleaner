// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/ProjectCleanerWindowIndirectAssets.h"
#include "ProjectCleanerStyles.h"
#include "ProjectCleanerCmds.h"
// Engine Headers
#include "ContentBrowserModule.h"
#include "ProjectCleanerLibrary.h"
#include "Widgets/Layout/SScrollBox.h"

void SProjectCleanerWindowIndirectAssets::Construct(const FArguments& InArgs)
{
	for (const auto& Item : InArgs._ListItems)
	{
		const TSharedPtr<FProjectCleanerIndirectAsset> NewItem = MakeShareable(new FProjectCleanerIndirectAsset);
		if (!NewItem) continue;

		NewItem->AssetData = Item.AssetData;
		NewItem->FilePath = Item.FilePath;
		NewItem->LineNum = Item.LineNum;
		ListItems.Add(NewItem);
	}

	Cmds = MakeShareable(new FUICommandList);
	Cmds->MapAction(
		FProjectCleanerCmds::Get().Cmd_ShowInContentBrowser,
		FUIAction(
			FExecuteAction::CreateLambda([&]()
			{
				if (!ListView.IsValid()) return;

				const auto& SelectedItems = ListView->GetSelectedItems();
				if (SelectedItems.Num() == 0) return;

				UProjectCleanerLibrary::FocusOnDirectory(SelectedItems[0]->AssetData.PackagePath.ToString());
			})
		)
	);

	Cmds->MapAction(
		FProjectCleanerCmds::Get().Cmd_OpenAsset,
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
					Assets.Add(Item->AssetData.GetAsset());
				}

				GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAssets(Assets);
			})
		)
	);

	if (!ListView)
	{
		SAssignNew(ListView, SListView<TSharedPtr<FProjectCleanerIndirectAsset>>)
		.ListItemsSource(&ListItems)
		.SelectionMode(ESelectionMode::SingleToggle)
		.OnGenerateRow(this, &SProjectCleanerWindowIndirectAssets::OnGenerateRow)
		.OnMouseButtonDoubleClick_Raw(this, &SProjectCleanerWindowIndirectAssets::OnListItemDblClick)
		.OnContextMenuOpening_Raw(this, &SProjectCleanerWindowIndirectAssets::OnListContextMenu)
		.HeaderRow(GetListHeaderRow());
	}

	ListView->RequestListRefresh();

	ChildSlot
	[
		SNew(SOverlay)
		+ SOverlay::Slot()
		.Padding(20.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			  .Padding(FMargin{0.0f, 0.0f, 0.0f, 10.0f})
			  .AutoHeight()
			[
				SNew(STextBlock)
				.AutoWrapText(true)
				.Font(FProjectCleanerStyles::GetFont("Light", 15))
				.Text(FText::FromString(TEXT("List of assets that are used in source code, config or other files indirectly")))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.AutoWrapText(true)
				.Font(FProjectCleanerStyles::GetFont("Light", 8))
				.Text(FText::FromString(TEXT("Double click on row to open in Explorer")))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.AutoWrapText(true)
				.Font(FProjectCleanerStyles::GetFont("Light", 8))
				.Text(FText::FromString(TEXT("Right click for ContextMenu")))
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
		]
	];
}

TSharedPtr<SHeaderRow> SProjectCleanerWindowIndirectAssets::GetListHeaderRow() const
{
	return
		SNew(SHeaderRow)
		+ SHeaderRow::Column(FName("AssetName"))
		  .HAlignCell(HAlign_Center)
		  .VAlignCell(VAlign_Center)
		  .HAlignHeader(HAlign_Center)
		  .HeaderContentPadding(FMargin(10.0f))
		  .FillWidth(0.2f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Asset Name")))
		]
		+ SHeaderRow::Column(FName("AssetPath"))
		  .HAlignCell(HAlign_Center)
		  .VAlignCell(VAlign_Center)
		  .HAlignHeader(HAlign_Center)
		  .HeaderContentPadding(FMargin(10.0f))
		  .FillWidth(0.2f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Asset Path")))
		]
		+ SHeaderRow::Column(FName("FilePath"))
		  .HAlignCell(HAlign_Center)
		  .VAlignCell(VAlign_Center)
		  .HAlignHeader(HAlign_Center)
		  .HeaderContentPadding(FMargin(10.0f))
		  .FillWidth(0.5f)
		[
			SNew(STextBlock)
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
			.Text(FText::FromString(TEXT("Line Number")))
		];
}

TSharedPtr<SWidget> SProjectCleanerWindowIndirectAssets::OnListContextMenu() const
{
	FMenuBuilder MenuBuilder{true, Cmds};
	MenuBuilder.BeginSection(TEXT("Actions"));
	{
		MenuBuilder.AddMenuEntry(FProjectCleanerCmds::Get().Cmd_ShowInContentBrowser);
		MenuBuilder.AddMenuEntry(FProjectCleanerCmds::Get().Cmd_OpenAsset);
	}
	MenuBuilder.EndSection();

	return MenuBuilder.MakeWidget();
}

void SProjectCleanerWindowIndirectAssets::OnListItemDblClick(TSharedPtr<FProjectCleanerIndirectAsset> Item) const
{
	if (!Item.IsValid()) return;

	const FString DirPath = FPaths::GetPath(Item->FilePath);
	if (!FPaths::DirectoryExists(DirPath)) return;

	FPlatformProcess::ExploreFolder(*DirPath);
}

TSharedRef<ITableRow> SProjectCleanerWindowIndirectAssets::OnGenerateRow(TSharedPtr<FProjectCleanerIndirectAsset> InItem, const TSharedRef<STableViewBase>& OwnerTable) const
{
	return SNew(SProjectCleanerIndirectAssetListItem, OwnerTable).ListItem(InItem);
}
