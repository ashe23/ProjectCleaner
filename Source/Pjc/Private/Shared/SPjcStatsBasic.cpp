// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/Shared/SPjcStatsBasic.h"

#include "PjcConstants.h"
#include "Slate/Items/SPjcItemStat.h"
#include "PjcStyles.h"
#include "PjcSubsystem.h"
#include "Libs/PjcLibAsset.h"

void SPjcStatsBasic::Construct(const FArguments& InArgs)
{
	SubsystemPtr = GEditor->GetEditorSubsystem<UPjcSubsystem>();
	SubsystemPtr->OnScanAssetsSuccess().AddRaw(this, &SPjcStatsBasic::StatItemsUpdate);
	
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

SPjcStatsBasic::~SPjcStatsBasic()
{
	if (SubsystemPtr)
	{
		SubsystemPtr->OnScanAssetsSuccess().RemoveAll(this);
	}
}

void SPjcStatsBasic::StatItemsUpdate()
{
	StatItems.Reset();

	const FMargin FirstLvl{5.0f, 0.0f, 0.0f, 0.0f};
	const FMargin SecondLvl{20.0f, 0.0f, 0.0f, 0.0f};
	const auto ColorRed = FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Red").GetSpecifiedColor();
	const auto ColorYellow = FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Yellow").GetSpecifiedColor();

	StatItems.Emplace(
		MakeShareable(
			new FPjcStatItem{
				FText::FromString(TEXT("Unused")),
				FText::AsNumber(SubsystemPtr->GetAssetsUnused().Num()),
				FText::AsMemory(SubsystemPtr->GetSizeAssetsUnusedInPath(PjcConstants::PathRoot.ToString()), IEC),
				FText::FromString(TEXT("Unused Assets")),
				FText::FromString(TEXT("Total number of unused assets")),
				FText::FromString(TEXT("Total size of unused assets")),
				SubsystemPtr->GetAssetsUnused().Num() > 0 ? ColorRed : FLinearColor::White,
				FirstLvl
			}
		)
	);

	StatItems.Emplace(
		MakeShareable(
			new FPjcStatItem{
				FText::FromString(TEXT("Used")),
				FText::AsNumber(SubsystemPtr->GetAssetsUsed().Num()),
				FText::AsMemory(SubsystemPtr->GetSizeAssetsUsedInPath(PjcConstants::PathRoot.ToString()), IEC),
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
				FText::AsNumber(SubsystemPtr->GetAssetsPrimary().Num()),
				FText::AsMemory(FPjcLibAsset::GetAssetsTotalSize(SubsystemPtr->GetAssetsPrimary()), IEC),
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
				FText::AsNumber(SubsystemPtr->GetAssetsEditor().Num()),
				FText::AsMemory(FPjcLibAsset::GetAssetsTotalSize(SubsystemPtr->GetAssetsEditor()), IEC),
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
				FText::AsNumber(SubsystemPtr->GetAssetsIndirect().Num()),
				FText::AsMemory(FPjcLibAsset::GetAssetsTotalSize(SubsystemPtr->GetAssetsIndirect()), IEC),
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
				FText::AsNumber(SubsystemPtr->GetAssetsExtReferenced().Num()),
				FText::AsMemory(FPjcLibAsset::GetAssetsTotalSize(SubsystemPtr->GetAssetsExtReferenced()), IEC),
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
				FText::AsNumber(SubsystemPtr->GetAssetsExcluded().Num()),
				FText::AsMemory(FPjcLibAsset::GetAssetsTotalSize(SubsystemPtr->GetAssetsExcluded()), IEC),
				FText::FromString(TEXT("Excluded Assets")),
				FText::FromString(TEXT("Total number of Excluded assets")),
				FText::FromString(TEXT("Total size of Excluded assets")),
				SubsystemPtr->GetAssetsExcluded().Num() > 0 ? ColorYellow : FLinearColor::White,
				SecondLvl
			}
		)
	);

	StatItems.Emplace(
		MakeShareable(
			new FPjcStatItem{
				FText::FromString(TEXT("Total")),
				FText::AsNumber(SubsystemPtr->GetAssetsAll().Num()),
				FText::AsMemory(SubsystemPtr->GetSizeAssetsTotalInPath(PjcConstants::PathRoot.ToString()), IEC),
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
