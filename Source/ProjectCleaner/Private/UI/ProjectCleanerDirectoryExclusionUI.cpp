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
	
	ChildSlot
	[	
		ExcludeDirectoriesFilterSettingsProperty.ToSharedRef()
	];
}

#undef LOCTEXT_NAMESPACE