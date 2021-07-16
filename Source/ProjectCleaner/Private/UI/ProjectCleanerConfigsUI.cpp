// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#include "UI/ProjectCleanerConfigsUI.h"
#include "StructsContainer.h"
#include "Widgets/Layout/SScrollBox.h"

#define LOCTEXT_NAMESPACE "FProjectCleanerModule"

void SProjectCleanerConfigsUI::Construct(const FArguments& InArgs)
{
	if (InArgs._CleanerConfigs)
	{
		CleanerConfigs = InArgs._CleanerConfigs;
	}

	ensure(CleanerConfigs);
	
	FPropertyEditorModule& PropertyEditor = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.bUpdatesFromSelection = false;
	DetailsViewArgs.bLockable = false;
	DetailsViewArgs.bAllowSearch = false;
	DetailsViewArgs.bShowOptions = false;
	DetailsViewArgs.bAllowFavoriteSystem = false;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	DetailsViewArgs.ViewIdentifier = "ProjectCleanerConfigs";
	ConfigsProperty = PropertyEditor.CreateDetailView(DetailsViewArgs);
	ConfigsProperty->SetObject(CleanerConfigs);

	ChildSlot
	[
		SNew(SScrollBox)
		.ScrollWhenFocusChanges(EScrollWhenFocusChanges::AnimatedScroll)
		.AnimateWheelScrolling(true)
		.AllowOverscroll(EAllowOverscroll::No)
		+ SScrollBox::Slot()
		[
			ConfigsProperty.ToSharedRef()
		]
	];
}

void SProjectCleanerConfigsUI::SetCleanerConfigs(UCleanerConfigs* Configs)
{
	if (!Configs) return;
	if (!Configs->IsValidLowLevel()) return;

	CleanerConfigs = Configs;
}

#undef LOCTEXT_NAMESPACE