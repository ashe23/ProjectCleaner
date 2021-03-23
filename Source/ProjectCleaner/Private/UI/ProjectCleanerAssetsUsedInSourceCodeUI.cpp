#include "UI/ProjectCleanerAssetsUsedInSourceCodeUI.h"

#define LOCTEXT_NAMESPACE "FProjectCleanerModule"

void SProjectCleanerAssetsUsedInSourceCodeUI::Construct(const FArguments& InArgs)
{
    AssetsUsedInSourceCode = InArgs._AssetsUsedInSourceCode;

    
    const FSlateFontInfo FontInfo = FSlateFontInfo(
        FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Light.ttf"),
        20
    );
 
	ChildSlot
	[
	    SNew(SVerticalBox)
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
            SNew(SListView<TWeakObjectPtr<UAssetsUsedInSourceCodeUIStruct>>)
            .ListItemsSource(&AssetsUsedInSourceCode)
            .SelectionMode(ESelectionMode::SingleToggle)
            .OnGenerateRow(this, &SProjectCleanerAssetsUsedInSourceCodeUI::OnGenerateRow)
            .OnMouseButtonDoubleClick_Raw(this, &SProjectCleanerAssetsUsedInSourceCodeUI::OnMouseDoubleClick)
            .HeaderRow
            (
                SNew(SHeaderRow)
                + SHeaderRow::Column(FName("AssetName"))
                .FillWidth(0.1f)
                [
                    SNew(STextBlock)
                    .Text(LOCTEXT("AssetName", "Asset Name"))
                ]
                + SHeaderRow::Column(FName("AssetPath"))
                .FillWidth(0.45f)
                [
                    SNew(STextBlock)
                    .Text(LOCTEXT("AssetPath", "Asset Path"))
                ]
                + SHeaderRow::Column(FName("SourceCodePath"))
                .FillWidth(0.45f)
                [
                    SNew(STextBlock)
                    .Text(LOCTEXT("SourceCodePath", "SourceCode file path where asset used"))
                ]
            )
        ]
	];
}

TSharedRef<ITableRow> SProjectCleanerAssetsUsedInSourceCodeUI::OnGenerateRow(
	TWeakObjectPtr<UAssetsUsedInSourceCodeUIStruct> InItem, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SAssetUsedInSourceCodeUISelectionRow, OwnerTable).SelectedRowItem(InItem);
}

void SProjectCleanerAssetsUsedInSourceCodeUI::OnMouseDoubleClick(TWeakObjectPtr<UAssetsUsedInSourceCodeUIStruct> Item)
{
    FPlatformProcess::ExploreFolder(*(FPaths::GetPath(Item->SourceCodePath)));
}


#undef LOCTEXT_NAMESPACE