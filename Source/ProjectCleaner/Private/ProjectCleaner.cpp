// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "ProjectCleaner.h"
#include "ProjectCleanerStyles.h"
#include "ProjectCleanerCmds.h"
#include "ProjectCleanerConstants.h"
// #include "ProjectCleanerScanner.h"
// #include "ProjectCleanerScanSettings.h"
#include "Slate/SProjectCleaner.h"
// Engine Headers
#include "ToolMenus.h"
#include "AssetToolsModule.h"
#include "AssetRegistry/AssetRegistryModule.h"

DEFINE_LOG_CATEGORY(LogProjectCleaner);

#define LOCTEXT_NAMESPACE "FProjectCleanerModule"

// TSharedPtr<FProjectCleanerScanner> FProjectCleanerModule::ScannerInstance = nullptr;

void FProjectCleanerModule::StartupModule()
{
	RegisterStyles();
	RegisterCmds();
	RegisterMenus();
	RegisterTabs();
	// RegisterScanner();
}

void FProjectCleanerModule::ShutdownModule()
{
	UnregisterTabs();
	UnregisterMenus();
	UnregisterCmds();
	UnregisterStyles();
	// UnregisterScanner();
}

bool FProjectCleanerModule::IsGameModule() const
{
	return false;
}

bool FProjectCleanerModule::SupportsDynamicReloading()
{
	return false;
}

// const TSharedPtr<FProjectCleanerScanner>& FProjectCleanerModule::GetScanner()
// {
// 	return ScannerInstance;
// }

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

			                        return DockTab;
		                        }))
	                        .SetDisplayName(LOCTEXT("FProjectCleanerTabTitle", "Project Cleaner"))
	                        .SetMenuType(ETabSpawnerMenuType::Hidden)
	                        .SetIcon(FSlateIcon(FProjectCleanerStyles::GetStyleSetName(), "ProjectCleaner.IconBin16"));
}

// void FProjectCleanerModule::RegisterScanner() const
// {
// 	if (!ScannerInstance.IsValid())
// 	{
// 		const TWeakObjectPtr<UProjectCleanerScanSettings> ScanSettings = GetMutableDefault<UProjectCleanerScanSettings>();
// 		ScannerInstance = MakeShareable(new FProjectCleanerScanner(ScanSettings));
// 	}
// }

void FProjectCleanerModule::UnregisterMenus()
{
	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(this);
}

void FProjectCleanerModule::UnregisterTabs()
{
	FGlobalTabmanager::Get()->UnregisterTabSpawner(ProjectCleanerConstants::TabProjectCleaner);
}

// void FProjectCleanerModule::UnregisterScanner() const
// {
// 	ensure(ScannerInstance.IsUnique());
// 	ScannerInstance.Reset();
// }

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
