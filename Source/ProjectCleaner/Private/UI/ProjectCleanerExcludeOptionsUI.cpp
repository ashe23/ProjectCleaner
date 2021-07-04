// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#include "UI/ProjectCleanerExcludeOptionsUI.h"
#include "UI/ProjectCleanerStyle.h"
#include "StructsContainer.h"

#define LOCTEXT_NAMESPACE "FProjectCleanerModule"

void SProjectCleanerExcludeOptionsUI::Construct(const FArguments& InArgs)
{
	FPropertyEditorModule& PropertyEditor = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.bUpdatesFromSelection = false;
	DetailsViewArgs.bLockable = false;
	DetailsViewArgs.bAllowSearch = false;
	DetailsViewArgs.bShowOptions = false;
	DetailsViewArgs.bAllowFavoriteSystem = false;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	DetailsViewArgs.ViewIdentifier = "ProjectCleanerExcludeOptions";
	
	ExcludeOptionsProperty = PropertyEditor.CreateDetailView(DetailsViewArgs);
	if (InArgs._ExcludeOptions)
	{
		ExcludeOptions = InArgs._ExcludeOptions;
		ExcludeOptionsProperty->SetObject(ExcludeOptions);
	}
	
	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.Padding(FMargin{0.0f, 10.0f})
		.AutoHeight()
		[
			SNew(STextBlock)
			.AutoWrapText(true)
			.Font(FProjectCleanerStyle::Get().GetFontStyle("ProjectCleaner.Font.Light10"))
			.Text(LOCTEXT("applyfilters", "Click \"Refresh\" button to apply filters"))
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			ExcludeOptionsProperty.ToSharedRef()
		]
	];
}

#undef LOCTEXT_NAMESPACE