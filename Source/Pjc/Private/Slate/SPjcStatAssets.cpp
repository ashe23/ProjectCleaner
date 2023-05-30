// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/SPjcStatAssets.h"
#include "Slate/SPjcItemStat.h"
#include "PjcSubsystem.h"
#include "PjcStyles.h"
#include "PjcTypes.h"

void SPjcStatAssets::Construct(const FArguments& InArgs)
{
	SAssignNew(ListView, SListView<TSharedPtr<FPjcStatItem>>)
	.ListItemsSource(&Items)
	.OnGenerateRow(this, &SPjcStatAssets::OnGenerateRow)
	.SelectionMode(ESelectionMode::None)
	.IsFocusable(false)
	.HeaderRow(GetHeaderRow());

	UpdateView();

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
				.Text(FText::FromString(TEXT("Asset Statistics Summary")))
			]
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			ListView.ToSharedRef()
		]
	];
}

void SPjcStatAssets::UpdateView()
{
	TArray<FAssetData> AssetsAll;
	TArray<FAssetData> AssetsUsed;
	TArray<FAssetData> AssetsUnused;
	TArray<FAssetData> AssetsPrimary;
	TArray<FAssetData> AssetsIndirect;
	TArray<FAssetData> AssetsEditor;
	TArray<FAssetData> AssetsExcluded;
	TArray<FAssetData> AssetsExtReferenced;
	TArray<FPjcAssetIndirectInfo> AssetIndirectInfos;

	UPjcSubsystem::GetAssetsAll(AssetsAll);
	UPjcSubsystem::GetAssetsUsed(AssetsUsed, false);
	UPjcSubsystem::GetAssetsUnused(AssetsUnused, false);
	UPjcSubsystem::GetAssetsPrimary(AssetsPrimary, false);
	UPjcSubsystem::GetAssetsIndirect(AssetsIndirect, AssetIndirectInfos, false);
	UPjcSubsystem::GetAssetsEditor(AssetsEditor, false);
	UPjcSubsystem::GetAssetsExcluded(AssetsExcluded, false);
	UPjcSubsystem::GetAssetsExtReferenced(AssetsExtReferenced, false);

	const int32 NumAssetsAll = AssetsAll.Num();
	const int32 NumAssetsUsed = AssetsUsed.Num();
	const int32 NumAssetsUnused = AssetsUnused.Num();
	const int32 NumAssetsPrimary = AssetsPrimary.Num();
	const int32 NumAssetsIndirect = AssetsIndirect.Num();
	const int32 NumAssetsEditor = AssetsEditor.Num();
	const int32 NumAssetsExcluded = AssetsExcluded.Num();
	const int32 NumAssetsExtReferenced = AssetsExtReferenced.Num();

	const int64 SizeAssetsAll = UPjcSubsystem::GetAssetsTotalSize(AssetsAll);
	const int64 SizeAssetsUsed = UPjcSubsystem::GetAssetsTotalSize(AssetsUsed);
	const int64 SizeAssetsUnused = UPjcSubsystem::GetAssetsTotalSize(AssetsUnused);
	const int64 SizeAssetsPrimary = UPjcSubsystem::GetAssetsTotalSize(AssetsPrimary);
	const int64 SizeAssetsIndirect = UPjcSubsystem::GetAssetsTotalSize(AssetsIndirect);
	const int64 SizeAssetsEditor = UPjcSubsystem::GetAssetsTotalSize(AssetsEditor);
	const int64 SizeAssetsExcluded = UPjcSubsystem::GetAssetsTotalSize(AssetsExcluded);
	const int64 SizeAssetsExtReferenced = UPjcSubsystem::GetAssetsTotalSize(AssetsExtReferenced);
	
	Items.Reset();

	const FMargin FirstLvl{5.0f, 0.0f, 0.0f, 0.0f};
	const FMargin SecondLvl{20.0f, 0.0f, 0.0f, 0.0f};
	const auto ColorRed = FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Red").GetSpecifiedColor();
	const auto ColorYellow = FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Yellow").GetSpecifiedColor();

	Items.Emplace(
		MakeShareable(
			new FPjcStatItem{
				FText::FromString(TEXT("Unused")),
				FText::AsNumber(NumAssetsUnused),
				FText::AsMemory(SizeAssetsUnused, IEC),
				FText::FromString(TEXT("Unused Assets")),
				FText::FromString(TEXT("Total number of unused assets")),
				FText::FromString(TEXT("Total size of unused assets")),
				NumAssetsUnused > 0 ? ColorRed : FLinearColor::White,
				FirstLvl
			}
		)
	);

	Items.Emplace(
		MakeShareable(
			new FPjcStatItem{
				FText::FromString(TEXT("Used")),
				FText::AsNumber(NumAssetsUsed),
				FText::AsMemory(SizeAssetsUsed, IEC),
				FText::FromString(TEXT("Used Assets")),
				FText::FromString(TEXT("Total number of used assets")),
				FText::FromString(TEXT("Total size of used assets")),
				FLinearColor::White,
				FirstLvl
			}
		)
	);

	Items.Emplace(
		MakeShareable(
			new FPjcStatItem{
				FText::FromString(TEXT("Primary")),
				FText::AsNumber(NumAssetsPrimary),
				FText::AsMemory(SizeAssetsPrimary, IEC),
				FText::FromString(TEXT("Primary Assets that defined in AssetManager. Level assets are primary by default.")),
				FText::FromString(TEXT("Total number of primary assets")),
				FText::FromString(TEXT("Total size of primary assets")),
				FLinearColor::White,
				SecondLvl
			}
		)
	);

	Items.Emplace(
		MakeShareable(
			new FPjcStatItem{
				FText::FromString(TEXT("Editor")),
				FText::AsNumber(NumAssetsEditor),
				FText::AsMemory(SizeAssetsEditor, IEC),
				FText::FromString(TEXT("Editor specific assets. Like EditorUtilitWidgets or EditorTutorial assets.")),
				FText::FromString(TEXT("Total number of Editor assets")),
				FText::FromString(TEXT("Total size of Editor assets")),
				FLinearColor::White,
				SecondLvl
			}
		)
	);

	Items.Emplace(
		MakeShareable(
			new FPjcStatItem{
				FText::FromString(TEXT("Indirect")),
				FText::AsNumber(NumAssetsIndirect),
				FText::AsMemory(SizeAssetsIndirect, IEC),
				FText::FromString(TEXT("Assets that used in source code or config files.")),
				FText::FromString(TEXT("Total number of Indirect assets")),
				FText::FromString(TEXT("Total size of Indirect assets")),
				FLinearColor::White,
				SecondLvl
			}
		)
	);

	Items.Emplace(
		MakeShareable(
			new FPjcStatItem{
				FText::FromString(TEXT("ExtReferenced")),
				FText::AsNumber(NumAssetsExtReferenced),
				FText::AsMemory(SizeAssetsExtReferenced, IEC),
				FText::FromString(TEXT("Assets that have external referencers outside Content folder.")),
				FText::FromString(TEXT("Total number of ExtReferenced assets")),
				FText::FromString(TEXT("Total size of ExtReferenced assets")),
				FLinearColor::White,
				SecondLvl
			}
		)
	);

	Items.Emplace(
		MakeShareable(
			new FPjcStatItem{
				FText::FromString(TEXT("Excluded")),
				FText::AsNumber(NumAssetsExcluded),
				FText::AsMemory(SizeAssetsExcluded, IEC),
				FText::FromString(TEXT("Excluded Assets")),
				FText::FromString(TEXT("Total number of Excluded assets")),
				FText::FromString(TEXT("Total size of Excluded assets")),
				NumAssetsExcluded > 0 ? ColorYellow : FLinearColor::White,
				SecondLvl
			}
		)
	);

	Items.Emplace(
		MakeShareable(
			new FPjcStatItem{
				FText::FromString(TEXT("Total")),
				FText::AsNumber(NumAssetsAll),
				FText::AsMemory(SizeAssetsAll, IEC),
				FText::FromString(TEXT("All Assets")),
				FText::FromString(TEXT("Total number of assets")),
				FText::FromString(TEXT("Total size of assets")),
				FLinearColor::White,
				FirstLvl
			}
		)
	);

	if (ListView.IsValid())
	{
		ListView->RebuildList();
	}
}

