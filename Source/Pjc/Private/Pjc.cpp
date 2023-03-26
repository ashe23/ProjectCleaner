﻿// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Pjc.h"
#include "PjcCmds.h"
#include "PjcConstants.h"
#include "PjcStyles.h"
#include "Slate/SPjcMainWindow.h"
// Engine Headers
#include "ToolMenus.h"

DEFINE_LOG_CATEGORY(LogProjectCleaner);

void FPjc::StartupModule()
{
	IModuleInterface::StartupModule();

	FPjcStyles::Initialize();
	FPjcStyles::ReloadTextures();
	FPjcCmds::Register();

	Cmds = MakeShareable(new FUICommandList);
	Cmds->MapAction(
		FPjcCmds::Get().OpenMainWindow,
		FExecuteAction::CreateLambda([&]()
		{
			FGlobalTabmanager::Get()->TryInvokeTab(PjcConstants::TabProjectCleaner);
		})
	);

	UToolMenus::RegisterStartupCallback(
		FSimpleMulticastDelegate::FDelegate::CreateLambda([&]()
			{
				FToolMenuOwnerScoped OwnerScoped(this);

				UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
				FToolMenuSection& Section = Menu->AddSection(
					"SectionAssetManagementTools",
					FText::FromString(TEXT("Project Management Tools")),
					FToolMenuInsert("WindowLayout", EToolMenuInsertType::After)
				);
				Section.AddMenuEntryWithCommandList(FPjcCmds::Get().OpenMainWindow, Cmds);

				UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar");
				FToolMenuSection& ToolbarSection = ToolbarMenu->FindOrAddSection("Settings");
				FToolMenuEntry& Entry = ToolbarSection.AddEntry(
					FToolMenuEntry::InitToolBarButton(FPjcCmds::Get().OpenMainWindow)
				);
				Entry.SetCommandList(Cmds);
			}
		)
	);

	FGlobalTabmanager::Get()
		->RegisterTabSpawner(
			PjcConstants::TabProjectCleaner,
			FOnSpawnTab::CreateLambda([&](const FSpawnTabArgs& SpawnTabArgs) -> TSharedRef<SDockTab>
			{
				const TSharedRef<SDockTab> DockTab = SNew(SDockTab).TabRole(MajorTab);
				const TSharedRef<SPjcMainWindow> Frontend = SNew(SPjcMainWindow, DockTab, SpawnTabArgs.GetOwnerWindow());

				DockTab->SetContent(Frontend);

				return DockTab;
			})
		)
		.SetDisplayName(FText::FromString(PjcConstants::ModuleTitle.ToString()))
		.SetMenuType(ETabSpawnerMenuType::Hidden)
		.SetIcon(FPjcStyles::GetIcon("ProjectCleaner.IconBin16"));
}

void FPjc::ShutdownModule()
{
	FGlobalTabmanager::Get()->UnregisterTabSpawner(PjcConstants::TabProjectCleaner);
	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(this);
	FPjcCmds::Unregister();
	FPjcStyles::Shutdown();

	IModuleInterface::ShutdownModule();
}

bool FPjc::SupportsDynamicReloading()
{
	return false;
}

bool FPjc::IsGameModule() const
{
	return false;
}

IMPLEMENT_MODULE(FPjc, Pjc)