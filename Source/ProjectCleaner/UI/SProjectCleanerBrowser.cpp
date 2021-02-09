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
}

// TSharedRef<SWidget> SProjectCleanerBrowser::MakeWidgetForOption(FComboItemType InOption)
// {
// 	return SNew(STextBlock).Text(FText::FromString(*InOption));
// }
//
// void SProjectCleanerBrowser::OnSelectionChanged(FComboItemType NewValue, ESelectInfo::Type)
// {
// 	CurrentItem = NewValue;
// }
//
// FText SProjectCleanerBrowser::GetCurrentItemLabel() const
// {
// 	if(CurrentItem.IsValid())
// 	{
// 		return FText::FromString(*CurrentItem);
// 	}
//
// 	return FText::FromString("<<Invalid Option>>");
// }
