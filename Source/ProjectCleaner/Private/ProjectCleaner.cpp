// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "ProjectCleaner.h"
#include "ProjectCleanerStyles.h"
#include "ProjectCleanerCmds.h"
#include "ProjectCleanerConstants.h"
#include "Slate/ProjectCleanerWindowMain.h"
#include "Slate/ProjectCleanerStatTreeItem.h"
#include "Slate/ProjectCleanerAssetBrowser.h"
// Engine Headers
#include "ToolMenus.h"
#include "AssetToolsModule.h"
#include "AssetRegistry/AssetRegistryModule.h"

DEFINE_LOG_CATEGORY(LogProjectCleaner);

#define LOCTEXT_NAMESPACE "FProjectCleanerModule"

void FProjectCleanerModule::StartupModule()
{
	RegisterStyles();
	RegisterCmds();
	RegisterMenus();
	RegisterTabs();
}

void FProjectCleanerModule::ShutdownModule()
{
	UnregisterTabs();
	UnregisterMenus();
	UnregisterCmds();
	UnregisterStyles();
}

bool FProjectCleanerModule::IsGameModule() const
{
	return false;
}

bool FProjectCleanerModule::SupportsDynamicReloading()
{
	return false;
}

void FProjectCleanerModule::RegisterStyles()
{
	FProjectCleanerStyles::Initialize();
	FProjectCleanerStyles::ReloadTextures();
	FProjectCleanerCmds::Register();
}

void FProjectCleanerModule::RegisterCmds()
{
	Cmds = MakeShareable(new FUICommandList);
	Cmds->MapAction(
		FProjectCleanerCmds::Get().Cmd_OpenCleanerWindow,
		FExecuteAction::CreateLambda([&]()
		{
			FGlobalTabmanager::Get()->TryInvokeTab(ProjectCleanerConstants::TabProjectCleaner);
		})
	);
}

void FProjectCleanerModule::RegisterMenus()
{
	UToolMenus::RegisterStartupCallback(
		FSimpleMulticastDelegate::FDelegate::CreateLambda([&]()
			{
				FToolMenuOwnerScoped OwnerScoped(this);

				UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
				FToolMenuSection& Section = Menu->FindOrAddSection("WindowLayout");
				Section.AddMenuEntryWithCommandList(FProjectCleanerCmds::Get().Cmd_OpenCleanerWindow, Cmds);

				UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar");
				FToolMenuSection& ToolbarSection = ToolbarMenu->FindOrAddSection("Settings");
				FToolMenuEntry& Entry = ToolbarSection.AddEntry(
					FToolMenuEntry::InitToolBarButton(FProjectCleanerCmds::Get().Cmd_OpenCleanerWindow)
				);
				Entry.SetCommandList(Cmds);
			}
		)
	);
}

void FProjectCleanerModule::RegisterTabs() const
{
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
		                        ProjectCleanerConstants::TabProjectCleaner,
		                        FOnSpawnTab::CreateLambda([](const FSpawnTabArgs& SpawnTabArgs) -> TSharedRef<SDockTab>
		                        {
			                        return SNew(SDockTab).TabRole(MajorTab)
			                        [
			                        	SNew(SProjectCleanerWindowMain)
			                        ];
		                        }))
	                        .SetDisplayName(LOCTEXT("FProjectCleanerTabTitle", "ProjectCleaner"))
	                        .SetMenuType(ETabSpawnerMenuType::Hidden)
	                        .SetIcon(FSlateIcon(FProjectCleanerStyles::GetStyleSetName(), "ProjectCleaner.IconBin20"));
}

void FProjectCleanerModule::UnregisterMenus()
{
	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(this);
}

void FProjectCleanerModule::UnregisterTabs()
{
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(ProjectCleanerConstants::TabProjectCleaner);
}

void FProjectCleanerModule::UnregisterStyles()
{
	FProjectCleanerStyles::Shutdown();
}

void FProjectCleanerModule::UnregisterCmds()
{
	FProjectCleanerCmds::Unregister();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FProjectCleanerModule, ProjectCleaner)