TSharedRef<SHeaderRow> SPjcStatAssets::GetHeaderRow() const
{
	const FMargin HeaderMargin{5.0f};

	return
		SNew(SHeaderRow)
		+ SHeaderRow::Column("Name")
		  .FillWidth(0.4f)
		  .HAlignCell(HAlign_Left)
		  .VAlignCell(VAlign_Center)
		  .HAlignHeader(HAlign_Center)
		  .HeaderContentPadding(HeaderMargin)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Category")))
			.Font(FPjcStyles::GetFont("Light", 10.0f))
			.ColorAndOpacity(FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
		]
		+ SHeaderRow::Column("Num")
		  .FillWidth(0.3f)
		  .HAlignCell(HAlign_Center)
		  .VAlignCell(VAlign_Center)
		  .HAlignHeader(HAlign_Center)
		  .HeaderContentPadding(HeaderMargin)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Num")))
			.Font(FPjcStyles::GetFont("Light", 10.0f))
			.ColorAndOpacity(FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
		]
		+ SHeaderRow::Column("Size")
		  .FillWidth(0.3f)
		  .HAlignCell(HAlign_Center)
		  .VAlignCell(VAlign_Center)
		  .HAlignHeader(HAlign_Center)
		  .HeaderContentPadding(HeaderMargin)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Size")))
			.Font(FPjcStyles::GetFont("Light", 10.0f))
			.ColorAndOpacity(FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
		];
}

TSharedRef<ITableRow> SPjcStatAssets::OnGenerateRow(TSharedPtr<FPjcStatItem> Item, const TSharedRef<STableViewBase>& OwnerTable) const
{
	return SNew(SPjcItemStat, OwnerTable).Item(Item);
}
