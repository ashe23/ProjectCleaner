// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/Shared/SPjcStatsBasic.h"
#include "Slate/Items/SPjcItemStat.h"
#include "PjcStyles.h"
#include "Libs/PjcLibAsset.h"

void SPjcStatsBasic::Construct(const FArguments& InArgs)
{
	Title = InArgs._Title;
	HeaderMargin = InArgs._HeaderMargin;

	SAssignNew(StatView, SListView<TSharedPtr<FPjcStatItem>>)
	.ListItemsSource(&StatItems)
	.OnGenerateRow(this, &SPjcStatsBasic::OnStatGenerateRow)
	.SelectionMode(ESelectionMode::None)
	.IsFocusable(false)
	.HeaderRow(GetStatHeaderRow());
	
	StatItemsUpdate();

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
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
				.Text(Title)
			]
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			StatView.ToSharedRef()
		]
	];
}

void SPjcStatsBasic::StatItemsUpdate()
{
	StatItems.Reset();

	const FMargin FirstLvl{5.0f, 0.0f, 0.0f, 0.0f};
	const FMargin SecondLvl{20.0f, 0.0f, 0.0f, 0.0f};
	const auto ColorRed = FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Red").GetSpecifiedColor();
	const auto ColorYellow = FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Yellow").GetSpecifiedColor();

	// todo:ashe23 change data update here later
	const TArray<FAssetData> AssetsUnused;
	const TArray<FAssetData> AssetsUsed;
	const TArray<FAssetData> AssetsPrimary;
	const TArray<FAssetData> AssetsEditor;
	const TArray<FAssetData> AssetsIndirect;
	const TArray<FAssetData> AssetsExcluded;
	const TArray<FAssetData> AssetsExtReferenced;
	const TArray<FAssetData> AssetsAll;

	StatItems.Emplace(
		MakeShareable(
			new FPjcStatItem{
				FText::FromString(TEXT("Unused")),
				FText::AsNumber(AssetsUnused.Num()),
				FText::AsMemory(FPjcLibAsset::GetAssetsTotalSize(AssetsUnused), IEC),
				FText::FromString(TEXT("Unused Assets")),
				FText::FromString(TEXT("Total number of unused assets")),
				FText::FromString(TEXT("Total size of unused assets")),
				AssetsUnused.Num() > 0 ? ColorRed : FLinearColor::White,
				FirstLvl
			}
		)
	);

	StatItems.Emplace(
		MakeShareable(
			new FPjcStatItem{
				FText::FromString(TEXT("Used")),
				FText::AsNumber(AssetsUsed.Num()),
				FText::AsMemory(FPjcLibAsset::GetAssetsTotalSize(AssetsUsed), IEC),
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
				FText::AsNumber(AssetsPrimary.Num()),
				FText::AsMemory(FPjcLibAsset::GetAssetsTotalSize(AssetsPrimary), IEC),
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
				FText::AsNumber(AssetsEditor.Num()),
				FText::AsMemory(FPjcLibAsset::GetAssetsTotalSize(AssetsEditor), IEC),
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
				FText::AsNumber(AssetsIndirect.Num()),
				FText::AsMemory(FPjcLibAsset::GetAssetsTotalSize(AssetsIndirect), IEC),
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
				FText::AsNumber(AssetsExtReferenced.Num()),
				FText::AsMemory(FPjcLibAsset::GetAssetsTotalSize(AssetsExtReferenced), IEC),
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
				FText::AsNumber(AssetsExcluded.Num()),
				FText::AsMemory(FPjcLibAsset::GetAssetsTotalSize(AssetsExcluded), IEC),
				FText::FromString(TEXT("Excluded Assets")),
				FText::FromString(TEXT("Total number of Excluded assets")),
				FText::FromString(TEXT("Total size of Excluded assets")),
				AssetsExcluded.Num() > 0 ? ColorYellow : FLinearColor::White,
				SecondLvl
			}
		)
	);

	StatItems.Emplace(
		MakeShareable(
			new FPjcStatItem{
				FText::FromString(TEXT("Total")),
				FText::AsNumber(AssetsAll.Num()),
				FText::AsMemory(FPjcLibAsset::GetAssetsTotalSize(AssetsAll), IEC),
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

TSharedRef<SHeaderRow> SPjcStatsBasic::GetStatHeaderRow() const
{
	return
		SNew(SHeaderRow)
		+ SHeaderRow::Column("Name").FillWidth(0.4f).HAlignCell(HAlign_Left).VAlignCell(VAlign_Center).HAlignHeader(HAlign_Center).HeaderContentPadding(HeaderMargin)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Category")))
			.ColorAndOpacity(FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
			.Font(FPjcStyles::GetFont("Light", 10.0f))
		]
		+ SHeaderRow::Column("Num").FillWidth(0.3f).HAlignCell(HAlign_Center).VAlignCell(VAlign_Center).HAlignHeader(HAlign_Center).HeaderContentPadding(HeaderMargin)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Num")))
			.ColorAndOpacity(FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
			.Font(FPjcStyles::GetFont("Light", 10.0f))
		]
		+ SHeaderRow::Column("Size").FillWidth(0.3f).HAlignCell(HAlign_Center).VAlignCell(VAlign_Center).HAlignHeader(HAlign_Center).HeaderContentPadding(HeaderMargin)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Size")))
			.ColorAndOpacity(FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
			.Font(FPjcStyles::GetFont("Light", 10.0f))
		];
}

TSharedRef<ITableRow> SPjcStatsBasic::OnStatGenerateRow(TSharedPtr<FPjcStatItem> Item, const TSharedRef<STableViewBase>& OwnerTable) const
{
	return SNew(SPjcItemStat, OwnerTable).Item(Item);
}
