#include "UI/SProjectCleanerBrowser.h"

// Engine Headers
#include "PropertyEditorModule.h"
#include "Modules/ModuleManager.h"
#include "Widgets/Layout/SBorder.h"


void SProjectCleanerBrowser::Construct(const FArguments& InArgs)
{

	FPropertyEditorModule& PropertyEditor = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

	FDetailsViewArgs FolderFilterArgs;
	FolderFilterArgs.bUpdatesFromSelection = false;
	FolderFilterArgs.bLockable = false;
	FolderFilterArgs.bAllowSearch = false;
	FolderFilterArgs.bShowOptions = false;
	FolderFilterArgs.bAllowFavoriteSystem = false;
	FolderFilterArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	FolderFilterArgs.ViewIdentifier = "ExcludeDirectoriesFromScan";

	DirectoryFilterProperty = PropertyEditor.CreateDetailView(FolderFilterArgs);
	
	ChildSlot
	[
		SNew(SVerticalBox)
		+SVerticalBox::Slot()
		.Padding(20)
		[
			SNew(SBorder)
			.Padding(FMargin(20))
			[
				DirectoryFilterProperty.ToSharedRef()
			]
		]
		
	];

	if(InArgs._DirectoryFilterSettings)
	{
		DirectoryFilterSettings = InArgs._DirectoryFilterSettings;
		DirectoryFilterProperty->SetObject(DirectoryFilterSettings);
	}
}