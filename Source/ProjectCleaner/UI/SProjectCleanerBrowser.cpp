// Fill out your copyright notice in the Description page of Project Settings.


#include "SProjectCleanerBrowser.h"
#include "PropertyEditorModule.h"
#include "UObject/ObjectMacros.h"


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

	DirFilterSettings = PropertyEditor.CreateDetailView(FolderFilterArgs);
	
	ChildSlot
	[
		SNew(SVerticalBox)
		+SVerticalBox::Slot()
		.Padding(20)
		[
			SNew(SBorder)
			.Padding(FMargin(20))
			[
				DirFilterSettings.ToSharedRef()
			]
		]
		
	];

	if(InArgs._DirectoryFilterSettings)
	{
		DirectoryFilterSettings = InArgs._DirectoryFilterSettings;
		DirFilterSettings->SetObject(DirectoryFilterSettings);
	}
}