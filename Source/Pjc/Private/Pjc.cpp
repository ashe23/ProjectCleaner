// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Pjc.h"
#include "PjcShim.h"
#include "PjcSettings.h"
#include "PjcCmds.h"
#include "PjcStyles.h"
#include "PjcConstants.h"
#include "Slate/SPjcTabMain.h"
#include "Slate/SPjcTabHelp.h"
// Engine Headers
#include "ToolMenus.h"
#include "Widgets/Docking/SDockTab.h"
#include "Framework/Docking/TabManager.h"

DEFINE_LOG_CATEGORY(LogProjectCleaner);

void FPjc::StartupModule() {
	IModuleInterface::StartupModule();

	FPjcStyles::Initialize();
	FPjcStyles::ReloadTextures();
	FPjcCmds::Register();

	Cmds = MakeShareable(new FUICommandList);
	Cmds->MapAction(FPjcCmds::Get().TabMain, FExecuteAction::CreateLambda([&]() {
						FGlobalTabmanager::Get()->TryInvokeTab(PjcConstants::TabProjectCleaner);
					}));

	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FPjc::RegisterMenus));
	GetMutableDefault<UPjcEditorSettings>()->OnChanged.AddRaw(this, &FPjc::OnSettingsChanged);

	const FOnSpawnTab OnSpawnTab = FOnSpawnTab::CreateLambda([](const FSpawnTabArgs& SpawnTabArgs) -> TSharedRef<SDockTab> {
		const FString Title = PjcConstants::ModulePjcTitle.ToString() + TEXT(" v") + SPjcTabHelp::GetPluginVersion();
		const TSharedRef<SDockTab> DockTab = SNew(SDockTab).TabRole(MajorTab).Label(FText::FromString(Title));
		DockTab->SetContent(SNew(SPjcTabMain, DockTab, SpawnTabArgs.GetOwnerWindow()));
		return DockTab;
	});
	const FText TabTitle = FText::FromString(PjcConstants::ModulePjcTitle.ToString());
	FGlobalTabmanager::Get()
		->RegisterTabSpawner(PjcConstants::TabProjectCleaner, OnSpawnTab)
		.SetDisplayName(TabTitle)
		.SetMenuType(ETabSpawnerMenuType::Hidden)
		.SetIcon(FPjcStyles::GetIcon("ProjectCleaner.Icon.App16"));
}

void FPjc::ShutdownModule() {
	if (UObjectInitialized()) {
		GetMutableDefault<UPjcEditorSettings>()->OnChanged.RemoveAll(this);
	}

	FGlobalTabmanager::Get()->UnregisterTabSpawner(PjcConstants::TabProjectCleaner);
	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(this);
	FPjcCmds::Unregister();
	FPjcStyles::Shutdown();

	IModuleInterface::ShutdownModule();
}

void FPjc::RegisterMenus() {
	FToolMenuOwnerScoped OwnerScoped(this);

	UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
	const FToolMenuInsert Insert {"WindowLayout", EToolMenuInsertType::After};
	FToolMenuSection& Section = Menu->AddSection("PjcSectionAssetManagementTools", FText::FromString(TEXT("Project Management Tools")), Insert);
	Section.AddMenuEntryWithCommandList(FPjcCmds::Get().TabMain, Cmds);

	const EPjcToolbarLocation ToolbarLocation = GetDefault<UPjcEditorSettings>()->ToolbarLocation;
	if (ToolbarLocation == EPjcToolbarLocation::Hidden) return;

	UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu(PjcShim::GetLevelEditorToolBarName(ToolbarLocation));
	FToolMenuSection& ToolbarSection = ToolbarMenu->FindOrAddSection("Settings");
	FToolMenuEntry& Entry = ToolbarSection.AddEntry(FToolMenuEntry::InitToolBarButton(FPjcCmds::Get().TabMain));
	Entry.SetCommandList(Cmds);
}

void FPjc::OnSettingsChanged() {
	if (!UToolMenus::IsToolMenuUIEnabled()) return;

	// toolbar location can change at any time, rebuild entries owned by this module
	UToolMenus::UnregisterOwner(this);
	RegisterMenus();
	UToolMenus::Get()->RefreshAllWidgets();
}

bool FPjc::SupportsDynamicReloading() {
	return false;
}

bool FPjc::IsGameModule() const {
	return false;
}

IMPLEMENT_MODULE(FPjc, Pjc)
