#include "UI/ProjectCleanerDirectoryExclusionUI.h"

#define LOCTEXT_NAMESPACE "FProjectCleanerModule"

void SProjectCleanerDirectoryExclusionUI::Construct(const FArguments& InArgs)
{

	FPropertyEditorModule& PropertyEditor = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs DirectoryFilterDetailsViewArgs;
	DirectoryFilterDetailsViewArgs.bUpdatesFromSelection = false;
	DirectoryFilterDetailsViewArgs.bLockable = false;
	DirectoryFilterDetailsViewArgs.bAllowSearch = false;
	DirectoryFilterDetailsViewArgs.bShowOptions = false;
	DirectoryFilterDetailsViewArgs.bAllowFavoriteSystem = false;
	DirectoryFilterDetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	DirectoryFilterDetailsViewArgs.ViewIdentifier = "ExcludeDirectoriesFromScan";
	
	ExcludeDirectoriesFilterSettingsProperty = PropertyEditor.CreateDetailView(DirectoryFilterDetailsViewArgs);
	if(InArgs._ExcludeDirectoriesFilterSettings)
	{
		ExcludeDirectoriesFilterSettings = InArgs._ExcludeDirectoriesFilterSettings;
		ExcludeDirectoriesFilterSettingsProperty->SetObject(ExcludeDirectoriesFilterSettings);
	}

	const auto FontInfo = FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Light.ttf"), 20);
	
	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.Padding(FMargin{0.0f, 10.0f})
		.AutoHeight()
		[
			SNew(STextBlock)
			.AutoWrapText(true)
			.Font(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Light.ttf"), 8))
			.Text(LOCTEXT("applyfilters", "Click \"Refresh\" button to Apply Filters"))
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			ExcludeDirectoriesFilterSettingsProperty.ToSharedRef()
		]
	];
}

#undef LOCTEXT_NAMESPACE