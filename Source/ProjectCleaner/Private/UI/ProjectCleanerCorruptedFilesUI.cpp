#include "UI/ProjectCleanerCorruptedFilesUI.h"
// Engine Headers
#include "IContentBrowserSingleton.h"
#include "Editor/ContentBrowser/Public/ContentBrowserModule.h"

#define LOCTEXT_NAMESPACE "FProjectCleanerModule"

void SProjectCleanerCorruptedFilesUI::Construct(const FArguments& InArgs)
{
	CorruptedFiles = InArgs._CorruptedFiles;

	RefreshUIContent();
	
	ChildSlot
	[
		WidgetRef
	];
}

void SProjectCleanerCorruptedFilesUI::RefreshUIContent()
{
	FAssetPickerConfig Config;
	Config.InitialAssetViewType = EAssetViewType::List;
	Config.bAddFilterUI = true;
	Config.bShowPathInColumnView = true;
	Config.bSortByPathInColumnView = true;
	Config.bForceShowEngineContent = false;
	Config.bShowBottomToolbar = true;
	Config.bCanShowDevelopersFolder = false;
	Config.bAllowDragging = false;	
	Config.AssetShowWarningText = FText::FromName("No assets");

	FARFilter Filter;
	if(CorruptedFiles.Num() == 0)
	{
		// this is needed when there is no assets to show ,
		// asset picker will show remaining assets in content browser,
		// we must not show them
		Filter.TagsAndValues.Add(FName{"ProjectCleanerEmptyTag"}, FString{"ProjectCleanerEmptyTag"});
	}
	else
	{
		// excluding level assets from showing and filtering
		Filter.bRecursiveClasses = true;
		Filter.RecursiveClassesExclusionSet.Add(UWorld::StaticClass()->GetFName());
	}
	for(const auto& Asset : CorruptedFiles)
	{
		Filter.PackageNames.Add(Asset->PackageName);
	}
	Config.Filter = Filter;
	FContentBrowserModule& ContentBrowser = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");

	
	WidgetRef = SNew(SVerticalBox)
	+ SVerticalBox::Slot()
	.AutoHeight()
	[
		SNew(SVerticalBox)
		+SVerticalBox::Slot()
		.AutoHeight()    	
		[
			SNew(STextBlock)
			.AutoWrapText(true)
			.Font(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Light.ttf"),20))
			.Text(LOCTEXT("corruptedfiles", "Possibly Corrupted Files"))
		]
		+SVerticalBox::Slot()
		.AutoHeight()
		.Padding(FMargin{0.0f, 10.0f})
		[
			SNew(STextBlock)
			.AutoWrapText(true)
			.Font(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Light.ttf"),8))
			.Text(LOCTEXT("corruptedfilesfixtext", "To fix this files close the Editor and delete them manually from Windows Explorer"))
		]
	]
	+ SVerticalBox::Slot()
	.AutoHeight()
	[
		ContentBrowser.Get().CreateAssetPicker(Config)
	];
	
	ChildSlot
	[
		WidgetRef
	];
}

void SProjectCleanerCorruptedFilesUI::SetCorruptedFiles(TArray<FAssetData*> NewCorruptedFiles)
{
	CorruptedFiles = NewCorruptedFiles;

	RefreshUIContent();
}

#undef LOCTEXT_NAMESPACE
