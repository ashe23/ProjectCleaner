// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "ProjectCleaner.h"
#include "ProjectCleanerStyles.h"
#include "ProjectCleanerCmds.h"
#include "ProjectCleanerConstants.h"
#include "Slate/SProjectCleaner.h"
// Engine Headers
#include "ProjectCleanerSubsystem.h"
#include "ToolMenus.h"

DEFINE_LOG_CATEGORY(LogProjectCleaner);

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
		FProjectCleanerCmds::Get().OpenProjectCleanerWindow,
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
				FToolMenuSection& Section = Menu->AddSection(
					"SectionAssetManagementTools",
					FText::FromString(TEXT("Asset Management Tools")),
					FToolMenuInsert("WindowLayout", EToolMenuInsertType::After)
				);
				Section.AddMenuEntryWithCommandList(FProjectCleanerCmds::Get().OpenProjectCleanerWindow, Cmds);

				UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar");
				FToolMenuSection& ToolbarSection = ToolbarMenu->FindOrAddSection("Settings");
				FToolMenuEntry& Entry = ToolbarSection.AddEntry(
					FToolMenuEntry::InitToolBarButton(FProjectCleanerCmds::Get().OpenProjectCleanerWindow)
				);
				Entry.SetCommandList(Cmds);
			}
		)
	);
}

void FProjectCleanerModule::RegisterTabs() const
{
	FGlobalTabmanager::Get()->RegisterTabSpawner(
		                        ProjectCleanerConstants::TabProjectCleaner,
		                        FOnSpawnTab::CreateLambda([&](const FSpawnTabArgs& SpawnTabArgs) -> TSharedRef<SDockTab>
		                        {
			                        const TSharedRef<SDockTab> DockTab = SNew(SDockTab).TabRole(MajorTab);
			                        const TSharedRef<SProjectCleaner> Frontend = SNew(SProjectCleaner, DockTab, SpawnTabArgs.GetOwnerWindow());

			                        DockTab->SetContent(Frontend);
			                        DockTab->SetOnTabActivated(
				                        SDockTab::FOnTabActivatedCallback::CreateLambda([](TSharedRef<SDockTab>, ETabActivationCause)
				                        {
					                        if (!GEditor) return;

					                        GEditor->GetEditorSubsystem<UProjectCleanerSubsystem>()->NotifyMainTabActivated();
				                        })
			                        );
			                        DockTab->SetOnTabClosed(
				                        SDockTab::FOnTabClosedCallback::CreateLambda([](TSharedRef<SDockTab>)
				                        {
					                        if (!GEditor) return;

					                        GEditor->GetEditorSubsystem<UProjectCleanerSubsystem>()->NotifyMainTabClosed();
				                        })
			                        );

			                        return DockTab;
		                        }))
	                        .SetDisplayName(FText::FromString(ProjectCleanerConstants::ModuleTitle.ToString()))
	                        .SetMenuType(ETabSpawnerMenuType::Hidden)
	                        .SetIcon(FSlateIcon(FProjectCleanerStyles::GetStyleSetName(), "ProjectCleaner.IconBin16"));
}

void FProjectCleanerModule::UnregisterMenus()
{
	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(this);
}

void FProjectCleanerModule::UnregisterTabs()
{
	FGlobalTabmanager::Get()->UnregisterTabSpawner(ProjectCleanerConstants::TabProjectCleaner);
}

void FProjectCleanerModule::UnregisterStyles()
{
	FProjectCleanerStyles::Shutdown();
}

void FProjectCleanerModule::UnregisterCmds()
{
	FProjectCleanerCmds::Unregister();
}

IMPLEMENT_MODULE(FProjectCleanerModule, ProjectCleaner)
