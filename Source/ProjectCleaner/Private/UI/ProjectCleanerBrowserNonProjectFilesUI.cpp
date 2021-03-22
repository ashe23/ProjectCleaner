#include "UI/ProjectCleanerBrowserNonProjectFilesUI.h"
#include "IPropertyTable.h"
#include "IPropertyTableColumn.h"
#include "PropertyEditorModule.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "FProjectCleanerModule"
#pragma optimize("", off)
void SProjectCleanerBrowserNonProjectFilesUI::Construct(const FArguments& InArgs)
{
	FPropertyEditorModule& PropertyEditor = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

	FDetailsViewArgs NonProjectFilesSettings;
	NonProjectFilesSettings.bUpdatesFromSelection = false;
	NonProjectFilesSettings.bLockable = false;
	NonProjectFilesSettings.bShowOptions = false;
	NonProjectFilesSettings.bAllowFavoriteSystem = false;
	NonProjectFilesSettings.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	NonProjectFilesSettings.bShowPropertyMatrixButton = false;
	NonProjectFilesSettings.ViewIdentifier = "NonProjectFileList";

	NonProjectFilesProperty = PropertyEditor.CreateDetailView(NonProjectFilesSettings);

	if (InArgs._NonProjectFiles)
	{
		NonUProjectFilesInfo = InArgs._NonProjectFiles;
		NonProjectFilesProperty->SetObject(NonUProjectFilesInfo);
	}

	const FSlateFontInfo FontInfo = FSlateFontInfo(
		FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Light.ttf"),
		20
	);

	// converting paths of directories to full path
	TArray<FString> EmptyFoldersConverted;
	for (auto& Folder : NonUProjectFilesInfo->EmptyFolders)
	{
		EmptyFoldersConverted.AddUnique(FPaths::ConvertRelativePathToFull(Folder));
	}
	NonUProjectFilesInfo->EmptyFolders = EmptyFoldersConverted;

	auto TestObj = NewObject<UAssetsUsedInSourceCode>();
	TestObj->AssetName = "AA1";
	TestObj->AssetPath = "/Game/Content/AA1";
	TestObj->SourceCodePath = "/Source/main.cpp";
	AssetsUsedInSourceCodes.Add(TestObj);
	
	auto TestObj2 = NewObject<UAssetsUsedInSourceCode>();
	TestObj2->AssetName = "AA2";
	TestObj2->AssetPath = "/Game/Content/AA2";
	TestObj2->SourceCodePath = "/Source/main1.cpp";
	AssetsUsedInSourceCodes.Add(TestObj2);
	
	ChildSlot
	[
		SNew(SBorder)
		.Padding(FMargin(40.0f, 20.0f, 40.0f, 20.0f))
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
        .BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
		[
			SNew(SSplitter)
			.Orientation(EOrientation::Orient_Vertical)		
			+SSplitter::Slot()
			.Value(0.8f)
			[
				NonProjectFilesProperty.ToSharedRef()
			]
			+SSplitter::Slot()
			.Value(0.2f)
			[
				SNew(SVerticalBox)
				+SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(STextBlock)
					.Text(LOCTEXT("assetsusedinsourcecode","Assets that used in source code files"))
					.Font(FontInfo)
				]
				+SVerticalBox::Slot()
				.Padding(FMargin{0.0f, 20.0f})
				.AutoHeight()
				[
					SNew(SListView<TWeakObjectPtr<UAssetsUsedInSourceCode>>)
					.ListItemsSource(&AssetsUsedInSourceCodes)
					.OnGenerateRow(this, &SProjectCleanerBrowserNonProjectFilesUI::OnGenerateRow)
					.HeaderRow
	                    (
	                        SNew(SHeaderRow)
	                        + SHeaderRow::Column(FName("AssetName"))
	                        .FillWidth(0.3f)
	                        [
	                            SNew(STextBlock)
	                            .Text(LOCTEXT("NameColumn", "AssetName"))
	                        ]
	                        + SHeaderRow::Column(FName("AssetPath"))
	                        .FillWidth(0.3f)
	                        [
	                            SNew(STextBlock)
	                            .Text(LOCTEXT("PathColumn", "AssetPath"))
	                        ]
	                        + SHeaderRow::Column(FName("SourceCodePath"))
	                        .FillWidth(0.3f)
	                        [
	                            SNew(STextBlock)
	                            .Text(LOCTEXT("SourceCodePathColumn", "SourceCodePath"))
	                        ]
	                    )
				]
			]
		]
	];
}

TSharedRef<ITableRow> SProjectCleanerBrowserNonProjectFilesUI::OnGenerateRow(
	TWeakObjectPtr<UAssetsUsedInSourceCode> InItem,
	const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SAssetsUsedInSourceCodeSelectionRow, OwnerTable).SelectedObjItem(InItem);
}

#pragma optimize("", on)
#undef LOCTEXT_NAMESPACE
