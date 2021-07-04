// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#include "UI/ProjectCleanerConfigsUI.h"
#include "StructsContainer.h"

#define LOCTEXT_NAMESPACE "FProjectCleanerModule"

void SProjectCleanerConfigsUI::Construct(const FArguments& InArgs)
{
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
	if (InArgs._CleanerConfigs)
	{
		CleanerConfigs = InArgs._CleanerConfigs;
		ConfigsProperty->SetObject(CleanerConfigs);
	}

	InitUI();
}

void SProjectCleanerConfigsUI::SetCleanerConfigs(UCleanerConfigs* Configs)
{
	if (!Configs) return;
	if (!Configs->IsValidLowLevel()) return;

	CleanerConfigs = Configs;
}

void SProjectCleanerConfigsUI::InitUI()
{
	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			ConfigsProperty.ToSharedRef()
		]
	];
}

#undef LOCTEXT_NAMESPACE