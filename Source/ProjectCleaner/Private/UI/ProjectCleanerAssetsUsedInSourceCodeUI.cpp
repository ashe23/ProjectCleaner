#include "UI/ProjectCleanerAssetsUsedInSourceCodeUI.h"

#define LOCTEXT_NAMESPACE "FProjectCleanerModule"

void SProjectCleanerAssetsUsedInSourceCodeUI::Construct(const FArguments& InArgs)
{
	AssetsUsedInSourceCode = InArgs._AssetsUsedInSourceCode;

	RefreshUIContent();
 
	ChildSlot
	[
		WidgetRef
	];
}

void SProjectCleanerAssetsUsedInSourceCodeUI::RefreshUIContent()
{
	const FSlateFontInfo FontInfo = FSlateFontInfo(
		FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Light.ttf"),
		20
	);

	
	WidgetRef = SNew(SVerticalBox)
		+SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(STextBlock)
			.AutoWrapText(true)
			.Font(FontInfo)
			.Text(LOCTEXT("assetsusedinsourcecodefiles", "Assets used in source code files"))
		]
		+SVerticalBox::Slot()
		.Padding(FMargin{0.0f, 10.0f})
		.AutoHeight()
		[
			SNew(STextBlock)
			.AutoWrapText(true)
			.Font(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Light.ttf"),8))
			.Text(LOCTEXT("dblclickonrow", "Double click on row to open in Explorer"))
		]
		+SVerticalBox::Slot()
		.AutoHeight()
		.Padding(FMargin{0.0f, 20.0f})
		[
			SNew(SListView<TWeakObjectPtr<USourceCodeAsset>>)
			.ListItemsSource(&AssetsUsedInSourceCode)
			.SelectionMode(ESelectionMode::SingleToggle)
			.OnGenerateRow(this, &SProjectCleanerAssetsUsedInSourceCodeUI::OnGenerateRow)
			.OnMouseButtonDoubleClick_Raw(this, &SProjectCleanerAssetsUsedInSourceCodeUI::OnMouseDoubleClick)
			.HeaderRow
			(
				SNew(SHeaderRow)
				+ SHeaderRow::Column(FName("AssetName"))
				.HAlignCell(HAlign_Center)
				.VAlignCell(VAlign_Center)
				.HAlignHeader(HAlign_Center)
				.HeaderContentPadding(FMargin(10.0f))
				.FillWidth(0.15f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("AssetName", "Asset Name"))
				]
				+ SHeaderRow::Column(FName("AssetPath"))
				.HAlignCell(HAlign_Center)
				.VAlignCell(VAlign_Center)
				.HAlignHeader(HAlign_Center)
				.HeaderContentPadding(FMargin(10.0f))
				.FillWidth(0.25f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("AssetPath", "Asset Path"))
				]
				+ SHeaderRow::Column(FName("SourceCodePath"))
				.HAlignCell(HAlign_Center)
				.VAlignCell(VAlign_Center)
				.HAlignHeader(HAlign_Center)
				.HeaderContentPadding(FMargin(10.0f))
				.FillWidth(0.6f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("SourceCodePath", "SourceCode file path where asset used"))
				]
			)
		];

	ChildSlot
	[
		WidgetRef
	];
}

void SProjectCleanerAssetsUsedInSourceCodeUI::SetAssetsUsedInSourceCode(
	TArray<TWeakObjectPtr<USourceCodeAsset>>& NewAssetsUsedInSourceCode)
{
	AssetsUsedInSourceCode.Reset();
	AssetsUsedInSourceCode.Reserve(NewAssetsUsedInSourceCode.Num());
	
	AssetsUsedInSourceCode = NewAssetsUsedInSourceCode;

	RefreshUIContent();
}

TSharedRef<ITableRow> SProjectCleanerAssetsUsedInSourceCodeUI::OnGenerateRow(
	TWeakObjectPtr<USourceCodeAsset> InItem, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SAssetUsedInSourceCodeUISelectionRow, OwnerTable).SelectedRowItem(InItem);
}

void SProjectCleanerAssetsUsedInSourceCodeUI::OnMouseDoubleClick(TWeakObjectPtr<USourceCodeAsset> Item)
{
	FPlatformProcess::ExploreFolder(*(FPaths::GetPath(Item->SourceCodePath)));
}


#undef LOCTEXT_NAMESPACE