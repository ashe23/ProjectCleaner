// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/Tabs/SProjectCleanerTabIndirect.h"
#include "ProjectCleanerStyles.h"
#include "ProjectCleanerCmds.h"
#include "ProjectCleanerLibrary.h"
// Engine Headers
#include "ProjectCleanerConstants.h"
#include "Widgets/Layout/SScrollBox.h"

void SProjectCleanerTabIndirect::Construct(const FArguments& InArgs)
{
	CmdsRegister();
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
				.Text(FText::FromString(TEXT("List of assets used in source code, config or other files indirectly")))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.AutoWrapText(true)
				.Font(FProjectCleanerStyles::GetFont("Light", 8))
				.Text(FText::FromString(TEXT("Double click on row to open file location in Explorer")))
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
				.Text(FText::FromString(FString::Printf(TEXT("%d item%s"), ListItems.Num(), ListItems.Num() > 1 ? TEXT("s") : TEXT(""))))
			]
		]
	];
}

void SProjectCleanerTabIndirect::ListUpdate()
{
	ListItems.Reset();
	
	TArray<FProjectCleanerIndirectAsset> IndirectAssets;
	UProjectCleanerLibrary::GetAssetsIndirectAdvanced(IndirectAssets);

	for (const auto& IndirectAsset : IndirectAssets)
	{
		const TSharedPtr<FProjectCleanerIndirectAsset> NewItem = MakeShareable(new FProjectCleanerIndirectAsset);
		if (!NewItem) continue;

		NewItem->AssetData = IndirectAsset.AssetData;
		NewItem->FilePath = IndirectAsset.FilePath;
		NewItem->LineNum = IndirectAsset.LineNum;
		ListItems.Add(NewItem);
	}

	if (!ListView)
	{
		SAssignNew(ListView, SListView<TSharedPtr<FProjectCleanerIndirectAsset>>)
		.ListItemsSource(&ListItems)
		.SelectionMode(ESelectionMode::SingleToggle)
		.OnGenerateRow(this, &SProjectCleanerTabIndirect::OnGenerateRow)
		.OnMouseButtonDoubleClick_Raw(this, &SProjectCleanerTabIndirect::OnListItemDblClick)
		.OnContextMenuOpening_Raw(this, &SProjectCleanerTabIndirect::OnListContextMenu)
		.HeaderRow(GetListHeaderRow());
	}

	ListView->RequestListRefresh();
}

void SProjectCleanerTabIndirect::CmdsRegister()
{
	Cmds = MakeShareable(new FUICommandList);
	
	Cmds->MapAction(
		FProjectCleanerCmds::Get().OpenFile,
		FUIAction(
			FExecuteAction::CreateLambda([&]()
			{
				if (!GEditor) return;
				if (!ListView.IsValid()) return;

				const auto& SelectedItems = ListView->GetSelectedItems();
				if (SelectedItems.Num() == 0) return;

				const FString FilePath = SelectedItems[0]->FilePath;
				if (!FPaths::FileExists(FilePath)) return;
				
				FPlatformProcess::LaunchFileInDefaultExternalApplication(*FilePath);
			})
		)
	);

	Cmds->MapAction(
		FProjectCleanerCmds::Get().OpenAsset,
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
	
	Cmds->MapAction(
		FProjectCleanerCmds::Get().ShowInContentBrowser,
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
				
				UProjectCleanerLibrary::FocusOnAssets(Assets);
			})
		)
	);
}

TSharedPtr<SHeaderRow> SProjectCleanerTabIndirect::GetListHeaderRow() const
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
			.ColorAndOpacity(FProjectCleanerStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
			.Font(FProjectCleanerStyles::GetFont("Light", ProjectCleanerConstants::HeaderRowFontSize))
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
			.ColorAndOpacity(FProjectCleanerStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
			.Font(FProjectCleanerStyles::GetFont("Light", ProjectCleanerConstants::HeaderRowFontSize))
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
	MenuBuilder.BeginSection(TEXT("Actions"));
	{
		MenuBuilder.AddMenuEntry(FProjectCleanerCmds::Get().OpenAsset);
		MenuBuilder.AddMenuEntry(FProjectCleanerCmds::Get().OpenFile);
		MenuBuilder.AddMenuEntry(FProjectCleanerCmds::Get().ShowInContentBrowser);
	}
	MenuBuilder.EndSection();

	return MenuBuilder.MakeWidget();
}

void SProjectCleanerTabIndirect::OnListItemDblClick(TSharedPtr<FProjectCleanerIndirectAsset> Item) const
{
	if (!Item.IsValid()) return;

	const FString DirPath = FPaths::GetPath(Item->FilePath);
	if (!FPaths::DirectoryExists(DirPath)) return;

	FPlatformProcess::ExploreFolder(*DirPath);
}

TSharedRef<ITableRow> SProjectCleanerTabIndirect::OnGenerateRow(TSharedPtr<FProjectCleanerIndirectAsset> InItem, const TSharedRef<STableViewBase>& OwnerTable) const
{
	return SNew(SProjectCleanerTabIndirectItem, OwnerTable).ListItem(InItem);
}
