// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/AssetStats/SPjcAssetStats.h"
#include "Slate/AssetStats/SPjcAssetStatsItem.h"
#include "PjcStyles.h"
#include "PjcSubsystem.h"
#include "Libs/PjcLibAsset.h"

void SPjcAssetStats::Construct(const FArguments& InArgs)
{
	Padding = InArgs._Padding;

	if (!GEditor) return;

	SubsystemPtr = GEditor->GetEditorSubsystem<UPjcSubsystem>();
	if (!SubsystemPtr) return;

	SubsystemPtr->OnScanAssets().AddRaw(this, &SPjcAssetStats::OnScanAssets);

	SAssignNew(ListView, SListView<TSharedPtr<FPjcAssetStatsItem>>)
	.ListItemsSource(&ListItems)
	.OnGenerateRow(this, &SPjcAssetStats::OnGenerateRow)
	.SelectionMode(ESelectionMode::None)
	.IsFocusable(false)
	.HeaderRow(GetHeaderRow());

	// initializing stats
	OnScanAssets(FPjcScanDataAssets{});

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(Padding)
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
				.Text(FText::FromString(TEXT("Project Assets Statistics")))
			]
		]
		+ SVerticalBox::Slot().FillHeight(1.0f).Padding(Padding)
		[
			ListView.ToSharedRef()
		]
	];
}

SPjcAssetStats::~SPjcAssetStats()
{
	if (!SubsystemPtr) return;

	SubsystemPtr->OnScanAssets().RemoveAll(this);
}

void SPjcAssetStats::OnScanAssets(const FPjcScanDataAssets& InScanDataAssets)
{
	if (!ListView.IsValid()) return;

	ListItems.Reset();

	const FMargin FirstLvl{5.0f, 0.0f, 0.0f, 0.0f};
	const FMargin SecondLvl{20.0f, 0.0f, 0.0f, 0.0f};

	const FLinearColor TextColorWhite = FPjcStyles::Get().GetSlateColor(TEXT("ProjectCleaner.Color.White")).GetSpecifiedColor();
	const FLinearColor TextColorRed = FPjcStyles::Get().GetSlateColor(TEXT("ProjectCleaner.Color.Red")).GetSpecifiedColor();
	const FLinearColor TextColorYellow = FPjcStyles::Get().GetSlateColor(TEXT("ProjectCleaner.Color.Yellow")).GetSpecifiedColor();

	ListItems.Emplace(
		MakeShareable(
			new FPjcAssetStatsItem{
				FPjcLibAsset::GetAssetsSize(InScanDataAssets.AssetsUnused),
				InScanDataAssets.AssetsUnused.Num(),
				TEXT("Unused"),
				FirstLvl,
				TextColorWhite,
				TextColorRed,
				TEXT("Unused Assets"),
				TEXT("Total number of unused assets in project."),
				TEXT("Total size of unused assets in project.")
			}
		)
	);

	ListItems.Emplace(
		MakeShareable(
			new FPjcAssetStatsItem{
				FPjcLibAsset::GetAssetsSize(InScanDataAssets.AssetsUsed),
				InScanDataAssets.AssetsUsed.Num(),
				TEXT("Used"),
				FirstLvl,
				TextColorWhite,
				TextColorWhite,
				TEXT("Used Assets"),
				TEXT("Total number of used assets in project."),
				TEXT("Total size of used assets in project.")
			}
		)
	);

	ListItems.Emplace(
		MakeShareable(
			new FPjcAssetStatsItem{
				FPjcLibAsset::GetAssetsSize(InScanDataAssets.AssetsPrimary),
				InScanDataAssets.AssetsPrimary.Num(),
				TEXT("Primary"),
				SecondLvl,
				TextColorWhite,
				TextColorWhite,
				TEXT("Primary Assets. See AssetManager class list for more information."),
				TEXT("Total number of primary assets in project."),
				TEXT("Total size of primary assets in project.")
			}
		)
	);

	ListItems.Emplace(
		MakeShareable(
			new FPjcAssetStatsItem{
				FPjcLibAsset::GetAssetsSize(InScanDataAssets.AssetsEditor),
				InScanDataAssets.AssetsEditor.Num(),
				TEXT("Editor"),
				SecondLvl,
				TextColorWhite,
				TextColorWhite,
				TEXT("Editor specific Assets, like EditorUtilityWidgets, EditoryUtilityBlueprints, EditorTutorials etc."),
				TEXT("Total number of editor assets in project."),
				TEXT("Total size of editor assets in project.")
			}
		)
	);

	TArray<FAssetData> AssetsIndirect;
	InScanDataAssets.AssetsIndirect.GetKeys(AssetsIndirect);

	ListItems.Emplace(
		MakeShareable(
			new FPjcAssetStatsItem{
				FPjcLibAsset::GetAssetsSize(AssetsIndirect),
				AssetsIndirect.Num(),
				TEXT("Indirect"),
				SecondLvl,
				TextColorWhite,
				TextColorWhite,
				TEXT("Assets that used in source code or config files."),
				TEXT("Total number of indirect assets in project."),
				TEXT("Total size of indirect assets in project.")
			}
		)
	);

	ListItems.Emplace(
		MakeShareable(
			new FPjcAssetStatsItem{
				FPjcLibAsset::GetAssetsSize(InScanDataAssets.AssetsExtReferenced),
				InScanDataAssets.AssetsExtReferenced.Num(),
				TEXT("ExtReferenced"),
				SecondLvl,
				TextColorWhite,
				TextColorWhite,
				TEXT("Assets that have external referencers outside 'Content' folder."),
				TEXT("Total number of ExtReferenced assets in project."),
				TEXT("Total size of ExtReferenced assets in project.")
			}
		)
	);

	ListItems.Emplace(
		MakeShareable(
			new FPjcAssetStatsItem{
				FPjcLibAsset::GetAssetsSize(InScanDataAssets.AssetsExcluded),
				InScanDataAssets.AssetsExcluded.Num(),
				TEXT("Excluded"),
				SecondLvl,
				TextColorWhite,
				TextColorYellow,
				TEXT("Assets that are excluded from settings manually."),
				TEXT("Total number of Excluded assets in project."),
				TEXT("Total size of Excluded assets in project.")
			}
		)
	);

	ListItems.Emplace(
		MakeShareable(
			new FPjcAssetStatsItem{
				FPjcLibAsset::GetAssetsSize(InScanDataAssets.AssetsAll),
				InScanDataAssets.AssetsAll.Num(),
				TEXT("Total"),
				FirstLvl,
				TextColorWhite,
				TextColorWhite,
				TEXT("All assets"),
				TEXT("Total number of assets in project."),
				TEXT("Total size of assets in project.")
			}
		)
	);

	ListView->RebuildList();
}

TSharedRef<ITableRow> SPjcAssetStats::OnGenerateRow(TSharedPtr<FPjcAssetStatsItem> Item, const TSharedRef<STableViewBase>& OwnerTable) const
{
	return SNew(SPjcAssetStatsItem, OwnerTable).Item(Item);
}

TSharedRef<SHeaderRow> SPjcAssetStats::GetHeaderRow() const
{
	const FMargin HeaderContentPadding{5.0f};

	return
		SNew(SHeaderRow)
		+ SHeaderRow::Column("Category").FillWidth(0.4f).HAlignCell(HAlign_Left).VAlignCell(VAlign_Center).HAlignHeader(HAlign_Center).HeaderContentPadding(HeaderContentPadding)
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
