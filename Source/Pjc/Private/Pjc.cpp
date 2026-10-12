// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Pjc.h"
#include "PjcShim.h"
#include "PjcSettings.h"
#include "PjcCmds.h"
#include "PjcStyles.h"
#include "PjcConstants.h"
#include "Slate/SPjcTabMain.h"
// Engine Headers
#include "ToolMenus.h"
#include "Widgets/Docking/SDockTab.h"

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

	FGlobalTabmanager::Get()
		->RegisterTabSpawner(
			PjcConstants::TabProjectCleaner, FOnSpawnTab::CreateLambda([&](const FSpawnTabArgs& SpawnTabArgs) -> TSharedRef<SDockTab> {
				const TSharedRef<SDockTab> DockTab = SNew(SDockTab).TabRole(MajorTab);
				const TSharedRef<SPjcTabMain> Frontend = SNew(SPjcTabMain, DockTab, SpawnTabArgs.GetOwnerWindow());

				DockTab->SetContent(Frontend);

				return DockTab;
			})
		)
		.SetDisplayName(FText::FromString(PjcConstants::ModulePjcTitle.ToString()))
		.SetMenuType(ETabSpawnerMenuType::Hidden)
		.SetIcon(FPjcStyles::GetIcon("ProjectCleaner.Icon.Bin16"));
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
	FToolMenuSection& Section = Menu->AddSection(
		"PjcSectionAssetManagementTools",
		FText::FromString(TEXT("Project Management Tools")),
		FToolMenuInsert("WindowLayout", EToolMenuInsertType::After)
	);
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
